#include "portapapeles.h"

#include "util/rutas.h"

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

namespace {

QStringList rutasDelPortapapelesAbierto()
{
    QStringList rutas;
    if (HANDLE contenido = GetClipboardData(CF_HDROP)) {
        const HDROP soltado = static_cast<HDROP>(contenido);
        const UINT cuantos = DragQueryFileW(soltado, 0xFFFFFFFF, nullptr, 0);
        rutas.reserve(int(cuantos));
        for (UINT i = 0; i < cuantos; ++i) {
            const UINT largo = DragQueryFileW(soltado, i, nullptr, 0);
            if (largo == 0)
                continue;
            QString ruta;
            ruta.resize(int(largo) + 1);
            if (DragQueryFileW(soltado, i, reinterpret_cast<wchar_t *>(ruta.data()), largo + 1) > 0)
            {
                ruta.resize(int(largo));
                rutas.append(ruta);
            }
        }
    }
    return rutas;
}

bool portapapelesMarcaMover()
{
    const UINT formato = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT);
    if (!formato)
        return false;

    const HANDLE contenido = GetClipboardData(formato);
    if (!contenido)
        return false;

    const auto *efecto = static_cast<const DWORD *>(GlobalLock(contenido));
    const bool mover = efecto && (*efecto & DROPEFFECT_MOVE) != 0;
    if (efecto)
        GlobalUnlock(contenido);
    return mover;
}

QStringList comparables(QStringList rutas)
{
    for (QString &ruta : rutas)
        ruta = maxcopier::rutaComparable(ruta);
    rutas.removeAll(QString());
    std::sort(rutas.begin(), rutas.end());
    return rutas;
}

} // namespace
#endif

namespace maxcopier {

bool limpiarPortapapelesCortado(const QStringList &origenes)
{
#ifdef Q_OS_WIN
    if (origenes.isEmpty() || !OpenClipboard(nullptr))
        return false;

    const bool esElMismoCorte = portapapelesMarcaMover()
        && comparables(rutasDelPortapapelesAbierto()) == comparables(origenes);
    const bool limpiado = esElMismoCorte && EmptyClipboard() != FALSE;
    CloseClipboard();
    return limpiado;
#else
    Q_UNUSED(origenes);
    return false;
#endif
}

} // namespace maxcopier
