#pragma once

#include <QString>
#include <cstdint>

namespace maxcopier {

/// Tamaño legible en unidades binarias: "1,45 GB".
QString formatearTamano(qint64 bytes);

/// Velocidad legible: "212 MB/s".
QString formatearVelocidad(double bytesPorSegundo);

/// Duración en "hh:mm:ss" (o "mm:ss" si es menor de una hora). Negativo = "--:--".
QString formatearDuracion(qint64 segundos);

/// Porcentaje 0..100 a partir de hechos/total, tolerante a total 0.
int porcentaje(qint64 hecho, qint64 total);

} // namespace maxcopier
