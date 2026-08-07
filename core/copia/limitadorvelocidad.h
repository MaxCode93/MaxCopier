#pragma once

#include <QAtomicInt>
#include <QAtomicInteger>

namespace maxcopier {

/// Límite de velocidad compartido por una transferencia. Varios motores gastan
/// de un mismo presupuesto, así el total de la ventana no supera el límite
/// aunque haya N archivos copiándose a la vez. Es seguro llamarlo desde varios
/// hilos.
class LimitadorVelocidad {
public:
    LimitadorVelocidad();

    void establecerLimite(qint64 bytesPorSegundo);
    qint64 limite() const { return m_limite.loadRelaxed(); }

    /// Registra `bytes` gastados y duerme lo necesario para respetar el límite
    /// acumulado desde el inicio. Vuelve enseguida si `pausa` o `cancelar`
    /// están activos durante la espera (el bucle del motor gestiona entonces
    /// la pausa o la cancelación).
    void gastar(qint64 bytes, const QAtomicInt &pausa, const QAtomicInt &cancelar);

private:
    QAtomicInteger<qint64> m_limite { 0 };
    QAtomicInteger<qint64> m_gastados { 0 };
    QAtomicInteger<qint64> m_inicioMs { 0 };
};

} // namespace maxcopier
