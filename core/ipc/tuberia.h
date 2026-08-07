#pragma once

/// Tubería con nombre del canal local, en Win32 puro: la comparten la app (que
/// la crea y escucha) y la DLL del Explorador (que escribe). Aquí no puede
/// entrar nada de Qt, igual que en `protocolo.h`.
///
/// Antes la app abría el canal con `QLocalServer`, que crea la tubería con la
/// seguridad por defecto. Eso deja fuera al Explorador en cuanto los dos
/// procesos no están al mismo nivel de integridad (MaxCopier abierto «como
/// administrador» y el Explorador normal, que es lo corriente): el Explorador
/// recibía ERROR_ACCESS_DENIED al abrirla y solo se veía «no se ha podido
/// hablar con MaxCopier». Por eso la tubería se crea aquí a mano, con una lista
/// de control que da paso al usuario de la sesión y con la etiqueta de
/// integridad baja, que es lo que permite escribir «hacia arriba».

#ifdef _WIN32

#include "protocolo.h"

#include <windows.h>

#include <sddl.h>

namespace maxcopier::ipc {

inline constexpr DWORD kMsEsperaCanal = 5000;
inline constexpr DWORD kBufferDeCanal = 64u * 1024u;

namespace detalle {

    /// SID del usuario de este proceso, en texto («S-1-5-21-…»).
    inline std::wstring sidDelUsuario()
    {
        HANDLE testigo = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &testigo))
            return {};

        DWORD tamano = 0;
        GetTokenInformation(testigo, TokenUser, nullptr, 0, &tamano);
        if (tamano == 0) {
            CloseHandle(testigo);
            return {};
        }

        std::string espacio(tamano, '\0');
        auto *usuario = reinterpret_cast<TOKEN_USER *>(espacio.data());
        std::wstring sid;
        if (GetTokenInformation(testigo, TokenUser, usuario, tamano, &tamano)) {
            wchar_t *texto = nullptr;
            if (ConvertSidToStringSidW(usuario->User.Sid, &texto)) {
                sid = texto;
                LocalFree(texto);
            }
        }
        CloseHandle(testigo);
        return sid;
    }

    /// Descriptor de seguridad de la tubería: paso para el sistema, los
    /// administradores y el usuario de la sesión, y etiqueta de integridad
    /// baja (`S:(ML;;NW;;;LW)`) para que el Explorador pueda escribir aunque
    /// MaxCopier esté elevado.
    inline PSECURITY_DESCRIPTOR descriptorDelCanal()
    {
        std::wstring texto = L"D:(A;;GA;;;SY)(A;;GA;;;BA)";
        const std::wstring usuario = sidDelUsuario();
        if (!usuario.empty())
            texto += L"(A;;GA;;;" + usuario + L")";
        else
            texto += L"(A;;GA;;;IU)"; // sin SID, al menos los que han iniciado sesión
        texto += L"S:(ML;;NW;;;LW)";

        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                texto.c_str(), SDDL_REVISION_1, &descriptor, nullptr))
            return nullptr;
        return descriptor;
    }

    /// Espera a que termine una operación solapada sin quedarse colgada.
    inline bool esperarSolapada(HANDLE archivo, OVERLAPPED &solapada, DWORD ms, DWORD &transferidos)
    {
        if (WaitForSingleObject(solapada.hEvent, ms) != WAIT_OBJECT_0) {
            CancelIo(archivo);
            WaitForSingleObject(solapada.hEvent, INFINITE);
            SetLastError(WAIT_TIMEOUT);
            return false;
        }
        return GetOverlappedResult(archivo, &solapada, &transferidos, FALSE) != FALSE;
    }

    /// Las dos puntas del canal abren la tubería como solapada, así que hasta
    /// las lecturas y escrituras cortas pasan por aquí: nadie se queda colgado
    /// esperando al otro lado.
    inline bool leerDeTuberia(HANDLE tuberia, char *destino, DWORD cuanto, DWORD &leidos)
    {
        OVERLAPPED solapada = {};
        solapada.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!solapada.hEvent)
            return false;

        bool bien = true;
        if (!ReadFile(tuberia, destino, cuanto, &leidos, &solapada)) {
            if (GetLastError() == ERROR_IO_PENDING)
                bien = esperarSolapada(tuberia, solapada, kMsEsperaCanal, leidos);
            else
                bien = false;
        }
        const DWORD error = GetLastError();
        CloseHandle(solapada.hEvent);
        SetLastError(error);
        return bien && leidos > 0;
    }

    inline bool escribirEnTuberia(HANDLE tuberia, const char *datos, DWORD cuanto)
    {
        OVERLAPPED solapada = {};
        solapada.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!solapada.hEvent)
            return false;

        bool bien = true;
        DWORD escritos = 0;
        while (bien && escritos < cuanto) {
            ResetEvent(solapada.hEvent);
            DWORD ahora = 0;
            if (WriteFile(tuberia, datos + escritos, cuanto - escritos, &ahora, &solapada)) {
                // Ha terminado sin esperar.
            } else if (GetLastError() == ERROR_IO_PENDING) {
                bien = esperarSolapada(tuberia, solapada, kMsEsperaCanal, ahora);
            } else {
                bien = false;
            }
            if (ahora == 0)
                bien = false;
            escritos += ahora;
        }
        const DWORD error = GetLastError();
        CloseHandle(solapada.hEvent);
        SetLastError(error);
        return bien;
    }

} // namespace detalle

