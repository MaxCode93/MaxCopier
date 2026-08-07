#pragma once

#include <QLabel>

namespace maxcopier {

/// Etiqueta de una ruta: nunca ensancha la ventana, recorta por el medio cuando
/// no cabe y muestra la ruta completa en el tooltip.
class EtiquetaRuta : public QLabel {
    Q_OBJECT

public:
    explicit EtiquetaRuta(QWidget *parent = nullptr);

    void establecerRuta(const QString &ruta);
    QString ruta() const { return m_ruta; }

    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *evento) override;

private:
    void recortar();

    QString m_ruta;
};

} // namespace maxcopier
