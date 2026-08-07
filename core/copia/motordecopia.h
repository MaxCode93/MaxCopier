#pragma once

#include "copia/limitadorvelocidad.h"
#include "copia/metododecopia.h"

#include <QAtomicInt>
#include <QAtomicInteger>
#include <QObject>

namespace maxcopier {

/// Copia un archivo por bloques emitiendo progreso, velocidad y tiempo
/// restante. Está pensado para vivir en su propio hilo: `copiar()` bloquea ese
/// hilo hasta terminar, y `reiniciar()`, `alternarPausa()`, `saltar()` y
/// `cancelar()` son seguras desde cualquier hilo (solo tocan banderas
/// atómicas). Las banderas no se borran al empezar una copia: quien la pide
/// llama antes a `reiniciar()`, así una pausa o una cancelación pedidas antes
/// de arrancar siguen valiendo.
///
/// El motor copia siempre a un `.mcpart` lateral y lo renombra al terminar: si
/// la copia se corta (cancelar, error o caída) el parcial queda para reanudarla
/// la próxima vez, y al sobrescribir el archivo anterior solo se toca al final.
/// Al terminar replica fechas y atributos del origen. En Windows usa un backend
/// Win32 sin búferes de tamaño fijo y con soporte de rutas largas (`\\?\`); en
/// el resto de sistemas, `QFile`. La política de colisión la decide quien pide
/// la copia: el motor solo sabe si puede sobrescribir el destino o no.
class MotorDeCopia : public QObject
{
    Q_OBJECT

public:
    enum class Resultado {
        Terminada, ///< el archivo se copió entero
        Saltada,   ///< el usuario saltó el archivo en curso
        Cancelada, ///< el usuario canceló la copia
        Error,     ///< no se pudo leer, escribir o crear el destino
    };
    Q_ENUM(Resultado)

    explicit MotorDeCopia(QObject *parent = nullptr);

    /// Tamaño de bloque adaptativo: bloques grandes para archivos grandes.
    static qint64 tamanoDeBloque(qint64 tamanoArchivo);

    bool pausada() const { return m_pausa.loadRelaxed() != 0; }

    /// Límite por transferencia, en bytes por segundo. Cero significa sin
    /// límite. Es seguro cambiarlo mientras `copiar()` está trabajando.
    void establecerLimiteVelocidad(qint64 bytesPorSegundo);
    qint64 limiteVelocidad() const
    {
        return m_limitador != nullptr ? m_limitador->limite() : 0;
    }

    /// Estrategia de E/S para las siguientes copias. Fuera de Windows el
    /// asíncrono cae al síncrono: el motor siempre copia correctamente.
    void establecerMetodo(MetodoDeCopia metodo);
    MetodoDeCopia metodo() const
    {
        return MetodoDeCopia(m_metodo.loadRelaxed());
    }

    /// Comparte el límite de velocidad con otros motores de la misma ventana.
    /// Por defecto cada motor usa el suyo.
    void establecerLimitadorCompartido(LimitadorVelocidad *limitador);

public slots:
    /// Copia `origen` en `destino` (ruta completa del archivo de llegada). Sin
    /// `sobrescribir`, un destino que ya exista termina en `Error`. Con
    /// `sobrescribir`, se copia a un archivo lateral y se reemplaza el destino
    /// al final, así una copia cortada no destruye el archivo que ya estaba.
    /// Si `mover` es true, el origen se borra solo después de terminar la copia.
    void copiar(const QString &origen, const QString &destino, bool sobrescribir = false,
        bool mover = false);

    /// Baja las banderas de pausa, salto y cancelación.
    void reiniciar();

    /// Fija explícitamente el estado de pausa. Es segura desde cualquier hilo
    /// y evita que varias peticiones de una ventana con motores paralelos se
    /// inviertan entre sí.
    void establecerPausa(bool pausada);
    void alternarPausa();
    void saltar();
    void cancelar();

signals:
    void iniciada(const QString &origen, const QString &destino, qint64 tamano);
    void progreso(qint64 copiado, qint64 total, double bytesPorSegundo, qint64 segundosRestantes);
    void pausaCambiada(bool pausada);
    void terminada(maxcopier::MotorDeCopia::Resultado resultado, const QString &error);

private:
    /// Implementaciones de la copia de un archivo: comparten la preparación y
    /// la finalización (parcial, renombrado, metadatos, mover) de `copiar()`.
    void copiarSincrono(const QString &origen, const QString &parcial, qint64 total,
        qint64 resumido, Resultado *resultado, QString *error, qint64 *copiado);
#ifdef Q_OS_WIN
    void copiarAsincrono(const QString &origen, const QString &parcial, qint64 total,
        qint64 resumido, Resultado *resultado, QString *error, qint64 *copiado);
#endif

    QAtomicInt m_pausa{0};
    QAtomicInt m_saltar{0};
    QAtomicInt m_cancelar{0};
    QAtomicInt m_metodo{int(MetodoDeCopia::Sincrono)};
    LimitadorVelocidad m_limitadorPropio;
    LimitadorVelocidad *m_limitador = nullptr;
};

} // namespace maxcopier
