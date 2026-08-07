#include "vistas/cargando.h"

#include "temas/temas.h"

#include <QHideEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace maxcopier {

class IndicadorCircular : public QWidget {
public:
    explicit IndicadorCircular(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("indicadorCircular"));
        setFixedSize(34, 34);
        m_reloj = new QTimer(this);
        m_reloj->setInterval(45);
        connect(m_reloj, &QTimer::timeout, this, [this] {
            m_angulo = (m_angulo + 30) % 360;
            update();
        });
    }

    void establecerAnimacion(bool activa)
    {
        if (activa)
            m_reloj->start();
        else
            m_reloj->stop();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter pintor(this);
        pintor.setRenderHint(QPainter::Antialiasing);
        const QRectF area = QRectF(rect()).adjusted(4.5, 4.5, -4.5, -4.5);

        QPen fondo(paletaActual().linea, 3.0);
        fondo.setCapStyle(Qt::RoundCap);
        pintor.setPen(fondo);
        pintor.drawArc(area, 0, 360 * 16);

        QPen progreso(paletaActual().relleno1, 3.0);
        progreso.setCapStyle(Qt::RoundCap);
        pintor.setPen(progreso);
        pintor.drawArc(area, (90 - m_angulo) * 16, -275 * 16);
    }

private:
    QTimer *m_reloj = nullptr;
    int m_angulo = 0;
};

Cargando::Cargando(QWidget *padre)
    : QWidget(padre)
{
    setObjectName(QStringLiteral("cargando"));
    hide();

    m_tarjeta = new QWidget(this);
    m_tarjeta->setObjectName(QStringLiteral("tarjetaCargando"));
    m_tarjeta->setFixedSize(280, 126);

    auto *columna = new QVBoxLayout(m_tarjeta);
    columna->setContentsMargins(18, 12, 18, 12);
    columna->setSpacing(7);

    m_texto = new QLabel(tr("Enumerando archivos…"), m_tarjeta);
    m_texto->setObjectName(QStringLiteral("textoCargando"));
    m_texto->setAlignment(Qt::AlignCenter);

    m_indicador = new IndicadorCircular(m_tarjeta);

    auto *cancelar = new QPushButton(tr("Cancelar"), m_tarjeta);
    cancelar->setCursor(Qt::PointingHandCursor);
    connect(cancelar, &QPushButton::clicked, this, &Cargando::cancelarPedido);

    columna->addWidget(m_texto);
    columna->addWidget(m_indicador, 0, Qt::AlignHCenter);
    columna->addWidget(cancelar, 0, Qt::AlignHCenter);
}

void Cargando::mostrarCargando(const QString &texto)
{
    if (!texto.isEmpty())
        establecerTexto(texto);
    m_tarjeta->move((width() - m_tarjeta->width()) / 2, (height() - m_tarjeta->height()) / 2);
    show();
    raise();
    m_indicador->establecerAnimacion(true);
}

void Cargando::establecerTexto(const QString &texto)
{
    if (m_texto && !texto.isEmpty())
        m_texto->setText(texto);
}

void Cargando::resizeEvent(QResizeEvent *)
{
    m_tarjeta->move((width() - m_tarjeta->width()) / 2, (height() - m_tarjeta->height()) / 2);
}

void Cargando::paintEvent(QPaintEvent *)
{
    // Atenúa el resto de la ventana y bloquea los clics de debajo.
    QPainter pintor(this);
    pintor.fillRect(rect(), QColor(0, 0, 0, 110));
}

void Cargando::hideEvent(QHideEvent *evento)
{
    if (m_indicador)
        m_indicador->establecerAnimacion(false);
    QWidget::hideEvent(evento);
}

} // namespace maxcopier
