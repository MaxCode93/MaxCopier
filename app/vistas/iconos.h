#pragma once

#include <QIcon>

namespace maxcopier {

/// Icono de la aplicación (ventana, barra de tareas y base del de la bandeja).
/// Sale de los PNG de `:/iconos`, generados desde `app/recursos/icono.svg` y
/// `icono-pequeno.svg`, así que no hace falta el módulo Qt Svg.
QIcon iconoDeLaApp();

/// El icono de la app con el estado de una transferencia superpuesto: el
/// porcentaje durante la copia, una pausa cuando está detenida y «OK» al
/// terminar correctamente. El porcentaje no lleva el símbolo `%` para que
/// también sea legible en el tamaño reducido de la bandeja.
QIcon iconoDeBandeja(int porcentaje, bool enCurso, bool pausada,
    bool terminada = false);

} // namespace maxcopier
