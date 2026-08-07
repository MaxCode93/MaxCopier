#pragma once

#include <QStringList>

namespace maxcopier {

/// Vacía el portapapeles solo si todavía contiene exactamente los orígenes
/// cortados que acaba de mover la aplicación. Devuelve true si se pudo limpiar;
/// en otras plataformas o si el usuario cambió el portapapeles no hace nada.
bool limpiarPortapapelesCortado(const QStringList &origenes);

} // namespace maxcopier
