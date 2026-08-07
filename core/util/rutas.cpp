#include "util/rutas.h"

#include <QDir>
#include <QFileInfo>

namespace maxcopier {

QString rutaComparable(const QString &ruta)
{
    if (ruta.isEmpty())
        return {};

    QString resultado = QDir::fromNativeSeparators(QFileInfo(ruta).absoluteFilePath());
    resultado = QDir::cleanPath(resultado);
#ifdef Q_OS_WIN
    resultado = resultado.toCaseFolded();
#endif
    return resultado;
}

bool rutasIguales(const QString &una, const QString &otra)
{
    const QString izquierda = rutaComparable(una);
    const QString derecha = rutaComparable(otra);
    return !izquierda.isEmpty() && izquierda == derecha;
}

bool rutaDescendienteDe(const QString &posibleHija, const QString &directorioPadre)
{
    const QString hija = rutaComparable(posibleHija);
    const QString padre = rutaComparable(directorioPadre);
    if (hija.isEmpty() || padre.isEmpty() || hija == padre)
        return false;

    const QString separador = padre.endsWith(QChar(u'/')) ? padre : padre + QChar(u'/');
    return hija.startsWith(separador);
}

bool rutaMismaODescendiente(const QString &posibleHija, const QString &directorioPadre)
{
    return rutasIguales(posibleHija, directorioPadre)
        || rutaDescendienteDe(posibleHija, directorioPadre);
}

QString destinoDeOrigen(const QString &origen, const QString &carpetaDestino)
{
    return QDir(carpetaDestino).absoluteFilePath(QFileInfo(origen).fileName());
}

bool destinoSeSolapaConOrigen(const QString &origen, const QString &carpetaDestino)
{
    const QFileInfo info(origen);
    if (!info.exists())
        return false;

    const QString destino = destinoDeOrigen(origen, carpetaDestino);
    if (rutasIguales(origen, destino))
        return true;

    return info.isDir() && rutaDescendienteDe(destino, origen);
}

} // namespace maxcopier
