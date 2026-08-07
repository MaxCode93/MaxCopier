#include "vistas/chip.h"

#include "temas/temas.h"

#include <QMouseEvent>
#include <QStyle>

namespace maxcopier {

Chip::Chip(const QString &etiqueta, const QString &valor, QWidget *parent)
    : QLabel(parent)
    , m_etiqueta(etiqueta)
    , m_valor(valor)
{
    setObjectName(QStringLiteral("chip"));
    setTextFormat(Qt::RichText);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Cambiar este ajuste"));
    refrescar();
}

void Chip::establecerValor(const QString &valor)
{
    m_valor = valor;
    refrescar();
}

void Chip::establecerDestacado(bool destacado)
{
    m_destacado = destacado;
    setObjectName(destacado ? QStringLiteral("chipActivo") : QStringLiteral("chip"));
    style()->unpolish(this);
    style()->polish(this);
    refrescar();
}

void Chip::refrescar()
{
    setText(QStringLiteral("%1 <span style=\"color:%2;font-weight:600\">%3</span>")
                .arg(m_etiqueta.toHtmlEscaped(), paletaActual().texto.name(), m_valor.toHtmlEscaped()));
}

void Chip::mousePressEvent(QMouseEvent *evento)
{
    if (evento->button() == Qt::LeftButton) {
        emit clicado();
        evento->accept();
        return;
    }
    QLabel::mousePressEvent(evento);
}

} // namespace maxcopier
