#pragma once

#include <QWidget>

namespace maxcopier {

class MiniGrafica;

/// Barra de progreso del estilo del mockup: relleno con degradado, texto a la
/// izquierda, porcentaje centrado y texto (más mini-gráfica en la barra total) a
/// la derecha.
class BarraProgreso : public QWidget {
    Q_OBJECT

public:
    enum class Variante {
        Total, ///< barra alta, con mini-gráfica de velocidad
        Archivo, ///< barra baja, solo texto
    };

    explicit BarraProgreso(Variante variante, QWidget *parent = nullptr);

    void establecerPorcentaje(int porcentaje);
    void establecerTextoIzquierda(const QString &texto);
    void establecerTextoDerecha(const QString &texto);

    /// Mini-gráfica de velocidad; nula en la variante `Archivo`.
    MiniGrafica *grafica() const { return m_grafica; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *evento) override;
    void resizeEvent(QResizeEvent *evento) override;

private:
    void recolocarGrafica();

    Variante m_variante;
    int m_porcentaje = 0;
    QString m_izquierda;
    QString m_derecha;
    MiniGrafica *m_grafica = nullptr;
};

} // namespace maxcopier
