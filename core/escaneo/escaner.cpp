#include "escaneo/escaner.h"

#include "util/rutas.h"

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QThread>

namespace maxcopier {
namespace {

constexpr int kArchivosPorLote = 200;
constexpr int kMsEntreLotes = 150; // así la lista se va viendo mientras escanea

} // namespace

Escaner::Escaner(QObject *parent)
    : QObject(parent)
{
}

QString Escaner::destinoDe(const QString &origen, const QString &carpetaDestino)
{
    return destinoDeOrigen(origen, carpetaDestino);
}

void Escaner::reiniciar()
{
    m_cancelar.storeRelaxed(0);
    m_pausa.storeRelaxed(0);
}

void Escaner::alternarPausa()
{
    const bool pausar = m_pausa.loadRelaxed() == 0;
    m_pausa.storeRelaxed(pausar ? 1 : 0);
    emit pausaCambiada(pausar);
}

void Escaner::cancelar()
{
    m_cancelar.storeRelaxed(1);
    // Despierta un escaneo pausado para que pueda terminar cuanto antes.
    m_pausa.storeRelaxed(0);
}

void Escaner::escanear(const QStringList &origenes, const QString &carpetaDestino, bool mover)
{
    const QDir destino(carpetaDestino);
    ElementosDeCopia lote;
    int archivos = 0;
    qint64 bytes = 0;
    QElapsedTimer reloj;
    reloj.start();
    qint64 msUltimoLote = 0;

    const auto esperarSiPausado = [this]() {
        while (m_pausa.loadRelaxed() != 0 && m_cancelar.loadRelaxed() == 0)
            QThread::msleep(50);
        return m_cancelar.loadRelaxed() != 0;
    };

    const auto anotar = [&](const QString &fuente, const QString &rutaDestino, qint64 tamano) {
        lote.append({ fuente, rutaDestino, tamano });
        ++archivos;
        bytes += tamano;
        const qint64 ms = reloj.elapsed();
        if (lote.size() >= kArchivosPorLote || ms - msUltimoLote >= kMsEntreLotes) {
            msUltimoLote = ms;
            emit encontrados(lote);
            lote.clear();
        }
    };

    for (const QString &origen : origenes) {
        if (esperarSiPausado())
            break;

        const QFileInfo info(origen);
        if (!info.exists())
            continue;

        // Nunca escanear dentro del propio origen. Si esto llegara desde un
        // llamador que no pasó por la UI, crear el destino durante la copia
        // podría hacer que el iterador se recorriera a sí mismo. Marcarlo como
        // cancelado evita además que la ventana intente limpiar el origen.
        if (destinoSeSolapaConOrigen(info.absoluteFilePath(), carpetaDestino)) {
            m_cancelar.storeRelaxed(1);
            break;
        }

        if (info.isFile()) {
            anotar(info.absoluteFilePath(), destinoDe(info.absoluteFilePath(), carpetaDestino), info.size());
            continue;
        }
        if (!info.isDir())
            continue;

        // Una carpeta se replica dentro del destino: D:\Fotos → E:\Backup\Fotos\...
        const QDir raiz(info.absoluteFilePath());
        const QString subcarpeta = destino.absoluteFilePath(raiz.dirName());
        if (mover) {
            // El motor trabaja por archivos, pero un mover también tiene que
            // conservar las carpetas vacías. Solo emitimos la estructura; la
            // ventana la crea después de que todos los archivos hayan acabado.
            QStringList rutasDirectorios { subcarpeta };
            QDirIterator iteradorDirectorios(raiz.absolutePath(),
                QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                QDirIterator::Subdirectories);
            while (iteradorDirectorios.hasNext()) {
                if (esperarSiPausado())
                    break;
                const QFileInfo carpeta(iteradorDirectorios.next());
                const QString relativa = raiz.relativeFilePath(carpeta.absoluteFilePath());
                rutasDirectorios.append(QDir(subcarpeta).absoluteFilePath(relativa));
            }
            emit directoriosEncontrados(rutasDirectorios);
        }
        QDirIterator it(raiz.absolutePath(), QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
            QDirIterator::Subdirectories);
        while (it.hasNext()) {
            if (esperarSiPausado())
                break;
            const QFileInfo archivo(it.next());
            const QString relativa = raiz.relativeFilePath(archivo.absoluteFilePath());
            anotar(archivo.absoluteFilePath(), QDir(subcarpeta).absoluteFilePath(relativa), archivo.size());
        }
    }

    if (!lote.isEmpty())
        emit encontrados(lote);
    emit terminado(archivos, bytes, m_cancelar.loadRelaxed() != 0);
}

void Escaner::comprobarEspacio(const ElementosDeCopia &pendientes, quint64 generacion)
{
    emit espacioComprobado(faltasDeEspacio(pendientes), generacion);
}

} // namespace maxcopier
