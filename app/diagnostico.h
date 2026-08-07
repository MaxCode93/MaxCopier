#pragma once

/// Registro mínimo para poder mirar qué pasó con el canal del Explorador sin
/// tener que depurar en la máquina del usuario. Se escribe en
/// `%LOCALAPPDATA%\MaxCopier\maxcopier.log` (una línea por suceso, el archivo
/// se recorta cuando pasa de 256 KB).

#include <QString>

namespace maxcopier {

void anotar(const QString &texto);

/// Ruta del archivo de registro, para enseñarla en la interfaz o en la ayuda.
QString rutaDelRegistro();

} // namespace maxcopier
