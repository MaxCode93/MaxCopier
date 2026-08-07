#pragma once

#include "bandejatarea.h"

#include <QObject>
#include <QPointer>
#include <QString>

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace maxcopier {

class VentanaPrincipal;

/// Icono de bandeja propio de una ventana de copia.
///
/// El icono global de la aplicación sirve para crear y cancelar copias; este
/// icono representa una transferencia concreta y permite restaurarla,
/// pausarla o cancelarla cuando su ventana está escondida.
class BandejaCopia : public QObject {
    Q_OBJECT

public:
    explicit BandejaCopia(VentanaPrincipal *ventana);
    ~BandejaCopia() override;

    void mostrar();
    void ocultar();
    bool estaVisible() const;

    /// Repinta el progreso, el texto y el estado de las acciones.
    void actualizar();

    /// Enseña una notificación en este icono si la copia está minimizada aquí.
    void avisar(const QString &titulo, const QString &texto);

private:
    void construirMenu();
    void actualizarBarraDeTarea();
    void alActivado(int razon);

    QPointer<VentanaPrincipal> m_ventana;
    QSystemTrayIcon *m_icono = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_restaurar = nullptr;
    QAction *m_pausa = nullptr;
    QAction *m_cancelar = nullptr;
    BandejaDeTarea m_barraDeTarea;
};

} // namespace maxcopier
