#pragma once

#include "acciones.h"

#include <QDialog>

class QCheckBox;

namespace maxcopier {

/// Diálogo modal «Ya hay una copia en curso» (mockup
/// `docs/mockups/v3-dlg-lista-activa.png`). Cerrarlo equivale a cancelar.
class DialogoListaActiva : public QDialog {
    Q_OBJECT

public:
    /// `peticion` describe lo que se quiere copiar y `enCurso` lo que la
    /// ventana ocupada está copiando. Sin `permitirAnadir` (destino distinto al
    /// de esa lista) solo quedan la ventana nueva y cancelar.
    DialogoListaActiva(
        const QString &peticion, const QString &enCurso, bool permitirAnadir, QWidget *parent = nullptr);

    /// Acción elegida (`Cancelar` si se cerró el diálogo sin pulsar ningún botón).
    AccionListaActiva accion() const { return m_accion; }

    /// No volver a preguntar mientras la app siga abierta.
    bool recordar() const;

protected:
    void mousePressEvent(QMouseEvent *evento) override;
    void resizeEvent(QResizeEvent *evento) override;

private:
    void elegir(AccionListaActiva accion);

    AccionListaActiva m_accion = AccionListaActiva::Cancelar;
    QCheckBox *m_recordar = nullptr;
};

} // namespace maxcopier
