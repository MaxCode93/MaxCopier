#include "cliente.h"

#include "modulo.h"
#include "registro.h"

#include "ipc/tuberia.h"

#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>

#include <string>

namespace maxcopier::shell {
namespace {

    constexpr int kIntentosTrasArrancar = 25; ///< ~5 s, lo que tarda Qt en abrir el canal
    constexpr DWORD kMsEntreIntentos = 200;

    std::wstring carpetaDeDatos()
    {
        wchar_t *local = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local)))
            return {};
        std::wstring carpeta(local);
        CoTaskMemFree(local);
        carpeta += L"\\MaxCopier";
        CreateDirectoryW(carpeta.c_str(), nullptr);
        return carpeta;
    }

    /// Deja constancia de lo que pasa aquí dentro: esto corre en el Explorador,
    /// donde no hay forma de ver un mensaje de error que no sea un cuadro.
    void anotar(const std::wstring &texto)
    {
        const std::wstring carpeta = carpetaDeDatos();
        if (carpeta.empty())
            return;

        const std::wstring ruta = carpeta + L"\\shell.log";
        HANDLE archivo = CreateFileW(ruta.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (archivo == INVALID_HANDLE_VALUE)
            return;

        SYSTEMTIME ahora = {};
        GetLocalTime(&ahora);
        wchar_t marca[32] = { 0 };
        wsprintfW(marca, L"%04d-%02d-%02d %02d:%02d:%02d ", ahora.wYear, ahora.wMonth, ahora.wDay,
            ahora.wHour, ahora.wMinute, ahora.wSecond);

        const std::wstring linea = marca + texto + L"\r\n";
        const int octetos
            = WideCharToMultiByte(CP_UTF8, 0, linea.c_str(), int(linea.size()), nullptr, 0, nullptr, nullptr);
        if (octetos > 0) {
            std::string utf8(size_t(octetos), '\0');
            WideCharToMultiByte(
                CP_UTF8, 0, linea.c_str(), int(linea.size()), utf8.data(), octetos, nullptr, nullptr);
            DWORD escritos = 0;
            WriteFile(archivo, utf8.data(), DWORD(utf8.size()), &escritos, nullptr);
        }
        CloseHandle(archivo);
    }

    std::wstring conNumero(const std::wstring &texto, unsigned long numero)
    {
        wchar_t cifras[16] = { 0 };
        wsprintfW(cifras, L"%lu", numero);
        return texto + cifras;
    }

    /// Dónde está MaxCopier.exe: primero lo que apuntó la instalación, y si no,
    /// al lado de la DLL (que es donde lo deja el paquete).
    std::wstring rutaDeLaApp()
    {
        const std::wstring registrada = rutaRegistradaDeLaApp();
        if (!registrada.empty() && PathFileExistsW(registrada.c_str()))
            return registrada;

        wchar_t ruta[MAX_PATH] = { 0 };
        if (!GetModuleFileNameW(moduloDeLaExtension(), ruta, MAX_PATH))
            return {};
        if (!PathRemoveFileSpecW(ruta))
            return {};
        std::wstring aplicacion(ruta);
        aplicacion += L"\\MaxCopier.exe";
        return PathFileExistsW(aplicacion.c_str()) ? aplicacion : std::wstring();
    }

    bool lanzar(const std::wstring &aplicacion, const std::wstring &argumentos)
    {
        std::wstring orden = L"\"" + aplicacion + L"\"";
        if (!argumentos.empty())
            orden += L" " + argumentos;

        // Que la app pueda ponerse delante aunque quien manda es el Explorador.
        AllowSetForegroundWindow(ASFW_ANY);

        STARTUPINFOW arranque = {};
        arranque.cb = sizeof(STARTUPINFOW);
        PROCESS_INFORMATION proceso = {};
        if (!CreateProcessW(aplicacion.c_str(), orden.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                &arranque, &proceso)) {
            anotar(conNumero(L"no se ha podido arrancar MaxCopier.exe, error ", GetLastError()));
            return false;
        }
        CloseHandle(proceso.hThread);
        CloseHandle(proceso.hProcess);
        return true;
    }

    /// Guarda la petición en un archivo y se la pasa a MaxCopier por la línea
    /// de órdenes. Es el respaldo de cuando el canal no responde: la copia se
    /// hace igual, aunque la app tenga que abrir su propia ventana.
    bool lanzarConPeticion(const std::string &mensaje)
    {
        const std::wstring aplicacion = rutaDeLaApp();
        if (aplicacion.empty()) {
            anotar(L"no se encuentra MaxCopier.exe: ni en el registro ni al lado de la DLL");
            return false;
        }

        const std::wstring carpeta = carpetaDeDatos();
        if (carpeta.empty())
            return false;

        wchar_t nombre[64] = { 0 };
        wsprintfW(nombre, L"\\peticion-%lu-%lu.mxc", GetCurrentProcessId(), GetTickCount());
        const std::wstring ruta = carpeta + nombre;

        HANDLE archivo = CreateFileW(
            ruta.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (archivo == INVALID_HANDLE_VALUE) {
            anotar(conNumero(L"no se ha podido escribir la petición, error ", GetLastError()));
            return false;
        }
        DWORD escritos = 0;
        const bool bien = WriteFile(archivo, mensaje.data(), DWORD(mensaje.size()), &escritos, nullptr)
            && escritos == mensaje.size();
        CloseHandle(archivo);
        if (!bien) {
            DeleteFileW(ruta.c_str());
            return false;
        }

        return lanzar(aplicacion, L"--peticion \"" + ruta + L"\"");
    }

    /// El envío se hace en su propio hilo: el Explorador no se puede quedar
    /// esperando a que MaxCopier arranque. Mientras el hilo vive, la DLL queda
    /// retenida para que no la descarguen debajo.
    DWORD WINAPI hiloDeEnvio(LPVOID parametro)
    {
        std::string *mensaje = static_cast<std::string *>(parametro);

        AllowSetForegroundWindow(ASFW_ANY);
        DWORD error = ERROR_SUCCESS;
        bool enviado = ipc::escribirEnCanal(*mensaje, error);

        if (!enviado) {
            anotar(conNumero(L"el canal no ha contestado, error ", error));
            // Sin nadie al otro lado, la app está cerrada: se arranca con la
            // petición puesta, así no depende de que el canal llegue a tiempo.
            if (lanzarConPeticion(*mensaje)) {
                enviado = true;
            } else if (error == ERROR_ACCESS_DENIED) {
                // La app está corriendo pero no nos deja escribir (elevada con
                // una versión antigua): se reintenta un rato por si se reabre.
                for (int intento = 0; intento < kIntentosTrasArrancar && !enviado; ++intento) {
                    Sleep(kMsEntreIntentos);
                    enviado = ipc::escribirEnCanal(*mensaje, error);
                }
            }
        }

        if (!enviado) {
            anotar(conNumero(L"petición perdida, último error ", error));
            std::wstring aviso = L"No se ha podido hablar con MaxCopier.\n\n";
            aviso += conNumero(L"Canal: " + ipc::rutaDeCanal() + L"\nError de Windows: ", error);
            aviso += L"\n\nHay más detalles en %LOCALAPPDATA%\\MaxCopier\\shell.log";
            MessageBoxW(nullptr, aviso.c_str(), L"MaxCopier", MB_OK | MB_ICONWARNING);
        }

        delete mensaje;
        soltarModulo();
        return 0;
    }

} // namespace

bool enviarPeticion(
    ipc::Operacion operacion, const std::vector<std::wstring> &origenes,
    const std::wstring &destino, bool desdePortapapeles)
{
    if (origenes.empty())
        return false;

    ipc::Peticion peticion;
    peticion.operacion = operacion;
    peticion.origenes.reserve(origenes.size());
    for (const std::wstring &origen : origenes) // wchar_t es UTF-16 en Windows
        peticion.origenes.emplace_back(origen.begin(), origen.end());
    peticion.destino.assign(destino.begin(), destino.end());
    peticion.desdePortapapeles = desdePortapapeles;

    auto *mensaje = new std::string(ipc::serializar(peticion));
    retenerModulo();
    HANDLE hilo = CreateThread(nullptr, 0, hiloDeEnvio, mensaje, 0, nullptr);
    if (!hilo) {
        delete mensaje;
        soltarModulo();
        return false;
    }
    CloseHandle(hilo);
    return true;
}

} // namespace maxcopier::shell
