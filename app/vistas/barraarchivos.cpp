#include "vistas/barraarchivos.h"

#include "temas/temas.h"

#include <QFontMetrics>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <utility>

namespace maxcopier {
namespace {

constexpr int kAltoBarra = 21;
constexpr int kAltoEtiquetas = 18;
constexpr int kSeparacion = 2;
constexpr int kMargen = 8;
constexpr int kSeparadorSegmentos = 1;

} // namespace

BarraArchivos::BarraArchivos(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kAltoBarra + kSeparacion + kAltoEtiquetas);
}

void BarraArchivos::establecerArchivos(const QList<ArchivoEnCurso> &archivos)
{
    m_archivos = archivos;
    m_mensaje.clear();
    reorganizarEtiquetas();
    update();
}

void BarraArchivos::establecerMensajeVacio(const QString &mensaje)
{
    m_archivos.clear();
    m_mensaje = mensaje;
    limpiarEtiquetas();
    update();
}

QSize BarraArchivos::sizeHint() const
{
    return QSize(400, height());
}

void BarraArchivos::limpiarEtiquetas()
{
    for (QLabel *etiqueta : std::as_const(m_etiquetas)) {
        etiqueta->hide();
        etiqueta->deleteLater();
    }
    m_etiquetas.clear();
}

void BarraArchivos::reorganizarEtiquetas()
{
    limpiarEtiquetas();
    if (m_archivos.isEmpty())
        return;

    const int cantidad = m_archivos.size();
    const int ancho = width();
    const int margen = cantidad == 1 ? kMargen : 4;
    const int espacio = ancho - 2 * margen - (cantidad - 1) * 8;
    const int anchoEtiqueta = qMax(1, espacio / cantidad);

    for (int i = 0; i < cantidad; ++i) {
        const ArchivoEnCurso &archivo = m_archivos.at(i);
        auto *etiqueta = new QLabel(this);
        etiqueta->setObjectName(QStringLiteral("etiquetaArchivo"));
        etiqueta->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        const QFontMetrics metrica = etiqueta->fontMetrics();
        if (cantidad == 1) {
            etiqueta->setText(metrica.elidedText(archivo.nombre, Qt::ElideMiddle, anchoEtiqueta));
        } else {
            etiqueta->setText(metrica.elidedText(
                QStringLiteral("%1 · %2").arg(archivo.nombre, archivo.velocidad),
                Qt::ElideMiddle, anchoEtiqueta));
        }
        etiqueta->setToolTip(archivo.restante.isEmpty()
                ? archivo.nombre
                : QStringLiteral("%1\n%2").arg(archivo.nombre, archivo.restante));
        const int x = margen + i * (anchoEtiqueta + 8);
        etiqueta->setGeometry(x, kAltoBarra + kSeparacion, anchoEtiqueta, kAltoEtiquetas);
        etiqueta->show();
        m_etiquetas.append(etiqueta);
    }
}

void BarraArchivos::resizeEvent(QResizeEvent *)
{
    reorganizarEtiquetas();
}

int BarraArchivos::indiceDeSegmento(int x) const
{
    if (m_archivos.isEmpty() || x < 0)
        return -1;
    if (m_archivos.size() == 1)
        return 0;
    const int segmento = (x * m_archivos.size()) / qMax(1, width());
    return qBound(0, segmento, int(m_archivos.size()) - 1);
}

void BarraArchivos::mousePressEvent(QMouseEvent *evento)
{
    if (evento->button() == Qt::LeftButton && !m_archivos.isEmpty()) {
        const int indice = indiceDeSegmento(evento->position().toPoint().x());
        if (indice >= 0)
            emit segmentoClicado(indice);
    }
    QWidget::mousePressEvent(evento);
}

