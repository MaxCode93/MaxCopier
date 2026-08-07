#pragma once

#include <QFrame>

class QLabel;
class QPushButton;

namespace maxcopier {

/// Barra de título propia (la ventana no usa el marco del sistema): marca,
/// porcentaje, título y botones de ventana.
class BarraTitulo : public QFrame {
    Q_OBJECT

public:
    explicit BarraTitulo(QWidget *parent = nullptr);

    void establecerPorcentaje(int porcentaje);
    void establecerTitulo(const QString &titulo);

signals:
    void temaPedido();
    void bandejaPedida();
    void minimizarPedido();
    void cerrarPedido();

protected:
    void mousePressEvent(QMouseEvent *evento) override;

private:
    QPushButton *crearBoton(const QString &glifo, const QString &ayuda);

    QLabel *m_porcentaje = nullptr;
    QLabel *m_titulo = nullptr;
};

} // namespace maxcopier
