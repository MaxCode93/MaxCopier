#include "vistas/esquinas.h"

#include <QPainterPath>
#include <QRegion>
#include <QWidget>

namespace maxcopier {

void redondearEsquinas(QWidget *ventana, int radio)
{
    if (!ventana || ventana->rect().isEmpty())
        return;

    QPainterPath contorno;
    contorno.addRoundedRect(QRectF(ventana->rect()), radio, radio);
    ventana->setMask(QRegion(contorno.toFillPolygon().toPolygon()));
}

} // namespace maxcopier
