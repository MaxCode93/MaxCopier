#pragma once

/// Identificadores COM de la extensión del Explorador. El CLSID es fijo y
/// propio de MaxCopier: cambiarlo dejaría claves huérfanas en el registro de
/// quien ya la tuviera instalada.

#include <windows.h>

// {6E5B3D21-9C4A-4F52-A0C6-2D7F1B8E44A1}
DEFINE_GUID(
    CLSID_ExtensionMaxCopier, 0x6e5b3d21, 0x9c4a, 0x4f52, 0xa0, 0xc6, 0x2d, 0x7f, 0x1b, 0x8e, 0x44, 0xa1);

#define MAXCOPIER_CLSID_TEXTO L"{6E5B3D21-9C4A-4F52-A0C6-2D7F1B8E44A1}"
#define MAXCOPIER_NOMBRE_EXTENSION L"MaxCopier"
