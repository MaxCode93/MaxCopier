#pragma once

#include "configuracion.h"
#include "vistas/barraarchivos.h"

#include <QWidget>

class QPushButton;

namespace maxcopier {

class BarraProgreso;
class Chip;
class EtiquetaRuta;
class UnidadDestino;

/// Cuerpo de la ventana compacta: rutas, barra total, barra del archivo actual,
/// fila de metadatos y botones de acción. En F1 muestra datos de ejemplo; el
/// motor de copia (F2) usará los métodos `mostrar*` para alimentarla.
class PanelCompacto : public QWidget {
    Q_OBJECT

public:
    explicit PanelCompacto(QWidget *parent = nullptr);

    void mostrarRutas(const QString &desde, const QString &hacia);
    void mostrarTotal(int porcentajeTotal, const QString &detalle, const QString &velocidad);
    void mostrarArchivos(const QList<ArchivoEnCurso> &archivos);
    void mostrarSinArchivo(const QString &detalle);
    void mostrarVelocidades(const QList<double> &muestras);
    void mostrarUnidad(const QString &tipo, const QString &nombre, const QString &libre, double ocupado);
    void mostrarAlTerminar(const QString &accion,
        AccionAlTerminar accionConfigurada = AccionAlTerminar::Nada);

    /// El botón principal pasa a «Reanudar» y vuelve a «Pausar».
    void mostrarPausado(bool pausado);

    /// Pausar y Cancelar funcionan durante la enumeración o la transferencia;
    /// Saltar solo se habilita cuando hay un archivo en el motor.
    void habilitarControles(bool copiando, bool escaneando = false);

    /// Recompone los colores dependientes del tema.
    void refrescarTema();

signals:
    void abrirOrigenPedido();
    void abrirDestinoPedido();
    void pausarPedido();
    void saltarPedido();
    void cancelarPedido();
    void segmentoClicado(int indice);
    void anadirPedido();
    void detallesPedido();
    void accionFinalPedida(AccionAlTerminar accion);

private:
    QWidget *construirCabecera();
    QWidget *construirAcciones();

private slots:
    void abrirMenuAlTerminar();

private:
    EtiquetaRuta *m_desde = nullptr;
    EtiquetaRuta *m_hacia = nullptr;
    BarraProgreso *m_total = nullptr;
    BarraArchivos *m_archivos = nullptr;
    UnidadDestino *m_unidad = nullptr;
    Chip *m_alTerminar = nullptr;
    AccionAlTerminar m_accionFinal = AccionAlTerminar::Nada;
    QPushButton *m_pausar = nullptr;
    QPushButton *m_saltar = nullptr;
    QPushButton *m_cancelar = nullptr;
};

} // namespace maxcopier
