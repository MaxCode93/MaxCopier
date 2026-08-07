#include "ventanaprincipal.h"

#include "bandejacopia.h"
#include "copia/limitadorvelocidad.h"
#include "dialogos/dialogocolision.h"
#include "dialogos/dialogoerror.h"
#include "escaneo/escaner.h"
#include "lista/listadecopia.h"
#include "portapapeles.h"
#include "util/formatos.h"
#include "util/espaciolibre.h"
#include "util/rutas.h"
#include "util/titulos.h"
#include "vistas/barratitulo.h"
#include "vistas/cargando.h"
#include "vistas/esquinas.h"
#include "vistas/panelcompacto.h"
#include "vistas/panelexpandido.h"

#include <QDesktopServices>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QStorageInfo>
#include <QSystemTrayIcon>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace maxcopier {
namespace {

    // Muestras que caben en la mini-gráfica de velocidad.
    constexpr int kMuestrasGrafica = 13;
    constexpr int kAnchoVentana = 580;
    constexpr int kAltoLista = 300; // lo que crece la ventana al expandirse
    constexpr int kMsRefrescoEstado = 500;

    QString nombreDeUnidad(const QStorageInfo &unidad)
    {
        const QString etiqueta = unidad.displayName();
        const QString raiz = QDir::toNativeSeparators(unidad.rootPath());
        return etiqueta.isEmpty() || etiqueta == raiz ? raiz : QStringLiteral("[%1] %2").arg(raiz, etiqueta);
    }

    /// Rutas locales de lo que se ha soltado en la ventana (lo demás se ignora:
    /// de una página web o del correo no hay nada que copiar).
    QStringList rutasSoltadas(const QMimeData *datos)
    {
        QStringList rutas;
        if (!datos || !datos->hasUrls())
            return rutas;
        for (const QUrl &url : datos->urls()) {
            const QString ruta = url.toLocalFile();
            if (!ruta.isEmpty())
                rutas.append(ruta);
        }
        return rutas;
    }

    bool quitarDirectoriosVacios(const QStringList &raices)
    {
        QStringList directorios;
        for (const QString &raiz : raices) {
            const QFileInfo info(raiz);
            if (!info.exists() || !info.isDir())
                continue;

            QDirIterator it(info.absoluteFilePath(),
                QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                QDirIterator::Subdirectories);
            while (it.hasNext())
                directorios.append(it.next());
            directorios.append(info.absoluteFilePath());
        }

        // Primero se quitan las hojas; así solo se borran carpetas que siguen
        // vacías después de haber movido todos sus archivos.
        std::sort(directorios.begin(), directorios.end(), [](const QString &izquierda, const QString &derecha) {
            return izquierda.size() > derecha.size();
        });
        for (const QString &directorio : directorios)
            QDir().rmdir(directorio);

        for (const QString &raiz : raices) {
            if (QFileInfo::exists(raiz))
                return false;
        }
        return true;
    }

    /// Explorer puede entregar un archivo y la carpeta que ya lo contiene.
    /// Mantener ambos orígenes duplica filas y, en modo mover, provoca que la
    /// segunda intente borrar un archivo que la primera ya movió.
    QStringList quitarOrigenesRedundantes(const QStringList &origenes)
    {
        QStringList resultado;
        for (const QString &origen : origenes) {
            if (origen.isEmpty())
                continue;

            const QString absoluto = QFileInfo(origen).absoluteFilePath();
            bool redundante = false;
            for (const QString &existente : resultado) {
                if (rutasIguales(absoluto, existente)
                    || (QFileInfo(existente).isDir() && rutaDescendienteDe(absoluto, existente))) {
                    redundante = true;
                    break;
                }
            }
            if (redundante)
                continue;

            for (int i = resultado.size() - 1; i >= 0; --i) {
                if (QFileInfo(absoluto).isDir() && rutaDescendienteDe(resultado.at(i), absoluto))
                    resultado.removeAt(i);
            }
            resultado.append(absoluto);
        }
        return resultado;
    }

    QString motivoDeSolapamiento(const QString &origen, const QString &destino, const QString &verbo)
    {
        const QFileInfo info(origen);
        const QString destinoFinal = destinoDeOrigen(origen, destino);
        if (info.isDir() && rutaDescendienteDe(destinoFinal, origen))
            return QObject::tr("No se puede %1 una carpeta dentro de sí misma. Elige otra carpeta de destino.")
                .arg(verbo);
        if (rutasIguales(origen, destinoFinal))
            return QObject::tr("El origen y el destino final son el mismo archivo. Elige otra carpeta de destino.");
        return {};
    }

    /// Cambia la raíz de una ruta cuando el volumen volvió con otra letra.
    QString remapearRuta(const QString &ruta, const QString &raizVieja, const QString &raizNueva)
    {
        if (ruta.isEmpty() || raizVieja.isEmpty() || raizVieja.size() < 2
            || raizVieja == raizNueva)
            return ruta;
        if (ruta == raizVieja)
            return raizNueva;
        if (ruta.startsWith(raizVieja + QLatin1Char('/')))
            return raizNueva + ruta.mid(raizVieja.size());
        return ruta;
    }

} // namespace

