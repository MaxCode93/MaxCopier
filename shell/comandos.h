#pragma once

/// Decodificación del verbo que Explorer pasa a IContextMenu::InvokeCommand.
/// El comando con desplazamiento cero se representa como MAKEINTRESOURCE(0),
/// que es un puntero nulo: hay que comprobar IS_INTRESOURCE antes de rechazar
/// un puntero nulo.

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>

namespace maxcopier::shell {
namespace detalle {

    inline constexpr wchar_t kVerboCopiaW[] = L"maxcopier-copy";
    inline constexpr wchar_t kVerboMovimientoW[] = L"maxcopier-move";
    inline constexpr char kVerboCopiaA[] = "maxcopier-copy";
    inline constexpr char kVerboMovimientoA[] = "maxcopier-move";

} // namespace detalle

inline int indiceDelVerbo(const CMINVOKECOMMANDINFO *informacion)
{
    if (!informacion)
        return -1;

    if (informacion->cbSize >= sizeof(CMINVOKECOMMANDINFOEX)
        && (informacion->fMask & CMIC_MASK_UNICODE)) {
        const auto *ampliada = reinterpret_cast<const CMINVOKECOMMANDINFOEX *>(informacion);
        if (IS_INTRESOURCE(ampliada->lpVerbW))
            return int(LOWORD(ampliada->lpVerbW));
        if (!ampliada->lpVerbW)
            return -1;
        if (StrCmpIW(ampliada->lpVerbW, L"maxcopier") == 0
            || StrCmpIW(ampliada->lpVerbW, detalle::kVerboCopiaW) == 0)
            return 0;
        if (StrCmpIW(ampliada->lpVerbW, detalle::kVerboMovimientoW) == 0)
            return 1;
        return -1;
    }

    if (IS_INTRESOURCE(informacion->lpVerb))
        return int(LOWORD(informacion->lpVerb));
    if (!informacion->lpVerb)
        return -1;
    if (lstrcmpiA(informacion->lpVerb, "maxcopier") == 0
        || lstrcmpiA(informacion->lpVerb, detalle::kVerboCopiaA) == 0)
        return 0;
    if (lstrcmpiA(informacion->lpVerb, detalle::kVerboMovimientoA) == 0)
        return 1;
    return -1;
}

} // namespace maxcopier::shell
