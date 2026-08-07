#include "gestordeventanas.h"

#include "accionfinal.h"
#include "bandeja.h"
#include "diagnostico.h"
#include "dialogos/dialogoopciones.h"
#include "ventanaprincipal.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

#include <utility>

namespace maxcopier {
namespace {

    // Cada ventana nueva se coloca un poco más abajo y a la derecha.
    constexpr int kDesplazamiento = 28;

    QString resumenDePeticion(
        Operacion operacion, const QStringList &origenes, const QString &carpetaDestino)
    {
        const QString destino = QDir::toNativeSeparators(carpetaDestino);
        const QString verbo = operacion == Operacion::Mover
            ? QCoreApplication::translate("gestor", "mover")
            : QCoreApplication::translate("gestor", "copiar");
        if (origenes.size() == 1)
            return QCoreApplication::translate("gestor", "%1 «%2» a %3")
                .arg(verbo, QFileInfo(origenes.first()).fileName(), destino);
        return QCoreApplication::translate("gestor", "%1 %2 elementos a %3")
            .arg(verbo)
            .arg(origenes.size())
            .arg(destino);
    }

} // namespace

GestorDeVentanas::GestorDeVentanas(Configuracion *configuracion, QObject *parent)
    : QObject(parent)
    , m_configuracion(configuracion)
{
    if (!Bandeja::disponible())
        return;

    m_bandeja = new Bandeja(this);
    connect(m_bandeja, &Bandeja::nuevaCopiaPedido, this, &GestorDeVentanas::crearCopia);
    connect(m_bandeja, &Bandeja::cancelarTodasPedido, this, &GestorDeVentanas::cancelarTodas);
    connect(m_bandeja, &Bandeja::opcionesPedido, this, &GestorDeVentanas::mostrarOpciones);
    connect(m_bandeja, &Bandeja::salirPedido, this, &GestorDeVentanas::salir);
}

VentanaPrincipal *GestorDeVentanas::arrancar(
    const QStringList &origenes, const QString &carpetaDestino, Operacion operacion,
    bool desdePortapapeles)
{
    VentanaPrincipal *ventana = crearVentana(operacion);
    ventana->show();
    if (!origenes.isEmpty())
        ventana->iniciarCopia(operacion, origenes, carpetaDestino, desdePortapapeles);
    return ventana;
}

void GestorDeVentanas::crearCopia(Operacion operacion)
{
    if (m_saliendo)
        return;
    arrancar({}, QString(), operacion);
}

void GestorDeVentanas::mostrarOpciones()
{
    if (m_dialogoOpciones) {
        m_dialogoOpciones->show();
        m_dialogoOpciones->raise();
        m_dialogoOpciones->activateWindow();
        return;
    }

    m_dialogoOpciones = new DialogoOpciones(m_configuracion);
    m_dialogoOpciones->setAttribute(Qt::WA_DeleteOnClose);
    m_dialogoOpciones->show();
    m_dialogoOpciones->raise();
    m_dialogoOpciones->activateWindow();
}

VentanaPrincipal *GestorDeVentanas::crearVentana(Operacion operacion)
{
    auto *ventana = new VentanaPrincipal(operacion, m_configuracion);
    ventana->setAttribute(Qt::WA_DeleteOnClose);
    connect(ventana, &VentanaPrincipal::peticionDeCopia, this,
        [this](Operacion pedido, const QStringList &origenes, const QString &carpetaDestino) {
            atender(pedido, origenes, carpetaDestino, false);
        });
    connect(ventana, &VentanaPrincipal::accionFinalPedida, this,
        [this, ventana](AccionAlTerminar accion) { ejecutarAccionFinal(ventana, accion); });
    connect(ventana, &VentanaPrincipal::tandaTerminada, this,
        [this, ventana](bool completa) { alTerminarTanda(ventana, completa); });

    if (VentanaPrincipal *anterior = m_ventanas.isEmpty() ? nullptr : m_ventanas.last().data())
        ventana->move(anterior->pos() + QPoint(kDesplazamiento, kDesplazamiento));

    m_ventanas.append(ventana);

    if (m_bandeja) {
        m_bandeja->anadirCopia(ventana);
        connect(ventana, &VentanaPrincipal::estadoDeBandejaCambiado, this,
            [this, ventana] {
                if (m_bandeja)
                    m_bandeja->actualizarEstado(ventana);
            });
        connect(ventana, &VentanaPrincipal::avisoDeBandeja, this,
            [this, ventana](const QString &titulo, const QString &texto) {
                if (!ventana)
                    return;
                if (ventana->minimizadaEnBandeja())
                    ventana->avisarDesdeBandeja(titulo, texto);
                else if (m_bandeja)
                    m_bandeja->avisar(titulo, texto);
            });
    }

    connect(ventana, &QObject::destroyed, this, [this, ventana] {
        // Si una copia secundaria se cierra mientras había una acción de
        // energía pendiente, cancelar la acción es la opción segura.
        if (m_hayAccionDeEnergia)
            m_hayAccionDeEnergia = false;
        if (m_bandeja)
            m_bandeja->quitarVentana(ventana);
        m_ventanas.removeIf([](const QPointer<VentanaPrincipal> &v) { return v.isNull(); });
        if (m_ventanas.isEmpty() && (m_saliendo || !m_bandeja))
            QCoreApplication::quit();
    });
    return ventana;
}

void GestorDeVentanas::cancelarTodas()
{
    const QList<QPointer<VentanaPrincipal>> ventanas = m_ventanas;
    for (const QPointer<VentanaPrincipal> &ventana : ventanas) {
        if (ventana && ventana->ocupada())
            ventana->cerrarDefinitivamente();
    }
}

void GestorDeVentanas::salir()
{
    if (m_saliendo)
        return;

    // Si hay copias en curso (una o varias), se confirma antes de cancelarlas:
    // el usuario debe aceptar explícitamente perder esas transferencias.
    bool hayActivas = false;
    for (const QPointer<VentanaPrincipal> &ventana : std::as_const(m_ventanas)) {
        if (ventana && ventana->ocupada()) {
            hayActivas = true;
            break;
        }
    }
    if (hayActivas) {
        const QMessageBox::StandardButton respuesta = QMessageBox::question(
            nullptr, tr("Salir de MaxCopier"),
            tr("Hay copias en curso.\n¿Cancelarlas todas y salir de MaxCopier?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (respuesta != QMessageBox::Yes)
            return;
    }

    m_saliendo = true;
    if (m_dialogoOpciones)
        m_dialogoOpciones->close();

    // La salida no depende de QWidget::close(): una copia que ya está
    // escondida en su icono propio cancela explícitamente y programa su
    // destrucción. El proceso solo termina cuando la última emite
    // `destroyed` y sus hilos han quedado detenidos.
    const QList<QPointer<VentanaPrincipal>> ventanas = m_ventanas;
    for (const QPointer<VentanaPrincipal> &ventana : ventanas) {
        if (ventana)
            ventana->cerrarDefinitivamente();
    }
    if (m_ventanas.isEmpty())
        QCoreApplication::quit();
    else {
        // Red de seguridad: si alguna ventana no llega a destruirse (hilo
        // atascado en E/S), el proceso no puede quedarse vivo en segundo
        // plano; el sistema operativo limpia lo que falte.
        QTimer::singleShot(10000, qApp, &QCoreApplication::quit);
    }
}

void GestorDeVentanas::ejecutarAccionFinal(VentanaPrincipal *ventana, AccionAlTerminar accion)
{
    if (m_saliendo || !ventana || accion == AccionAlTerminar::Nada)
        return;

    if (accion == AccionAlTerminar::Cerrar) {
        ventana->cerrarDefinitivamente();
        return;
    }

    if (m_accionDeEnergiaLanzada)
        return;

    // Una acción de energía se ejecuta una sola vez. Si todavía hay otras
    // copias, queda pendiente hasta que terminen correctamente.
    if (!m_hayAccionDeEnergia) {
        m_hayAccionDeEnergia = true;
        m_accionDeEnergia = accion;
    }
    intentarAccionDeEnergia();
}

void GestorDeVentanas::alTerminarTanda(VentanaPrincipal *ventana, bool completa)
{
    Q_UNUSED(ventana);
    if (m_hayAccionDeEnergia && !completa) {
        m_hayAccionDeEnergia = false;
        if (m_bandeja)
            m_bandeja->avisar(tr("Acción final cancelada"),
                tr("No se ejecutó la acción de energía porque una transferencia no terminó correctamente."));
        return;
    }
    intentarAccionDeEnergia();
}

void GestorDeVentanas::intentarAccionDeEnergia()
{
    if (!m_hayAccionDeEnergia || m_saliendo)
        return;

    for (const QPointer<VentanaPrincipal> &ventana : std::as_const(m_ventanas)) {
        if (ventana && ventana->ocupada())
            return;
    }

    const AccionAlTerminar accion = m_accionDeEnergia;
    m_hayAccionDeEnergia = false;
    QString error;
    m_accionDeEnergiaLanzada = ejecutarAccionDeEnergia(accion, &error);
    if (!m_accionDeEnergiaLanzada) {
        if (m_bandeja)
            m_bandeja->avisar(tr("No se pudo ejecutar la acción final"), error);
        else
            anotar(error);
    }
}

void GestorDeVentanas::atender(
    Operacion operacion, const QStringList &origenes, const QString &carpetaDestino,
    bool desdePortapapeles)
{
    if (m_saliendo)
        return;

    VentanaPrincipal *primera = m_ventanas.isEmpty() ? nullptr : m_ventanas.first().data();

    // Con bandeja no existe una UI principal que traer al frente. Sin bandeja
    // conservamos una ventana vacía como fallback para que la app siga siendo
    // utilizable en escritorios que no ofrecen un área de notificación.
    if (origenes.isEmpty()) {
        if (m_bandeja)
            return;
        if (!primera)
            primera = arrancar({}, QString(), operacion);
        primera->mostrarDesdeBandeja();
        return;
    }

    // El menú «Copiar con MaxCopier» del Explorador manda los archivos sin
    // destino: es aquí donde se pregunta adónde van.
    QString pedido = carpetaDestino;
    if (pedido.isEmpty()) {
        if (primera)
            primera->mostrarDesdeBandeja();
        pedido = preguntarDestino(origenes);
        if (pedido.isEmpty())
            return;
    }

    const QString destino = QDir(pedido).absolutePath();
    VentanaPrincipal *destinataria = ventanaParaDestino(destino, operacion);
    if (destinataria && !destinataria->ocupada()) {
        destinataria->iniciarCopia(operacion, origenes, destino, desdePortapapeles);
        destinataria->mostrarDesdeBandeja();
        return;
    }

    if (!destinataria) {
        arrancar(origenes, destino, operacion, desdePortapapeles);
        return;
    }

    // La ventana que tocaba está copiando: lo decide el usuario.
    const bool permitirAnadir = destinataria->carpetaDestino() == destino
        && destinataria->operacion() == operacion;
    switch (decidir(destinataria, operacion, origenes, destino, permitirAnadir)) {
    case AccionListaActiva::AnadirALaActual:
        destinataria->iniciarCopia(operacion, origenes, destino, desdePortapapeles);
        destinataria->mostrarDesdeBandeja();
        return;
    case AccionListaActiva::VentanaNueva:
        arrancar(origenes, destino, operacion, desdePortapapeles);
        return;
    case AccionListaActiva::Cancelar:
        return;
    case AccionListaActiva::Preguntar:
        break;
    }
}

QString GestorDeVentanas::preguntarDestino(const QStringList &origenes) const
{
    VentanaPrincipal *primera = m_ventanas.isEmpty() ? nullptr : m_ventanas.first().data();
    return QFileDialog::getExistingDirectory(primera,
        QCoreApplication::translate("gestor", "Carpeta de destino"),
        QFileInfo(origenes.first()).absolutePath());
}

VentanaPrincipal *GestorDeVentanas::ventanaParaDestino(
    const QString &carpetaDestino, Operacion operacion) const
{
    VentanaPrincipal *ocupadaDelDestinoYOperacion = nullptr;
    VentanaPrincipal *ocupadaDelDestino = nullptr;
    VentanaPrincipal *primeraDisponible = nullptr;
    for (const QPointer<VentanaPrincipal> &ventana : m_ventanas) {
        if (!ventana || ventana->cancelando())
            continue;
        if (!primeraDisponible)
            primeraDisponible = ventana;
        if (!ventana->ocupada())
            return ventana;
        if (ventana->carpetaDestino() != carpetaDestino)
            continue;
        if (!ocupadaDelDestino)
            ocupadaDelDestino = ventana;
        if (!ocupadaDelDestinoYOperacion && ventana->operacion() == operacion)
            ocupadaDelDestinoYOperacion = ventana;
    }
    // Todas copiando: se pregunta en la del mismo destino y, si no hay, en la
    // primera, conservando el diálogo existente para decidir si abrir otra.
    if (ocupadaDelDestinoYOperacion)
        return ocupadaDelDestinoYOperacion;
    if (ocupadaDelDestino)
        return ocupadaDelDestino;
    return primeraDisponible;
}

AccionListaActiva GestorDeVentanas::decidir(VentanaPrincipal *ocupada, Operacion operacion,
    const QStringList &origenes, const QString &carpetaDestino, bool permitirAnadir)
{
    // «Añadir a la lista actual» no vale para otro destino: una lista, un destino.
    if (m_hayRecordada && (permitirAnadir || m_recordada != AccionListaActiva::AnadirALaActual))
        return m_recordada;

    if (!m_hayRecordada && m_configuracion) {
        const AccionListaActiva configurada = m_configuracion->accionListaActiva();
        if (configurada != AccionListaActiva::Preguntar
            && (permitirAnadir || configurada != AccionListaActiva::AnadirALaActual))
            return configurada;
    }

    // La ventana ocupada puede estar en su icono propio: se saca antes de
    // preguntar para que el diálogo no quede detrás de todo.
    ocupada->mostrarDesdeBandeja();
    DialogoListaActiva dialogo(resumenDePeticion(operacion, origenes, carpetaDestino),
        ocupada->resumenEnCurso(), permitirAnadir, ocupada);
    dialogo.exec(); // cerrarlo sin elegir equivale a cancelar
    if (dialogo.recordar()) {
        m_recordada = dialogo.accion();
        m_hayRecordada = true;
    }
    return dialogo.accion();
}

} // namespace maxcopier
