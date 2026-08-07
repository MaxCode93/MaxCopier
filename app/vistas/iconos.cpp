#include "vistas/iconos.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>

// Los recursos pertenecen a maxcopier_app, una biblioteca estática. Sin una
// referencia explícita el enlazador puede descartar el objeto generado por
// AUTORCC y dejar los QIcon vacíos en el ejecutable final. Este helper vive en
// el espacio global porque Q_INIT_RESOURCE declara el símbolo generado por
// Qt en ese espacio.
static void inicializarRecursosMaxCopier()
{
    static const bool inicializados = [] {
        Q_INIT_RESOURCE(recursos);
        return true;
    }();
    Q_UNUSED(inicializados);
}

namespace maxcopier {
namespace {

    // Tamaños de los PNG del icono, del más pequeño al más grande.
    constexpr int kLados[] = { 16, 20, 24, 32, 48, 64, 128, 256 };

    // Lados con los que se compone el icono de la bandeja: los que pide Windows
    // (16 y 32) y uno grande para las pantallas con escalado.
    constexpr int kLadosBandeja[] = { 16, 32, 64 };

    QPixmap conEstado(const QPixmap &base, int porcentaje, bool pausada,
        bool terminada)
    {
        const qreal lado = base.width();
        // La placa queda por encima del borde inferior: el área de notificación
        // enmascara los bordes, y antes el texto se recortaba abajo.
        const qreal alto = qMax(8.0, lado * 0.44);
        const qreal margen = qMax(1.0, lado * 0.06);
        const qreal abajo = lado - qMax(1.0, lado * 0.08);
        const QRectF placa(margen, abajo - alto, lado - 2 * margen, alto);

        QPixmap lienzo = base;
        QPainter pintor(&lienzo);
        pintor.setRenderHint(QPainter::Antialiasing);
        pintor.setPen(Qt::NoPen);

        const qreal radio = placa.height() / 3.0;
        pintor.setBrush(QColor(0, 0, 0, 150));
        pintor.drawRoundedRect(placa, radio, radio);

        pintor.setBrush(QColor(Qt::white));
        if (pausada) {
            const qreal barra = qMax(1.0, placa.width() * 0.16);
            const qreal separacion = qMax(1.0, placa.width() * 0.10);
            const qreal total = 2 * barra + separacion;
            const qreal izquierda = placa.center().x() - total / 2.0;
            const qreal altoBarra = placa.height() * 0.56;
            const qreal arriba = placa.top() + (placa.height() - altoBarra) / 2.0;
            const QRectF primera(izquierda, arriba, barra, altoBarra);
            const QRectF segunda(izquierda + barra + separacion, primera.top(),
                barra, primera.height());
            pintor.drawRoundedRect(primera, barra / 2.0, barra / 2.0);
            pintor.drawRoundedRect(segunda, barra / 2.0, barra / 2.0);
        } else {
            const QString texto = terminada
                ? QStringLiteral("OK")
                : QString::number(qBound(0, porcentaje, 100));
            QFont fuente = pintor.font();
            fuente.setBold(true);
            fuente.setPixelSize(qMax(5, int(placa.height() * 0.80)));
            QFontMetrics metrica(fuente);
            // El texto no debe sobresalir de la placa: se ajusta por altura y
            // por ancho, no solo por ancho.
            while ((metrica.height() > placa.height()
                       || metrica.horizontalAdvance(texto) > placa.width() - 2)
                && fuente.pixelSize() > 5) {
                fuente.setPixelSize(fuente.pixelSize() - 1);
                metrica = QFontMetrics(fuente);
            }
            pintor.setFont(fuente);
            pintor.setPen(QColor(Qt::white));
            pintor.drawText(placa, Qt::AlignCenter, texto);
        }
        return lienzo;
    }

} // namespace

QIcon iconoDeLaApp()
{
    inicializarRecursosMaxCopier();
    QIcon icono;
    for (int lado : kLados)
        icono.addFile(QStringLiteral(":/iconos/maxcopier-%1.png").arg(lado), QSize(lado, lado));
    return icono;
}

QIcon iconoDeBandeja(int porcentaje, bool enCurso, bool pausada, bool terminada)
{
    const QIcon app = iconoDeLaApp();
    if (!enCurso && !pausada && !terminada)
        return app;

    QIcon icono;
    for (int lado : kLadosBandeja)
        icono.addPixmap(conEstado(app.pixmap(lado, lado), porcentaje, pausada,
            terminada));
    return icono;
}

} // namespace maxcopier
