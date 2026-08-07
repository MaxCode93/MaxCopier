#pragma once

/// Lectura de las rutas y de la intención (copiar o cortar) de un objeto de
/// datos del shell, venga del portapapeles o de un arrastre.

#include "ipc/protocolo.h"

#include <windows.h>
#include <shellapi.h>
#include <objidl.h>

#include <string>
#include <vector>

namespace maxcopier::shell {

/// Rutas del formato `CF_HDROP` de `datos`. Vacío si no las trae.
std::vector<std::wstring> rutasDe(IDataObject *datos);

/// Rutas de los archivos que hay en el portapapeles.
std::vector<std::wstring> rutasDelPortapapeles();

/// Convierte una combinación de efectos de Windows en la operación preferida.
/// MOVE tiene prioridad aunque el proveedor también publique COPY como efecto
/// permitido.
inline ipc::Operacion operacionDeEfectoPreferido(DWORD efecto)
{
    return (efecto & DROPEFFECT_MOVE) != 0 ? ipc::Operacion::Mover : ipc::Operacion::Copiar;
}

/// Lo que pedía el origen (`Preferred DropEffect`): cortar es mover.
ipc::Operacion operacionDe(IDataObject *datos);
ipc::Operacion operacionDelPortapapeles();

} // namespace maxcopier::shell
