#pragma once

#include "lista/elementodecopia.h"
#include "util/espaciolibre.h"

#include <QAtomicInt>
#include <QObject>
#include <QStringList>

namespace maxcopier {

/// Recorre recursivamente los orígenes que se le pasan y va emitiendo lotes de
/// archivos con su ruta de llegada dentro de la carpeta de destino.
///
/// Está pensado para vivir en su propio hilo: `escanear()` bloquea ese hilo
/// hasta terminar y `cancelar()`/`reiniciar()` son seguras desde cualquier otro
/// (solo tocan una bandera atómica). Las carpetas vacías no generan filas: la
/// lista de copia es plana y solo contiene archivos; en modo mover se emiten
/// aparte para crearlas únicamente cuando toda la tanda ha terminado bien.
class Escaner : public QObject {
    Q_OBJECT

public:
    explicit Escaner(QObject *parent = nullptr);

    /// Ruta de llegada de `origen` (archivo o carpeta) dentro de `carpetaDestino`.
    static QString destinoDe(const QString &origen, const QString &carpetaDestino);

public slots:
    /// Escanea `origenes` (archivos o carpetas) hacia `carpetaDestino`. En modo
    /// mover emite también las carpetas de destino para que no se pierdan las
    /// vacías, sin modificar el destino mientras el escaneo aún puede
    /// cancelarse.
    void escanear(const QStringList &origenes, const QString &carpetaDestino, bool mover = false);

    /// Calcula fuera del hilo de la interfaz el espacio necesario por volumen.
    /// Se mantiene en este hilo porque el escáner ya dispone de un worker
    /// dedicado y, después de `terminado`, vuelve a su bucle de eventos.
    void comprobarEspacio(const maxcopier::ElementosDeCopia &pendientes, quint64 generacion);

    void reiniciar();
    /// Pausa o reanuda la enumeración. Es seguro invocarlo desde otro hilo:
    /// solo cambia una bandera atómica y el escaneo la observa entre entradas.
    void alternarPausa();
    void cancelar();

signals:
    /// Un lote de archivos encontrados, listo para añadir a la lista.
    void encontrados(const maxcopier::ElementosDeCopia &lote);

    /// Directorios que deben existir al terminar correctamente una tanda de
    /// movimiento. Se emiten antes de sus archivos y no se crean aquí: así
    /// cancelar o fallar no deja una estructura vacía engañosa en el destino.
    void directoriosEncontrados(const QStringList &directorios);

    /// Fin del escaneo: cuántos archivos y bytes se han encontrado en total.
    void terminado(int archivos, qint64 bytes, bool cancelado);

    void espacioComprobado(const QList<maxcopier::FaltaDeEspacio> &faltas, quint64 generacion);

    void pausaCambiada(bool pausada);

private:
    QAtomicInt m_cancelar { 0 };
    QAtomicInt m_pausa { 0 };
};

} // namespace maxcopier
