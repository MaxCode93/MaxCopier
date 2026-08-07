#include "copia/backendwin32.h"

// Solo Windows: fuera de él este archivo es una unidad vacía (el motor usa el
// backend de QFile, y así se puede compilar el proyecto en Linux sin Win32).
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>

namespace maxcopier {
namespace win32 {
namespace {

std::wstring mensajeDeError(DWORD codigo)
{
    wchar_t *texto = nullptr;
    const DWORD tamano = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, codigo, 0, reinterpret_cast<wchar_t *>(&texto), 0, nullptr);
    std::wstring resultado = tamano > 0 && texto != nullptr
        ? std::wstring(texto, tamano)
        : std::wstring(L"error ") + std::to_wstring(codigo);
    if (texto != nullptr)
        LocalFree(texto);
    while (!resultado.empty()
        && (resultado.back() == L'\r' || resultado.back() == L'\n'))
        resultado.pop_back();
    return resultado;
}

HANDLE abrir(const std::wstring &ruta, DWORD acceso, DWORD comparticion,
    DWORD creacion, DWORD atributos)
{
    return CreateFileW(rutaLarga(ruta).c_str(), acceso, comparticion, nullptr,
        creacion, atributos, nullptr);
}

bool darError(std::wstring *error, const std::wstring &motivo)
{
    if (error != nullptr)
        *error = motivo;
    return false;
}

} // namespace

std::wstring rutaLarga(const std::wstring &ruta)
{
    if (ruta.empty())
        return ruta;

    std::wstring normalizada = ruta;
    for (wchar_t &caracter : normalizada) {
        if (caracter == L'/')
            caracter = L'\\';
    }
    if (normalizada.rfind(L"\\\\?\\", 0) == 0)
        return normalizada;
    if (normalizada.rfind(L"\\\\", 0) == 0)
        return L"\\\\?\\UNC\\" + normalizada.substr(2);
    return L"\\\\?\\" + normalizada;
}

bool abrirLectura(const std::wstring &ruta, void **manejo, std::uint64_t *tamano,
    std::wstring *error)
{
    if (manejo == nullptr)
        return false;
    *manejo = nullptr;

    HANDLE h = abrir(ruta, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL);
    if (h == INVALID_HANDLE_VALUE)
        return darError(error, mensajeDeError(GetLastError()));

    LARGE_INTEGER tam;
    if (!GetFileSizeEx(h, &tam)) {
        const std::wstring motivo = mensajeDeError(GetLastError());
        CloseHandle(h);
        return darError(error, motivo);
    }

    *manejo = h;
    if (tamano != nullptr)
        *tamano = std::uint64_t(tam.QuadPart);
    return true;
}

bool abrirEscritura(const std::wstring &ruta, ModoEscritura modo, void **manejo,
    std::wstring *error)
{
    if (manejo == nullptr)
        return false;
    *manejo = nullptr;

    const DWORD creacion = modo == ModoEscritura::Nuevo ? CREATE_NEW : OPEN_ALWAYS;
    HANDLE h = abrir(ruta, GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, creacion,
        FILE_ATTRIBUTE_NORMAL);
    if (h == INVALID_HANDLE_VALUE)
        return darError(error, mensajeDeError(GetLastError()));

    if (modo == ModoEscritura::Reanudar) {
        LARGE_INTEGER final;
        final.QuadPart = 0;
        if (!SetFilePointerEx(h, final, nullptr, FILE_END)) {
            const std::wstring motivo = mensajeDeError(GetLastError());
            CloseHandle(h);
            return darError(error, motivo);
        }
    }

    *manejo = h;
    return true;
}

bool abrirLecturaAsincrona(const std::wstring &ruta, void **manejo, std::uint64_t *tamano,
    std::wstring *error)
{
    if (manejo == nullptr)
        return false;
    *manejo = nullptr;

    HANDLE h = abrir(ruta, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED);
    if (h == INVALID_HANDLE_VALUE)
        return darError(error, mensajeDeError(GetLastError()));

    LARGE_INTEGER tam;
    if (!GetFileSizeEx(h, &tam)) {
        const std::wstring motivo = mensajeDeError(GetLastError());
        CloseHandle(h);
        return darError(error, motivo);
    }

    *manejo = h;
    if (tamano != nullptr)
        *tamano = std::uint64_t(tam.QuadPart);
    return true;
}

bool abrirEscrituraAsincrona(const std::wstring &ruta, ModoEscritura modo, void **manejo,
    std::wstring *error)
{
    if (manejo == nullptr)
        return false;
    *manejo = nullptr;

    const DWORD creacion = modo == ModoEscritura::Nuevo ? CREATE_NEW : OPEN_ALWAYS;
    HANDLE h = abrir(ruta, GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, creacion,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED);
    if (h == INVALID_HANDLE_VALUE)
        return darError(error, mensajeDeError(GetLastError()));

    if (modo == ModoEscritura::Reanudar) {
        LARGE_INTEGER final;
        final.QuadPart = 0;
        if (!SetFilePointerEx(h, final, nullptr, FILE_END)) {
            const std::wstring motivo = mensajeDeError(GetLastError());
            CloseHandle(h);
            return darError(error, motivo);
        }
    }

    *manejo = h;
    return true;
}

struct IoAsincrono {
    OVERLAPPED overlapped {};
    HANDLE evento = nullptr;
    HANDLE manejo = nullptr;
    bool inmediato = false;
    DWORD bytes = 0;
};

IoAsincrono *crearIo(std::wstring *error)
{
    auto *io = new IoAsincrono;
    io->evento = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (io->evento == nullptr) {
        darError(error, mensajeDeError(GetLastError()));
        delete io;
        return nullptr;
    }
    io->overlapped.hEvent = io->evento;
    return io;
}

void liberarIo(IoAsincrono *io)
{
    if (io == nullptr)
        return;
    if (io->evento != nullptr)
        CloseHandle(io->evento);
    delete io;
}

void *eventoDeIo(IoAsincrono *io)
{
    return io != nullptr ? io->evento : nullptr;
}

bool esperarEventos(void *const *eventos, int cantidad, int timeoutMs, bool todos)
{
    if (eventos == nullptr || cantidad <= 0 || cantidad > MAXIMUM_WAIT_OBJECTS)
        return false;

    HANDLE manejos[MAXIMUM_WAIT_OBJECTS];
    for (int i = 0; i < cantidad; ++i)
        manejos[i] = static_cast<HANDLE>(eventos[i]);
    const DWORD resultado = WaitForMultipleObjects(
        static_cast<DWORD>(cantidad), manejos, todos ? TRUE : FALSE,
        static_cast<DWORD>(timeoutMs));
    return resultado >= WAIT_OBJECT_0
        && resultado < WAIT_OBJECT_0 + static_cast<DWORD>(cantidad);
}

std::int64_t lanzarLectura(IoAsincrono *io, void *manejo, void *datos, std::int64_t n,
    std::uint64_t offset, std::wstring *error)
{
    if (io == nullptr || manejo == nullptr || datos == nullptr || n <= 0) {
        darError(error, L"operación de E/S no válida");
        return -1;
    }

    io->manejo = manejo;
    io->inmediato = false;
    io->bytes = 0;
    io->overlapped.Offset = static_cast<DWORD>(offset & 0xFFFFFFFFu);
    io->overlapped.OffsetHigh = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFu);
    io->overlapped.hEvent = io->evento;

