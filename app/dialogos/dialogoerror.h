#pragma once

#include "politicas/acceso.h"

#include <QDialog>

class QCheckBox;

namespace maxcopier {

/// Diálogo modal de error de acceso (mockup
/// `docs/mockups/v3-dlg-error-no-encontrado.png`): el archivo de la lista no se
/// puede leer y hay que elegir **Reintentar**, **Poner al final** o **Saltar**,
/// con la casilla *Hacer lo mismo para todo*. Cerrarlo equivale a saltarlo.
class DialogoError : public QDialog {
    Q_OBJECT

public:
    DialogoError(const QString &origen, const QString &motivo, QWidget *parent = nullptr);

    /// Acción elegida (`Saltar` si se cerró el diálogo sin pulsar ningún botón).
    AccionError accion() const { return m_accion; }

    /// La acción vale para el resto de errores de la lista.
    bool paraTodo() const;

protected:
    void mousePressEvent(QMouseEvent *evento) override;
    void resizeEvent(QResizeEvent *evento) override;

private:
    void elegir(AccionError accion);

    AccionError m_accion = AccionError::Saltar;
    QCheckBox *m_paraTodo = nullptr;
};

} // namespace maxcopier
