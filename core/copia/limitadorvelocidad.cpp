#include "copia/limitadorvelocidad.h"

#include <QThread>

#include <chrono>

namespace maxcopier {
namespace {

qint64 ahoraMs()
{
    return qint64(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
}

} // namespace

LimitadorVelocidad::LimitadorVelocidad()
    : m_inicioMs(ahoraMs())
{
}

void LimitadorVelocidad::establecerLimite(qint64 bytesPorSegundo)
{
    m_limite.storeRelaxed(qMax<qint64>(0, bytesPorSegundo));
}

void LimitadorVelocidad::gastar(qint64 bytes, const QAtomicInt &pausa, const QAtomicInt &cancelar)
{
    const qint64 limite = m_limite.loadRelaxed();
    if (limite <= 0) {
        // Sin límite no se acumula deuda: si el usuario pone un límite a mitad
        // de la transferencia, el presupuesto empieza de cero.
        m_gastados.storeRelaxed(0);
        m_inicioMs.storeRelaxed(ahoraMs());
        return;
    }

    if (bytes <= 0)
        return;

    m_gastados.fetchAndAddRelaxed(bytes);

    while (true) {
        const qint64 gastados = m_gastados.loadRelaxed();
        const qint64 permitido = (ahoraMs() - m_inicioMs.loadRelaxed()) * limite / 1000;
        if (gastados <= permitido)
            return;
        if (pausa.loadRelaxed() != 0 || cancelar.loadRelaxed() != 0)
            return;
        const qint64 deficit = gastados - permitido;
        const qint64 espera
            = qBound<qint64>(qint64(1), deficit * 1000 / limite, qint64(50));
        QThread::msleep(static_cast<unsigned long>(espera));
    }
}

} // namespace maxcopier
