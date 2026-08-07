#include "vistas/unidaddestino.h"

#include "vistas/barralibre.h"

#include <QHBoxLayout>
#include <QLabel>

namespace maxcopier {

UnidadDestino::UnidadDestino(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("unidad"));

    auto *fila = new QHBoxLayout(this);
    fila->setContentsMargins(4, 3, 9, 3);
    fila->setSpacing(7);

    m_tipo = new QLabel(this);
    m_tipo->setObjectName(QStringLiteral("insignia"));
    m_nombre = new QLabel(this);
    m_barra = new BarraLibre(this);
    m_libre = new QLabel(this);

    fila->addWidget(m_tipo);
    fila->addWidget(m_nombre);
    fila->addWidget(m_barra);
    fila->addWidget(m_libre);
}

void UnidadDestino::establecerUnidad(
    const QString &tipo, const QString &nombre, const QString &libre, double ocupado)
{
    m_tipo->setText(tipo);
    m_nombre->setText(nombre);
    m_libre->setText(libre);
    m_barra->establecerOcupado(ocupado);
}

} // namespace maxcopier
