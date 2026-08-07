#pragma once

#include <QString>

namespace maxcopier {

/// Qué hacer cuando el archivo de destino ya existe.
enum class AccionColision {
    Preguntar, ///< abrir el diálogo de colisión
    Sobrescribir, ///< reemplazar el archivo que ya está en el destino
    Renombrar, ///< copiar junto al existente con un nombre libre
    Saltar, ///< no copiar este archivo
};

/// Ruta libre para `destino`: si ya existe, intercala « (2)», « (3)»… antes de
/// la extensión hasta encontrar un nombre que no esté ocupado. Si `destino` no
/// existe, se devuelve tal cual.
QString rutaLibre(const QString &destino);

/// Nombre en español de la política, para el chip de la ventana.
QString nombreAccionColision(AccionColision accion);

} // namespace maxcopier
