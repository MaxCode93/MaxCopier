#include "util/titulos.h"

#include <QCoreApplication>

namespace maxcopier {

QString tituloDeTransferencia(ipc::Operacion operacion, int enCurso,
    const QString &origen, const QString &destino)
{
    const QString verbo = operacion == ipc::Operacion::Mover
        ? QCoreApplication::translate("titulos", "Movimiento")
        : QCoreApplication::translate("titulos", "Copia");
    if (enCurso <= 0)
        return verbo;
    if (enCurso == 1 && !origen.isEmpty() && !destino.isEmpty())
        return QStringLiteral("%1 · %2 → %3").arg(verbo, origen, destino);
    return QStringLiteral("%1 · %2 archivos en curso").arg(verbo).arg(enCurso);
}

} // namespace maxcopier
