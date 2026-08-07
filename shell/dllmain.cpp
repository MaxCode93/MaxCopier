/// Puntos de entrada de la DLL. Las cuatro funciones que llama `regsvr32`:
///
/// - `regsvr32 /n /i:menus MaxCopierShell.dll`  → menú contextual
/// - `regsvr32 /n /i:todo MaxCopierShell.dll`   → handler predeterminado + menú
/// - `regsvr32 MaxCopierShell.dll`              → lo mismo que `/i:menus`
/// - `regsvr32 /u ...`                          → lo quita todo
///
/// Todo se escribe en `HKCU`, así que no hace falta ser administrador (con
/// `/n` `regsvr32` ni siquiera intenta usar `HKLM`).

#include <initguid.h> // define aquí el CLSID; en el resto de archivos es extern

#include "fabrica.h"
#include "identificadores.h"
#include "modulo.h"
#include "registro.h"

#include <shlwapi.h>
#include <windows.h>

#include <new>

namespace maxcopier::shell {
namespace {

    HINSTANCE g_instancia = nullptr;
    LONG g_referencias = 0;

} // namespace

HINSTANCE moduloDeLaExtension()
{
    return g_instancia;
}

void fijarModuloDeLaExtension(HINSTANCE instancia)
{
    g_instancia = instancia;
}

void retenerModulo()
{
    InterlockedIncrement(&g_referencias);
}

void soltarModulo()
{
    InterlockedDecrement(&g_referencias);
}

bool moduloEnUso()
{
    return InterlockedCompareExchange(&g_referencias, 0, 0) > 0;
}

} // namespace maxcopier::shell

BOOL WINAPI DllMain(HINSTANCE instancia, DWORD motivo, LPVOID)
{
    if (motivo == DLL_PROCESS_ATTACH) {
        maxcopier::shell::fijarModuloDeLaExtension(instancia);
        DisableThreadLibraryCalls(instancia);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clase, REFIID riid, void **objeto)
{
    if (!objeto)
        return E_POINTER;
    *objeto = nullptr;
    if (clase != CLSID_ExtensionMaxCopier)
        return CLASS_E_CLASSNOTAVAILABLE;

    auto *fabrica = new (std::nothrow) maxcopier::shell::Fabrica;
    if (!fabrica)
        return E_OUTOFMEMORY;
    const HRESULT resultado = fabrica->QueryInterface(riid, objeto);
    fabrica->Release();
    return resultado;
}

STDAPI DllCanUnloadNow()
{
    return maxcopier::shell::moduloEnUso() ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer()
{
    return maxcopier::shell::registrarMenus();
}

STDAPI DllUnregisterServer()
{
    const HRESULT soltar = maxcopier::shell::quitarSoltar();
    const HRESULT menus = maxcopier::shell::quitarMenus();
    const HRESULT clase = maxcopier::shell::quitarClase();
    if (FAILED(soltar))
        return soltar;
    return FAILED(menus) ? menus : clase;
}

STDAPI DllInstall(BOOL instalar, LPCWSTR orden)
{
    const bool todo = orden && StrCmpIW(orden, L"todo") == 0;

    if (!instalar) {
        return todo ? DllUnregisterServer() : maxcopier::shell::quitarSoltar();
    }

    const HRESULT menus = maxcopier::shell::registrarMenus();
    if (FAILED(menus) || !todo)
        return menus;
    return maxcopier::shell::registrarSoltar();
}
