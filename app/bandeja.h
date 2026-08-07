#pragma once

#include "ipc/protocolo.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace maxcopier {

class VentanaPrincipal;

/// Controlador global de la bandeja, equivalente al icono principal de
/// SuperCopier: no representa una ventana de copia y no muestra una UI propia.
/// Su menú sirve para crear transferencias, ver las activas, abrir opciones,
/// cancelarlas y cerrar el proceso.
class Bandeja : public QObject {
    Q_OBJECT

public:
    explicit Bandeja(QObject *parent = nullptr);
    ~Bandeja() override;

    /// Hay bandeja del sistema donde ponerse (en Linux sin bandeja, no).
    static bool disponible();

    void anadirCopia(VentanaPrincipal *ventana);
    void quitarVentana(VentanaPrincipal *ventana);

    /// Actualiza el menú y el icono global después de que cambie una copia.
    void actualizarEstado(VentanaPrincipal *ventana);

    /// Globo de notificación para eventos que no tienen un icono individual
    /// visible (por ejemplo, una copia que terminó mientras estaba abierta).
    void avisar(const QString &titulo, const QString &texto);

signals:
    void nuevaCopiaPedido(ipc::Operacion operacion);
    void cancelarTodasPedido();
    void opcionesPedido();
    void salirPedido();

private:
    void construirMenu();
    void refrescarMenu();
    void refrescarIcono();
    void alActivado(int razon);

    QList<QPointer<VentanaPrincipal>> m_copias;
    QSystemTrayIcon *m_icono = nullptr;
    QMenu *m_menu = nullptr;
    QMenu *m_nuevaMenu = nullptr;
    QAction *m_estado = nullptr;
    QAction *m_cancelarTodas = nullptr;
    QString m_ultimoToolTip;
};

} // namespace maxcopier
