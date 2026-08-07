#pragma once

#include <QDialog>

class QMouseEvent;
class QFrame;
class QResizeEvent;

namespace maxcopier {

class Configuracion;

/// Ventana de opciones que se abre desde la bandeja cuando no hay una copia
/// visible. El contenido se comparte con la pestaña de opciones.
class DialogoOpciones : public QDialog {
    Q_OBJECT

public:
    explicit DialogoOpciones(Configuracion *configuracion, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *evento) override;
    void mousePressEvent(QMouseEvent *evento) override;

private:
    QFrame *m_barra = nullptr;
};

} // namespace maxcopier