    DWORD leidos = 0;
    if (ReadFile(manejo, datos, static_cast<DWORD>(n), &leidos, &io->overlapped)) {
        io->inmediato = true;
        io->bytes = leidos;
        return 0;
    }
    if (GetLastError() == ERROR_IO_PENDING)
        return 1;
    darError(error, mensajeDeError(GetLastError()));
    return -1;
}

std::int64_t lanzarEscritura(IoAsincrono *io, void *manejo, const void *datos, std::int64_t n,
    std::uint64_t offset, std::wstring *error)
{
    if (io == nullptr || manejo == nullptr || datos == nullptr || n <= 0) {
        darError(error, L"operación de E/S no válida");
        return -1;
    }

    io->manejo = manejo;
    io->inmediato = false;
    io->bytes = 0;
    io->overlapped.Offset = static_cast<DWORD>(offset & 0xFFFFFFFFu);
    io->overlapped.OffsetHigh = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFu);
    io->overlapped.hEvent = io->evento;

    DWORD escritos = 0;
    if (WriteFile(manejo, datos, static_cast<DWORD>(n), &escritos, &io->overlapped)) {
        io->inmediato = true;
        io->bytes = escritos;
        return 0;
    }
    if (GetLastError() == ERROR_IO_PENDING)
        return 1;
    darError(error, mensajeDeError(GetLastError()));
    return -1;
}

bool ioCompletado(IoAsincrono *io)
{
    return io != nullptr && io->manejo != nullptr && HasOverlappedIoCompleted(&io->overlapped);
}

std::int64_t resultadoIo(IoAsincrono *io, std::wstring *error)
{
    if (io == nullptr)
        return 0;
    if (io->inmediato)
        return io->bytes;

    DWORD bytes = 0;
    if (GetOverlappedResult(io->manejo, &io->overlapped, &bytes, FALSE))
        return bytes;
    darError(error, mensajeDeError(GetLastError()));
    return -1;
}

