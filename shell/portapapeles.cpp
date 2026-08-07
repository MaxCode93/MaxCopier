#include "portapapeles.h"

#include <shellapi.h>
#include <shlobj.h>

namespace maxcopier::shell {
namespace {

    std::vector<std::wstring> rutasDeHDrop(HDROP soltado)
    {
        std::vector<std::wstring> rutas;
        const UINT cuantos = DragQueryFileW(soltado, 0xFFFFFFFF, nullptr, 0);
        rutas.reserve(cuantos);
        for (UINT i = 0; i < cuantos; ++i) {
            const UINT largo = DragQueryFileW(soltado, i, nullptr, 0);
            if (largo == 0)
                continue;
            std::wstring ruta(largo + 1, L'\0');
            if (DragQueryFileW(soltado, i, ruta.data(), UINT(ruta.size())) > 0) {
                ruta.resize(largo);
                rutas.push_back(std::move(ruta));
            }
        }
        return rutas;
    }

    UINT formatoDeEfectoPreferido()
    {
        return RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT);
    }

    bool leerEfectoPreferido(IDataObject *datos, DWORD &efecto)
    {
        if (!datos)
            return false;

        const UINT formatoRegistrado = formatoDeEfectoPreferido();
        if (!formatoRegistrado)
            return false;

        FORMATETC formato = { CLIPFORMAT(formatoRegistrado), nullptr, DVASPECT_CONTENT, -1,
            TYMED_HGLOBAL };
        STGMEDIUM medio = {};
        if (FAILED(datos->GetData(&formato, &medio)))
            return false;

        bool bien = false;
        if (const auto *valor = static_cast<const DWORD *>(GlobalLock(medio.hGlobal))) {
            efecto = *valor;
            GlobalUnlock(medio.hGlobal);
            bien = true;
        }
        ReleaseStgMedium(&medio);
        return bien;
    }

} // namespace

std::vector<std::wstring> rutasDe(IDataObject *datos)
{
    if (!datos)
        return {};

    FORMATETC formato = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medio = {};
    if (FAILED(datos->GetData(&formato, &medio)))
        return {};

    std::vector<std::wstring> rutas;
    if (auto soltado = static_cast<HDROP>(GlobalLock(medio.hGlobal))) {
        rutas = rutasDeHDrop(soltado);
        GlobalUnlock(medio.hGlobal);
    }
    ReleaseStgMedium(&medio);
    return rutas;
}

std::vector<std::wstring> rutasDelPortapapeles()
{
    if (!OpenClipboard(nullptr))
        return {};

    std::vector<std::wstring> rutas;
    if (HANDLE contenido = GetClipboardData(CF_HDROP)) {
        // GetClipboardData ya devuelve un HDROP listo para DragQueryFile;
        // GlobalLock aquí lo convertía en un puntero a DROPFILES y hacía que
        // cortar+pegar pareciera no traer ninguna ruta.
        rutas = rutasDeHDrop(static_cast<HDROP>(contenido));
    }
    CloseClipboard();
    return rutas;
}

ipc::Operacion operacionDe(IDataObject *datos)
{
    DWORD efecto = DROPEFFECT_COPY;
    return leerEfectoPreferido(datos, efecto) ? operacionDeEfectoPreferido(efecto)
                                               : ipc::Operacion::Copiar;
}

ipc::Operacion operacionDelPortapapeles()
{
    const UINT formato = formatoDeEfectoPreferido();
    if (!formato || !OpenClipboard(nullptr))
        return ipc::Operacion::Copiar;

    ipc::Operacion operacion = ipc::Operacion::Copiar;
    if (HANDLE contenido = GetClipboardData(formato)) {
        if (const auto *efecto = static_cast<const DWORD *>(GlobalLock(contenido))) {
            operacion = operacionDeEfectoPreferido(*efecto);
            GlobalUnlock(contenido);
        }
    }
    CloseClipboard();
    return operacion;
}

} // namespace maxcopier::shell
