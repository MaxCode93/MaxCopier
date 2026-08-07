#include "util/formatos.h"

#include <QLocale>
#include <cmath>

namespace maxcopier {

namespace {
constexpr double kKiB = 1024.0;

QString conUnidad(double valor, const char *unidad, int decimales)
{
    return QLocale().toString(valor, 'f', decimales) + QLatin1Char(' ') + QLatin1String(unidad);
}
} // namespace

QString formatearTamano(qint64 bytes)
{
    if (bytes < 0)
        bytes = 0;
    if (bytes < 1024)
        return QLocale().toString(bytes) + QStringLiteral(" B");

    static const char *unidades[] = {"KB", "MB", "GB", "TB", "PB"};
    double valor = static_cast<double>(bytes) / kKiB;
    int i = 0;
    while (valor >= kKiB && i < 4) {
        valor /= kKiB;
        ++i;
    }
    const int decimales = valor < 10.0 ? 2 : (valor < 100.0 ? 1 : 0);
    return conUnidad(valor, unidades[i], decimales);
}

QString formatearVelocidad(double bytesPorSegundo)
{
    if (bytesPorSegundo <= 0.0)
        return QStringLiteral("0 B/s");
    return formatearTamano(static_cast<qint64>(std::llround(bytesPorSegundo))) + QStringLiteral("/s");
}

QString formatearDuracion(qint64 segundos)
{
    if (segundos < 0)
        return QStringLiteral("--:--");

    const qint64 horas = segundos / 3600;
    const qint64 minutos = (segundos % 3600) / 60;
    const qint64 segs = segundos % 60;

    if (horas > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(horas)
            .arg(minutos, 2, 10, QLatin1Char('0'))
            .arg(segs, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(minutos, 2, 10, QLatin1Char('0')).arg(segs, 2, 10, QLatin1Char('0'));
}

int porcentaje(qint64 hecho, qint64 total)
{
    if (total <= 0)
        return 0;
    const double p = 100.0 * static_cast<double>(hecho) / static_cast<double>(total);
    return qBound(0, static_cast<int>(p), 100);
}

} // namespace maxcopier
