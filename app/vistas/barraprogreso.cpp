#include "vistas/barraprogreso.h"

#include "temas/temas.h"
#include "vistas/minigrafica.h"

#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

namespace maxcopier {
namespace {

    constexpr int kMargen = 8; // margen lateral de los textos
    constexpr int kHueco = 9; // hueco entre la gráfica y la velocidad
    constexpr int kAltoTotal = 23;
    constexpr int kAltoArchivo = 21;

} // namespace

BarraProgreso::BarraProgreso(Variante variante, QWidget *parent)
    : QWidget(parent)
    , m_variante(variante)
{
    setFixedHeight(variante == Variante::Total ? kAltoTotal : kAltoArchivo);
    if (variante == Variante::Total)
        m_grafica = new MiniGrafica(this);
}

void BarraProgreso::establecerPorcentaje(int porcentaje)
{
    m_porcentaje = qBound(0, porcentaje, 100);
    update();
}

void BarraProgreso::establecerTextoIzquierda(const QString &texto)
{
    m_izquierda = texto;
    update();
}

void BarraProgreso::establecerTextoDerecha(const QString &texto)
{
    m_derecha = texto;
    recolocarGrafica();
    update();
}

QSize BarraProgreso::sizeHint() const
{
    return QSize(400, height());
}

void BarraProgreso::resizeEvent(QResizeEvent *)
{
    recolocarGrafica();
}

void BarraProgreso::recolocarGrafica()
{
    if (!m_grafica)
        return;

    const int anchoDerecha = fontMetrics().horizontalAdvance(m_derecha);
    const int x = width() - kMargen - anchoDerecha - kHueco - m_grafica->width();
    const int y = (height() - m_grafica->height()) / 2;
    m_grafica->move(qMax(kMargen, x), y);
    m_grafica->setVisible(x > kMargen);
}

void BarraProgreso::paintEvent(QPaintEvent *)
{
    const Paleta &p = paletaActual();

    QPainter pintor(this);
    pintor.setRenderHint(QPainter::Antialiasing);

    const QRectF marco = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath recorte;
    recorte.addRoundedRect(marco, 4, 4);

    pintor.setPen(Qt::NoPen);
    pintor.setBrush(p.barra);
    pintor.drawPath(recorte);

    if (m_porcentaje > 0) {
        QRectF relleno = marco;
        relleno.setWidth(marco.width() * m_porcentaje / 100.0);
        QLinearGradient degradado(relleno.topLeft(), relleno.bottomLeft());
        degradado.setColorAt(0.0, p.relleno1);
        degradado.setColorAt(1.0, p.relleno2);

        pintor.save();
        pintor.setClipPath(recorte);
        pintor.setBrush(degradado);
        pintor.drawRect(relleno);
        pintor.restore();
    }

    pintor.setPen(QPen(p.linea, 1));
    pintor.setBrush(Qt::NoBrush);
    pintor.drawPath(recorte);

    const QFontMetrics metrica = fontMetrics();
    const int anchoDerecha = metrica.horizontalAdvance(m_derecha);
    const QRect interior = rect().adjusted(kMargen, 0, -kMargen, 0);

    pintor.setPen(p.texto);
    const int limiteIzquierda = int(interior.width() * 0.56);
    pintor.drawText(QRect(interior.left(), interior.top(), limiteIzquierda, interior.height()),
        Qt::AlignVCenter | Qt::AlignLeft, metrica.elidedText(m_izquierda, Qt::ElideRight, limiteIzquierda));

    QFont negrita = font();
    negrita.setBold(true);
    pintor.setFont(negrita);
    pintor.drawText(rect(), Qt::AlignCenter, tr("%1 %").arg(m_porcentaje));
    pintor.setFont(font());

    pintor.setPen(p.atenuado);
    pintor.drawText(QRect(interior.right() - anchoDerecha, interior.top(), anchoDerecha, interior.height()),
        Qt::AlignVCenter | Qt::AlignRight, m_derecha);
}

} // namespace maxcopier
