#include "dialogos/dialogoerror.h"

#include "vistas/esquinas.h"

#include <QCheckBox>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>
#include <QWindow>

namespace maxcopier {
namespace {

    constexpr int kAnchoDialogo = 430;

} // namespace

DialogoError::DialogoError(const QString &origen, const QString &motivo, QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("ventana")); // comparte el fondo de la ventana principal
    setWindowTitle(tr("No se encuentra el archivo"));
    setModal(true);
    setFixedWidth(kAnchoDialogo);

    auto *columna = new QVBoxLayout(this);
    columna->setContentsMargins(1, 1, 1, 1);
    columna->setSpacing(0);

    // Barra de título propia, como en la ventana principal (no hay marco del sistema).
    auto *barra = new QFrame(this);
    barra->setObjectName(QStringLiteral("barraTitulo"));
    barra->setFixedHeight(28);
    auto *filaBarra = new QHBoxLayout(barra);
    filaBarra->setContentsMargins(7, 0, 7, 0);
    filaBarra->setSpacing(8);
    auto *marca = new QLabel(QStringLiteral("MC"), barra);
    marca->setObjectName(QStringLiteral("marca"));
    marca->setAlignment(Qt::AlignCenter);
    marca->setFixedSize(18, 18);
    filaBarra->addWidget(marca);
    filaBarra->addWidget(new QLabel(windowTitle(), barra));
    filaBarra->addStretch();

    auto *cuerpo = new QWidget(this);
    auto *filas = new QVBoxLayout(cuerpo);
    filas->setContentsMargins(13, 12, 13, 12);
    filas->setSpacing(10);

    auto *aviso = new QLabel(tr("No se puede acceder a:"), cuerpo);

    // Tarjeta con la ruta que ha fallado y el porqué.
    auto *tarjeta = new QFrame(cuerpo);
    tarjeta->setObjectName(QStringLiteral("tarjeta"));
    auto *detalle = new QVBoxLayout(tarjeta);
    detalle->setContentsMargins(9, 7, 9, 7);
    detalle->setSpacing(2);
    auto *ruta = new QLabel(QDir::toNativeSeparators(origen), tarjeta);
    ruta->setObjectName(QStringLiteral("datosTarjeta"));
    ruta->setWordWrap(true);
    auto *porque = new QLabel(motivo, tarjeta);
    porque->setObjectName(QStringLiteral("datosTarjeta"));
    porque->setWordWrap(true);
    detalle->addWidget(ruta);
    detalle->addWidget(porque);

    auto *reintentar = new QPushButton(tr("Reintentar"), cuerpo);
    reintentar->setObjectName(QStringLiteral("primario"));
    reintentar->setDefault(true);
    auto *alFinal = new QPushButton(tr("Poner al final"), cuerpo);
    auto *saltar = new QPushButton(tr("Saltar"), cuerpo);

    auto *botones = new QHBoxLayout;
    botones->setSpacing(7);
    for (QPushButton *boton : { reintentar, alFinal, saltar })
        botones->addWidget(boton);
    botones->addStretch();

    m_paraTodo = new QCheckBox(tr("Hacer lo mismo para todo"), cuerpo);

    filas->addWidget(aviso);
    filas->addWidget(tarjeta);
    filas->addLayout(botones);
    filas->addWidget(m_paraTodo);

    columna->addWidget(barra);
    columna->addWidget(cuerpo);

    connect(reintentar, &QPushButton::clicked, this, [this] { elegir(AccionError::Reintentar); });
    connect(alFinal, &QPushButton::clicked, this, [this] { elegir(AccionError::PonerAlFinal); });
    connect(saltar, &QPushButton::clicked, this, [this] { elegir(AccionError::Saltar); });

    // Sin marco del sistema nadie coloca el diálogo: se centra a mano en la
    // pantalla de la ventana (que recién abierta puede no estar colocada todavía).
    adjustSize();
    if (const QScreen *pantalla = parentWidget() ? parentWidget()->screen() : screen())
        move(pantalla->availableGeometry().center() - rect().center());
}

bool DialogoError::paraTodo() const
{
    return m_paraTodo->isChecked();
}

void DialogoError::elegir(AccionError accion)
{
    m_accion = accion;
    accept();
}

void DialogoError::resizeEvent(QResizeEvent *evento)
{
    QDialog::resizeEvent(evento);
    redondearEsquinas(this);
}

void DialogoError::mousePressEvent(QMouseEvent *evento)
{
    if (evento->button() == Qt::LeftButton) {
        if (QWindow *ventana = windowHandle()) {
            ventana->startSystemMove();
            return;
        }
    }
    QDialog::mousePressEvent(evento);
}

} // namespace maxcopier
