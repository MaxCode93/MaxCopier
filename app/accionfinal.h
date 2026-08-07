#pragma once

#include "configuracion.h"

#include <QString>

namespace maxcopier {

/// Ejecuta una acción de energía del sistema. `Nada` y `Cerrar` se resuelven
/// en el gestor de ventanas; esta función solo trata Suspender/Apagar.
bool ejecutarAccionDeEnergia(AccionAlTerminar accion, QString *error = nullptr);

} // namespace maxcopier
