#pragma once

#include "lista/elementodecopia.h"
#include "politicas/colision.h"

#include <QDialog>

class QCheckBox;

namespace maxcopier {

/// Diálogo modal de colisión (mockup `docs/mockups/v3-dlg-colision.png`): el
/// archivo de llegada ya existe y hay que elegir **Sobrescribir**, **Renombrar**
/// o **Saltar**, con la casilla *Hacer lo mismo para todo* para no volver a
/// preguntar en el resto de la lista. Cerrarlo equivale a saltar el archivo.
class DialogoColision : public QDialog {
    Q_OBJECT

public:
    DialogoColision(const ElementoDeCopia &elemento, QWidget *parent = nullptr);

    /// Acción elegida (`Saltar` si se cerró el diálogo sin pulsar ningún botón).
    AccionColision accion() const { return m_accion; }

    /// La acción vale para el resto de colisiones de la lista.
    bool paraTodo() const;

protected:
    void mousePressEvent(QMouseEvent *evento) override;
    void resizeEvent(QResizeEvent *evento) override;

private:
    void elegir(AccionColision accion);

    AccionColision m_accion = AccionColision::Saltar;
    QCheckBox *m_paraTodo = nullptr;
};

} // namespace maxcopier
