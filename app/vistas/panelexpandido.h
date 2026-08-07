#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSortFilterProxyModel;
class QStandardItemModel;
class QTableView;
class QTabWidget;

namespace maxcopier {

class ListaDeCopia;
class Configuracion;

/// Cuerpo de la vista expandida: pestañas **Lista de copia · Errores ·
/// Registro · Opciones**, con la tabla `Fuente · Tamaño · Destino`, el buscador,
/// la barra vertical de cola (reordenar, añadir, quitar, cargar/guardar lista)
/// y la barra de estado con transcurrido, media, máxima y restante. La lista se
/// puede reorganizar pulsando las cabeceras Fuente, Tamaño o Destino.
///
/// Errores y Registro siguen siendo marcadores; Opciones es funcional desde F10.
class PanelExpandido : public QWidget {
    Q_OBJECT

public:
    explicit PanelExpandido(Configuracion *configuracion, QWidget *parent = nullptr);

    /// Conecta la tabla con el modelo de la lista (a través del filtro).
    void establecerLista(ListaDeCopia *lista);

    void mostrarResumen(int archivos, qint64 bytes);
    void mostrarEstado(
        const QString &transcurrido, const QString &media, const QString &maxima, const QString &restante);

    /// Añade un error a la pestaña Errores y actualiza su contador.
    void anadirError(const QString &hora, const QString &accion, const QString &archivo,
        const QString &motivo);
    void limpiarErrores();

    /// Añade una línea al Registro de la sesión de esta ventana.
    void anadirRegistro(const QString &linea);

    /// Repinta widgets propios después de cambiar la hoja de estilos global.
    void refrescarTema();

signals:
    void anadirPedido();
    void guardarListaPedida();
    void cargarListaPedida();
    /// La selección de la tabla cambió: fuente del archivo seleccionado (para
    /// que «Saltar» sepa a quién apunta).
    void archivoSeleccionado(const QString &fuente);

private slots:
    void moverAlPrincipio();
    void moverArriba();
    void moverAbajo();
    void moverAlFinal();
    void quitarSeleccion();
    void ordenarColumna(int seccion);

private:
    QWidget *construirPestanaLista();
    QWidget *construirPestanaErrores();
    QWidget *construirPestanaRegistro();
    QWidget *construirPestanaOpciones();
    QWidget *construirBarraDeCola();
    void actualizarContadorErrores();
    void guardarRegistro();

    /// Filas seleccionadas en coordenadas del modelo, de menor a mayor.
    QList<int> filasSeleccionadas() const;
    void seleccionar(const QList<int> &filas);

    ListaDeCopia *m_lista = nullptr;
    Configuracion *m_configuracion = nullptr;
    QTabWidget *m_pestanas = nullptr;
    QSortFilterProxyModel *m_filtro = nullptr;
    QTableView *m_tabla = nullptr;
    QLineEdit *m_buscador = nullptr;
    QLabel *m_resumen = nullptr;
    QLabel *m_transcurrido = nullptr;
    QLabel *m_media = nullptr;
    QLabel *m_maxima = nullptr;
    QLabel *m_restante = nullptr;
    QStandardItemModel *m_modeloErrores = nullptr;
    QPlainTextEdit *m_registro = nullptr;
    int m_columnaOrden = -1;
    Qt::SortOrder m_orden = Qt::AscendingOrder;
};

} // namespace maxcopier