/// Crea una instancia de la tubería. Con `primera` se pide además que no
/// hubiera ninguna: es la forma atómica de saber si esta es la única app.
inline HANDLE crearInstanciaDeCanal(bool primera, DWORD &error)
{
    PSECURITY_DESCRIPTOR descriptor = detalle::descriptorDelCanal();
    SECURITY_ATTRIBUTES atributos = {};
    atributos.nLength = sizeof(atributos);
    atributos.lpSecurityDescriptor = descriptor;
    atributos.bInheritHandle = FALSE;

    DWORD modo = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
    if (primera)
        modo |= FILE_FLAG_FIRST_PIPE_INSTANCE;

    const std::wstring ruta = rutaDeCanal();
    HANDLE tuberia = CreateNamedPipeW(ruta.c_str(), modo, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, kBufferDeCanal, kBufferDeCanal, kMsEsperaCanal,
        descriptor ? &atributos : nullptr);
    error = tuberia == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;

    if (descriptor)
        LocalFree(descriptor);
    return tuberia;
}

/// Manda un mensaje ya serializado y espera el «MXOK» de la app. Devuelve
/// `false` (y el error de Windows) si no hay nadie escuchando, si la app no
/// tiene permiso o si no contesta a tiempo.
inline bool escribirEnCanal(const std::string &mensaje, DWORD &error)
{
    const std::wstring ruta = rutaDeCanal();

    // Un par de reintentos cortos: la app puede estar justo entre una conexión
    // y la siguiente, y ese instante no significa que no esté.
    HANDLE tuberia = INVALID_HANDLE_VALUE;
    for (int intento = 0; intento < 4 && tuberia == INVALID_HANDLE_VALUE; ++intento) {
        tuberia = CreateFileW(ruta.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED, nullptr);
        if (tuberia != INVALID_HANDLE_VALUE)
            break;

        error = GetLastError();
        if (error == ERROR_PIPE_BUSY) { // ocupada: la app abre otra en cuanto puede
            if (!WaitNamedPipeW(ruta.c_str(), kMsEsperaCanal)) {
                error = GetLastError();
                return false;
            }
        } else if (error == ERROR_FILE_NOT_FOUND) {
            Sleep(100);
        } else {
            return false;
        }
    }
    if (tuberia == INVALID_HANDLE_VALUE)
        return false;

    bool bien = detalle::escribirEnTuberia(tuberia, mensaje.data(), DWORD(mensaje.size()));

    if (bien) { // la app confirma que ha entendido la petición
        char respuesta[sizeof(kConforme)] = { 0 };
        DWORD leidos = 0;
        bien = detalle::leerDeTuberia(tuberia, respuesta, DWORD(sizeof(respuesta)), leidos)
            && leidos == sizeof(kConforme) && std::memcmp(respuesta, kConforme, sizeof(kConforme)) == 0;
        if (!bien && GetLastError() == ERROR_SUCCESS)
            SetLastError(ERROR_INVALID_DATA);
    }

    error = bien ? ERROR_SUCCESS : GetLastError();
    CloseHandle(tuberia);
    return bien;
}

} // namespace maxcopier::ipc

#endif // _WIN32
