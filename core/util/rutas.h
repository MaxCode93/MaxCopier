#pragma once

#include <QString>

namespace maxcopier {

/// Devuelve una ruta absoluta, limpia y comparable para el sistema operativo.
/// En Windows la comparación de nombres de archivo no distingue mayúsculas.
QString rutaComparable(const QString &ruta);

/// Comparaciones de rutas que respetan los límites de cada componente. Por
/// ejemplo, «C:/Fotos2» no está dentro de «C:/Fotos».
bool rutasIguales(const QString &una, const QString &otra);
bool rutaDescendienteDe(const QString &posibleHija, const QString &directorioPadre);
bool rutaMismaODescendiente(const QString &posibleHija, const QString &directorioPadre);

/// Ruta final que corresponde a un origen dentro de una carpeta de destino.
QString destinoDeOrigen(const QString &origen, const QString &carpetaDestino);

/// Impide copiar/mover un archivo sobre sí mismo o una carpeta dentro de sí
/// misma. Se usa tanto en la UI como en el escáner/motor como última defensa.
bool destinoSeSolapaConOrigen(const QString &origen, const QString &carpetaDestino);

} // namespace maxcopier