std::int64_t leer(void *manejo, void *datos, std::int64_t max, std::wstring *error)
{
    if (manejo == nullptr || max <= 0) {
        darError(error, L"archivo no abierto");
        return -1;
    }

    DWORD pedidos = static_cast<DWORD>(max > 0x7FFFFFFF ? 0x7FFFFFFF : max);
    DWORD leidos = 0;
    if (ReadFile(manejo, datos, pedidos, &leidos, nullptr))
        return std::int64_t(leidos);
    darError(error, mensajeDeError(GetLastError()));
    return -1;
}

bool escribir(void *manejo, const void *datos, std::int64_t n, std::wstring *error)
{
    if (manejo == nullptr || n < 0)
        return darError(error, L"archivo no abierto");

    const char *origen = static_cast<const char *>(datos);
    std::int64_t restante = n;
    while (restante > 0) {
        DWORD pedidos = static_cast<DWORD>(restante > 0x7FFFFFFF ? 0x7FFFFFFF : restante);
        DWORD escritos = 0;
        if (!WriteFile(manejo, origen, pedidos, &escritos, nullptr) || escritos == 0)
            return darError(error, mensajeDeError(GetLastError()));
        origen += escritos;
        restante -= escritos;
    }
    return true;
}

bool buscar(void *manejo, std::uint64_t posicion, std::wstring *error)
{
    if (manejo == nullptr)
        return darError(error, L"archivo no abierto");
    LARGE_INTEGER pos;
    pos.QuadPart = static_cast<LONGLONG>(posicion);
    if (SetFilePointerEx(manejo, pos, nullptr, FILE_BEGIN))
        return true;
    return darError(error, mensajeDeError(GetLastError()));
}

bool vaciar(void *manejo, std::wstring *error)
{
    if (manejo == nullptr)
        return darError(error, L"archivo no abierto");
    if (FlushFileBuffers(manejo))
        return true;
    return darError(error, mensajeDeError(GetLastError()));
}

void cerrar(void *manejo)
{
    if (manejo != nullptr)
        CloseHandle(manejo);
}

bool tamanoDe(const std::wstring &ruta, std::uint64_t *tamano, std::wstring *error)
{
    WIN32_FILE_ATTRIBUTE_DATA datos {};
    if (!GetFileAttributesExW(rutaLarga(ruta).c_str(), GetFileExInfoStandard, &datos))
        return darError(error, mensajeDeError(GetLastError()));
    if (tamano != nullptr)
        *tamano = (std::uint64_t(datos.nFileSizeHigh) << 32) | datos.nFileSizeLow;
    return true;
}

bool eliminar(const std::wstring &ruta, std::wstring *error)
{
    if (DeleteFileW(rutaLarga(ruta).c_str()))
        return true;
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
        return true;
    return darError(error, mensajeDeError(GetLastError()));
}

bool renombrar(const std::wstring &origen, const std::wstring &destino, bool reemplazar,
    std::wstring *error)
{
    if (reemplazar)
        quitarSoloLectura(destino);
    const DWORD flags = MOVEFILE_WRITE_THROUGH
        | (reemplazar ? MOVEFILE_REPLACE_EXISTING : 0);
    if (MoveFileExW(rutaLarga(origen).c_str(), rutaLarga(destino).c_str(), flags))
        return true;
    return darError(error, mensajeDeError(GetLastError()));
}

bool existe(const std::wstring &ruta)
{
    return GetFileAttributesW(rutaLarga(ruta).c_str()) != INVALID_FILE_ATTRIBUTES;
}

void quitarSoloLectura(const std::wstring &ruta)
{
    const std::wstring larga = rutaLarga(ruta);
    const DWORD atributos = GetFileAttributesW(larga.c_str());
    if (atributos != INVALID_FILE_ATTRIBUTES
        && (atributos & FILE_ATTRIBUTE_READONLY) != 0) {
        SetFileAttributesW(larga.c_str(), atributos & ~FILE_ATTRIBUTE_READONLY);
    }
}

bool copiarMetadatos(const std::wstring &origen, const std::wstring &destino,
    std::wstring *error)
{
    const std::wstring a = rutaLarga(origen);
    const std::wstring b = rutaLarga(destino);

    WIN32_FILE_ATTRIBUTE_DATA datos {};
    if (!GetFileAttributesExW(a.c_str(), GetFileExInfoStandard, &datos))
        return darError(error, mensajeDeError(GetLastError()));

    bool fechas = false;
    HANDLE h = CreateFileW(b.c_str(), FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        fechas = SetFileTime(h, &datos.ftCreationTime, &datos.ftLastAccessTime,
                   &datos.ftLastWriteTime)
            != 0;
        CloseHandle(h);
    }

    const bool atributos = SetFileAttributesW(b.c_str(), datos.dwFileAttributes) != 0;
    if (!fechas && !atributos)
        return darError(error, mensajeDeError(GetLastError()));
    return true;
}

} // namespace win32
} // namespace maxcopier

#endif // _WIN32
