#include "vistas/minigrafica.h"

#include "temas/temas.h"

#include <QPainter>
#include <QPainterPath>

namespace maxcopier {

MiniGrafica::MiniGrafica(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedSize(sizeHint());
}

void MiniGrafica::establecerMuestras(const QList<double> &muestras)
{
    m_muestras = muestras;
    update();
}

QSize MiniGrafica::sizeHint() const
{
    return QSize(74, 15);
}

void MiniGrafica::paintEvent(QPaintEvent *)
{
    if (m_muestras.size() < 2)
        return;

    QPainter pintor(this);
    pintor.setRenderHint(QPainter::Antialiasing);

    const double paso = double(width() - 1) / double(m_muestras.size() - 1);
    const double alto = height() - 3.0;

    QPainterPath trazo;
    for (int i = 0; i < m_muestras.size(); ++i) {
        const double valor = qBound(0.0, m_muestras.at(i), 1.0);
        const QPointF punto(i * paso, 1.5 + alto * (1.0 - valor));
        if (i == 0)
            trazo.moveTo(punto);
        else
            trazo.lineTo(punto);
    }

    pintor.setPen(QPen(paletaActual().acento, 1.4));
    pintor.drawPath(trazo);
}

} // namespace maxcopier
