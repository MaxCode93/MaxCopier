#pragma once

#include "configuracion.h"
#include "copia/motordecopia.h"
#include "ipc/protocolo.h"
#include "lista/elementodecopia.h"
#include "politicas/acceso.h"
#include "politicas/colision.h"
#include "util/espaciolibre.h"

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QWidget>

class QThread;
class QTimer;

class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;

namespace maxcopier {

class Cargando;
class BarraTitulo;
class BandejaCopia;
class Escaner;
class LimitadorVelocidad;
class ListaDeCopia;
class PanelCompacto;
class PanelExpandido;

/// Ventana de MaxCopier: barra de título propia (ventana sin marco del sistema),
/// panel compacto y, al pulsar **Detalles**, la vista expandida con la lista de
/// copia. El ancho es fijo (580 px) y solo cambia el alto al expandirse.
/// F3: escaneo recursivo en su hilo, lista de copia y copia secuencial de toda
/// la lista, borrando cada fila al terminarla. F4: diálogo de colisión antes de
/// arrancar cada archivo cuyo destino ya exista. F5: diálogo de error de acceso
/// cuando el origen no se puede leer, antes de empezar o a media copia. F7:
/// un controlador global de bandeja, un icono propio por transferencia,
/// arrastrar y soltar sobre la ventana y aviso al terminar la tanda. Todas las
/// ventanas son copias independientes del mismo proceso.
class VentanaPrincipal : public QWidget {
    Q_OBJECT

public:
    explicit VentanaPrincipal(ipc::Operacion operacion = ipc::Operacion::Copiar,
        Configuracion *configuracion = nullptr,
        QWidget *parent = nullptr);
    ~VentanaPrincipal() override;

    /// Añade `origenes` (archivos o carpetas) a la lista con destino
    /// `carpetaDestino` y arranca la transferencia si no había ninguna en
    /// curso. Quien llama se asegura de que el destino y la operación sean los
    /// de la lista (o que esté vacía).
    void iniciarCopia(ipc::Operacion operacion, const QStringList &origenes,
        const QString &carpetaDestino, bool desdePortapapeles = false);

    /// Está copiando, escaneando o le quedan archivos en la lista.
    bool ocupada() const;

    /// Carpeta a la que va la lista de esta ventana (vacía si aún no hay).
    QString carpetaDestino() const { return m_carpetaDestino; }

    /// Operación de la tanda actual: una lista no mezcla copiar y mover.
    ipc::Operacion operacion() const { return m_operacion; }

    bool copiando() const { return m_copiando; }
    bool escaneando() const { return m_escaneando; }
    bool pausada() const { return m_pausada; }
    bool cancelando() const { return m_cancelandoTrabajo; }
    int porcentajeBandeja() const { return m_porcentaje; }
    bool transferenciaTerminadaCorrectamente() const
    {
        return !ocupada() && m_huboTrabajo && m_movimientoCompleto;
    }
    QString tituloBandeja() const { return m_titulo; }
    QString velocidadBandeja() const;
    bool minimizadaEnBandeja() const { return m_minimizadaEnBandeja; }

    /// Lo que está copiando, para el diálogo «Ya hay una copia en curso».
    QString resumenEnCurso() const;

signals:
    /// Copia pedida desde esta ventana (botón **+**): la reparte el gestor, que
    /// puede mandarla a esta misma lista o a una ventana nueva.
    void peticionDeCopia(ipc::Operacion operacion, QStringList origenes, QString carpetaDestino);

    void escaneoPedido(QStringList origenes, QString carpetaDestino, bool mover);
    void comprobacionEspacioPedida(const maxcopier::ElementosDeCopia &pendientes, quint64 generacion);

public slots:
    /// La ventana desaparece de la barra de tareas y se queda en su icono de
    /// bandeja; sin bandeja del sistema, se minimiza como siempre.
    void ocultarEnBandeja();

    /// Vuelve de la bandeja: se muestra, se restaura si estaba minimizada y se
    /// pone delante.
    void mostrarDesdeBandeja();

    /// Acciones que el icono global o el icono individual de la bandeja aplican
    /// a una ventana.
    void pausarDesdeBandeja();
    void cancelarDesdeBandeja();
    void cerrarDefinitivamente();

    /// Enseña una notificación en el icono individual de esta copia.
    void avisarDesdeBandeja(const QString &titulo, const QString &texto);

