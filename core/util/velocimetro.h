#pragma once

#include <QtGlobal>

namespace maxcopier {

/// Mide la velocidad de una copia a partir de muestras (bytes copiados en un
/// instante) y estima el tiempo restante. No usa el reloj: el instante lo pasa
/// quien mide, así se puede probar sin esperas.
class Velocimetro
{
public:
    void reiniciar();

    /// Registra el total copiado (`bytesTotales`) en el instante `ms`.
    void registrar(qint64 bytesTotales, qint64 ms);

    /// Velocidad instantánea suavizada, en bytes por segundo.
    double velocidad() const { return m_velocidad; }

    /// Velocidad media desde la primera muestra, en bytes por segundo.
    double media() const;

    /// Velocidad instantánea más alta vista, en bytes por segundo.
    double maxima() const { return m_maxima; }

    /// Segundos que faltan para copiar `pendientes` bytes; -1 si aún no se
    /// puede estimar.
    qint64 segundosRestantes(qint64 pendientes) const;

private:
    bool m_iniciado = false;
    qint64 m_msInicio = 0;
    qint64 m_bytesInicio = 0;
    qint64 m_msUltimo = 0;
    qint64 m_bytesUltimo = 0;
    double m_velocidad = 0.0;
    double m_maxima = 0.0;
};

} // namespace maxcopier
