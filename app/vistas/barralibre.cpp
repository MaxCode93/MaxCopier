#include "vistas/barralibre.h"

#include "temas/temas.h"

#include <QPainter>

namespace maxcopier {

BarraLibre::BarraLibre(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedSize(sizeHint());
}

void BarraLibre::establecerOcupado(double fraccion)
{
    m_ocupado = qBound(0.0, fraccion, 1.0);
    update();
}

QSize BarraLibre::sizeHint() const
{
    return QSize(86, 7);
}

void BarraLibre::paintEvent(QPaintEvent *)
{
    const Paleta &p = paletaActual();

    QPainter pintor(this);
    pintor.setRenderHint(QPainter::Antialiasing);

    const QRectF marco = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    pintor.setPen(QPen(p.linea, 1));
    pintor.setBrush(p.barra);
    pintor.drawRoundedRect(marco, 3, 3);

    if (m_ocupado <= 0.0)
        return;

    QRectF relleno = marco.adjusted(1, 1, -1, -1);
    relleno.setWidth(relleno.width() * m_ocupado);
    pintor.setPen(Qt::NoPen);
    pintor.setBrush(p.acento);
    pintor.drawRoundedRect(relleno, 2, 2);
}

} // namespace maxcopier