    /// Reaplica la hoja de estilos y repinta los widgets propios tras un
    /// cambio global de tema.
    void refrescarTema();

protected:
    void closeEvent(QCloseEvent *evento) override;
    void resizeEvent(QResizeEvent *evento) override;
    void dragEnterEvent(QDragEnterEvent *evento) override;
    void dropEvent(QDropEvent *evento) override;

private slots:
    void alternarTema();
    void alternarDetalles();
    void elegirOrigenes();
    void guardarLista();
    void cargarLista();
    void abrirOrigen();
    void abrirDestino();
    void abrirCarpeta(const QString &carpeta);
    void alEncontrados(const maxcopier::ElementosDeCopia &lote);
    void alDirectoriosEncontrados(const QStringList &directorios);
    void alEscaneoTerminado(int archivos, qint64 bytes, bool cancelado);
    void alEspacioComprobado(const QList<maxcopier::FaltaDeEspacio> &faltas, quint64 generacion);
    void alIniciada(MotorDeCopia *motor, const QString &origen, const QString &destino, qint64 tamano);
    void alProgreso(
        MotorDeCopia *motor, qint64 copiado, qint64 total, double bytesPorSegundo, qint64 segundosRestantes);
    void alPausaCambiada(bool pausada);
    void alPausaMotorCambiada(MotorDeCopia *motor, bool pausada);
    void alTerminada(MotorDeCopia *motor, maxcopier::MotorDeCopia::Resultado resultado,
        const QString &error);
    void refrescarEstado();
    void saltarArchivo();
    void archivoSeleccionado(const QString &fuente);
    void comprobarDispositivoDestino();
    void cancelarEnumeracion();

signals:
    /// El gestor de la bandeja repinta el estado de esta ventana.
    void estadoDeBandejaCambiado();
    void avisoDeBandeja(QString titulo, QString texto);
    void tandaTerminada(bool completa);
    void accionFinalPedida(maxcopier::AccionAlTerminar accion);

private:
    void construirInterfaz();
    void arrancarHilos();

    /// Reparte por el gestor una copia pedida desde esta ventana (botón **+** o
    /// archivos soltados encima), preguntando el destino si hace falta.
    void pedirCopia(const QStringList &origenes, ipc::Operacion operacion);

    /// Estado que se ve en la barra de título y en la bandeja.
    void mostrarTitulo(const QString &titulo);
    void mostrarPorcentaje(int porcentajeHecho);
    void mostrarProgresoEscaneo();
    void establecerPausaDeMotores(bool pausada);

    /// Notificación del sistema con el resumen de la tanda recién terminada.
    void avisarDelFinal();
    void cancelarTrabajo();
    void mostrarTransferenciaCancelada();
    void mostrarInactivo();
    void mostrarUnidadDe(const QString &carpeta);
    void anotarVelocidad(double bytesPorSegundo);

    /// Una transferencia con «Archivos a la vez» > 1: motor, archivo que
    /// copia y su progreso.
    struct CopiaActiva {
        MotorDeCopia *motor = nullptr;
        ElementoDeCopia elemento;
        bool sobrescribir = false;
        qint64 copiado = 0;
        double velocidad = 0.0;
    };

    /// Asigna el siguiente archivo pendiente al primer motor libre y resuelve
    /// sus colisiones/errores de acceso; si no hay motor libre o pendientes,
    /// deja la ventana como esté.
    void asignarSiguiente();

    /// Llena todos los motores libres con archivos pendientes (el «pool» de
    /// «Archivos a la vez»).
    void rellenarMotores();

    bool estaActivo(MotorDeCopia *motor) const;
    CopiaActiva *activaDe(MotorDeCopia *motor);
    int siguienteFilaPendiente() const;
    int filaDe(const QString &fuente) const;
    void quitarActiva(MotorDeCopia *motor);
    void mostrarArchivosEnCurso();
    void descartarPendiente(const ElementoDeCopia &elemento);
    void anotarSesion(const QString &texto);
    bool dispositivoDestinoAusente() const;
    void pausarPorDispositivo();
    void reanudarPorDispositivo();
    void reanudarConOtraLetra(const QString &nuevaRaiz);
    void comprobarEspacioYPresupuestar();
    bool aceptarFaltasDeEspacio(const QList<FaltaDeEspacio> &faltas);
    void detenerPorFaltaDeEspacio(const ElementoDeCopia &elemento);

