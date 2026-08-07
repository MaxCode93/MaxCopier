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

bool crearArchivo(const QString &ruta, char relleno, qint64 bytes = 1024 * 1024)
{
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly))
        return false;
    const QByteArray bloque(1024 * 1024, relleno);
    qint64 escritos = 0;
    while (escritos < bytes) {
        const qint64 parte = qMin<qint64>(bloque.size(), bytes - escritos);
        const qint64 resultado = archivo.write(bloque.constData(), parte);
        if (resultado != parte)
            return false;
        escritos += resultado;
    }
    return true;
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
    for (int i = 0; i < 12; ++i) {
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

    const int porcentajePausado = ventana.porcentajeBandeja();
    ventana.pausarDesdeBandeja();
    const bool reanudada = esperar(aplicacion, [&] { return !ventana.pausada(); }, 1500);
    comprobar(reanudada,
        "reanudar una copia activa vuelve a activar los motores");
    comprobar(esperar(aplicacion, [&] {
        return !ventana.ocupada() || ventana.porcentajeBandeja() > porcentajePausado;
    }, 5000), "reanudar una cola grande hace avanzar de nuevo la transferencia");
    comprobar(esperar(aplicacion, [&] { return !ventana.ocupada(); }, 25000),
        "la copia reanudada termina sin dejar la ventana bloqueada");

    Configuracion configuracionLenta(temporal.filePath(QStringLiteral("config-lenta.mc")));
    configuracionLenta.establecerComprobarEspacioLibre(false);
    configuracionLenta.establecerArchivosALaVez(1);
    // Configuracion normaliza el límite a MiB/s; 1 MiB/s mantiene estos
    // archivos activos el tiempo suficiente para ejercitar la cancelación y
    // el cierre sin hacer lenta la prueba una vez cancelados.
    configuracionLenta.establecerLimiteVelocidad(1 * 1024 * 1024);

    const QString origenCancelacion = temporal.filePath(QStringLiteral("origen-cancelacion.bin"));
    const QString destinoCancelacion = temporal.filePath(QStringLiteral("destino-cancelacion"));
    comprobar(crearArchivo(origenCancelacion, 'c', 32 * 1024 * 1024),
        "se prepara un archivo grande para cancelar");
    comprobar(QDir().mkpath(destinoCancelacion), "se crea el destino de la cancelación");

    VentanaPrincipal ventanaCancelacion(ipc::Operacion::Copiar, &configuracionLenta);
    ventanaCancelacion.show();
    ventanaCancelacion.iniciarCopia(ipc::Operacion::Copiar, { origenCancelacion }, destinoCancelacion);
    comprobar(esperar(aplicacion, [&] { return ventanaCancelacion.copiando(); }, 5000),
        "la transferencia cancelable llega a un motor activo");
    ventanaCancelacion.pausarDesdeBandeja();
    comprobar(esperar(aplicacion, [&] { return ventanaCancelacion.pausada(); }, 2000),
        "la transferencia cancelable puede pausarse antes de cancelar");
    ventanaCancelacion.cancelarDesdeBandeja();
    comprobar(esperar(aplicacion, [&] {
        return !ventanaCancelacion.ocupada() && !ventanaCancelacion.cancelando();
    }, 8000), "cancelar espera al motor y libera el estado de la ventana");

    const QString origenTrasCancelar = temporal.filePath(QStringLiteral("origen-tras-cancelar.bin"));
    comprobar(crearArchivo(origenTrasCancelar, 'r'), "se prepara una segunda copia tras cancelar");
    ventanaCancelacion.iniciarCopia(
        ipc::Operacion::Copiar, { origenTrasCancelar }, destinoCancelacion);
    comprobar(esperar(aplicacion, [&] { return !ventanaCancelacion.ocupada(); }, 8000),
        "la ventana puede iniciar otra copia después de cancelar");
    comprobar(QFileInfo::exists(QDir(destinoCancelacion).filePath(
                    QFileInfo(origenTrasCancelar).fileName())),
        "la segunda copia posterior a la cancelación llega al destino");

    const QString origenCierre = temporal.filePath(QStringLiteral("origen-cierre.bin"));
    const QString destinoCierre = temporal.filePath(QStringLiteral("destino-cierre"));
    comprobar(crearArchivo(origenCierre, 'x', 32 * 1024 * 1024),
        "se prepara un archivo grande para cerrar durante la copia");
    comprobar(QDir().mkpath(destinoCierre), "se crea el destino del cierre");

    auto *ventanaCierre = new VentanaPrincipal(ipc::Operacion::Copiar, &configuracionLenta);
    bool destruida = false;
    QObject::connect(ventanaCierre, &QObject::destroyed, &aplicacion,
        [&destruida] { destruida = true; });
    ventanaCierre->show();
    ventanaCierre->iniciarCopia(ipc::Operacion::Copiar, { origenCierre }, destinoCierre);
    comprobar(esperar(aplicacion, [&] { return ventanaCierre->copiando(); }, 5000),
        "la ventana llega a copiar antes de solicitar el cierre");
    ventanaCierre->close();
    comprobar(esperar(aplicacion, [&] { return destruida; }, 10000),
        "cerrar durante una copia espera a que terminen los hilos antes de destruir la ventana");

    std::printf(fallos == 0 ? "\nTodo bien.\n" : "\n%d comprobaciones falladas.\n", fallos);
    return fallos == 0 ? 0 : 1;
}
