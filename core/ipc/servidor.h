#pragma once

/// Lado que escucha del canal local, en Win32 puro (ver el porqué en
/// `tuberia.h`). Lo usa la app dentro de `InstanciaUnica`; aquí no entra Qt
/// para que se pueda probar sin arrastrar la interfaz detrás.

#ifdef _WIN32

#include "tuberia.h"

#include <atomic>
#include <functional>
#include <string>

namespace maxcopier::ipc {

class Servidor {
public:
    /// Se llama desde el hilo del canal con cada mensaje completo: quien lo
    /// reciba tiene que saltar a su propio hilo por su cuenta.
    using Entrega = std::function<void(const std::string &)>;

    explicit Servidor(Entrega entrega)
        : m_entrega(std::move(entrega))
    {
        // Testigo de instancia única: el primero que lo crea se queda el canal.
        // Si ya existe (o lo tiene un MaxCopier elevado, y entonces ni siquiera
        // se puede abrir), esta instancia se limita a mandarle la petición.
        const std::string canal = nombreDeCanal(); // ASCII: ensanchar es exacto
        const std::wstring nombre = L"Local\\" + std::wstring(canal.begin(), canal.end()) + L"-instancia";
        m_testigo = CreateMutexW(nullptr, FALSE, nombre.c_str());
        m_error = GetLastError();
        if (!m_testigo || m_error == ERROR_ALREADY_EXISTS) {
            if (m_testigo) {
                CloseHandle(m_testigo);
                m_testigo = nullptr;
            }
            return;
        }
        m_error = ERROR_SUCCESS;

        DWORD error = ERROR_SUCCESS;
        HANDLE tuberia = crearInstanciaDeCanal(true, error);
        m_error = error;
        if (tuberia == INVALID_HANDLE_VALUE)
            return; // ERROR_ACCESS_DENIED: el canal ya es de otra instancia

        m_parada = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!m_parada) {
            m_error = GetLastError();
            CloseHandle(tuberia);
            return;
        }

        m_escuchando = true;
        m_primera = tuberia;
        // Hilo de Win32 y no `std::thread`: así esto compila igual con las
        // versiones de MinGW que no traen los hilos de la biblioteca estándar.
        m_hilo = CreateThread(nullptr, 0, &Servidor::arranque, this, 0, nullptr);
        if (!m_hilo) {
            m_error = GetLastError();
            m_escuchando = false;
            CloseHandle(tuberia);
            CloseHandle(m_parada);
            m_parada = nullptr;
        }
    }

    ~Servidor()
    {
        if (m_testigo)
            CloseHandle(m_testigo);
        if (m_parada)
            SetEvent(m_parada);
        if (m_hilo) {
            WaitForSingleObject(m_hilo, INFINITE);
            CloseHandle(m_hilo);
        }
        if (m_parada)
            CloseHandle(m_parada);
    }

    Servidor(const Servidor &) = delete;
    Servidor &operator=(const Servidor &) = delete;

    /// `false` si el canal ya era de otro MaxCopier (o no se pudo abrir).
    bool escuchando() const { return m_escuchando; }

    /// Error de Windows al abrir el canal, para el registro de diagnóstico.
    DWORD error() const { return m_error; }

private:
    static DWORD WINAPI arranque(LPVOID parametro)
    {
        auto *servidor = static_cast<Servidor *>(parametro);
        servidor->bucle(servidor->m_primera);
        return 0;
    }

    void bucle(HANDLE primera)
    {
        HANDLE tuberia = primera;

        while (tuberia != INVALID_HANDLE_VALUE && tuberia != nullptr) {
            OVERLAPPED solapada = {};
            solapada.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (!solapada.hEvent) {
                CloseHandle(tuberia);
                break;
            }

            bool conectado = ConnectNamedPipe(tuberia, &solapada) != FALSE;
            bool cortar = false;
            if (!conectado) {
                const DWORD error = GetLastError();
                if (error == ERROR_PIPE_CONNECTED) {
                    conectado = true;
                } else if (error == ERROR_IO_PENDING) {
                    HANDLE esperas[2] = { solapada.hEvent, m_parada };
                    if (WaitForMultipleObjects(2, esperas, FALSE, INFINITE) == WAIT_OBJECT_0) {
                        DWORD nada = 0;
                        conectado = GetOverlappedResult(tuberia, &solapada, &nada, FALSE) != FALSE;
                    } else {
                        cortar = true;
                    }
                } else {
                    cortar = true;
                }
            }

            if (cortar) {
                CancelIo(tuberia);
                CloseHandle(solapada.hEvent);
                CloseHandle(tuberia);
                break;
            }

            // La siguiente instancia se abre *antes* de atender a esta: si no,
            // entre una y otra el nombre de la tubería no existe y a quien
            // llame justo entonces le dicen que MaxCopier no está.
            HANDLE siguiente = INVALID_HANDLE_VALUE;
            if (WaitForSingleObject(m_parada, 0) != WAIT_OBJECT_0) {
                DWORD error = ERROR_SUCCESS;
                siguiente = crearInstanciaDeCanal(false, error);
                if (siguiente == INVALID_HANDLE_VALUE)
                    m_error = error;
            }

            if (conectado)
                atender(tuberia);

            FlushFileBuffers(tuberia);
            DisconnectNamedPipe(tuberia);
            CloseHandle(solapada.hEvent);
            CloseHandle(tuberia);

            tuberia = siguiente;
        }

        m_escuchando = false;
    }

    void atender(HANDLE tuberia)
    {
        // El mensaje puede llegar troceado: se acumula hasta tenerlo entero.
        std::string mensaje;
        char trozo[4096];
        long completo = 0;
        while (true) {
            completo = tamanoDeMensaje(mensaje.data(), mensaje.size());
            if (completo < 0 || (completo > 0 && mensaje.size() >= size_t(completo)))
                break;
            if (mensaje.size() > size_t(kTamanoMaximo)) {
                completo = -1;
                break;
            }

            DWORD leidos = 0;
            if (!detalle::leerDeTuberia(tuberia, trozo, DWORD(sizeof(trozo)), leidos))
                return;
            mensaje.append(trozo, leidos);
        }

        if (completo <= 0)
            return;

        mensaje.resize(size_t(completo));
        detalle::escribirEnTuberia(tuberia, kConforme, DWORD(sizeof(kConforme)));

        if (m_entrega)
            m_entrega(mensaje);
    }

    Entrega m_entrega;
    HANDLE m_testigo = nullptr; ///< mutex con nombre: «esta app es la primera»
    HANDLE m_primera = nullptr; ///< instancia con la que arranca el bucle
    HANDLE m_parada = nullptr;
    HANDLE m_hilo = nullptr;
    std::atomic_bool m_escuchando { false };
    std::atomic<DWORD> m_error { 0 };
};

} // namespace maxcopier::ipc

#endif // _WIN32
