#pragma once

#include <QString>

namespace maxcopier {

/// Copia las fechas (modificación, acceso y, donde se puede, creación) y los
/// atributos (solo lectura, oculto, sistema…) de `origen` a `destino` una vez
/// el destino está completo. En Windows usa las APIs nativas (con rutas
/// largas); en el resto de sistemas, Qt. Devuelve false solo si no se pudo
/// replicar nada: un fallo parcial no debe fracasar la copia del archivo.
bool copiarMetadatos(const QString &origen, const QString &destino,
    QString *error = nullptr);

} // namespace maxcopier