VentanaPrincipal::VentanaPrincipal(ipc::Operacion operacion, Configuracion *configuracion, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
    , m_configuracion(configuracion)
    , m_operacion(operacion)
{
    setObjectName(QStringLiteral("ventana"));
    setWindowTitle(tr("MaxCopier"));

    m_lista = new ListaDeCopia(this);

    construirInterfaz();
    arrancarHilos();
    if (m_configuracion) {
        connect(m_configuracion, &Configuracion::configuracionCambiada, this,
            &VentanaPrincipal::aplicarConfiguracion);
        connect(m_configuracion, &Configuracion::temaCambiado, this,
            &VentanaPrincipal::refrescarTema);
    }
    mostrarInactivo();
    m_bandejaCopia = new BandejaCopia(this);
    connect(this, &VentanaPrincipal::estadoDeBandejaCambiado, m_bandejaCopia,
        &BandejaCopia::actualizar);

    // Arrastrar y soltar sobre cualquier parte de la ventana: los widgets de
    // dentro no aceptan sueltas, así que el evento llega hasta aquí.
    setAcceptDrops(true);

    m_altoCompacto = sizeHint().height();
    setFixedSize(kAnchoVentana, m_altoCompacto);

    // Overlay que bloquea la ventana mientras se enumera la lista.
    m_cargando = new Cargando(this);
    m_cargando->setGeometry(rect());
    connect(m_cargando, &Cargando::cancelarPedido, this,
        &VentanaPrincipal::cancelarEnumeracion);

    // Vigilancia del dispositivo de destino: si se desconecta, la copia se
    // pausa sola y se reanuda al volver a aparecer.
    m_relojDispositivo = new QTimer(this);
    m_relojDispositivo->setInterval(1000);
    connect(m_relojDispositivo, &QTimer::timeout, this,
        &VentanaPrincipal::comprobarDispositivoDestino);
    m_relojDispositivo->start();
}

VentanaPrincipal::~VentanaPrincipal()
{
    // La destrucción normal solo se programa después de que todos los hilos
    // hayan terminado. Este drenaje defensivo también cubre una destrucción
    // directa desde un llamador externo: el limitador no se libera mientras un
    // motor pueda seguir usándolo.
    cancelarTrabajo();
    for (QThread *hilo : m_hilos) {
        if (hilo)
            hilo->quit();
    }
    if (m_hiloEscaner)
        m_hiloEscaner->quit();
    for (QThread *hilo : m_hilos) {
        if (hilo)
            hilo->wait();
    }
    if (m_hiloEscaner)
        m_hiloEscaner->wait();
    delete m_limitador;
}

void VentanaPrincipal::closeEvent(QCloseEvent *evento)
{
    if (!m_cierreDefinitivo) {
        // `WA_DeleteOnClose` no debe poder destruir la ventana mientras un
        // motor está dentro de `copiar()`. La destrucción se solicita después
        // de cancelar y esperar las señales `finished` de todos los hilos.
        evento->ignore();
        cerrarDefinitivamente();
        return;
    }
    evento->accept();
}

void VentanaPrincipal::cancelarTrabajo()
{
    m_cancelandoTrabajo = true;
    if (m_escaner)
        m_escaner->cancelar();
    for (MotorDeCopia *motor : m_motores)
        if (motor)
            motor->cancelar();
    comprobarCancelacion();
}

void VentanaPrincipal::comprobarCancelacion()
{
    if (m_cierreDefinitivo || !m_cancelandoTrabajo || m_escaneando
        || m_comprobandoEspacio || !m_activas.isEmpty())
        return;
    mostrarTransferenciaCancelada();
}

void VentanaPrincipal::pausarDesdeBandeja()
{
    // La pausa manual no compite con la pausa por dispositivo desconectado.
    if (m_pausaPorDispositivo)
        return;
    if (m_copiando && !m_activas.isEmpty()) {
        // La decisión se toma una sola vez y se envía como estado absoluto a
        // todos los motores. Con varios archivos, alternar cada motor a partir
        // de su estado observado podía dejar uno de ellos en pausa mientras
        // los demás reanudaban.
        const bool pausada = !m_pausada;
        m_pausada = pausada;
        establecerPausaDeMotores(pausada);
        m_pausada = pausada;
        m_panel->mostrarPausado(pausada);
        // La pausa no termina ninguna fila: conservar el último progreso y
        // repintarlo evita que la barra segmentada parezca una lista vacía
        // mientras los motores esperan en su hilo.
        mostrarArchivosEnCurso();
        refrescarTotal();
        anotarSesion(pausada ? tr("Transferencia pausada") : tr("Transferencia reanudada"));
        if (!pausada)
            rellenarMotores();
        emit estadoDeBandejaCambiado();
    } else if (m_escaner && m_escaneando) {
        // El hilo del escáner está ocupado dentro de `escanear()`, por lo que
        // una llamada encolada no se ejecutaría hasta terminar. El método solo
        // toca una bandera atómica y es seguro invocarlo directamente.
        const bool pausada = !m_pausada;
        m_pausada = pausada;
        m_escaner->establecerPausa(pausada);
        m_panel->mostrarPausado(pausada);
        anotarSesion(pausada ? tr("Enumeración pausada") : tr("Enumeración reanudada"));
    }
}

void VentanaPrincipal::establecerPausaDeMotores(bool pausada)
{
    for (const CopiaActiva &activa : std::as_const(m_activas)) {
        if (!activa.motor)
            continue;
        // Fijar el estado es idempotente: una pausa automática o una petición
        // recibida justo antes no puede invertir accidentalmente este objetivo.
        activa.motor->establecerPausa(pausada);
    }
}

void VentanaPrincipal::cancelarDesdeBandeja()
{
    cancelarTrabajo();
}

void VentanaPrincipal::cancelarEnumeracion()
{
    if (m_copiando) {
        // Hay una tanda en curso: el overlay solo cancela la enumeración de los
        // archivos añadidos; la copia sigue.
        if (m_escaner)
            m_escaner->cancelar();
        anotarSesion(tr("Enumeración añadida cancelada"));
    } else {
        cancelarTrabajo();
        anotarSesion(tr("Tanda cancelada desde la enumeración"));
    }
}

void VentanaPrincipal::archivoSeleccionado(const QString &fuente)
{
    m_archivoParaSaltar = fuente;
}

void VentanaPrincipal::saltarArchivo()
{
    MotorDeCopia *objetivo = nullptr;
    if (!m_archivoParaSaltar.isEmpty()) {
        for (const CopiaActiva &activa : m_activas) {
            if (activa.elemento.fuente == m_archivoParaSaltar) {
                objetivo = activa.motor;
                break;
            }
        }
    }
    if (!objetivo && !m_activas.isEmpty())
        objetivo = m_activas.first().motor;
    if (objetivo)
        objetivo->saltar();
}

bool VentanaPrincipal::dispositivoDestinoAusente() const
{
    if (m_carpetaDestino.isEmpty())
        return false;
    const QStorageInfo unidad(m_carpetaDestino);
    return !unidad.isValid() || unidad.bytesTotal() <= 0;
}

void VentanaPrincipal::pausarPorDispositivo()
{
    if (m_pausaPorDispositivo)
        return;
    m_pausaPorDispositivo = true;

    // Solo se pausan los motores que están copiando; la pausa manual del
    // usuario no se toca.
    m_pausadosPorDispositivo.clear();
    for (MotorDeCopia *motor : m_motores) {
        if (estaActivo(motor) && !motor->pausada()) {
            motor->establecerPausa(true);
            m_pausadosPorDispositivo.append(motor);
        }
    }

    m_panel->mostrarPausado(true);
    mostrarArchivosEnCurso();
    mostrarTitulo(tr("Copia en pausa · dispositivo desconectado"));
    anotarSesion(tr("Dispositivo desconectado: copia en pausa"));
    emit avisoDeBandeja(tr("Dispositivo desconectado"),
        tr("La copia se ha pausado. Se reanudará al volver a conectar el dispositivo."));
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::reanudarPorDispositivo()
{
    if (!m_pausaPorDispositivo)
        return;
    m_pausaPorDispositivo = false;

    for (MotorDeCopia *motor : std::as_const(m_pausadosPorDispositivo)) {
        if (motor->pausada())
            motor->establecerPausa(false);
    }
    m_pausadosPorDispositivo.clear();

    // Al reconectar puede haber cambiado el espacio disponible: el presupuesto
    // de «Continuar de todas formas» vuelve a mirar el volumen real.
    for (auto it = m_presupuesto.begin(); it != m_presupuesto.end(); ++it)
        it.value() = qMax<qint64>(0, QStorageInfo(it.key()).bytesAvailable());

    m_panel->mostrarPausado(m_pausada);
    mostrarArchivosEnCurso();
    mostrarTitulo(tr("Copia reanudada"));
    anotarSesion(tr("Dispositivo reconectado: copia reanudada"));
    emit avisoDeBandeja(tr("Dispositivo reconectado"),
        tr("La copia se ha reanudado desde donde se quedó."));
    rellenarMotores();
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::comprobarDispositivoDestino()
{
    if (!m_copiando && !m_escaneando && !m_pausaPorDispositivo)
        return;

    const bool ausente = dispositivoDestinoAusente();
    if (!ausente && !m_pausaPorDispositivo) {
        // Mientras está conectado, se recuerda la identidad del volumen (no la
        // letra): así se le reconoce aunque vuelva como otra letra de unidad.
        m_identidadDestino = identidadDeVolumen(m_carpetaDestino);
        return;
    }
    if (ausente && !m_pausaPorDispositivo) {
        pausarPorDispositivo();
        return;
    }
    if (!m_pausaPorDispositivo)
        return;

    // Pausado por dispositivo: ¿volvió la misma letra?
    if (!ausente) {
        reanudarPorDispositivo();
        return;
    }

    // ¿Volvió con otra letra? Se busca por identidad y se re-mapean los destinos.
    const QString nuevaRaiz = raizConIdentidad(m_identidadDestino);
    if (!nuevaRaiz.isEmpty() && nuevaRaiz != volumenDe(m_carpetaDestino))
        reanudarConOtraLetra(nuevaRaiz);
}

void VentanaPrincipal::reanudarConOtraLetra(const QString &nuevaRaiz)
{
    const QString viejaRaiz = volumenDe(m_carpetaDestino);
    if (viejaRaiz.isEmpty() || viejaRaiz == nuevaRaiz)
        return;

    m_remapeoViejo = viejaRaiz;
    m_remapeoNuevo = nuevaRaiz;

    // Destino de la ventana y filas pendientes.
    m_carpetaDestino = remapearRuta(m_carpetaDestino, viejaRaiz, nuevaRaiz);
    m_lista->remapearDestinos(viejaRaiz, nuevaRaiz);

    // Los archivos en curso siguen apuntando a la letra vieja: se cancelan y
    // se vuelven a colar con el destino re-mapeado (reanudan su `.mcpart`).
    m_recolocandoActivas = true;
    for (MotorDeCopia *motor : m_motores) {
        if (estaActivo(motor))
            motor->cancelar();
    }

    m_pausaPorDispositivo = false;
    m_pausadosPorDispositivo.clear();
    m_presupuesto.clear(); // el espacio del volumen nuevo se vuelve a mirar
    m_panel->mostrarPausado(m_pausada);
    mostrarUnidadDe(m_carpetaDestino);
    mostrarTitulo(tr("Copia reanudada · %1").arg(QDir::toNativeSeparators(nuevaRaiz)));
    anotarSesion(tr("Dispositivo reconectado como %1: destinos re-mapeados")
                     .arg(QDir::toNativeSeparators(nuevaRaiz)));
    emit avisoDeBandeja(tr("Dispositivo reconectado"),
        tr("El dispositivo volvió como %1. La copia continúa desde donde se quedó.")
            .arg(QDir::toNativeSeparators(nuevaRaiz)));
    rellenarMotores();
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::comprobarEspacioYPresupuestar()
{
    m_detenidaPorEspacio = false;
    m_presupuesto.clear();
    if (m_configuracion && !m_configuracion->comprobarEspacioLibre()) {
        m_comprobandoEspacio = false;
        if (m_cargando)
            m_cargando->hide();
        rellenarMotores();
        return;
    }

    // Solo se cuentan los pendientes: los archivos en curso ya están usando su
    // espacio (y al añadir a una copia activa no hay que pedirlo otra vez).
    ElementosDeCopia pendientes;
    for (const ElementoDeCopia &elemento : m_lista->elementos()) {
        bool activo = false;
        for (const CopiaActiva &copiando : m_activas) {
            if (copiando.elemento.fuente == elemento.fuente) {
                activo = true;
                break;
            }
        }
        if (!activo)
            pendientes.append(elemento);
    }
    if (pendientes.isEmpty()) {
        m_comprobandoEspacio = false;
        if (m_cargando)
            m_cargando->hide();
        rellenarMotores();
        return;
    }

    // `faltasDeEspacio` puede recorrer miles de destinos y consultar los
    // volúmenes montados. Nunca se ejecuta en el hilo de la UI: el escáner ya
    // está libre y reutiliza su hilo de trabajo para esta preparación final.
    m_comprobandoEspacio = true;
    m_generacionComprobacion = m_generacionEscaneo;
    if (m_cargando)
        m_cargando->establecerTexto(tr("Comprobando espacio libre…"));
    emit comprobacionEspacioPedida(pendientes, m_generacionComprobacion);
}

bool VentanaPrincipal::aceptarFaltasDeEspacio(const QList<FaltaDeEspacio> &faltas)
{
    m_comprobandoEspacio = false;
    if (m_cargando)
        m_cargando->hide();
    if (faltas.isEmpty()) {
        rellenarMotores();
        return true;
    }

    mostrarDesdeBandeja();
    QStringList lineas;
    for (const FaltaDeEspacio &falta : faltas) {
        lineas.append(tr("%1 — se necesitan %2 y hay %3 libres (faltan %4)")
                          .arg(QDir::toNativeSeparators(falta.volumen),
                              formatearTamano(falta.necesitado),
                              formatearTamano(falta.disponible),
                              formatearTamano(falta.falta())));
    }
    anotarSesion(tr("Falta espacio en el destino: %1").arg(lineas.join(QStringLiteral("; "))));
    const QMessageBox::StandardButton respuesta = QMessageBox::question(this,
        tr("Espacio insuficiente"),
        tr("No hay espacio suficiente en el destino:\n\n%1\n\n"
           "¿Continuar de todas formas? Se copiará solo hasta donde quepa.")
            .arg(lineas.join(QLatin1Char('\n'))),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (respuesta != QMessageBox::Yes) {
        // Se retiran solo los pendientes: si una copia está en curso sigue
        // viva; si no, es una tanda nueva que se cancela.
        m_lista->vaciar();
        if (m_copiando) {
            m_archivosTotales = m_lista->archivos();
            m_bytesTotales = m_lista->bytes();
            anotarSesion(tr("No se añadieron los archivos: falta espacio"));
            emit avisoDeBandeja(tr("No se añadieron los archivos"),
                tr("No hay espacio suficiente en el destino."));
        } else {
            anotarSesion(tr("Tanda cancelada: falta espacio en el destino"));
            mostrarTransferenciaCancelada();
        }
        return false;
    }

    anotarSesion(tr("Continuar de todas formas: se copiará hasta donde quepa"));
    // Presupuesto por volumen: al continuar, cada archivo descuenta de lo que
    // quedaba y la tanda se detiene cuando el siguiente ya no cabe.
    for (const FaltaDeEspacio &falta : faltas)
        m_presupuesto.insert(falta.volumen, falta.disponible);
    return true;
}

void VentanaPrincipal::detenerPorFaltaDeEspacio(const ElementoDeCopia &elemento)
{
    m_detenidaPorEspacio = true;
    m_movimientoCompleto = false;
    m_copiando = false;
    m_pausada = false;
    m_relojEstado->stop();
    m_panel->habilitarControles(false);
    m_panel->mostrarPausado(false);

    mostrarTitulo(tr("Copia detenida · sin espacio"));
    const QString volumen = QDir::toNativeSeparators(volumenDe(elemento.destino));
    anotarSesion(tr("Detenida por falta de espacio: «%1» no cabe en %2")
                     .arg(QFileInfo(elemento.fuente).fileName(), volumen));
    emit avisoDeBandeja(tr("Copia detenida · sin espacio"),
        tr("«%1» ya no cabe en %2.\nSe copiaron %3 archivos; los restantes quedan "
           "en la lista. Libera espacio y vuelve a intentarlo.")
            .arg(QFileInfo(elemento.fuente).fileName(), volumen)
            .arg(m_archivosHechos));
    mostrarArchivosEnCurso();
    refrescarEstado();
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::cerrarDefinitivamente()
{
    if (m_cierreDefinitivo)
        return;

    // La copia suele estar escondida cuando esta ruta viene del menú de la
    // bandeja. Se cancela explícitamente y se oculta, pero la destrucción queda
    // pendiente hasta que todos los motores y el escáner hayan abandonado sus
    // slots bloqueantes. Así el limitador y la ventana no pueden desaparecer
    // debajo de una E/S todavía activa.
    m_cierreDefinitivo = true;
    cancelarTrabajo();
    m_activas.clear();
    m_relojEstado->stop();
    m_relojDispositivo->stop();
    if (m_cargando)
        m_cargando->hide();
    hide();
    solicitarParadaDeHilos();
    comprobarCierreSeguro();
}

void VentanaPrincipal::solicitarParadaDeHilos()
{
    // `quit()` no interrumpe un slot que ya está ejecutándose, pero deja la
    // petición preparada: cuando `copiar()`/`escanear()` devuelva tras observar
    // la bandera atómica de cancelación, el event loop del hilo termina.
    for (QThread *hilo : m_hilos) {
        if (hilo)
            hilo->quit();
    }
    if (m_hiloEscaner)
        m_hiloEscaner->quit();
}

void VentanaPrincipal::comprobarCierreSeguro()
{
    if (!m_cierreDefinitivo || m_destruccionProgramada)
        return;
    for (QThread *hilo : m_hilos) {
        if (hilo && hilo->isRunning())
            return;
    }
    if (m_hiloEscaner && m_hiloEscaner->isRunning())
        return;

    m_destruccionProgramada = true;
    deleteLater();
}

void VentanaPrincipal::resizeEvent(QResizeEvent *evento)
{
    QWidget::resizeEvent(evento);
    if (m_cargando)
        m_cargando->setGeometry(rect());
    redondearEsquinas(this);
}

void VentanaPrincipal::construirInterfaz()
{
    auto *columna = new QVBoxLayout(this);
    columna->setContentsMargins(1, 1, 1, 1);
    columna->setSpacing(0);

    m_barraTitulo = new BarraTitulo(this);
    m_panel = new PanelCompacto(this);
    m_expandido = new PanelExpandido(m_configuracion, this);
    m_expandido->establecerLista(m_lista);
    m_expandido->hide();

    columna->addWidget(m_barraTitulo);
    columna->addWidget(m_panel);
    columna->addWidget(m_expandido, 1);

    connect(m_barraTitulo, &BarraTitulo::temaPedido, this, &VentanaPrincipal::alternarTema);
    connect(m_barraTitulo, &BarraTitulo::bandejaPedida, this, &VentanaPrincipal::ocultarEnBandeja);
    connect(m_barraTitulo, &BarraTitulo::minimizarPedido, this, &QWidget::showMinimized);
    connect(m_barraTitulo, &BarraTitulo::cerrarPedido, this, &QWidget::close);

    connect(m_panel, &PanelCompacto::anadirPedido, this, &VentanaPrincipal::elegirOrigenes);
    connect(m_panel, &PanelCompacto::abrirOrigenPedido, this, &VentanaPrincipal::abrirOrigen);
    connect(m_panel, &PanelCompacto::abrirDestinoPedido, this, &VentanaPrincipal::abrirDestino);
    connect(m_panel, &PanelCompacto::detallesPedido, this, &VentanaPrincipal::alternarDetalles);
    connect(m_panel, &PanelCompacto::accionFinalPedida, this,
        [this](AccionAlTerminar accion) {
            if (ocupada())
                m_accionFinalTanda = accion;
            if (m_configuracion)
                m_configuracion->establecerAccionAlTerminar(accion);
            aplicarConfiguracion();
        });

    connect(m_expandido, &PanelExpandido::anadirPedido, this, &VentanaPrincipal::elegirOrigenes);
    connect(m_expandido, &PanelExpandido::guardarListaPedida, this,
        &VentanaPrincipal::guardarLista);
    connect(m_expandido, &PanelExpandido::cargarListaPedida, this,
        &VentanaPrincipal::cargarLista);
    connect(m_panel, &PanelCompacto::saltarPedido, this, &VentanaPrincipal::saltarArchivo);
    connect(m_panel, &PanelCompacto::segmentoClicado, this,
        [this](int indice) {
            if (indice >= 0 && indice < m_activas.size())
                archivoSeleccionado(m_activas.at(indice).elemento.fuente);
        });
    connect(m_expandido, &PanelExpandido::archivoSeleccionado, this,
        &VentanaPrincipal::archivoSeleccionado);
    connect(m_lista, &ListaDeCopia::cambiada, this, &VentanaPrincipal::refrescarEstado);

    auto *atajoTema = new QShortcut(QKeySequence(QStringLiteral("Ctrl+T")), this);
    connect(atajoTema, &QShortcut::activated, this, &VentanaPrincipal::alternarTema);
    auto *atajoDetalles = new QShortcut(QKeySequence(QStringLiteral("Ctrl+D")), this);
    connect(atajoDetalles, &QShortcut::activated, this, &VentanaPrincipal::alternarDetalles);

    m_relojEstado = new QTimer(this);
    m_relojEstado->setInterval(kMsRefrescoEstado);
    connect(m_relojEstado, &QTimer::timeout, this, &VentanaPrincipal::refrescarEstado);
}

void VentanaPrincipal::arrancarHilos()
{
    qRegisterMetaType<QList<FaltaDeEspacio>>();

    // Pool de motores: uno por «Archivos a la vez» (1 por defecto). Comparten
    // el límite de velocidad de la ventana, de modo que N archivos a la vez no
    // multiplican el límite.
    m_limitador = new LimitadorVelocidad;
    if (m_configuracion)
        m_limitador->establecerLimite(m_configuracion->limiteVelocidad());
    m_archivosALaVez = m_configuracion ? m_configuracion->archivosALaVez() : 1;

    for (int i = 0; i < m_archivosALaVez; ++i) {
        auto *hilo = new QThread(this);
        auto *motor = new MotorDeCopia;
        motor->moveToThread(hilo);
        if (m_configuracion)
            motor->establecerMetodo(m_configuracion->metodoDeCopia());
        motor->establecerLimitadorCompartido(m_limitador);
        connect(hilo, &QThread::finished, motor, &QObject::deleteLater);
        connect(hilo, &QThread::finished, this, [this, motor] {
            // El motor se destruye al terminar su hilo. Quitar el puntero antes
            // de que pueda volver a entrar el destructor evita cancelar un
            // QObject ya liberado durante el cierre diferido.
            m_motores.removeAll(motor);
            comprobarCierreSeguro();
        });
        connect(motor, &MotorDeCopia::iniciada, this,
            [this, motor](const QString &origen, const QString &destino, qint64 tamano) {
                alIniciada(motor, origen, destino, tamano);
            });
        connect(motor, &MotorDeCopia::progreso, this,
            [this, motor](qint64 copiado, qint64 total, double velocidad, qint64 restante) {
                alProgreso(motor, copiado, total, velocidad, restante);
            });
        connect(motor, &MotorDeCopia::pausaCambiada, this,
            [this, motor](bool pausada) { alPausaMotorCambiada(motor, pausada); });
        connect(motor, &MotorDeCopia::terminada, this,
            [this, motor](MotorDeCopia::Resultado resultado, const QString &error) {
                alTerminada(motor, resultado, error);
            });
        m_hilos.append(hilo);
        m_motores.append(motor);
        hilo->start();
    }

    m_hiloEscaner = new QThread(this);
    m_escaner = new Escaner;
    m_escaner->moveToThread(m_hiloEscaner);
    connect(m_hiloEscaner, &QThread::finished, m_escaner, &QObject::deleteLater);
    connect(m_hiloEscaner, &QThread::finished, this, [this] {
        // Las señales de finalización del escáner pueden estar encoladas en la
        // UI cuando el hilo ya acabó. El objeto no debe volver a usarse desde
        // esos caminos ni desde el destructor.
        m_escaner = nullptr;
        comprobarCierreSeguro();
    });

    connect(this, &VentanaPrincipal::escaneoPedido, m_escaner, &Escaner::escanear);
    connect(this, &VentanaPrincipal::comprobacionEspacioPedida, m_escaner,
        &Escaner::comprobarEspacio);
    connect(m_escaner, &Escaner::encontrados, this, &VentanaPrincipal::alEncontrados);
    connect(m_escaner, &Escaner::directoriosEncontrados, this,
        &VentanaPrincipal::alDirectoriosEncontrados);
    connect(m_escaner, &Escaner::pausaCambiada, this, &VentanaPrincipal::alPausaCambiada);
    connect(m_escaner, &Escaner::terminado, this, &VentanaPrincipal::alEscaneoTerminado);
    connect(m_escaner, &Escaner::espacioComprobado, this,
        &VentanaPrincipal::alEspacioComprobado);
    connect(m_panel, &PanelCompacto::pausarPedido, this, &VentanaPrincipal::pausarDesdeBandeja);
    connect(m_panel, &PanelCompacto::cancelarPedido, this, &VentanaPrincipal::cancelarDesdeBandeja);

    m_hiloEscaner->start();
}

void VentanaPrincipal::ocultarEnBandeja()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable() || !m_bandejaCopia) {
        m_minimizadaEnBandeja = false;
        showMinimized();
        return;
    }

    m_minimizadaEnBandeja = true;
    hide();
    m_bandejaCopia->mostrar();
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::mostrarDesdeBandeja()
{
    m_minimizadaEnBandeja = false;
    if (m_bandejaCopia)
        m_bandejaCopia->ocultar();
    show();
    if (isMinimized())
        showNormal();
    raise();
    activateWindow();
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::mostrarTitulo(const QString &titulo)
{
    m_titulo = titulo;
    m_barraTitulo->establecerTitulo(titulo);
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::mostrarPorcentaje(int porcentajeHecho)
{
    m_porcentaje = qBound(0, porcentajeHecho, 100);
    m_barraTitulo->establecerPorcentaje(m_porcentaje);
    emit estadoDeBandejaCambiado();
}

QString VentanaPrincipal::velocidadBandeja() const
{
    return formatearVelocidad(m_velocidadMedia);
}

void VentanaPrincipal::avisarDesdeBandeja(const QString &titulo, const QString &texto)
{
    if (m_bandejaCopia)
        m_bandejaCopia->avisar(titulo, texto);
}

void VentanaPrincipal::dragEnterEvent(QDragEnterEvent *evento)
{
    if (!rutasSoltadas(evento->mimeData()).isEmpty()) {
        // Igual que el DropHandler del Explorador: el origen no debe borrarse
        // al terminar el evento, porque el motor lo elimina solo después de
        // verificar el archivo completo.
        evento->setDropAction(Qt::CopyAction);
        evento->accept();
    }
}

void VentanaPrincipal::dropEvent(QDropEvent *evento)
{
    const QStringList origenes = rutasSoltadas(evento->mimeData());
    if (origenes.isEmpty())
        return;

    const Qt::KeyboardModifiers teclas = evento->modifiers();
    const ipc::Operacion operacion = teclas & Qt::ShiftModifier
        ? ipc::Operacion::Mover
        : teclas & Qt::ControlModifier ? ipc::Operacion::Copiar : m_operacion;
    evento->setDropAction(Qt::CopyAction);
    evento->accept();
    pedirCopia(origenes, operacion);
}

void VentanaPrincipal::mostrarInactivo()
{
    if (m_cargando)
        m_cargando->hide();
    m_comprobandoEspacio = false;
    m_pausaPorDispositivo = false;
    m_pausadosPorDispositivo.clear();

    const QString guion = QStringLiteral("—");
    const QString verbo = m_operacion == ipc::Operacion::Mover ? tr("mover") : tr("copiar");

    mostrarPorcentaje(0);
    mostrarTitulo(tr("Sin copia en curso"));

    m_panel->mostrarRutas(guion, m_carpetaDestino.isEmpty() ? guion : m_carpetaDestino);
    m_panel->mostrarTotal(0, tr("Total · sin archivos en la lista"), tr("0 B/s"));
    m_panel->mostrarSinArchivo(tr("Pulsa + para elegir lo que %1").arg(verbo));
    m_panel->mostrarVelocidades({});
    aplicarConfiguracion();
    m_panel->mostrarPausado(false);
    m_panel->habilitarControles(false);
    mostrarUnidadDe(m_carpetaDestino.isEmpty() ? QDir::rootPath() : m_carpetaDestino);
    m_expandido->mostrarEstado(formatearDuracion(0), tr("0 B/s"), tr("0 B/s"), formatearDuracion(-1));
}

void VentanaPrincipal::aplicarConfiguracion()
{
    if (!m_configuracion)
        return;

    const qint64 limite = m_configuracion->limiteVelocidad();
    if (m_limitador)
        m_limitador->establecerLimite(limite);
    for (MotorDeCopia *motor : m_motores)
        motor->establecerMetodo(m_configuracion->metodoDeCopia());
    const AccionAlTerminar accion = ocupada() ? m_accionFinalTanda : m_configuracion->accionAlTerminar();
    m_panel->mostrarAlTerminar(Configuracion::nombre(accion), accion);
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::refrescarTema()
{
    m_panel->refrescarTema();
    m_expandido->refrescarTema();
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::mostrarUnidadDe(const QString &carpeta)
{
    const QStorageInfo unidad(carpeta);
    if (!unidad.isValid() || unidad.bytesTotal() <= 0) {
        m_panel->mostrarUnidad(tr("DISCO"), QDir::toNativeSeparators(carpeta), QString(), 0.0);
        return;
    }

    const qint64 libres = unidad.bytesAvailable();
    const double ocupado = 1.0 - double(libres) / double(unidad.bytesTotal());
    // El tipo real (HD/SSD/USB) necesita Win32; por ahora, el sistema de archivos.
    m_panel->mostrarUnidad(QString::fromUtf8(unidad.fileSystemType()).toUpper(), nombreDeUnidad(unidad),
        tr("%1 libres").arg(formatearTamano(libres)), ocupado);
}

void VentanaPrincipal::alternarDetalles()
{
    m_expandida = !m_expandida;
    m_expandido->setVisible(m_expandida);
    setFixedSize(kAnchoVentana, m_expandida ? m_altoCompacto + kAltoLista : m_altoCompacto);
}

void VentanaPrincipal::elegirOrigenes()
{
    const QString verbo = m_operacion == ipc::Operacion::Mover ? tr("mover") : tr("copiar");
    QMessageBox pregunta(QMessageBox::Question, tr("MaxCopier"),
        tr("¿Qué quieres %1?").arg(verbo), QMessageBox::NoButton, this);
    QPushButton *archivos = pregunta.addButton(tr("Archivos…"), QMessageBox::AcceptRole);
    QPushButton *carpeta = pregunta.addButton(tr("Una carpeta…"), QMessageBox::AcceptRole);
    pregunta.addButton(QMessageBox::Cancel);
    pregunta.exec();

    QStringList origenes;
    if (pregunta.clickedButton() == archivos)
        origenes = QFileDialog::getOpenFileNames(this, tr("Archivos que %1").arg(verbo), QDir::homePath());
    else if (pregunta.clickedButton() == carpeta)
        origenes = QStringList { QFileDialog::getExistingDirectory(
            this, tr("Carpeta que %1").arg(verbo), QDir::homePath()) };

    origenes.removeAll(QString());
    pedirCopia(origenes, m_operacion);
}

void VentanaPrincipal::pedirCopia(const QStringList &origenes, ipc::Operacion operacion)
{
    if (origenes.isEmpty())
        return;

    // El destino no se puede cambiar mientras esta ventana tenga lista activa.
    QString destino = m_carpetaDestino;
    if (destino.isEmpty() || !ocupada()) {
        destino = QFileDialog::getExistingDirectory(
            this,
            operacion == ipc::Operacion::Mover ? tr("Carpeta a la que mover")
                                                : tr("Carpeta de destino"),
            QFileInfo(origenes.first()).absolutePath());
        if (destino.isEmpty())
            return;
    }

    // La reparte el gestor: puede acabar en esta lista o en una ventana nueva.
    emit peticionDeCopia(operacion, origenes, destino);
}

bool VentanaPrincipal::ocupada() const
{
    return m_copiando || m_escaneando || !m_lista->vacia();
}

QString VentanaPrincipal::resumenEnCurso() const
{
    const QString destino = QDir::toNativeSeparators(m_carpetaDestino);
    if (!m_copiando || m_activas.isEmpty())
        return tr("%1 archivos pendientes hacia %2").arg(m_lista->archivos()).arg(destino);

    qint64 copiadoActivo = 0;
    for (const CopiaActiva &activa : m_activas)
        copiadoActivo += activa.copiado;
    return tr("%1 archivos en curso → %2 (%3 %)")
        .arg(m_activas.size())
        .arg(destino)
        .arg(porcentaje(m_bytesCopiados + copiadoActivo, m_bytesTotales));
}

void VentanaPrincipal::iniciarCopia(
    ipc::Operacion operacion, const QStringList &origenesOriginales,
    const QString &carpetaDestino, bool desdePortapapeles)
{
    const QStringList origenes = quitarOrigenesRedundantes(origenesOriginales);
    if (origenes.isEmpty())
        return;

    // Una lista, un destino: el gestor manda a otra ventana lo que va a otra carpeta.
    const QString destino = QDir(carpetaDestino).absolutePath();
    if (ocupada() && (destino != m_carpetaDestino || operacion != m_operacion))
        return;

    if (QFileInfo(destino).exists() && !QFileInfo(destino).isDir()) {
        mostrarDesdeBandeja();
        QMessageBox::warning(this, tr("Destino no válido"),
            tr("La ruta de destino no es una carpeta. Elige otra carpeta de destino."));
        return;
    }

    const QString verbo = operacion == ipc::Operacion::Mover ? tr("mover") : tr("copiar");
    for (const QString &origen : origenes) {
        const QString motivo = motivoDeSolapamiento(origen, destino, verbo);
        if (!motivo.isEmpty()) {
            mostrarDesdeBandeja();
            QMessageBox::warning(this, tr("Destino no válido"), motivo);
            return;
        }
    }

    if (m_carpetaDestino != destino) {
        m_carpetaDestino = destino;
        mostrarUnidadDe(m_carpetaDestino);
    }
    if (!m_copiando && !m_escaneando && m_lista->vacia()) {
        // Tanda nueva: los contadores del estado empiezan de cero.
        m_operacion = operacion;
        m_raicesMovimiento.clear();
        m_directoriosMovimiento.clear();
        m_directoriosCreadosMovimiento.clear();
        m_origenesPortapapeles.clear();
        m_desdePortapapeles = desdePortapapeles && operacion == ipc::Operacion::Mover;
        if (m_desdePortapapeles)
            m_origenesPortapapeles = origenes;
        m_huboTrabajo = false;
        m_cancelandoTrabajo = false;
        m_movimientoCompleto = true;
        m_archivosTotales = 0;
        m_archivosHechos = 0;
        m_bytesTotales = 0;
        m_bytesCopiados = 0;
        m_pausada = false;
        m_accionFinalTanda = m_configuracion ? m_configuracion->accionAlTerminar()
                                             : AccionAlTerminar::Nada;
        m_panel->mostrarPausado(false);
        m_velocidadMaxima = 0.0;
        m_velocidades.clear();
        m_relojTanda.start();
        // Las políticas «para todo» solo valen dentro de una tanda.
        m_colision = m_configuracion ? m_configuracion->accionColision()
                                     : AccionColision::Preguntar;
        m_error = m_configuracion ? m_configuracion->accionError() : AccionError::Preguntar;
        m_reintentados.clear();
        m_pausaPorDispositivo = false;
        m_pausadosPorDispositivo.clear();
        m_detenidaPorEspacio = false;
        m_comprobandoEspacio = false;
        m_presupuesto.clear();
        m_identidadDestino.clear();
        m_recolocandoActivas = false;
        if (m_expandido)
            m_expandido->limpiarErrores();
    }

    if (operacion == ipc::Operacion::Mover)
        m_raicesMovimiento.append(origenes);

    // Si una petición de otra fuente se añade a la misma lista, ya no podemos
    // afirmar que el portapapeles representa exactamente toda la tanda. En ese
    // caso es más seguro dejar intacta la selección cortada.
    if (m_desdePortapapeles
        && (!desdePortapapeles || m_origenesPortapapeles != origenes)) {
        m_desdePortapapeles = false;
        m_origenesPortapapeles.clear();
    }

    ++m_generacionEscaneo;
    m_escaneando = true;
    if (m_cargando)
        m_cargando->mostrarCargando(tr("Enumerando archivos…"));
    if (!m_copiando) {
        mostrarTitulo(operacion == ipc::Operacion::Mover ? tr("Buscando archivos para mover…")
                                                          : tr("Buscando archivos…"));
        mostrarProgresoEscaneo();
        m_panel->habilitarControles(false, true);
    }
    m_escaner->reiniciar();
    emit escaneoPedido(origenes, m_carpetaDestino, m_operacion == ipc::Operacion::Mover);
    anotarSesion(tr("Tanda nueva: %1 %2 elemento(s) → %3")
                     .arg(verbo)
                     .arg(origenes.size())
                     .arg(QDir::toNativeSeparators(m_carpetaDestino)));
}

void VentanaPrincipal::alEncontrados(const ElementosDeCopia &lote)
{
    if (m_cancelandoTrabajo)
        return;
    if (!lote.isEmpty())
        m_huboTrabajo = true;
    m_lista->anadir(lote);
    m_archivosTotales += int(lote.size());
    for (const ElementoDeCopia &elemento : lote)
        m_bytesTotales += elemento.tamano;
    // La lista puede verse mientras se prepara, pero la transferencia solo
    // puede arrancar cuando `terminado` confirme que ya no quedan lotes.
    if (!m_copiando)
        mostrarProgresoEscaneo();
}

void VentanaPrincipal::alDirectoriosEncontrados(const QStringList &directorios)
{
    if (m_cancelandoTrabajo || directorios.isEmpty())
        return;
    m_huboTrabajo = true;
    for (const QString &directorio : directorios) {
        if (!m_directoriosMovimiento.contains(directorio))
            m_directoriosMovimiento.append(directorio);
    }
    if (!m_copiando)
        mostrarProgresoEscaneo();
}

void VentanaPrincipal::alEscaneoTerminado(int archivos, qint64 bytes, bool cancelado)
{
    Q_UNUSED(bytes);
    m_escaneando = false;
    if (m_cierreDefinitivo) {
        m_comprobandoEspacio = false;
        comprobarCierreSeguro();
        return;
    }
    if (!m_copiando)
        m_panel->habilitarControles(false);
    if (m_cancelandoTrabajo) {
        m_movimientoCompleto = false;
        m_lista->vaciar();
        comprobarCancelacion();
        return;
    }
    if (cancelado)
        m_movimientoCompleto = false;

    if (archivos == 0 && !cancelado && !m_copiando && m_operacion != ipc::Operacion::Mover) {
        if (m_cargando)
            m_cargando->hide();
        mostrarTitulo(tr("No se ha encontrado ningún archivo"));
        return;
    }
    if (archivos == 0 && !cancelado && !m_copiando && m_operacion == ipc::Operacion::Mover) {
        terminarTanda();
        return;
    }
    // Con la lista completa ya se puede comprobar el espacio del destino
    // (también al añadir archivos a una copia en curso).
    comprobarEspacioYPresupuestar();
}

void VentanaPrincipal::alEspacioComprobado(
    const QList<FaltaDeEspacio> &faltas, quint64 generacion)
{
    if (m_cierreDefinitivo)
        return;
    if (m_cancelandoTrabajo) {
        m_comprobandoEspacio = false;
        comprobarCancelacion();
        return;
    }
    // Si mientras se preparaba el espacio llegó otra petición de enumeración,
    // el resultado anterior ya no describe la cola actual. El worker procesará
    // la petición más reciente y solo ese resultado puede arrancar motores.
    if (!m_comprobandoEspacio || generacion != m_generacionComprobacion)
        return;
    aceptarFaltasDeEspacio(faltas);
}

void VentanaPrincipal::mostrarProgresoEscaneo()
{
    const QString detalle = m_operacion == ipc::Operacion::Mover
        ? tr("Enumerando · %1 archivos · %2 carpetas")
              .arg(m_archivosTotales)
              .arg(m_directoriosMovimiento.size())
        : tr("Enumerando · %1 archivos encontrados").arg(m_archivosTotales);
    m_panel->mostrarTotal(0, detalle, tr("espera…"));
    m_panel->mostrarSinArchivo(tr("Preparando la lista de copia…"));
}

void VentanaPrincipal::rellenarMotores()
{
    if (m_asignando || m_cancelandoTrabajo || m_pausada || m_pausaPorDispositivo
        || m_detenidaPorEspacio || m_escaneando)
        return;
    while (true) {
        MotorDeCopia *libre = nullptr;
        for (MotorDeCopia *motor : m_motores) {
            if (!estaActivo(motor)) {
                libre = motor;
                break;
            }
        }
        if (!libre || siguienteFilaPendiente() < 0)
            break;
        asignarSiguiente();
    }
}

void VentanaPrincipal::asignarSiguiente()
{
    if (m_asignando || m_cancelandoTrabajo || m_pausada || m_pausaPorDispositivo || m_escaneando)
        return;

    // Guardia de reentrada: los diálogos de colisión/error abren un bucle de
    // eventos y un motor que termine mientras tanto no debe reasignar la misma
    // fila ni entrar en cascada.
    m_asignando = true;
    const auto cuerpo = [&]() {
        MotorDeCopia *libre = nullptr;
        for (MotorDeCopia *motor : m_motores) {
            if (!estaActivo(motor)) {
                libre = motor;
                break;
            }
        }
        if (!libre || siguienteFilaPendiente() < 0)
            return;

        // Se marca la copia como activa antes de preguntar por una colisión: el
        // diálogo es modal pero deja correr los eventos, y los lotes del escáner
        // que lleguen mientras esté abierto no deben arrancar otra copia.
        m_copiando = true;

        ElementoDeCopia siguiente;
        bool sobrescribir = false;
        while (true) {
            const int fila = siguienteFilaPendiente();
            if (fila < 0) {
                if (m_activas.isEmpty())
                    terminarTanda();
                return;
            }
            siguiente = m_lista->elemento(fila);

            // «Continuar de todas formas»: solo se copia hasta donde quepa. Si
            // el siguiente archivo ya no cabe en su volumen, la tanda se
            // detiene y los restantes quedan en la lista.
            if (!m_presupuesto.isEmpty()) {
                const QString volumen = volumenDe(siguiente.destino);
                if (m_presupuesto.contains(volumen)
                    && siguiente.tamano > m_presupuesto.value(volumen)) {
                    detenerPorFaltaDeEspacio(siguiente);
                    return;
                }
            }

            // El origen puede haber desaparecido entre el escaneo y su turno.
            const QString motivo = motivoInaccesible(siguiente.fuente);
            if (!motivo.isEmpty()) {
                const AccionError accion = decidirError(siguiente, motivo);
                m_expandido->anadirError(
                    QDateTime::currentDateTime().toString(Qt::ISODate),
                    m_operacion == ipc::Operacion::Mover ? tr("Mover") : tr("Copiar"),
                    QFileInfo(siguiente.fuente).fileName(), motivo);
                anotarSesion(tr("No se puede leer «%1»: %2")
                                 .arg(QFileInfo(siguiente.fuente).fileName(), motivo));
                if (accion == AccionError::Reintentar)
                    continue;
                // Con un solo archivo pendiente, mandarlo al final sería volver
                // a él: se salta.
                if (accion == AccionError::PonerAlFinal && m_lista->archivos() > 1)
                    m_lista->moverAlFinal({ fila });
                else {
                    m_movimientoCompleto = false;
                    descartarPendiente(siguiente);
                }
                continue;
            }

            if (resolverColision(siguiente, sobrescribir))
                break;

            // Saltado por la política de colisión: fuera de la lista y del total.
            m_movimientoCompleto = false;
            descartarPendiente(siguiente);
        }

        const int filaFinal = siguienteFilaPendiente();
        if (filaFinal < 0 || m_lista->elemento(filaFinal).fuente != siguiente.fuente)
            return;
        m_lista->marcarEnCurso(filaFinal, true);
        m_activas.append({ libre, siguiente, sobrescribir, 0, 0.0 });
        if (!m_presupuesto.isEmpty()) {
            const QString volumen = volumenDe(siguiente.destino);
            if (m_presupuesto.contains(volumen)) {
                m_presupuesto[volumen] =
                    qMax<qint64>(0, m_presupuesto.value(volumen) - siguiente.tamano);
            }
        }

        // Las subcarpetas del destino las crea la ventana: el motor solo copia.
        // En modo mover registramos las que no existían para poder quitar las
        // vacías si el usuario cancela o una parte de la tanda falla.
        prepararDirectorioDestino(siguiente.destino);

        m_panel->mostrarRutas(QFileInfo(siguiente.fuente).absolutePath(),
            QFileInfo(siguiente.destino).absolutePath());
        m_panel->habilitarControles(true);
        if (!m_relojEstado->isActive())
            m_relojEstado->start();

        libre->reiniciar();
        QMetaObject::invokeMethod(libre, "copiar", Qt::QueuedConnection,
            Q_ARG(QString, siguiente.fuente),
            Q_ARG(QString, siguiente.destino),
            Q_ARG(bool, sobrescribir),
            Q_ARG(bool, m_operacion == ipc::Operacion::Mover));
        mostrarArchivosEnCurso();
        refrescarTotal();
        emit estadoDeBandejaCambiado();
    };
    cuerpo();
    m_asignando = false;
    // Un motor pudo quedar libre mientras se resolvía una colisión: se rellena.
    rellenarMotores();
}

bool VentanaPrincipal::estaActivo(MotorDeCopia *motor) const
{
    for (const CopiaActiva &activa : m_activas) {
        if (activa.motor == motor)
            return true;
    }
    return false;
}

VentanaPrincipal::CopiaActiva *VentanaPrincipal::activaDe(MotorDeCopia *motor)
{
    for (CopiaActiva &activa : m_activas) {
        if (activa.motor == motor)
            return &activa;
    }
    return nullptr;
}

void VentanaPrincipal::quitarActiva(MotorDeCopia *motor)
{
    m_activas.removeIf([motor](const CopiaActiva &activa) { return activa.motor == motor; });
}

int VentanaPrincipal::siguienteFilaPendiente() const
{
    for (int i = 0; i < m_lista->archivos(); ++i) {
        const QString fuente = m_lista->elemento(i).fuente;
        bool activa = false;
        for (const CopiaActiva &copiando : m_activas) {
            if (copiando.elemento.fuente == fuente) {
                activa = true;
                break;
            }
        }
        if (!activa)
            return i;
    }
    return -1;
}

int VentanaPrincipal::filaDe(const QString &fuente) const
{
    for (int i = 0; i < m_lista->archivos(); ++i) {
        if (m_lista->elemento(i).fuente == fuente)
            return i;
    }
    return -1;
}

bool VentanaPrincipal::resolverColision(ElementoDeCopia &elemento, bool &sobrescribir)
{
    sobrescribir = false;
    if (!QFileInfo::exists(elemento.destino))
        return true;

    AccionColision accion = m_colision;
    if (accion == AccionColision::Preguntar) {
        // Si la ventana estaba en la bandeja, vuelve: la copia no puede seguir
        // hasta que se conteste, y el diálogo saldría sin nada detrás.
        mostrarDesdeBandeja();
        DialogoColision dialogo(elemento, this);
        dialogo.exec(); // cerrarlo sin elegir equivale a saltar el archivo
        accion = dialogo.accion();
        if (dialogo.paraTodo())
            m_colision = accion;
    }

    switch (accion) {
    case AccionColision::Sobrescribir:
        sobrescribir = true;
        return true;
    case AccionColision::Renombrar:
        elemento.destino = rutaLibre(elemento.destino);
        return true;
    case AccionColision::Preguntar:
    case AccionColision::Saltar:
        break;
    }
    return false;
}

AccionError VentanaPrincipal::decidirError(const ElementoDeCopia &elemento, const QString &motivo)
{
    AccionError accion = m_error;
    // Un archivo ya reintentado sin preguntar vuelve a preguntar al fallar otra vez.
    if (accion == AccionError::Reintentar && m_reintentados.contains(elemento.fuente))
        accion = AccionError::Preguntar;

    if (accion == AccionError::Preguntar) {
        mostrarDesdeBandeja();
        DialogoError dialogo(elemento.fuente, motivo, this);
        dialogo.exec(); // cerrarlo sin elegir equivale a saltar el archivo
        accion = dialogo.accion();
        if (dialogo.paraTodo())
            m_error = accion;
    }

    if (accion == AccionError::Reintentar)
        m_reintentados.append(elemento.fuente);
    else
        m_reintentados.removeAll(elemento.fuente);
    return accion;
}

void VentanaPrincipal::descartarPendiente(const ElementoDeCopia &elemento)
{
    const int fila = filaDe(elemento.fuente);
    if (fila >= 0)
        m_lista->quitarTerminada(fila);
    m_bytesTotales = qMax<qint64>(0, m_bytesTotales - elemento.tamano);
    --m_archivosTotales;
}

void VentanaPrincipal::anotarSesion(const QString &texto)
{
    const QString linea = QDateTime::currentDateTime().toString(Qt::ISODate)
        + QStringLiteral("  ") + texto;
    if (m_expandido)
        m_expandido->anadirRegistro(linea);
}

void VentanaPrincipal::terminarTanda()
{
    if (m_cargando)
        m_cargando->hide();
    m_comprobandoEspacio = false;

    if (m_operacion == ipc::Operacion::Mover && m_movimientoCompleto) {
        if (!crearDirectoriosMovidos()) {
            m_movimientoCompleto = false;
            emit avisoDeBandeja(tr("Movimiento incompleto"),
                tr("No se pudieron crear todas las carpetas de destino."));
        } else {
            if (!quitarDirectoriosMovidos()) {
                m_movimientoCompleto = false;
                emit avisoDeBandeja(tr("Movimiento incompleto"),
                    tr("Quedaron elementos en el origen y no se pudo completar el movimiento."));
            } else if (m_desdePortapapeles && m_huboTrabajo) {
                limpiarPortapapelesCortado(m_origenesPortapapeles);
            }
        }
    }
    if (m_operacion == ipc::Operacion::Mover && !m_movimientoCompleto)
        limpiarDirectoriosCreados();

    m_copiando = false;
    m_pausada = false;
    m_relojEstado->stop();
    m_lista->desmarcarTodas();
    m_panel->habilitarControles(false);
    m_panel->mostrarPausado(false);

    // Una acción final de energía o de cierre solo se puede pedir cuando no
    // hubo saltos, errores descartados ni partes incompletas. Esto también
    // evita tratar una copia parcialmente correcta como una tanda exitosa.
    const bool transferenciaTerminada = m_huboTrabajo && m_movimientoCompleto;
    if (transferenciaTerminada) {
        mostrarPorcentaje(100);
        const QString nombreOperacion = m_operacion == ipc::Operacion::Mover ? tr("Movimiento") : tr("Copia");
        mostrarTitulo(m_archivosHechos == 1
                ? tr("%1 terminado · 1 archivo").arg(nombreOperacion)
                : tr("%1 terminado · %2 archivos").arg(nombreOperacion).arg(m_archivosHechos));
        avisarDelFinal();
        m_panel->mostrarTotal(100,
            tr("Total · %1 archivos · %2").arg(m_archivosHechos).arg(formatearTamano(m_bytesCopiados)),
            formatearVelocidad(m_velocidadMedia));
        m_panel->mostrarArchivos({
            { tr("Lista vacía"), QString(), QString(),
                m_operacion == ipc::Operacion::Mover ? tr("movido") : tr("copiado"), 100, false },
        });
    } else {
        mostrarInactivo();
    }
    emit tandaTerminada(transferenciaTerminada);
    if (transferenciaTerminada && m_accionFinalTanda != AccionAlTerminar::Nada)
        emit accionFinalPedida(m_accionFinalTanda);
    refrescarEstado();
}

void VentanaPrincipal::avisarDelFinal()
{
    // Aviso del sistema al acabar la tanda: es lo que se ve cuando la ventana
    // está en la bandeja o detrás de otras.
    const qint64 segundos = m_relojTanda.isValid() ? m_relojTanda.elapsed() / 1000 : 0;
    emit avisoDeBandeja(m_operacion == ipc::Operacion::Mover ? tr("Movimiento terminado")
                                                              : tr("Copia terminada"),
        tr("%1 archivos · %2 en %3\nhacia %4")
            .arg(m_archivosHechos)
            .arg(formatearTamano(m_bytesCopiados), formatearDuracion(segundos),
                QDir::toNativeSeparators(m_carpetaDestino)));
}

void VentanaPrincipal::alIniciada(
    MotorDeCopia *motor, const QString &origen, const QString &destino, qint64 tamano)
{
    CopiaActiva *activa = activaDe(motor);
    if (!activa)
        return;
    activa->elemento.tamano = tamano;
    anotarSesion(tr("Copiando «%1» (%2)")
                     .arg(QFileInfo(origen).fileName(), formatearTamano(tamano)));
    mostrarTitulo(tituloDeTransferencia(m_operacion, m_activas.size(),
        QDir::toNativeSeparators(QFileInfo(origen).absolutePath()),
        QDir::toNativeSeparators(QFileInfo(destino).absolutePath())));
    mostrarArchivosEnCurso();
    refrescarTotal();
}

void VentanaPrincipal::alProgreso(
    MotorDeCopia *motor, qint64 copiado, qint64 total, double bytesPorSegundo, qint64 segundosRestantes)
{
    Q_UNUSED(total)
    Q_UNUSED(segundosRestantes)
    CopiaActiva *activa = activaDe(motor);
    if (!activa)
        return;
    activa->copiado = copiado;
    activa->velocidad = bytesPorSegundo;
    mostrarArchivosEnCurso();
    refrescarTotal();
    anotarVelocidad(m_velocidadMedia);
}

void VentanaPrincipal::refrescarTotal()
{
    qint64 copiadoActivo = 0;
    double velocidadActiva = 0.0;
    for (const CopiaActiva &activa : m_activas) {
        copiadoActivo += activa.copiado;
        velocidadActiva += activa.velocidad;
    }
    const qint64 copiado = m_bytesCopiados + copiadoActivo;
    const int hecho = porcentaje(copiado, m_bytesTotales);
    const qint64 quedan = qMax<qint64>(0, m_bytesTotales - copiado);

    m_velocidadMaxima = qMax(m_velocidadMaxima, velocidadActiva);
    const qint64 ms = m_relojTanda.isValid() ? m_relojTanda.elapsed() : 0;
    m_velocidadMedia = ms > 0 ? double(copiado) * 1000.0 / double(ms) : 0.0;
    m_segundosRestantes = m_velocidadMedia > 0.0 ? qint64(double(quedan) / m_velocidadMedia) : -1;

    mostrarPorcentaje(hecho);
    const int enCurso = m_activas.size();
    const QString detalle = enCurso > 1
        ? tr("Total · %1 archivos en curso · %2 de %3")
              .arg(enCurso)
              .arg(formatearTamano(copiado), formatearTamano(m_bytesTotales))
        : tr("Total · archivo %1 de %2 · %3 de %4")
              .arg(qMin(m_archivosHechos + 1, m_archivosTotales))
              .arg(m_archivosTotales)
              .arg(formatearTamano(copiado), formatearTamano(m_bytesTotales));
    m_panel->mostrarTotal(hecho, detalle, formatearVelocidad(m_velocidadMedia));
}

void VentanaPrincipal::mostrarArchivosEnCurso()
{
    QList<ArchivoEnCurso> archivos;
    for (const CopiaActiva &activa : m_activas) {
        ArchivoEnCurso info;
        info.nombre = QFileInfo(activa.elemento.fuente).fileName();
        info.tamano = formatearTamano(activa.elemento.tamano);
        info.velocidad = formatearVelocidad(activa.velocidad);
        const qint64 quedan = qMax<qint64>(0, activa.elemento.tamano - activa.copiado);
        info.restante = activa.velocidad > 0.0
            ? tr("%1 restante").arg(formatearDuracion(qint64(double(quedan) / activa.velocidad)))
            : tr("preparando…");
        info.porcentaje = porcentaje(activa.copiado, activa.elemento.tamano);
        info.pausado = m_pausada || m_pausaPorDispositivo
            || (activa.motor && activa.motor->pausada());
        archivos.append(info);
    }
    m_panel->mostrarArchivos(archivos);
}

void VentanaPrincipal::refrescarEstado()
{
    const qint64 transcurrido = m_relojTanda.isValid() ? m_relojTanda.elapsed() / 1000 : 0;
    m_expandido->mostrarEstado(formatearDuracion(transcurrido), formatearVelocidad(m_velocidadMedia),
        formatearVelocidad(m_velocidadMaxima), formatearDuracion(m_copiando ? m_segundosRestantes : -1));
}

void VentanaPrincipal::anotarVelocidad(double bytesPorSegundo)
{
    m_velocidades.append(qMax(0.0, bytesPorSegundo));
    while (m_velocidades.size() > kMuestrasGrafica)
        m_velocidades.removeFirst();

    double maxima = 0.0;
    for (double velocidad : m_velocidades)
        maxima = qMax(maxima, velocidad);
    if (maxima <= 0.0) {
        m_panel->mostrarVelocidades({});
        return;
    }

    QList<double> normalizadas;
    normalizadas.reserve(m_velocidades.size());
    for (double velocidad : m_velocidades)
        normalizadas.append(velocidad / maxima);
    m_panel->mostrarVelocidades(normalizadas);
}

void VentanaPrincipal::alPausaCambiada(bool pausada)
{
    m_pausada = pausada;
    m_panel->mostrarPausado(pausada);
    mostrarArchivosEnCurso();
    refrescarTotal();
    if (pausada) {
        const QString operacion = m_operacion == ipc::Operacion::Mover ? tr("Movimiento") : tr("Copia");
        mostrarTitulo(m_copiando
                ? tr("%1 en pausa · %2 archivo(s)").arg(operacion).arg(m_activas.size())
                : tr("Enumeración en pausa"));
    }
    else if (m_escaneando && !m_copiando)
        mostrarTitulo(m_operacion == ipc::Operacion::Mover ? tr("Buscando archivos para mover…")
                                                            : tr("Buscando archivos…"));
    else
        emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::alPausaMotorCambiada(MotorDeCopia *motor, bool pausada)
{
    Q_UNUSED(motor);
    Q_UNUSED(pausada);
    if (!m_copiando || m_activas.isEmpty())
        return;

    // La UI representa la pausa de la ventana: solo queda pausada cuando todos
    // los motores activos han confirmado ese estado. Esto evita que un primer
    // motor que responda antes haga desaparecer o reordene las barras de los
    // demás.
    bool todosPausados = true;
    bool todosReanudados = true;
    for (const CopiaActiva &activa : std::as_const(m_activas)) {
        if (!activa.motor)
            continue;
        todosPausados = todosPausados && activa.motor->pausada();
        todosReanudados = todosReanudados && !activa.motor->pausada();
    }
    // Mientras se propaga una petición a varios hilos hay un estado mixto.
    // Mantener la intención mostrada por la UI durante esa ventana evita que
    // una señal intermedia vuelva a cambiar «Reanudar» por «Pausar».
    if (!todosPausados && !todosReanudados)
        return;
    m_pausada = todosPausados;
    m_panel->mostrarPausado(m_pausada);
    mostrarArchivosEnCurso();
    refrescarTotal();
    emit estadoDeBandejaCambiado();
}

void VentanaPrincipal::alTerminada(
    MotorDeCopia *motor, MotorDeCopia::Resultado resultado, const QString &error)
{
    CopiaActiva *activa = activaDe(motor);
    if (!activa)
        return;
    const ElementoDeCopia terminado = activa->elemento;
    const int fila = filaDe(terminado.fuente);
    quitarActiva(motor);

    if (m_cierreDefinitivo)
        return;

    if (m_cancelandoTrabajo) {
        // La cancelación solo publica el estado cuando todos los motores y el
        // escáner han abandonado sus operaciones. Así no se puede iniciar una
        // tanda nueva con un motor todavía ejecutando la anterior.
        comprobarCancelacion();
        refrescarEstado();
        return;
    }

    // El dispositivo volvió con otra letra: el archivo en curso se vuelve a
    // colar con el destino re-mapeado en lugar de cancelar la tanda.
    if (m_recolocandoActivas && resultado == MotorDeCopia::Resultado::Cancelada) {
        if (fila >= 0)
            m_lista->quitarTerminada(fila);
        ElementoDeCopia recolocado = terminado;
        recolocado.destino = remapearRuta(recolocado.destino, m_remapeoViejo, m_remapeoNuevo);
        m_lista->anadir({ recolocado });
        m_lista->moverAlPrincipio({ m_lista->archivos() - 1 });
        if (m_activas.isEmpty())
            m_recolocandoActivas = false;
        rellenarMotores();
        return;
    }

    // La fila del archivo que acaba de pasar por el motor sale de la lista
    // (copiada, saltada o con error): la lista solo guarda lo pendiente y las
    // filas en curso de otros motores.
    if (resultado != MotorDeCopia::Resultado::Cancelada && fila >= 0)
        m_lista->quitarTerminada(fila);

    switch (resultado) {
    case MotorDeCopia::Resultado::Terminada:
        ++m_archivosHechos;
        m_bytesCopiados += terminado.tamano;
        anotarSesion(tr("OK «%1»").arg(QFileInfo(terminado.fuente).fileName()));
        break;
    case MotorDeCopia::Resultado::Saltada:
        // Lo saltado no se ha copiado: sale del total para que el % siga bien.
        m_movimientoCompleto = false;
        m_bytesTotales = qMax<qint64>(0, m_bytesTotales - terminado.tamano);
        --m_archivosTotales;
        anotarSesion(tr("Saltado «%1»").arg(QFileInfo(terminado.fuente).fileName()));
        break;
    case MotorDeCopia::Resultado::Cancelada:
        anotarSesion(tr("Copias canceladas"));
        m_movimientoCompleto = false;
        limpiarDirectoriosCreados();
        m_escaner->cancelar();
        for (MotorDeCopia *otro : m_motores) {
            if (otro != motor)
                otro->cancelar();
        }
        mostrarTransferenciaCancelada();
        return;
    case MotorDeCopia::Resultado::Error: {
        // Si el destino se acaba de desconectar no se molesta con diálogos:
        // el archivo vuelve a la cola y la copia queda en pausa hasta que el
        // dispositivo vuelva (se reanuda desde el `.mcpart`).
        if (m_pausaPorDispositivo || dispositivoDestinoAusente()) {
            m_lista->anadir({ terminado });
            pausarPorDispositivo();
            refrescarTotal();
            mostrarArchivosEnCurso();
            return;
        }
        // Disco lleno a mitad (comprobación desactivada, presupuesto descontado
        // de más por otro proceso…): se detiene la tanda igual que la falta de
        // espacio inicial y el archivo vuelve a la cola para reintentarlo.
        if (!faltasDeEspacio({ terminado }).isEmpty()) {
            m_lista->anadir({ terminado });
            detenerPorFaltaDeEspacio(terminado);
            refrescarTotal();
            mostrarArchivosEnCurso();
            return;
        }
        m_expandido->anadirError(QDateTime::currentDateTime().toString(Qt::ISODate),
            m_operacion == ipc::Operacion::Mover ? tr("Mover") : tr("Copiar"),
            QFileInfo(terminado.fuente).fileName(), error);
        anotarSesion(tr("Error «%1»: %2")
                         .arg(QFileInfo(terminado.fuente).fileName(), error));
        // La fila ya ha salido de la lista: reintentarla o mandarla al final es
        // volver a meterla, y solo saltarla la descuenta del total.
        const AccionError accion = decidirError(terminado, error);
        const bool noQuedanPendientes = m_lista->vacia() && m_activas.isEmpty();
        if (accion == AccionError::Saltar
            || (accion == AccionError::PonerAlFinal && noQuedanPendientes)) {
            m_movimientoCompleto = false;
            m_bytesTotales = qMax<qint64>(0, m_bytesTotales - terminado.tamano);
            --m_archivosTotales;
        } else {
            m_lista->anadir({ terminado });
            if (accion == AccionError::Reintentar)
                m_lista->moverAlPrincipio({ m_lista->archivos() - 1 });
        }
        break;
    }
    }

    // El título decía «N archivos en curso»: al terminar o saltar uno, se
    // actualiza con los que siguen copiando.
    if (m_activas.size() == 1) {
        const ElementoDeCopia &activa = m_activas.first().elemento;
        mostrarTitulo(tituloDeTransferencia(m_operacion, 1,
            QDir::toNativeSeparators(QFileInfo(activa.fuente).absolutePath()),
            QDir::toNativeSeparators(QFileInfo(activa.destino).absolutePath())));
    } else if (m_activas.size() > 1) {
        mostrarTitulo(tituloDeTransferencia(m_operacion, m_activas.size()));
    }

    refrescarTotal();
    mostrarArchivosEnCurso();
    if (m_lista->vacia() && m_activas.isEmpty())
        terminarTanda();
    else
        rellenarMotores();
}

void VentanaPrincipal::mostrarTransferenciaCancelada()
{
    if (m_cargando)
        m_cargando->hide();
    m_comprobandoEspacio = false;
    m_pausaPorDispositivo = false;
    m_pausadosPorDispositivo.clear();
    // `ListaDeCopia::vaciar()` conserva las filas activas por diseño. En una
    // cancelación ya no queda ningún motor asociado, así que primero se quitan
    // esas anclas para no dejar una fila fantasma que mantenga la ventana
    // ocupada para siempre.
    m_lista->desmarcarTodas();
    m_lista->vaciar();
    m_copiando = false;
    m_escaneando = false;
    m_cancelandoTrabajo = false;
    m_asignando = false;
    m_colision = AccionColision::Preguntar;
    m_error = AccionError::Preguntar;
    m_accionFinalTanda = AccionAlTerminar::Nada;
    m_reintentados.clear();
    m_detenidaPorEspacio = false;
    m_presupuesto.clear();
    m_identidadDestino.clear();
    m_recolocandoActivas = false;
    m_relojEstado->stop();
    m_pausada = false;
    mostrarPorcentaje(0);
    mostrarTitulo(m_operacion == ipc::Operacion::Mover ? tr("Movimiento cancelado")
                                                        : tr("Copia cancelada"));
    m_panel->mostrarTotal(0,
        m_operacion == ipc::Operacion::Mover ? tr("Total · movimiento cancelado")
                                             : tr("Total · copia cancelada"),
        tr("0 B/s"));
    mostrarArchivosEnCurso();
    m_panel->habilitarControles(false);
    m_panel->mostrarPausado(false);
    aplicarConfiguracion();
    emit tandaTerminada(false);
    refrescarEstado();
}

bool VentanaPrincipal::crearDirectoriosMovidos()
{
    for (const QString &directorio : m_directoriosMovimiento) {
        if (!QDir().mkpath(directorio))
            return false;
    }
    return true;
}

void VentanaPrincipal::prepararDirectorioDestino(const QString &archivo)
{
    const QString directorio = QFileInfo(archivo).absolutePath();
    if (m_operacion == ipc::Operacion::Mover) {
        QString candidato = directorio;
        while (!candidato.isEmpty() && !QFileInfo(candidato).exists()) {
            if (!m_directoriosCreadosMovimiento.contains(candidato))
                m_directoriosCreadosMovimiento.append(candidato);
            const QString padre = QFileInfo(candidato).absolutePath();
            if (rutasIguales(candidato, padre))
                break;
            candidato = padre;
        }
    }
    QDir().mkpath(directorio);
}

void VentanaPrincipal::limpiarDirectoriosCreados()
{
    std::sort(m_directoriosCreadosMovimiento.begin(), m_directoriosCreadosMovimiento.end(),
        [](const QString &izquierda, const QString &derecha) {
            return izquierda.size() > derecha.size();
        });
    for (const QString &directorio : std::as_const(m_directoriosCreadosMovimiento)) {
        if (rutaMismaODescendiente(directorio, m_carpetaDestino)
            && !rutasIguales(directorio, m_carpetaDestino))
            QDir().rmdir(directorio);
    }
    m_directoriosCreadosMovimiento.clear();
}

bool VentanaPrincipal::quitarDirectoriosMovidos()
{
    return quitarDirectoriosVacios(m_raicesMovimiento);
}

void VentanaPrincipal::guardarLista()
{
    if (m_lista->vacia()) {
        mostrarDesdeBandeja();
        QMessageBox::information(this, tr("Lista vacía"),
            tr("No hay archivos pendientes que guardar."));
        return;
    }

    mostrarDesdeBandeja();
    const QString carpetaInicial = m_carpetaDestino.isEmpty() ? QDir::homePath()
                                                               : m_carpetaDestino;
    const QString ruta = QFileDialog::getSaveFileName(this, tr("Guardar la lista de copia"),
        QDir(carpetaInicial).filePath(QStringLiteral("lista de copia.mclist")),
        tr("Listas de copia de MaxCopier (*.mclist)"));
    if (ruta.isEmpty())
        return;

    QSettings fichero(ruta, QSettings::IniFormat);
    fichero.setFallbacksEnabled(false);
    fichero.setAtomicSyncRequired(true);
    fichero.setValue(QStringLiteral("MaxCopierLista/version"), 1);
    fichero.setValue(QStringLiteral("MaxCopierLista/carpetaDestino"), m_carpetaDestino);
    const ElementosDeCopia elementos = m_lista->elementos();
    fichero.setValue(QStringLiteral("MaxCopierLista/archivos"), int(elementos.size()));
    for (int i = 0; i < elementos.size(); ++i) {
        const QString prefijo = QStringLiteral("MaxCopierLista/archivo%1/").arg(i);
        fichero.setValue(prefijo + QStringLiteral("fuente"), elementos.at(i).fuente);
        fichero.setValue(prefijo + QStringLiteral("destino"), elementos.at(i).destino);
        fichero.setValue(prefijo + QStringLiteral("tamano"), elementos.at(i).tamano);
    }
    fichero.sync();
    if (fichero.status() != QSettings::NoError) {
        QMessageBox::warning(this, tr("No se pudo guardar"),
            tr("No se pudo escribir la lista de copia en %1.").arg(ruta));
    } else {
        anotarSesion(tr("Lista guardada (%1 archivos)").arg(m_lista->archivos()));
    }
}

void VentanaPrincipal::cargarLista()
{
    if (ocupada()) {
        mostrarDesdeBandeja();
        QMessageBox::information(this, tr("Copia en curso"),
            tr("Cancela o espera a que termine la copia antes de cargar otra lista."));
        return;
    }

    mostrarDesdeBandeja();
    const QString ruta = QFileDialog::getOpenFileName(this, tr("Cargar lista de copia"),
        QDir::homePath(), tr("Listas de copia de MaxCopier (*.mclist)"));
    if (ruta.isEmpty())
        return;

    QSettings fichero(ruta, QSettings::IniFormat);
    fichero.setFallbacksEnabled(false);
    if (fichero.value(QStringLiteral("MaxCopierLista/version")).toInt() != 1) {
        QMessageBox::warning(this, tr("Lista no válida"),
            tr("El archivo no parece una lista de copia de MaxCopier."));
        return;
    }
    const int archivos = fichero.value(QStringLiteral("MaxCopierLista/archivos"), 0).toInt();
    ElementosDeCopia elementos;
    for (int i = 0; i < archivos; ++i) {
        const QString prefijo = QStringLiteral("MaxCopierLista/archivo%1/").arg(i);
        ElementoDeCopia elemento;
        elemento.fuente = fichero.value(prefijo + QStringLiteral("fuente")).toString();
        elemento.destino = fichero.value(prefijo + QStringLiteral("destino")).toString();
        elemento.tamano = fichero.value(prefijo + QStringLiteral("tamano"), 0).toLongLong();
        if (!elemento.fuente.isEmpty() && !elemento.destino.isEmpty())
            elementos.append(elemento);
    }
    if (elementos.isEmpty()) {
        QMessageBox::warning(this, tr("Lista vacía"),
            tr("La lista no contiene archivos válidos."));
        return;
    }

    // La lista cargada se comporta como una tanda nueva de copia: al cargarla
    // arranca y los `.mcpart` de F8 se reanudan automáticamente.
    m_lista->vaciar();
    m_lista->anadir(elementos);
    m_archivosTotales = int(elementos.size());
    m_bytesTotales = m_lista->bytes();
    m_archivosHechos = 0;
    m_bytesCopiados = 0;
    m_huboTrabajo = true;
    m_cancelandoTrabajo = false;
    m_movimientoCompleto = true;
    m_pausada = false;
    m_accionFinalTanda = m_configuracion ? m_configuracion->accionAlTerminar()
                                         : AccionAlTerminar::Nada;
    m_colision = m_configuracion ? m_configuracion->accionColision()
                                 : AccionColision::Preguntar;
    m_error = m_configuracion ? m_configuracion->accionError() : AccionError::Preguntar;
    m_reintentados.clear();
    m_raicesMovimiento.clear();
    m_directoriosMovimiento.clear();
    m_directoriosCreadosMovimiento.clear();
    m_origenesPortapapeles.clear();
    m_desdePortapapeles = false;
    m_operacion = ipc::Operacion::Copiar;

    const QString destinoGuardado =
        fichero.value(QStringLiteral("MaxCopierLista/carpetaDestino")).toString();
    const QString destino = destinoGuardado.isEmpty()
        ? QFileInfo(elementos.first().destino).absolutePath()
        : destinoGuardado;
    if (!destino.isEmpty() && m_carpetaDestino != destino) {
        m_carpetaDestino = destino;
        mostrarUnidadDe(m_carpetaDestino);
    }

    mostrarTitulo(tr("Lista cargada · %1 archivos").arg(m_archivosTotales));
    m_panel->mostrarTotal(0,
        tr("Total · %1 archivos · %2")
            .arg(m_archivosTotales)
            .arg(formatearTamano(m_bytesTotales)),
        tr("0 B/s"));
    m_relojTanda.start();
    anotarSesion(tr("Lista cargada (%1 archivos)").arg(m_archivosTotales));
    ++m_generacionEscaneo;
    if (m_cargando)
        m_cargando->mostrarCargando(tr("Comprobando espacio libre…"));
    comprobarEspacioYPresupuestar();
}

void VentanaPrincipal::abrirOrigen()
{
    const ElementoDeCopia *elemento = nullptr;
    for (const CopiaActiva &activa : m_activas) {
        if (activa.elemento.fuente == m_archivoParaSaltar) {
            elemento = &activa.elemento;
            break;
        }
    }
    if (!elemento && !m_activas.isEmpty())
        elemento = &m_activas.first().elemento;
    if (!elemento || elemento->fuente.isEmpty())
        return;
    abrirCarpeta(QFileInfo(elemento->fuente).absolutePath());
}

void VentanaPrincipal::abrirDestino()
{
    if (m_carpetaDestino.isEmpty())
        return;
    abrirCarpeta(m_carpetaDestino);
}

void VentanaPrincipal::abrirCarpeta(const QString &carpeta)
{
    if (!carpeta.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(carpeta));
}

void VentanaPrincipal::alternarTema()
{
    if (m_configuracion)
        m_configuracion->alternarTema();
}

} // namespace maxcopier
