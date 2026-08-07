#include "vistas/barratitulo.h"

#include "vistas/iconos.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWindow>

namespace maxcopier {

BarraTitulo::BarraTitulo(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("barraTitulo"));
    setFixedHeight(28);

    auto *fila = new QHBoxLayout(this);
    fila->setContentsMargins(7, 0, 4, 0);
    fila->setSpacing(8);

    auto *marca = new QLabel(this);
    marca->setObjectName(QStringLiteral("marca"));
    marca->setAlignment(Qt::AlignCenter);
    marca->setFixedSize(18, 18);
    marca->setPixmap(iconoDeLaApp().pixmap(16, 16));

    m_porcentaje = new QLabel(this);
    m_porcentaje->setObjectName(QStringLiteral("porcentajeTitulo"));

    m_titulo = new QLabel(this);
    m_titulo->setObjectName(QStringLiteral("tituloVentana"));

    fila->addWidget(marca);
    fila->addWidget(m_porcentaje);
    fila->addWidget(m_titulo);
    fila->addStretch();

    auto *tema = crearBoton(QStringLiteral("\u25D1"), tr("Cambiar de tema (Ctrl+T)"));
    auto *bandeja = crearBoton(QStringLiteral("\u2198"), tr("Minimizar a la bandeja"));
    auto *minimizar = crearBoton(QStringLiteral("\u2014"), tr("Minimizar"));
    auto *cerrar = crearBoton(QStringLiteral("\u2715"), tr("Cerrar"));
    cerrar->setObjectName(QStringLiteral("botonCerrar"));

    for (QPushButton *boton : { tema, bandeja, minimizar, cerrar })
        fila->addWidget(boton);

    connect(tema, &QPushButton::clicked, this, &BarraTitulo::temaPedido);
    connect(bandeja, &QPushButton::clicked, this, &BarraTitulo::bandejaPedida);
    connect(minimizar, &QPushButton::clicked, this, &BarraTitulo::minimizarPedido);
    connect(cerrar, &QPushButton::clicked, this, &BarraTitulo::cerrarPedido);
}

QPushButton *BarraTitulo::crearBoton(const QString &glifo, const QString &ayuda)
{
    auto *boton = new QPushButton(glifo, this);
    boton->setObjectName(QStringLiteral("botonVentana"));
    boton->setToolTip(ayuda);
    boton->setFixedSize(26, 22);
    boton->setFocusPolicy(Qt::NoFocus);
    boton->setCursor(Qt::ArrowCursor);
    return boton;
}

void BarraTitulo::establecerPorcentaje(int porcentaje)
{
    m_porcentaje->setText(tr("%1 %").arg(qBound(0, porcentaje, 100)));
}

void BarraTitulo::establecerTitulo(const QString &titulo)
{
    m_titulo->setText(titulo);
    m_titulo->setToolTip(titulo);
}

void BarraTitulo::mousePressEvent(QMouseEvent *evento)
{
    if (evento->button() == Qt::LeftButton) {
        if (QWindow *ventana = window()->windowHandle()) {
            ventana->startSystemMove();
            return;
        }
    }
    QFrame::mousePressEvent(evento);
}

} // namespace maxcopier
