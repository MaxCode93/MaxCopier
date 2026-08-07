#include "politicas/colision.h"

#include <QCoreApplication>
#include <QFileInfo>

namespace maxcopier {

QString rutaLibre(const QString &destino)
{
    if (!QFileInfo::exists(destino))
        return destino;

    const QFileInfo info(destino);
    const QString carpeta = info.absolutePath();
    const QString base = info.completeBaseName();
    const QString sufijo = info.suffix();
    // Los nombres que empiezan por punto («.gitignore») no tienen extensión que
    // respetar: `completeBaseName` ya devuelve el nombre entero.
    const QString extension = sufijo.isEmpty() ? QString() : QLatin1Char('.') + sufijo;

    for (int copia = 2;; ++copia) {
        const QString candidato = QStringLiteral("%1/%2 (%3)%4").arg(carpeta, base).arg(copia).arg(extension);
        if (!QFileInfo::exists(candidato))
            return candidato;
    }
}

QString nombreAccionColision(AccionColision accion)
{
    switch (accion) {
    case AccionColision::Preguntar:
        return QCoreApplication::translate("colision", "preguntar");
    case AccionColision::Sobrescribir:
        return QCoreApplication::translate("colision", "sobrescribir");
    case AccionColision::Renombrar:
        return QCoreApplication::translate("colision", "renombrar");
    case AccionColision::Saltar:
        return QCoreApplication::translate("colision", "saltar");
    }
    return QString();
}

} // namespace maxcopier
