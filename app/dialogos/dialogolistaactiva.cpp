#include "dialogos/dialogolistaactiva.h"

#include "vistas/esquinas.h"

#include <QCheckBox>
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

    constexpr int kAnchoDialogo = 520;

} // namespace

DialogoListaActiva::DialogoListaActiva(
    const QString &peticion, const QString &enCurso, bool permitirAnadir, QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("ventana")); // comparte el fondo de la ventana principal
    setWindowTitle(tr("Ya hay una copia en curso"));
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

    auto *aviso = new QLabel(cuerpo);
    aviso->setWordWrap(true);
    aviso->setText(tr("Quieres %1, pero MaxCopier está copiando %2.").arg(peticion, enCurso));

    auto *anadir = new QPushButton(tr("Añadir a la lista actual"), cuerpo);
    anadir->setObjectName(QStringLiteral("primario"));
    anadir->setEnabled(permitirAnadir);
    auto *ventanaNueva = new QPushButton(tr("Abrir en una ventana nueva"), cuerpo);
    auto *cancelar = new QPushButton(tr("Cancelar"), cuerpo);
    (permitirAnadir ? anadir : ventanaNueva)->setDefault(true);
    if (!permitirAnadir)
        anadir->setToolTip(tr("La lista en curso va a otra carpeta de destino."));

    auto *botones = new QHBoxLayout;
    botones->setSpacing(7);
    for (QPushButton *boton : { anadir, ventanaNueva, cancelar })
        botones->addWidget(boton);
    botones->addStretch();

    m_recordar = new QCheckBox(tr("Recordar mi elección"), cuerpo);

    filas->addWidget(aviso);
    filas->addLayout(botones);
    filas->addWidget(m_recordar);

    columna->addWidget(barra);
    columna->addWidget(cuerpo);

    connect(anadir, &QPushButton::clicked, this, [this] { elegir(AccionListaActiva::AnadirALaActual); });
    connect(ventanaNueva, &QPushButton::clicked, this, [this] { elegir(AccionListaActiva::VentanaNueva); });
    connect(cancelar, &QPushButton::clicked, this, [this] { elegir(AccionListaActiva::Cancelar); });

    // Sin marco del sistema nadie coloca el diálogo: se centra a mano sobre la
    // ventana ocupada (o en la pantalla si se abre sin ventana).
    adjustSize();
    if (const QWidget *ventana = parentWidget())
        move(ventana->frameGeometry().center() - rect().center());
    else if (const QScreen *pantalla = screen())
        move(pantalla->availableGeometry().center() - rect().center());
}

bool DialogoListaActiva::recordar() const
{
    return m_recordar->isChecked();
}

void DialogoListaActiva::elegir(AccionListaActiva accion)
{
    m_accion = accion;
    accept();
}

void DialogoListaActiva::resizeEvent(QResizeEvent *evento)
{
    QDialog::resizeEvent(evento);
    redondearEsquinas(this);
}

void DialogoListaActiva::mousePressEvent(QMouseEvent *evento)
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
