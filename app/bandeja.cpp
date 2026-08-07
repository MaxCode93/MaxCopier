#include "bandeja.h"

#include "ventanaprincipal.h"
#include "vistas/iconos.h"

#include <QAction>
#include <QMenu>
#include <QSystemTrayIcon>

#include <utility>

namespace maxcopier {
namespace {

    constexpr int kMsAviso = 6000;

} // namespace

Bandeja::Bandeja(QObject *parent)
    : QObject(parent)
{
    m_icono = new QSystemTrayIcon(this);
    construirMenu();
    m_icono->setContextMenu(m_menu);
    connect(m_icono, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason razon) { alActivado(int(razon)); });

    m_icono->setIcon(iconoDeLaApp());
    m_icono->setToolTip(tr("MaxCopier"));
    m_icono->show();
}

Bandeja::~Bandeja()
{
    if (m_icono)
        m_icono->setContextMenu(nullptr);
    delete m_menu;
}

bool Bandeja::disponible()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void Bandeja::anadirCopia(VentanaPrincipal *ventana)
{
    if (!ventana)
        return;
    for (const QPointer<VentanaPrincipal> &copia : std::as_const(m_copias)) {
        if (copia.data() == ventana)
            return;
    }
    m_copias.append(ventana);
    refrescarMenu();
}

void Bandeja::quitarVentana(VentanaPrincipal *ventana)
{
    m_copias.removeIf([ventana](const QPointer<VentanaPrincipal> &copia) {
        return copia.isNull() || copia.data() == ventana;
    });
    refrescarMenu();
}

void Bandeja::actualizarEstado(VentanaPrincipal *ventana)
{
    Q_UNUSED(ventana);
    // El menú se refresca en `aboutToShow`; aquí solo se toca el icono si el
    // texto cambió de verdad (evita que el icono principal parpadee a cada
    // avance de progreso).
    refrescarIcono();
}

void Bandeja::avisar(const QString &titulo, const QString &texto)
{
    if (m_icono)
        m_icono->showMessage(titulo, texto, m_icono->icon(), kMsAviso);
}

void Bandeja::construirMenu()
{
    m_menu = new QMenu;

    m_estado = m_menu->addAction(tr("MaxCopier"));
    m_estado->setEnabled(false);
    m_menu->addSeparator();

    m_nuevaMenu = m_menu->addMenu(tr("Nueva copia"));
    QAction *copiar = m_nuevaMenu->addAction(tr("Copiar…"));
    QAction *mover = m_nuevaMenu->addAction(tr("Mover…"));

    m_cancelarTodas = m_menu->addAction(tr("Cancelar todas"));
    QAction *opciones = m_menu->addAction(tr("Opciones…"));
    m_menu->addSeparator();
    QAction *salir = m_menu->addAction(tr("Salir de MaxCopier"));

    connect(copiar, &QAction::triggered, this, [this] {
        emit nuevaCopiaPedido(ipc::Operacion::Copiar);
    });
    connect(mover, &QAction::triggered, this, [this] {
        emit nuevaCopiaPedido(ipc::Operacion::Mover);
    });
    connect(m_cancelarTodas, &QAction::triggered, this, &Bandeja::cancelarTodasPedido);
    connect(salir, &QAction::triggered, this, &Bandeja::salirPedido);
    connect(opciones, &QAction::triggered, this, &Bandeja::opcionesPedido);
    connect(m_menu, &QMenu::aboutToShow, this, &Bandeja::refrescarMenu);
}

void Bandeja::refrescarMenu()
{
    m_copias.removeIf([](const QPointer<VentanaPrincipal> &copia) { return copia.isNull(); });

    int activas = 0;
    for (const QPointer<VentanaPrincipal> &copia : std::as_const(m_copias)) {
        if (copia && copia->ocupada())
            ++activas;
    }

    m_estado->setText(activas == 0
            ? tr("MaxCopier")
            : tr("MaxCopier · %1 copia(s) activa(s)").arg(activas));
    m_cancelarTodas->setEnabled(activas > 0);
}

void Bandeja::refrescarIcono()
{
    int activas = 0;
    for (const QPointer<VentanaPrincipal> &copia : std::as_const(m_copias)) {
        if (copia && copia->ocupada())
            ++activas;
    }

    if (!m_icono)
        return;
    const QString texto = activas == 0
        ? tr("MaxCopier")
        : tr("MaxCopier · %1 copia(s) activa(s)").arg(activas);
    if (texto != m_ultimoToolTip) {
        m_ultimoToolTip = texto;
        m_icono->setToolTip(texto);
    }
}

void Bandeja::alActivado(int razon)
{
    // El icono global no representa una ventana: como en SuperCopier, las
    // ventanas de copia se restauran desde su propio icono. El menú contextual
    // sigue disponible mediante el botón derecho.
    Q_UNUSED(razon);
}

} // namespace maxcopier
