/// Prueba del canal local sin Qt de por medio: levanta el servidor que usa la
/// app (`core/ipc/servidor.h`) y le escribe como lo hace la extensión del
/// Explorador (`core/ipc/tuberia.h`). Es lo único de MaxCopier que se puede
/// comprobar sin un usuario delante, y es justo la pieza que fallaba.

#include "ipc/servidor.h"
#include "ipc/tuberia.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace maxcopier;

namespace {

int fallos = 0;

void comprobar(bool bien, const char *que)
{
    std::printf("%s %s\n", bien ? "[ok]  " : "[FALLA]", que);
    if (!bien)
        ++fallos;
}

/// Como el servidor, la prueba se queda en Win32 puro (ver `servidor.h`).
class Cerrojo {
public:
    Cerrojo() { InitializeCriticalSection(&m_seccion); }
    ~Cerrojo() { DeleteCriticalSection(&m_seccion); }
    void tomar() { EnterCriticalSection(&m_seccion); }
    void soltar() { LeaveCriticalSection(&m_seccion); }

private:
    CRITICAL_SECTION m_seccion;
};

ipc::Peticion peticionDePrueba()
{
    ipc::Peticion peticion;
    peticion.operacion = ipc::Operacion::Mover;
    peticion.origenes.push_back(u"C:\\Documentos\\camión ñandú.iso");
    peticion.origenes.push_back(u"D:\\Fotos\\день.png");
    peticion.destino = u"E:\\Copias";
    peticion.desdePortapapeles = true;
    return peticion;
}

} // namespace

int main()
{
    const std::string nombre = ipc::nombreDeCanal();
    bool soloAscii = !nombre.empty();
    for (char caracter : nombre)
        soloAscii = soloAscii && static_cast<unsigned char>(caracter) < 0x80;
    comprobar(soloAscii, "el nombre del canal es ASCII puro");

    const ipc::Peticion mandada = peticionDePrueba();
    Cerrojo cerrojo;
    std::vector<std::string> recibidos;

    {
        ipc::Servidor servidor([&](const std::string &mensaje) {
            cerrojo.tomar();
            recibidos.push_back(mensaje);
            cerrojo.soltar();
        });
        comprobar(servidor.escuchando(), "el servidor abre el canal");

        { // el segundo servidor tiene que ver que el canal ya es de otro
            ipc::Servidor segundo([](const std::string &) {});
            comprobar(!segundo.escuchando(), "un segundo servidor no se queda el canal");
        }

        DWORD error = ERROR_SUCCESS;
        comprobar(
            ipc::escribirEnCanal(ipc::serializar(mandada), error), "el cliente entrega la petición");
        if (error != ERROR_SUCCESS)
            std::printf("       error de Windows: %lu\n", error);

        // Tres peticiones seguidas: el servidor tiene que reabrir la tubería
        // sin dejar ni un hueco en el que el canal parezca no existir.
        for (int i = 0; i < 3; ++i)
            ipc::escribirEnCanal(ipc::serializar(mandada), error);

        for (int espera = 0; espera < 50; ++espera) {
            cerrojo.tomar();
            const size_t cuantas = recibidos.size();
            cerrojo.soltar();
            if (cuantas >= 4)
                break;
            Sleep(100);
        }
    }

    cerrojo.tomar();
    std::printf("       peticiones recibidas: %u\n", unsigned(recibidos.size()));
    comprobar(recibidos.size() == 4, "llegan las cuatro peticiones");

    bool iguales = !recibidos.empty();
    for (const std::string &mensaje : recibidos) {
        ipc::Peticion llegada;
        iguales = iguales && ipc::deserializar(mensaje.data(), mensaje.size(), llegada)
            && llegada.operacion == mandada.operacion && llegada.origenes == mandada.origenes
            && llegada.destino == mandada.destino
            && llegada.desdePortapapeles == mandada.desdePortapapeles;
    }
    comprobar(iguales, "las rutas llegan enteras (acentos y cirílico incluidos)");
    cerrojo.soltar();

    // Con la app cerrada el cliente tiene que rendirse pronto (y así la
    // extensión pasa a arrancarla) en vez de dejar colgado al Explorador.
    const DWORD antes = GetTickCount();
    DWORD error = ERROR_SUCCESS;
    const bool colada = ipc::escribirEnCanal(ipc::serializar(mandada), error);
    const DWORD tardanza = GetTickCount() - antes;
    std::printf("       tardanza sin app: %lu ms (error %lu)\n", tardanza, error);
    comprobar(!colada && tardanza < 3000, "sin app detrás el cliente falla enseguida");

    std::printf(fallos == 0 ? "\nTodo bien.\n" : "\n%d comprobaciones falladas.\n", fallos);
    return fallos == 0 ? 0 : 1;
}
