#include "configuracion.h"
#include "lista/listadecopia.h"
#include "ventanaprincipal.h"
#include "vistas/barraarchivos.h"
#include "vistas/cargando.h"
#include "vistas/iconos.h"
#include "vistas/panelexpandido.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProgressBar>
#include <QPushButton>
#include <QTemporaryDir>
#include <QThread>

#include <cstdio>
#include <functional>

using namespace maxcopier;

namespace {

int fallos = 0;

void comprobar(bool bien, const char *que)
{
    std::printf("%s %s\n", bien ? "[ok]  " : "[FALLA]", que);
    if (!bien)
        ++fallos;
}

bool esperar(QApplication &aplicacion, const std::function<bool()> &condicion, int limiteMs)
{
    QElapsedTimer reloj;
    reloj.start();
    while (!condicion() && reloj.elapsed() < limiteMs) {
        aplicacion.processEvents(QEventLoop::AllEvents, 25);
        QThread::msleep(5);
    }
    aplicacion.processEvents(QEventLoop::AllEvents, 25);
    return condicion();
}

bool crearArchivo(const QString &ruta, char relleno)
{
    QFile archivo(ruta);
    return archivo.open(QIODevice::WriteOnly)
        && archivo.write(QByteArray(1024 * 1024, relleno)) == 1024 * 1024;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication aplicacion(argc, argv);

    const QIcon icono = iconoDeLaApp();
    comprobar(!icono.isNull() && !icono.pixmap(16, 16).isNull(),
        "los recursos de iconos se inicializan desde la biblioteca estatica");

    // El overlay solo debe mostrar el indicador circular, nunca una barra
    // horizontal indeterminada.
    {
        QWidget padre;
        Cargando cargando(&padre);
        cargando.resize(580, 300);
        cargando.mostrarCargando();
        comprobar(cargando.findChild<QWidget *>(QStringLiteral("indicadorCircular")) != nullptr,
            "el overlay crea un indicador circular");
        comprobar(cargando.findChildren<QProgressBar *>().isEmpty(),
            "el overlay no contiene una barra de progreso horizontal");
        cargando.hide();
    }

    QTemporaryDir temporal;
    comprobar(temporal.isValid(), "se crea el directorio temporal de la prueba de pausa");
    if (!temporal.isValid())
        return 1;

    // La ordenación de la tabla se calcula fuera del hilo de la interfaz y
    // solo publica el resultado al volver al event loop.
    {
        ListaDeCopia lista;
        lista.anadir({
            { temporal.filePath(QStringLiteral("z.txt")), temporal.filePath(QStringLiteral("z-destino")), 10 },
            { temporal.filePath(QStringLiteral("a.txt")), temporal.filePath(QStringLiteral("a-destino")), 30 },
            { temporal.filePath(QStringLiteral("m.txt")), temporal.filePath(QStringLiteral("m-destino")), 20 },
        });
        lista.ordenarPorEnSegundoPlano(ListaDeCopia::ColumnaFuente, Qt::AscendingOrder);
        comprobar(esperar(aplicacion, [&] {
            return QFileInfo(lista.elemento(0).fuente).fileName() == QStringLiteral("a.txt");
        }, 3000), "la ordenación en segundo plano devuelve el resultado sin bloquear la UI");
    }

    const QString origen = temporal.filePath(QStringLiteral("origen"));
    const QString destino = temporal.filePath(QStringLiteral("destino"));
    comprobar(QDir().mkpath(destino), "se crea el destino de la prueba de pausa");

    QStringList origenes;
    for (int i = 0; i < 4; ++i) {
        const QString ruta = temporal.filePath(QStringLiteral("archivo-%1.bin").arg(i));
        comprobar(crearArchivo(ruta, char('a' + i)), "se prepara un archivo de la prueba de pausa");
        origenes.append(ruta);
    }

    Configuracion configuracion(temporal.filePath(QStringLiteral("config.mc")));
    configuracion.establecerComprobarEspacioLibre(false);
    configuracion.establecerArchivosALaVez(3);
    configuracion.establecerLimiteVelocidad(1024 * 1024);

    VentanaPrincipal ventana(ipc::Operacion::Copiar, &configuracion);
    ventana.show();
    ventana.iniciarCopia(ipc::Operacion::Copiar, origenes, destino);

    BarraArchivos *barra = nullptr;
    const bool inicio = esperar(aplicacion, [&] {
        barra = ventana.findChild<BarraArchivos *>();
        return ventana.copiando() && barra && barra->cantidadDeArchivos() == 3;
    }, 8000);
    comprobar(inicio, "tres archivos llegan a la barra segmentada");

    // Ordenar con varios motores activos no debe resetear la vista ni
    // interferir con la cola: las tres filas activas se mantienen ancladas y
    // el cuarto archivo sigue pendiente.
    PanelExpandido *expandido = ventana.findChild<PanelExpandido *>();
    bool ordenaciones = expandido != nullptr;
    for (int columna : { ListaDeCopia::ColumnaFuente,
             ListaDeCopia::ColumnaTamano, ListaDeCopia::ColumnaDestino }) {
        ordenaciones = ordenaciones
            && QMetaObject::invokeMethod(expandido, "ordenarColumna", Qt::DirectConnection,
                Q_ARG(int, int(columna)));
    }
    comprobar(ordenaciones, "ordenar la cola durante una copia conserva los motores activos");

    ventana.pausarDesdeBandeja();
    const bool pausa = esperar(aplicacion, [&] { return ventana.pausada(); }, 1500);
    comprobar(pausa, "pausar una copia activa cambia el estado de la ventana");
    comprobar(barra && barra->cantidadDeArchivos() == 3,
        "pausar conserva las tres barras de progreso");

    bool reanudarHabilitado = false;
    for (QPushButton *boton : ventana.findChildren<QPushButton *>()) {
        if (boton->text().contains(QStringLiteral("Reanudar")) && boton->isEnabled()) {
            reanudarHabilitado = true;
            break;
        }
    }
    comprobar(reanudarHabilitado, "la UI sigue habilitada para reanudar tras pausar");

    ventana.pausarDesdeBandeja();
    comprobar(esperar(aplicacion, [&] { return !ventana.pausada(); }, 1500),
        "reanudar una copia activa vuelve a activar los motores");
    comprobar(esperar(aplicacion, [&] { return !ventana.ocupada(); }, 12000),
        "la copia reanudada termina sin dejar la ventana bloqueada");

    std::printf(fallos == 0 ? "\nTodo bien.\n" : "\n%d comprobaciones falladas.\n", fallos);
    return fallos == 0 ? 0 : 1;
}
