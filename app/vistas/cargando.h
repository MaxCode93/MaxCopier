#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QHideEvent;

namespace maxcopier {

class IndicadorCircular;

/// Superficie que bloquea la ventana mientras se enumera la lista de copia:
/// fondo translúcido, tarjeta centrada con indicador de actividad y un botón
/// para cancelar la enumeración. Se redimensiona con la ventana.
class Cargando : public QWidget {
    Q_OBJECT

public:
    explicit Cargando(QWidget *padre);

    void mostrarCargando(const QString &texto = QString());
    void establecerTexto(const QString &texto);

signals:
    void cancelarPedido();

protected:
    void paintEvent(QPaintEvent *evento) override;
    void resizeEvent(QResizeEvent *evento) override;
    void hideEvent(QHideEvent *evento) override;

private:
    QWidget *m_tarjeta = nullptr;
    QLabel *m_texto = nullptr;
    IndicadorCircular *m_indicador = nullptr;
};

} // namespace maxcopier