    /// Resuelve la colisión de `elemento` (preguntando si hace falta): devuelve
    /// `false` si el archivo se salta y, si no, deja en `sobrescribir` y en el
    /// destino de `elemento` lo que el motor tiene que hacer.
    bool resolverColision(ElementoDeCopia &elemento, bool &sobrescribir);

    /// Acción para un archivo que no se ha podido leer: aplica la política
    /// «para todo» o abre el diálogo de error. Nunca devuelve `Preguntar`.
    AccionError decidirError(const ElementoDeCopia &elemento, const QString &motivo);

    void terminarTanda();
    bool crearDirectoriosMovidos();
    bool quitarDirectoriosMovidos();
    void prepararDirectorioDestino(const QString &archivo);
    void limpiarDirectoriosCreados();
    void refrescarTotal();
    void aplicarConfiguracion();

    Configuracion *m_configuracion = nullptr;
    bool m_cierreDefinitivo = false;
    bool m_minimizadaEnBandeja = false;
    BandejaCopia *m_bandejaCopia = nullptr;
    BarraTitulo *m_barraTitulo = nullptr;
    PanelCompacto *m_panel = nullptr;
    PanelExpandido *m_expandido = nullptr;
    ListaDeCopia *m_lista = nullptr;
    QTimer *m_relojEstado = nullptr;

    QList<QThread *> m_hilos;
    QList<MotorDeCopia *> m_motores;
    LimitadorVelocidad *m_limitador = nullptr;
    QList<CopiaActiva> m_activas;
    QString m_archivoParaSaltar;
    int m_archivosALaVez = 1;
    bool m_asignando = false;
    bool m_pausaPorDispositivo = false;
    QList<MotorDeCopia *> m_pausadosPorDispositivo;
    bool m_comprobandoEspacio = false;
    quint64 m_generacionEscaneo = 0;
    quint64 m_generacionComprobacion = 0;
    bool m_detenidaPorEspacio = false;
    QHash<QString, qint64> m_presupuesto;
    QString m_identidadDestino;
    bool m_recolocandoActivas = false;
    QString m_remapeoViejo;
    QString m_remapeoNuevo;
    Cargando *m_cargando = nullptr;
    QTimer *m_relojDispositivo = nullptr;
    QThread *m_hiloEscaner = nullptr;
    Escaner *m_escaner = nullptr;

    int m_altoCompacto = 0;
    bool m_expandida = false;

    QString m_carpetaDestino;
    ipc::Operacion m_operacion = ipc::Operacion::Copiar;
    QStringList m_raicesMovimiento;
    QStringList m_directoriosMovimiento;
    QStringList m_directoriosCreadosMovimiento;
    QStringList m_origenesPortapapeles;
    bool m_desdePortapapeles = false;
    bool m_huboTrabajo = false;
    bool m_movimientoCompleto = true;
    bool m_cancelandoTrabajo = false;
    bool m_copiando = false;
    bool m_escaneando = false;
    bool m_pausada = false;
    AccionAlTerminar m_accionFinalTanda = AccionAlTerminar::Nada;

    // Lo último que se ha mostrado, para repintar el icono de la bandeja.
    QString m_titulo;
    int m_porcentaje = 0;

    /// Política elegida con «Hacer lo mismo para todo»; vale hasta que se acaba
    /// la tanda o se cancela.
    AccionColision m_colision = AccionColision::Preguntar;

    /// Lo mismo para los errores de acceso, más el archivo que ya se ha
    /// reintentado sin preguntar: si vuelve a fallar se pregunta otra vez, así
    /// «reintentar para todo» no deja la lista dando vueltas en un archivo.
    AccionError m_error = AccionError::Preguntar;
    QStringList m_reintentados;

    // Contadores de la tanda: la lista solo guarda lo que queda por copiar.
    int m_archivosTotales = 0;
    int m_archivosHechos = 0;
    qint64 m_bytesTotales = 0;
    qint64 m_bytesCopiados = 0;
    double m_velocidadMedia = 0.0;
    double m_velocidadMaxima = 0.0;
    qint64 m_segundosRestantes = -1;
    QElapsedTimer m_relojTanda;
    QList<double> m_velocidades;
};

} // namespace maxcopier
