#pragma once

#include <QString>

namespace maxcopier {

/// Qué hacer cuando un archivo de la lista no se puede leer (ya no está, la
/// unidad se ha desconectado, no hay permisos…).
enum class AccionError {
    Preguntar, ///< abrir el diálogo de error de acceso
    Reintentar, ///< volver a intentarlo con el mismo archivo
    PonerAlFinal, ///< mandarlo al final de la lista y seguir con el siguiente
    Saltar, ///< quitarlo de la lista
};

/// Motivo por el que `origen` no se puede copiar, o una cadena vacía si sí se
/// puede: el texto es el que muestra el diálogo de error.
QString motivoInaccesible(const QString &origen);

/// Nombre en español de la política, para las opciones y el registro.
QString nombreAccionError(AccionError accion);

} // namespace maxcopier
