#pragma once

#include "lista/elementodecopia.h"

#include <QAbstractTableModel>
#include <QList>

#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

namespace maxcopier {

/// Modelo de la única lista de copia: plana, con columnas
/// `· | Fuente | Tamaño | Destino`. El archivo que se está copiando se marca
/// con una flecha y, al terminar, **su fila se borra**.
///
/// Las filas se pueden reordenar a mano; la fila en curso no se mueve ni se
/// quita mientras se copia. Vive en el hilo de la interfaz: el motor y el
/// escáner solo hablan con él a través de la ventana.
class ListaDeCopia : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Columna {
        ColumnaMarca = 0,
        ColumnaFuente,
        ColumnaTamano,
        ColumnaDestino,
        NumeroDeColumnas,
    };

    explicit ListaDeCopia(QObject *parent = nullptr);
    ~ListaDeCopia() override;

    int rowCount(const QModelIndex &padre = QModelIndex()) const override;
    int columnCount(const QModelIndex &padre = QModelIndex()) const override;
    QVariant data(const QModelIndex &indice, int rol) const override;
    QVariant headerData(int seccion, Qt::Orientation orientacion, int rol) const override;

    void anadir(const ElementosDeCopia &elementos);
    void quitar(QList<int> filas);
    void vaciar();

    /// Reordenado a mano de las filas seleccionadas. Devuelven las filas que
    /// pasan a ocupar los elementos movidos, para volver a seleccionarlas.
    QList<int> moverAlPrincipio(QList<int> filas);
    QList<int> moverArriba(QList<int> filas);
    QList<int> moverAbajo(QList<int> filas);
    QList<int> moverAlFinal(QList<int> filas);

    /// Reorganiza la cola según `columna` y `orden`, anclando la fila en curso
    /// al principio como hace el reordenado a mano.
    void ordenarPor(Columna columna, Qt::SortOrder orden);

    /// Solicita la misma ordenación sin ejecutarla en el hilo de la interfaz.
    /// La tabla aplica el resultado cuando termina el hilo de trabajo; si la
    /// lista cambió mientras tanto, el resultado se descarta.
    void ordenarPorEnSegundoPlano(Columna columna, Qt::SortOrder orden);

    /// Filas que se están copiando ahora mismo (hasta «Archivos a la vez»);
    /// se pintan con la flecha y anclan el principio de la cola.
    QList<int> filasEnCurso() const { return m_enCurso; }
    bool esEnCurso(int fila) const { return m_enCurso.contains(fila); }
    void marcarEnCurso(int fila, bool activa);
    void desmarcarTodas();

    /// Quita una fila aunque esté en curso: es la que acaba de copiar el motor.
    void quitarTerminada(int fila);

    /// Reescribe la raíz del destino de todas las filas: el dispositivo volvió
    /// conectado con otra letra de unidad.
    void remapearDestinos(const QString &raizVieja, const QString &raizNueva);

    bool vacia() const { return m_elementos.isEmpty(); }
    int archivos() const { return int(m_elementos.size()); }
    qint64 bytes() const { return m_bytes; }

    const ElementoDeCopia &elemento(int fila) const { return m_elementos.at(fila); }
    const ElementosDeCopia &elementos() const { return m_elementos; }

signals:
    /// El número de archivos o los bytes pendientes han cambiado.
    void cambiada();

private:
    struct ResultadoOrdenacion {
        ElementosDeCopia elementos;
        QList<int> ordenOriginal;
        int cantidadAnclas = 0;
    };

    struct TrabajoOrdenacion {
        ElementosDeCopia elementos;
        QList<int> enCurso;
        Columna columna = ColumnaFuente;
        Qt::SortOrder orden = Qt::AscendingOrder;
        quint64 version = 0;
        quint64 generacion = 0;
    };

    static ResultadoOrdenacion calcularOrdenacion(const ElementosDeCopia &elementos,
        const QList<int> &enCurso, Columna columna, Qt::SortOrder orden);
    void aplicarOrdenacion(ResultadoOrdenacion resultado);
    void ejecutarOrdenaciones();
    QList<int> reubicar(QList<int> filas, bool haciaAbajo, bool alExtremo);
    void recalcularBytes();

    ElementosDeCopia m_elementos;
    qint64 m_bytes = 0;
    // Se mantiene ordenada (en el orden en que arrancaron): son las primeras
    // filas de la cola y sirven de anclas para reordenar y ordenar.
    QList<int> m_enCurso;
    quint64 m_version = 0;
    quint64 m_generacionOrdenacion = 0;

    std::thread m_hiloOrdenacion;
    std::mutex m_mutexOrdenacion;
    std::condition_variable m_condicionOrdenacion;
    std::optional<TrabajoOrdenacion> m_trabajoOrdenacion;
    bool m_detenerOrdenador = false;
};

} // namespace maxcopier
