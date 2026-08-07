#pragma once

#include <QWidget>

namespace maxcopier {

/// Barrita de espacio ocupado de la unidad de destino.
class BarraLibre : public QWidget {
    Q_OBJECT

public:
    explicit BarraLibre(QWidget *parent = nullptr);

    /// Fracción ocupada, 0..1.
    void establecerOcupado(double fraccion);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *evento) override;

private:
    double m_ocupado = 0.0;
};

} // namespace maxcopier
