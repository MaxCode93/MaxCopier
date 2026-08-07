#include "vistas/etiquetaruta.h"

#include <QFontMetrics>

namespace maxcopier {

EtiquetaRuta::EtiquetaRuta(QWidget *parent)
    : QLabel(parent)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

void EtiquetaRuta::establecerRuta(const QString &ruta)
{
    m_ruta = ruta;
    setToolTip(ruta);
    recortar();
}

QSize EtiquetaRuta::minimumSizeHint() const
{
    return QSize(0, QLabel::minimumSizeHint().height());
}

void EtiquetaRuta::resizeEvent(QResizeEvent *evento)
{
    QLabel::resizeEvent(evento);
    recortar();
}

void EtiquetaRuta::recortar()
{
    setText(fontMetrics().elidedText(m_ruta, Qt::ElideMiddle, width()));
}

} // namespace maxcopier
