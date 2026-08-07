#include "politicas/acceso.h"

#include <QCoreApplication>
#include <QFileInfo>

namespace maxcopier {

QString motivoInaccesible(const QString &origen)
{
    const QFileInfo info(origen);
    if (!info.exists())
        return QCoreApplication::translate(
            "acceso", "El archivo ya no existe o la unidad se ha desconectado.");
    if (!info.isFile())
        return QCoreApplication::translate("acceso", "La ruta ya no es un archivo.");
    if (!info.isReadable())
        return QCoreApplication::translate(
            "acceso", "El archivo no se puede leer: comprueba los permisos o si está en uso.");
    return QString();
}

QString nombreAccionError(AccionError accion)
{
    switch (accion) {
    case AccionError::Preguntar:
        return QCoreApplication::translate("acceso", "preguntar");
    case AccionError::Reintentar:
        return QCoreApplication::translate("acceso", "reintentar");
    case AccionError::PonerAlFinal:
        return QCoreApplication::translate("acceso", "poner al final");
    case AccionError::Saltar:
        return QCoreApplication::translate("acceso", "saltar");
    }
    return QString();
}

} // namespace maxcopier
