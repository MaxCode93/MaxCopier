#pragma once

/// Cliente del canal de MaxCopier: manda la petición a la app y, si no está
/// corriendo, la arranca y espera a que abra el canal. Esto vive dentro de
/// `explorer.exe`, así que no puede tardar ni bloquear: los tiempos de espera
/// son cortos y todo el trabajo de verdad lo hace la app.

#include "ipc/protocolo.h"

#include <string>
#include <vector>

namespace maxcopier::shell {

/// Manda la petición. `destino` vacío significa «que pregunte la app».
/// Devuelve `false` si no se ha podido hablar con MaxCopier.
bool enviarPeticion(
    ipc::Operacion operacion, const std::vector<std::wstring> &origenes, const std::wstring &destino,
    bool desdePortapapeles = false);

} // namespace maxcopier::shell
