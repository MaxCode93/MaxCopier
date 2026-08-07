#pragma once

#include <QList>
#include <QWidget>

class QLabel;

namespace maxcopier {

/// Datos visibles de un archivo que se está copiando ahora mismo.
struct ArchivoEnCurso {
    QString nombre;    ///< nombre del archivo
    QString tamano;    ///< formatearTamano(total)
    QString velocidad; ///< formatearVelocidad
    QString restante;  ///< formatearDuracion
    int porcentaje = 0;
    bool pausado = false;
};

/// Barra de progreso del archivo en curso que se divide en hasta cuatro
/// segmentos (uno por archivo activo, según «Archivos a la vez»). Cada
/// segmento muestra su porcentaje centrado y, debajo de la barra, una
/// mini-etiqueta con el nombre y la velocidad del archivo. Con un solo archivo
/// se comporta como la barra clásica de F2.
class BarraArchivos : public QWidget {
    Q_OBJECT

public:
    explicit BarraArchivos(QWidget *parent = nullptr);

    void establecerArchivos(const QList<ArchivoEnCurso> &archivos);
    void establecerMensajeVacio(const QString &mensaje);

    int cantidadDeArchivos() const { return m_archivos.size(); }

    QSize sizeHint() const override;

signals:
    /// El usuario pulsó el segmento `indice` (selecciona ese archivo).
    void segmentoClicado(int indice);

protected:
    void paintEvent(QPaintEvent *evento) override;
    void resizeEvent(QResizeEvent *evento) override;
    void mousePressEvent(QMouseEvent *evento) override;

private:
    void reorganizarEtiquetas();
    void limpiarEtiquetas();
    int indiceDeSegmento(int x) const;

    QList<ArchivoEnCurso> m_archivos;
    QString m_mensaje;
    QList<QLabel *> m_etiquetas;
};

} // namespace maxcopier
