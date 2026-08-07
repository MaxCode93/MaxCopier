#include "bandejacopia.h"

#include "ventanaprincipal.h"
#include "vistas/iconos.h"

#include <QAction>
#include <QMenu>
#include <QSystemTrayIcon>

namespace maxcopier {

BandejaCopia::BandejaCopia(VentanaPrincipal *ventana)
    : QObject(ventana)
    , m_ventana(ventana)
{
    m_icono = new QSystemTrayIcon(this);
    construirMenu();
    m_icono->setContextMenu(m_menu);
    connect(ventana, &VentanaPrincipal::estadoDeBandejaCambiado, this,
        &BandejaCopia::actualizar);
    connect(m_icono, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason razon) { alActivado(int(razon)); });

    m_icono->setIcon(iconoDeBandeja(0, false, false, false));
    actualizar();
}

BandejaCopia::~BandejaCopia()
{
    if (m_icono) {
        m_icono->hide();
        m_icono->setContextMenu(nullptr);
    }
    delete m_menu;
}

void BandejaCopia::mostrar()
{
    if (!m_icono || !QSystemTrayIcon::isSystemTrayAvailable())
        return;
    actualizar();
    m_icono->show();
}

void BandejaCopia::ocultar()
{
    if (m_icono)
        m_icono->hide();
}

bool BandejaCopia::estaVisible() const
{
    return m_icono && m_icono->isVisible();
}

void BandejaCopia::actualizar()
{
    if (!m_ventana)
        return;

    m_icono->setIcon(iconoDeBandeja(m_ventana->porcentajeBandeja(),
        m_ventana->ocupada(), m_ventana->pausada(),
        m_ventana->transferenciaTerminadaCorrectamente()));

    const QString velocidad = m_ventana->velocidadBandeja();
    const QString texto = m_ventana->ocupada()
        ? QStringLiteral("%1 · %2").arg(m_ventana->tituloBandeja(), velocidad)
        : m_ventana->tituloBandeja();
    m_icono->setToolTip(texto);

    m_pausa->setText(m_ventana->pausada() ? tr("Reanudar") : tr("Pausar"));
    m_pausa->setEnabled(m_ventana->copiando() || m_ventana->escaneando());
    m_cancelar->setEnabled(m_ventana->ocupada());

    actualizarBarraDeTarea();
}

void BandejaCopia::actualizarBarraDeTarea()
{
    if (!m_ventana)
        return;
    m_barraDeTarea.fijar(m_ventana, m_ventana->porcentajeBandeja(), 100,
        m_ventana->pausada(), m_ventana->ocupada());
}

void BandejaCopia::avisar(const QString &titulo, const QString &texto)
{
    if (estaVisible())
        m_icono->showMessage(titulo, texto, m_icono->icon(), 6000);
}

void BandejaCopia::construirMenu()
{
    m_menu = new QMenu;
    m_restaurar = m_menu->addAction(tr("Restaurar"));
    m_pausa = m_menu->addAction(tr("Pausar"));
    m_cancelar = m_menu->addAction(tr("Cancelar"));

    connect(m_restaurar, &QAction::triggered, this, [this] {
        if (m_ventana)
            m_ventana->mostrarDesdeBandeja();
    });
    connect(m_pausa, &QAction::triggered, this, [this] {
        if (m_ventana)
            m_ventana->pausarDesdeBandeja();
    });
    connect(m_cancelar, &QAction::triggered, this, [this] {
        if (m_ventana)
            m_ventana->cerrarDefinitivamente();
    });
}

void BandejaCopia::alActivado(int razon)
{
    if (!m_ventana)
        return;
    if (razon == int(QSystemTrayIcon::Trigger) || razon == int(QSystemTrayIcon::DoubleClick))
        m_ventana->mostrarDesdeBandeja();
}

} // namespace maxcopier