void BarraArchivos::paintEvent(QPaintEvent *)
{
    const Paleta &p = paletaActual();

    QPainter pintor(this);
    pintor.setRenderHint(QPainter::Antialiasing);

    const QRectF marco = QRectF(0, 0, width(), kAltoBarra).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath recorte;
    recorte.addRoundedRect(marco, 4, 4);

    pintor.setPen(Qt::NoPen);
    pintor.setBrush(p.barra);
    pintor.drawPath(recorte);

    if (m_archivos.isEmpty()) {
        if (!m_mensaje.isEmpty()) {
            pintor.setPen(p.atenuado);
            pintor.drawText(marco, Qt::AlignCenter, m_mensaje);
        }
        pintor.setPen(QPen(p.linea, 1));
        pintor.setBrush(Qt::NoBrush);
        pintor.drawPath(recorte);
        return;
    }

    const QFontMetrics metrica = fontMetrics();

    if (m_archivos.size() == 1) {
        const ArchivoEnCurso &archivo = m_archivos.first();
        if (archivo.porcentaje > 0) {
            QRectF relleno = marco;
            relleno.setWidth(marco.width() * archivo.porcentaje / 100.0);
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

        const QString izquierda = archivo.tamano.isEmpty()
            ? archivo.nombre
            : QStringLiteral("%1 · %2").arg(archivo.nombre, archivo.tamano);
        const int anchoDerecha = metrica.horizontalAdvance(archivo.restante);
        const QRect interior = QRect(0, 0, width(), kAltoBarra).adjusted(kMargen, 0, -kMargen, 0);

        pintor.setPen(p.texto);
        const int limiteIzquierda = int(interior.width() * 0.56);
        pintor.drawText(QRect(interior.left(), interior.top(), limiteIzquierda, interior.height()),
            Qt::AlignVCenter | Qt::AlignLeft,
            metrica.elidedText(izquierda, Qt::ElideRight, limiteIzquierda));

        QFont negrita = font();
        negrita.setBold(true);
        pintor.setFont(negrita);
        pintor.drawText(rect(), Qt::AlignCenter, tr("%1 %").arg(archivo.porcentaje));
        pintor.setFont(font());

        pintor.setPen(p.atenuado);
        pintor.drawText(QRect(interior.right() - anchoDerecha, interior.top(), anchoDerecha,
                            interior.height()),
            Qt::AlignVCenter | Qt::AlignRight, archivo.restante);
        return;
    }

    // Varios archivos: la barra se divide en segmentos iguales.
    const int cantidad = m_archivos.size();
    const qreal anchoSegmento =
        (marco.width() - (cantidad - 1) * kSeparadorSegmentos) / qreal(cantidad);
    for (int i = 0; i < cantidad; ++i) {
        const ArchivoEnCurso &archivo = m_archivos.at(i);
        QRectF segmento(marco.left() + i * (anchoSegmento + kSeparadorSegmentos), marco.top(),
            anchoSegmento, marco.height());
        if (archivo.porcentaje > 0) {
            QRectF relleno = segmento;
            relleno.setWidth(segmento.width() * archivo.porcentaje / 100.0);
            QLinearGradient degradado(relleno.topLeft(), relleno.bottomLeft());
            degradado.setColorAt(0.0, p.relleno1);
            degradado.setColorAt(1.0, p.relleno2);
            pintor.save();
            pintor.setClipPath(recorte);
            pintor.setBrush(degradado);
            pintor.drawRect(relleno);
            pintor.restore();
        }

        QFont pequena = font();
        pequena.setPointSizeF(qMax(6.5, font().pointSizeF() - 1.0));
        pintor.setFont(pequena);
        pintor.setPen(archivo.pausado ? p.atenuado : p.texto);
        pintor.drawText(segmento, Qt::AlignCenter, tr("%1 %").arg(archivo.porcentaje));
        pintor.setFont(font());
    }

    pintor.setPen(QPen(p.linea, 1));
    pintor.setBrush(Qt::NoBrush);
    pintor.drawPath(recorte);
}

} // namespace maxcopier
