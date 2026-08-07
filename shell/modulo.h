#pragma once

/// Estado del módulo: la instancia de la DLL y el contador de referencias que
/// decide si el Explorador puede descargarla.

#include <windows.h>

namespace maxcopier::shell {

HINSTANCE moduloDeLaExtension();
void fijarModuloDeLaExtension(HINSTANCE instancia);

void retenerModulo();
void soltarModulo();
bool moduloEnUso();

} // namespace maxcopier::shell
