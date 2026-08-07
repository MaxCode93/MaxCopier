#include "util/velocimetro.h"

#include <cmath>

namespace maxcopier {
namespace {
// Peso de la muestra nueva en la media exponencial de la velocidad instantánea.
constexpr double kPesoMuestra = 0.3;
} // namespace

void Velocimetro::reiniciar()
{
    *this = Velocimetro();
}

void Velocimetro::registrar(qint64 bytesTotales, qint64 ms)
{
    if (!m_iniciado) {
        m_iniciado = true;
        m_msInicio = m_msUltimo = ms;
        m_bytesInicio = m_bytesUltimo = bytesTotales;
        return;
    }

    const qint64 msTranscurridos = ms - m_msUltimo;
    if (msTranscurridos <= 0)
        return;

    const double instantanea = 1000.0 * double(bytesTotales - m_bytesUltimo) / double(msTranscurridos);
    m_velocidad = m_velocidad <= 0.0 ? instantanea
                                     : (1.0 - kPesoMuestra) * m_velocidad + kPesoMuestra * instantanea;
    m_maxima = qMax(m_maxima, instantanea);

    m_msUltimo = ms;
    m_bytesUltimo = bytesTotales;
}

double Velocimetro::media() const
{
    const qint64 msTranscurridos = m_msUltimo - m_msInicio;
    if (msTranscurridos <= 0)
        return 0.0;
    return 1000.0 * double(m_bytesUltimo - m_bytesInicio) / double(msTranscurridos);
}

qint64 Velocimetro::segundosRestantes(qint64 pendientes) const
{
    if (pendientes <= 0)
        return 0;
    if (m_velocidad <= 0.0)
        return -1;
    return qint64(std::llround(double(pendientes) / m_velocidad));
}

} // namespace maxcopier
