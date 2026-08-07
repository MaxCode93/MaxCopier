#pragma once

/// Alta y baja de la extensión en el registro. Todo va a
/// `HKCU\Software\Classes`, que es el registro del usuario: no hace falta ser
/// administrador. El DragDropHandler se registra como handler predeterminado
/// de la transferencia para que el pegar estándar pueda llegar a MaxCopier.

#include <windows.h>

#include <string>

namespace maxcopier::shell {

/// Menú contextual (archivos, carpetas, fondo de carpeta y unidades) y arrastre
/// con el botón derecho.
HRESULT registrarMenus();
HRESULT quitarMenus();

/// Quita del registro la clase COM en sí (la ruta de la DLL).
HRESULT quitarClase();

/// Controlador de soltado: MaxCopier pasa a atender el arrastrar y soltar
/// normal sobre carpetas y unidades, en lugar del copiador de Windows.
HRESULT registrarSoltar();
HRESULT quitarSoltar();

/// Ruta de `MaxCopier.exe` que anotó la instalación (`HKCU\Software\MaxCopier`,
/// valor `Aplicacion`). Vacía si no está: entonces se busca al lado de la DLL.
std::wstring rutaRegistradaDeLaApp();

} // namespace maxcopier::shell
