#pragma once

namespace maxcopier {

/// Qué hacer con una copia pedida cuando ya hay otra en curso.
enum class AccionListaActiva {
    Preguntar, ///< abrir el diálogo de lista activa
    AnadirALaActual, ///< meter los archivos en la lista de la ventana ocupada
    VentanaNueva, ///< abrir otra ventana que copie a la vez
    Cancelar, ///< no copiar nada
};

} // namespace maxcopier
