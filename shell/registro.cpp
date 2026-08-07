#include "registro.h"

#include "identificadores.h"
#include "modulo.h"

#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <string>

namespace maxcopier::shell {
namespace {

    constexpr const wchar_t *kTiposConMenu[] = { L"*", L"Directory", L"Directory\\Background", L"Drive" };
    constexpr const wchar_t *kTiposConArrastre[] = { L"Directory", L"Folder", L"Drive" };
    constexpr const wchar_t *kTiposConSoltar[] = { L"Directory", L"Folder", L"Drive" };

    constexpr const wchar_t *kClaveDeLaApp = L"Software\\MaxCopier";
    constexpr const wchar_t *kValorDeLaApp = L"Aplicacion";

    std::wstring claveDeClase()
    {
        return std::wstring(L"Software\\Classes\\CLSID\\") + MAXCOPIER_CLSID_TEXTO;
    }

    std::wstring claveDeTipo(const wchar_t *tipo, const wchar_t *enganche)
    {
        return std::wstring(L"Software\\Classes\\") + tipo + L"\\shellex\\" + enganche;
    }

    HRESULT escribirTexto(const std::wstring &clave, const wchar_t *valor, const std::wstring &texto)
    {
        HKEY manejador = nullptr;
        LONG resultado = RegCreateKeyExW(HKEY_CURRENT_USER, clave.c_str(), 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &manejador, nullptr);
        if (resultado != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(DWORD(resultado));

        resultado = RegSetValueExW(manejador, valor, 0, REG_SZ, reinterpret_cast<const BYTE *>(texto.c_str()),
            DWORD((texto.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(manejador);
        return resultado == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(DWORD(resultado));
    }

    HRESULT borrarClave(const std::wstring &clave)
    {
        const LONG resultado = SHDeleteKeyW(HKEY_CURRENT_USER, clave.c_str());
        if (resultado == ERROR_SUCCESS || resultado == ERROR_FILE_NOT_FOUND)
            return S_OK;
        return HRESULT_FROM_WIN32(DWORD(resultado));
    }

    /// Deja apuntado dónde está `MaxCopier.exe` (al lado de la DLL). La
    /// extensión lo lee para arrancar la app cuando no está abierta, sin tener
    /// que adivinar la carpeta desde dentro del Explorador.
    void apuntarLaApp(const std::wstring &carpetaDeLaDll)
    {
        const std::wstring aplicacion = carpetaDeLaDll + L"\\MaxCopier.exe";
        if (!PathFileExistsW(aplicacion.c_str()))
            return;

        HKEY manejador = nullptr;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, kClaveDeLaApp, 0, nullptr, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, nullptr, &manejador, nullptr)
            != ERROR_SUCCESS)
            return;
        RegSetValueExW(manejador, kValorDeLaApp, 0, REG_SZ,
            reinterpret_cast<const BYTE *>(aplicacion.c_str()),
            DWORD((aplicacion.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(manejador);
    }

    /// La clase COM: dónde está la DLL. La necesitan tanto los menús como el
    /// controlador de soltado, así que se registra con cualquiera de los dos.
    HRESULT registrarClase()
    {
        wchar_t ruta[MAX_PATH] = { 0 };
        if (!GetModuleFileNameW(moduloDeLaExtension(), ruta, MAX_PATH))
            return HRESULT_FROM_WIN32(GetLastError());

        wchar_t carpeta[MAX_PATH] = { 0 };
        if (SUCCEEDED(StringCchCopyW(carpeta, MAX_PATH, ruta)) && PathRemoveFileSpecW(carpeta))
            apuntarLaApp(carpeta);

        const std::wstring clave = claveDeClase();
        HRESULT resultado = escribirTexto(clave, nullptr, MAXCOPIER_NOMBRE_EXTENSION);
        if (FAILED(resultado))
            return resultado;
        resultado = escribirTexto(clave + L"\\InprocServer32", nullptr, ruta);
        if (FAILED(resultado))
            return resultado;
        resultado = escribirTexto(clave + L"\\InprocServer32", L"ThreadingModel", L"Apartment");
        if (FAILED(resultado))
            return resultado;

        // Permite que Explorer nos consulte cuando está construyendo la
        // acción predeterminada del menú de transferencia. Sin esta marca,
        // algunas versiones no cargan el DragDropHandler para Ctrl+V.
        return escribirTexto(clave + L"\\shellex\\MayChangeDefaultMenu", nullptr, L"");
    }

    HRESULT registrarEn(
        const wchar_t *const *tipos, size_t cuantos, const wchar_t *enganche, bool valorPorDefecto)
    {
        for (size_t i = 0; i < cuantos; ++i) {
            const std::wstring clave = valorPorDefecto
                ? claveDeTipo(tipos[i], enganche)
                : claveDeTipo(tipos[i], enganche) + L"\\" + MAXCOPIER_NOMBRE_EXTENSION;
            const HRESULT resultado = escribirTexto(clave, nullptr, MAXCOPIER_CLSID_TEXTO);
            if (FAILED(resultado))
                return resultado;
        }
        return S_OK;
    }

    HRESULT quitarDe(
        const wchar_t *const *tipos, size_t cuantos, const wchar_t *enganche, bool valorPorDefecto)
    {
        HRESULT peor = S_OK;
        for (size_t i = 0; i < cuantos; ++i) {
            const std::wstring clave = valorPorDefecto
                ? claveDeTipo(tipos[i], enganche)
                : claveDeTipo(tipos[i], enganche) + L"\\" + MAXCOPIER_NOMBRE_EXTENSION;
            const HRESULT resultado = borrarClave(clave);
            if (FAILED(resultado))
                peor = resultado;
        }
        return peor;
    }

    template <size_t N> constexpr size_t cuantos(const wchar_t *const (&)[N])
    {
        return N;
    }

} // namespace

HRESULT registrarMenus()
{
    HRESULT resultado = registrarClase();
    if (FAILED(resultado))
        return resultado;

    resultado = registrarEn(kTiposConMenu, cuantos(kTiposConMenu), L"ContextMenuHandlers", false);
    if (FAILED(resultado))
        return resultado;
    resultado = registrarEn(kTiposConArrastre, cuantos(kTiposConArrastre), L"DragDropHandlers", false);
    if (FAILED(resultado))
        return resultado;

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return S_OK;
}

HRESULT quitarMenus()
{
    const HRESULT menus = quitarDe(kTiposConMenu, cuantos(kTiposConMenu), L"ContextMenuHandlers", false);
    const HRESULT arrastre
        = quitarDe(kTiposConArrastre, cuantos(kTiposConArrastre), L"DragDropHandlers", false);
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return FAILED(menus) ? menus : arrastre;
}

HRESULT quitarClase()
{
    borrarClave(kClaveDeLaApp);
    return borrarClave(claveDeClase());
}

std::wstring rutaRegistradaDeLaApp()
{
    wchar_t ruta[MAX_PATH] = { 0 };
    DWORD tamano = sizeof(ruta);
    DWORD tipo = 0;
    if (RegGetValueW(HKEY_CURRENT_USER, kClaveDeLaApp, kValorDeLaApp, RRF_RT_REG_SZ, &tipo, ruta, &tamano)
        != ERROR_SUCCESS)
        return {};
    return ruta;
}

HRESULT registrarSoltar()
{
    HRESULT resultado = registrarClase();
    if (FAILED(resultado))
        return resultado;

    // `DropHandler` solo admite un controlador: mientras esté puesto, el
    // arrastrar y soltar de carpetas y unidades pasa por MaxCopier.
    resultado = registrarEn(kTiposConSoltar, cuantos(kTiposConSoltar), L"DropHandler", true);
    if (FAILED(resultado))
        return resultado;

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return S_OK;
}

HRESULT quitarSoltar()
{
    const HRESULT resultado = quitarDe(kTiposConSoltar, cuantos(kTiposConSoltar), L"DropHandler", true);
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return resultado;
}

} // namespace maxcopier::shell
