#include "configuracion.h"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

#include <cstdio>

using namespace maxcopier;

namespace {

int fallos = 0;

void comprobar(bool bien, const char *que)
{
    std::printf("%s %s\n", bien ? "[ok]  " : "[FALLA]", que);
    if (!bien)
        ++fallos;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication aplicacion(argc, argv);
    Q_UNUSED(aplicacion);

    QTemporaryDir temporal;
    comprobar(temporal.isValid(), "se crea el directorio temporal");
    if (!temporal.isValid())
        return 1;

    const QString ruta = temporal.filePath(QStringLiteral("config.mc"));
    {
        Configuracion ajustes(ruta);
        comprobar(ajustes.limiteVelocidad() == 0, "el límite predeterminado es ilimitado");
        comprobar(ajustes.accionAlTerminar() == AccionAlTerminar::Nada,
            "la acción final predeterminada es nada");
        comprobar(ajustes.temaPreferido() == TemaPreferido::Oscuro,
            "el tema predeterminado es oscuro");
        comprobar(ajustes.metodoDeCopia() == MetodoDeCopia::Sincrono,
            "el método de copia predeterminado es el compatible (síncrono)");
        comprobar(ajustes.archivosALaVez() == 1,
            "el predeterminado copia un archivo a la vez");
        comprobar(ajustes.comprobarEspacioLibre(),
            "el predeterminado comprueba el espacio libre del destino");

        ajustes.establecerLimiteVelocidad(40LL * 1024 * 1024);
        ajustes.establecerAccionAlTerminar(AccionAlTerminar::Suspender);
        ajustes.establecerAccionColision(AccionColision::Renombrar);
        ajustes.establecerAccionError(AccionError::PonerAlFinal);
        ajustes.establecerAccionListaActiva(AccionListaActiva::VentanaNueva);
        ajustes.establecerMetodoDeCopia(MetodoDeCopia::Asincrono);
        ajustes.establecerArchivosALaVez(4);
        ajustes.establecerComprobarEspacioLibre(false);
        ajustes.establecerTemaPreferido(TemaPreferido::Sistema);
    }

    comprobar(QFile::exists(ruta), "se crea config.mc junto a la ruta indicada");
    QSettings fichero(ruta, QSettings::IniFormat);
    fichero.setFallbacksEnabled(false);
    comprobar(fichero.value(QStringLiteral("Transfer/finishAction")).toString() == "suspend",
        "las acciones se guardan con token inglés");
    comprobar(fichero.value(QStringLiteral("Transfer/collisionAction")).toString() == "rename",
        "la colisión se guarda con token inglés");
    comprobar(fichero.value(QStringLiteral("Transfer/copyMethod")).toString() == "overlapped",
        "el método de copia se guarda con token inglés");
    comprobar(fichero.value(QStringLiteral("Transfer/parallelFiles")).toInt() == 4,
        "los archivos a la vez se guardan en el INI");
    comprobar(!fichero.value(QStringLiteral("Transfer/checkFreeSpace")).toBool(),
        "la comprobación de espacio se guarda en el INI");

    {
        Configuracion ajustes(ruta);
        comprobar(ajustes.limiteVelocidad() == 40LL * 1024 * 1024,
            "el límite se carga desde el INI");
        comprobar(ajustes.accionAlTerminar() == AccionAlTerminar::Suspender,
            "la acción final se carga desde el INI");
        comprobar(ajustes.accionColision() == AccionColision::Renombrar,
            "la colisión se carga desde el INI");
        comprobar(ajustes.accionError() == AccionError::PonerAlFinal,
            "el error se carga desde el INI");
        comprobar(ajustes.accionListaActiva() == AccionListaActiva::VentanaNueva,
            "la acción de lista activa se carga desde el INI");
        comprobar(ajustes.metodoDeCopia() == MetodoDeCopia::Asincrono,
            "el método de copia se carga desde el INI");
        comprobar(ajustes.archivosALaVez() == 4,
            "los archivos a la vez se cargan desde el INI");
        comprobar(!ajustes.comprobarEspacioLibre(),
            "la comprobación de espacio se carga desde el INI");
        comprobar(ajustes.temaPreferido() == TemaPreferido::Sistema,
            "el tema se carga desde el INI");
    }

    fichero.setValue(QStringLiteral("Transfer/finishAction"), QStringLiteral("invalid"));
    fichero.setValue(QStringLiteral("Transfer/activeCopyAction"), QStringLiteral("invalid"));
    fichero.setValue(QStringLiteral("Transfer/copyMethod"), QStringLiteral("invalid"));
    fichero.setValue(QStringLiteral("Transfer/parallelFiles"), 99);
    fichero.sync();
    Configuracion invalido(ruta);
    comprobar(invalido.accionAlTerminar() == AccionAlTerminar::Nada,
        "un token final inválido vuelve a nada");
    comprobar(invalido.accionListaActiva() == AccionListaActiva::Preguntar,
        "un token de lista inválido vuelve a preguntar");
    comprobar(invalido.metodoDeCopia() == MetodoDeCopia::Sincrono,
        "un token de método inválido vuelve al síncrono");
    comprobar(invalido.archivosALaVez() == 4,
        "un valor fuera de rango de archivos a la vez se acota");

    std::printf(fallos == 0 ? "\nTodo bien.\n" : "\n%d comprobaciones falladas.\n", fallos);
    return fallos == 0 ? 0 : 1;
}
