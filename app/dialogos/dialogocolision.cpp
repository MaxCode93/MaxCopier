#include "dialogos/dialogocolision.h"

#include "util/formatos.h"
#include "vistas/esquinas.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
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

    QString fechaDe(const QFileInfo &info)
    {
        return info.exists() ? info.lastModified().toString(QStringLiteral("dd/MM/yyyy HH:mm"))
                             : QStringLiteral("—");
    }

    /// Tarjeta «Origen»/«Destino» con el tamaño y la fecha del archivo.
    QFrame *crearTarjeta(const QString &titulo, const QString &ruta, QWidget *padre)
    {
        const QFileInfo info(ruta);

        auto *tarjeta = new QFrame(padre);
        tarjeta->setObjectName(QStringLiteral("tarjeta"));
        auto *columna = new QVBoxLayout(tarjeta);
        columna->setContentsMargins(9, 7, 9, 7);
        columna->setSpacing(2);

        auto *cabecera = new QLabel(titulo, tarjeta);
        cabecera->setObjectName(QStringLiteral("tituloTarjeta"));

        auto *datos
            = new QLabel(QStringLiteral("%1\n%2").arg(formatearTamano(info.size()), fechaDe(info)), tarjeta);
        datos->setObjectName(QStringLiteral("datosTarjeta"));

        columna->addWidget(cabecera);
        columna->addWidget(datos);
        return tarjeta;
    }

} // namespace

DialogoColision::DialogoColision(const ElementoDeCopia &elemento, QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("ventana")); // comparte el fondo de la ventana principal
    setWindowTitle(tr("El archivo ya existe"));
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
    aviso->setTextFormat(Qt::RichText);
    aviso->setWordWrap(true);
    aviso->setText(
        tr("<b>%1</b> ya existe en <span style='font-family:monospace'>%2</span>")
            .arg(QFileInfo(elemento.destino).fileName().toHtmlEscaped(),
                QDir::toNativeSeparators(QFileInfo(elemento.destino).absolutePath()).toHtmlEscaped()));

    auto *tarjetas = new QHBoxLayout;
    tarjetas->setSpacing(9);
    tarjetas->addWidget(crearTarjeta(tr("Origen"), elemento.fuente, cuerpo));
    tarjetas->addWidget(crearTarjeta(tr("Destino"), elemento.destino, cuerpo));

    auto *sobrescribir = new QPushButton(tr("Sobrescribir"), cuerpo);
    sobrescribir->setObjectName(QStringLiteral("primario"));
    sobrescribir->setDefault(true);
    auto *renombrar = new QPushButton(tr("Renombrar"), cuerpo);
    auto *saltar = new QPushButton(tr("Saltar"), cuerpo);

    auto *botones = new QHBoxLayout;
    botones->setSpacing(7);
    for (QPushButton *boton : { sobrescribir, renombrar, saltar })
        botones->addWidget(boton);
    botones->addStretch();

    m_paraTodo = new QCheckBox(tr("Hacer lo mismo para todo"), cuerpo);

    filas->addWidget(aviso);
    filas->addLayout(tarjetas);
    filas->addLayout(botones);
    filas->addWidget(m_paraTodo);

    columna->addWidget(barra);
    columna->addWidget(cuerpo);

    connect(sobrescribir, &QPushButton::clicked, this, [this] { elegir(AccionColision::Sobrescribir); });
    connect(renombrar, &QPushButton::clicked, this, [this] { elegir(AccionColision::Renombrar); });
    connect(saltar, &QPushButton::clicked, this, [this] { elegir(AccionColision::Saltar); });

    // Sin marco del sistema nadie coloca el diálogo: se centra a mano en la
    // pantalla de la ventana (que recién abierta puede no estar colocada todavía).
    adjustSize();
    if (const QScreen *pantalla = parentWidget() ? parentWidget()->screen() : screen())
        move(pantalla->availableGeometry().center() - rect().center());
}

bool DialogoColision::paraTodo() const
{
    return m_paraTodo->isChecked();
}

void DialogoColision::elegir(AccionColision accion)
{
    m_accion = accion;
    accept();
}

void DialogoColision::resizeEvent(QResizeEvent *evento)
{
    QDialog::resizeEvent(evento);
    redondearEsquinas(this);
}

void DialogoColision::mousePressEvent(QMouseEvent *evento)
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
