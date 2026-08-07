#include "diagnostico.h"
#include "configuracion.h"
#include "gestordeventanas.h"
#include "instanciaunica.h"
#include "ipc/protocolo.h"
#include "vistas/iconos.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QMessageBox>

namespace {

/// Lee una petición serializada de un archivo (la extensión del Explorador la
/// deja ahí cuando no consigue hablar por el canal) y la borra al leerla.
bool leerPeticionDeArchivo(const QString &ruta, maxcopier::Operacion &operacion, QStringList &origenes,
    QString &carpetaDestino, bool &desdePortapapeles)
{
    QFile archivo(ruta);
    if (!archivo.open(QIODevice::ReadOnly))
        return false;
    const QByteArray datos = archivo.readAll();
    archivo.close();
    QFile::remove(ruta);

    maxcopier::ipc::Peticion peticion;
    if (!maxcopier::ipc::deserializar(datos.constData(), size_t(datos.size()), peticion))
        return false;

    operacion = peticion.operacion;
    desdePortapapeles = peticion.desdePortapapeles;
    origenes.clear();
    for (const std::u16string &origen : peticion.origenes)
        origenes.append(QString::fromStdU16String(origen));
    carpetaDestino = QString::fromStdU16String(peticion.destino);
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication aplicacion(argc, argv);
    aplicacion.setApplicationName(QStringLiteral("MaxCopier"));
    aplicacion.setApplicationVersion(QStringLiteral(MAXCOPIER_VERSION));
    aplicacion.setOrganizationName(QStringLiteral("MaxCopier"));
    aplicacion.setWindowIcon(maxcopier::iconoDeLaApp());
    // Con la bandeja hay ventanas escondidas que siguen copiando: el gestor
    // controla la salida explícita y espera a que todos sus hilos terminen.
    aplicacion.setQuitOnLastWindowClosed(false);

    maxcopier::Configuracion configuracion;
    maxcopier::aplicarTema(configuracion.temaEfectivo());
    QObject::connect(&configuracion, &maxcopier::Configuracion::temaCambiado,
        &aplicacion, [&configuracion] { maxcopier::aplicarTema(configuracion.temaEfectivo()); });
    QObject::connect(&configuracion, &maxcopier::Configuracion::errorAlGuardar,
        &aplicacion, [](const QString &ruta) {
            QMessageBox::warning(nullptr, QObject::tr("Configuración no guardada"),
                QObject::tr("No se pudo escribir config.mc junto al ejecutable:\n%1\n\n"
                             "Comprueba los permisos de la carpeta.")
                    .arg(ruta));
        });

    QCommandLineParser opciones;
    opciones.setApplicationDescription(
        QCoreApplication::translate("main", "Copiador de archivos rapido y ligero."));
    opciones.addHelpOption();
    opciones.addVersionOption();
    const QCommandLineOption peticion(QStringLiteral("peticion"),
        QCoreApplication::translate("main", "Archivo con una peticion de copia del Explorador."),
        QCoreApplication::translate("main", "archivo"));
    opciones.addOption(peticion);
    const QCommandLineOption mover({ QStringLiteral("m"), QStringLiteral("move"), QStringLiteral("mover") },
        QCoreApplication::translate("main", "Mover los orígenes en lugar de copiarlos."));
    opciones.addOption(mover);
    opciones.addPositionalArgument(QCoreApplication::translate("main", "origen..."),
        QCoreApplication::translate("main", "Archivos o carpetas que copiar o mover."));
    opciones.addPositionalArgument(QCoreApplication::translate("main", "destino"),
        QCoreApplication::translate("main", "Carpeta de destino."));
    opciones.process(aplicacion);

    // El último argumento es la carpeta de destino; los anteriores, los orígenes.
    QStringList origenes = opciones.positionalArguments();
    QString destino;
    maxcopier::Operacion operacion = maxcopier::Operacion::Copiar;
    bool desdePortapapeles = false;
    if (origenes.size() >= 2)
        destino = origenes.takeLast();
    else
        origenes.clear();

    if (opciones.isSet(peticion)
        && !leerPeticionDeArchivo(
            opciones.value(peticion), operacion, origenes, destino, desdePortapapeles)) {
        maxcopier::anotar(QStringLiteral("no se ha podido leer la petición «%1»").arg(opciones.value(peticion)));
    }
    if (opciones.isSet(mover) && !opciones.isSet(peticion))
        operacion = maxcopier::Operacion::Mover;

    // Proceso único: si ya hay un MaxCopier, le pasamos la copia y salimos sin
    // abrir nada (las copias en paralelo son ventanas suyas, no procesos). Si
    // el canal no responde, seguimos y copiamos aquí: más vale una segunda
    // ventana que quedarse sin hacer nada.
    maxcopier::InstanciaUnica instancia;
    if (!instancia.esPrimera() && instancia.enviar(operacion, origenes, destino, desdePortapapeles))
        return 0;

    maxcopier::GestorDeVentanas gestor(&configuracion);
    QObject::connect(&instancia, &maxcopier::InstanciaUnica::peticionRecibida, &gestor,
        &maxcopier::GestorDeVentanas::atender);
    gestor.atender(operacion, origenes, destino, desdePortapapeles);
    // Con bandeja, la aplicación puede arrancar sin ventanas: el controlador
    // global vive en el área de notificación y crea cada copia bajo demanda.
    if (!gestor.puedeSeguirEjecutando())
        return 0;

    return aplicacion.exec();
}
