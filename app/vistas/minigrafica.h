#pragma once

#include <QList>
#include <QWidget>

namespace maxcopier {

/// Mini-gráfica de velocidad de la barra total: una polilínea con las últimas
/// muestras normalizadas a 0..1.
class MiniGrafica : public QWidget {
    Q_OBJECT

public:
    explicit MiniGrafica(QWidget *parent = nullptr);

    void establecerMuestras(const QList<double> &muestras);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *evento) override;

private:
    QList<double> m_muestras;
};

} // namespace maxcopier
