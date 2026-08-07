#include "dialogos/dialogoopciones.h"

#include "configuracion.h"
#include "vistas/esquinas.h"
#include "vistas/opcionespanel.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWindow>

namespace maxcopier {

DialogoOpciones::DialogoOpciones(Configuracion *configuracion, QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("dialogoOpciones"));
    setWindowTitle(tr("Opciones de MaxCopier"));
    setFixedSize(560, 500);

    auto *columna = new QVBoxLayout(this);
    columna->setContentsMargins(1, 1, 1, 1);
    columna->setSpacing(0);

    m_barra = new QFrame(this);
    m_barra->setObjectName(QStringLiteral("barraTitulo"));
    m_barra->setFixedHeight(34);
    auto *fila = new QHBoxLayout(m_barra);
    fila->setContentsMargins(10, 0, 5, 0);
    fila->setSpacing(8);

    auto *titulo = new QLabel(tr("Opciones de MaxCopier"), m_barra);
    titulo->setObjectName(QStringLiteral("tituloOpciones"));
    fila->addWidget(titulo);
    fila->addStretch();

    auto *cerrar = new QPushButton(QStringLiteral("\u2715"), m_barra);
    cerrar->setObjectName(QStringLiteral("botonCerrar"));
    cerrar->setFixedSize(26, 22);
    cerrar->setToolTip(tr("Cerrar"));
    cerrar->setFocusPolicy(Qt::NoFocus);
    fila->addWidget(cerrar);
    connect(cerrar, &QPushButton::clicked, this, &QDialog::accept);

    columna->addWidget(m_barra);
    columna->addWidget(new OpcionesPanel(configuracion, this), 1);

    auto *pie = new QHBoxLayout;
    pie->setContentsMargins(10, 6, 10, 8);
    pie->addStretch();
    auto *cerrarPie = new QPushButton(tr("Cerrar"), this);
    cerrarPie->setObjectName(QStringLiteral("fantasma"));
    cerrarPie->setDefault(true);
    connect(cerrarPie, &QPushButton::clicked, this, &QDialog::accept);
    pie->addWidget(cerrarPie);
    columna->addLayout(pie);
}

void DialogoOpciones::resizeEvent(QResizeEvent *evento)
{
    QDialog::resizeEvent(evento);
    redondearEsquinas(this);
}

void DialogoOpciones::mousePressEvent(QMouseEvent *evento)
{
    if (evento->button() == Qt::LeftButton) {
        if (QWindow *ventana = window()->windowHandle()) {
            ventana->startSystemMove();
            return;
        }
    }
    QDialog::mousePressEvent(evento);
}

} // namespace maxcopier
