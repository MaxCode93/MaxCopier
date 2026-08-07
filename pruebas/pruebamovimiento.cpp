/// Pruebas del contrato de mover que no necesitan una ventana ni Explorer:
/// borrado solo después de una copia completa, defensa contra el mismo archivo
/// y escaneo de carpetas vacías sin tocar el destino prematuramente.

#include "copia/motordecopia.h"
#include "escaneo/escaner.h"
#include "util/rutas.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
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

bool escribir(const QString &ruta, const QByteArray &contenido)
{
    QFile archivo(ruta);
    return archivo.open(QIODevice::WriteOnly) && archivo.write(contenido) == contenido.size();
}

QByteArray leer(const QString &ruta)
{
    QFile archivo(ruta);
    return archivo.open(QIODevice::ReadOnly) ? archivo.readAll() : QByteArray();
}

} // namespace

int main()
{
    QTemporaryDir temporal;
    comprobar(temporal.isValid(), "se crea el directorio temporal");
    if (!temporal.isValid())
        return 1;

    const QString base = temporal.path();
    const QString origen = QDir(base).filePath(QStringLiteral("origen.txt"));
    const QString destino = QDir(base).filePath(QStringLiteral("destino/origen.txt"));
    comprobar(escribir(origen, "contenido"), "se prepara el archivo de origen");

    MotorDeCopia motor;
    motor.establecerLimiteVelocidad(40LL * 1024 * 1024);
    comprobar(motor.limiteVelocidad() == 40LL * 1024 * 1024,
        "el motor acepta un límite de velocidad por transferencia");
    motor.establecerLimiteVelocidad(0);
    comprobar(motor.limiteVelocidad() == 0, "cero desactiva el límite de velocidad");

    MotorDeCopia::Resultado resultado = MotorDeCopia::Resultado::Error;
    QObject::connect(&motor, &MotorDeCopia::terminada,
        [&resultado](MotorDeCopia::Resultado r, const QString &) { resultado = r; });
    QDir().mkpath(QFileInfo(destino).absolutePath());
    motor.reiniciar();
    motor.copiar(origen, destino, false, true);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada,
        "un mover de archivo termina correctamente");
    comprobar(!QFileInfo::exists(origen) && leer(destino) == "contenido",
        "el origen se borra solo después de dejar el destino completo");

    const QString mismo = QDir(base).filePath(QStringLiteral("mismo.txt"));
    comprobar(escribir(mismo, "conservar"), "se prepara el caso de mismo archivo");
    resultado = MotorDeCopia::Resultado::Terminada;
    motor.reiniciar();
    motor.copiar(mismo, mismo, true, true);
    comprobar(resultado == MotorDeCopia::Resultado::Error,
        "el motor rechaza origen y destino iguales");
    comprobar(leer(mismo) == "conservar", "rechazar el mismo archivo no lo destruye");

    const QString carpetaOrigen = QDir(base).filePath(QStringLiteral("carpeta"));
    const QString carpetaDestino = QDir(base).filePath(QStringLiteral("salida"));
    const QString destinoDentro = QDir(carpetaOrigen).filePath(QStringLiteral("dentro"));
    const QString carpetaVacia = QDir(carpetaOrigen).filePath(QStringLiteral("vacia"));
    const QString carpetaConArchivo = QDir(carpetaOrigen).filePath(QStringLiteral("sub"));
    QDir().mkpath(carpetaVacia);
    QDir().mkpath(carpetaConArchivo);
    comprobar(escribir(QDir(carpetaConArchivo).filePath(QStringLiteral("dato.bin")), "dato"),
        "se prepara una carpeta con archivo y carpeta vacía");
    comprobar(destinoSeSolapaConOrigen(carpetaOrigen, base),
        "la validación detecta mover una carpeta sobre sí misma");
    comprobar(destinoSeSolapaConOrigen(carpetaOrigen, destinoDentro),
        "la validación detecta mover una carpeta dentro de sí misma");
    comprobar(!destinoSeSolapaConOrigen(carpetaOrigen, QDir(base).filePath(QStringLiteral("otra"))),
        "la validación respeta los límites de los nombres de carpeta");

    Escaner escaner;
    QStringList directorios;
    int archivos = 0;
    bool terminado = false;
    bool cancelado = false;
    QObject::connect(&escaner, &Escaner::directoriosEncontrados,
        [&directorios](const QStringList &rutas) { directorios.append(rutas); });
    QObject::connect(&escaner, &Escaner::encontrados,
        [&archivos](const ElementosDeCopia &lote) { archivos += int(lote.size()); });
    QObject::connect(&escaner, &Escaner::terminado,
        [&terminado, &cancelado](int, qint64, bool seCancelo) {
            terminado = true;
            cancelado = seCancelo;
        });
    escaner.reiniciar();
    escaner.escanear({ carpetaOrigen }, carpetaDestino, true);
    comprobar(terminado && !cancelado && archivos == 1,
        "el escaneo de mover encuentra sus archivos");
    comprobar(directorios.contains(QDir(carpetaDestino).filePath(QStringLiteral("carpeta/vacia"))),
        "el escaneo conserva la carpeta vacía como estructura pendiente");
    comprobar(!QFileInfo::exists(carpetaDestino),
        "el escaneo no crea el destino antes de completar la tanda");

    terminado = false;
    cancelado = false;
    escaner.reiniciar();
    escaner.escanear({ carpetaOrigen }, destinoDentro, true);
    comprobar(terminado && cancelado,
        "el escaneo cancela un destino dentro del origen");

    std::printf(fallos == 0 ? "\nTodo bien.\n" : "\n%d comprobaciones falladas.\n", fallos);
    return fallos == 0 ? 0 : 1;
}
