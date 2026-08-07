#include "extension.h"

#include "cliente.h"
#include "comandos.h"
#include "identificadores.h"
#include "modulo.h"
#include "portapapeles.h"

#include <shellapi.h> // CMIC_MASK_UNICODE
#include <shlwapi.h>
#include <strsafe.h>

namespace maxcopier::shell {
namespace {

    constexpr wchar_t kTextoCopiarPreguntando[] = L"Copiar con MaxCopier…";
    constexpr wchar_t kTextoMoverPreguntando[] = L"Mover con MaxCopier…";
    constexpr wchar_t kTextoCopiarAqui[] = L"Copiar aquí con MaxCopier";
    constexpr wchar_t kTextoMoverAqui[] = L"Mover aquí con MaxCopier";
    constexpr wchar_t kTextoPegar[] = L"Pegar con MaxCopier";

    bool hayArchivosEnElPortapapeles()
    {
        return IsClipboardFormatAvailable(CF_HDROP) != FALSE;
    }

    std::wstring rutaDe(PCIDLIST_ABSOLUTE carpeta)
    {
        if (!carpeta)
            return {};

        // `SHGetPathFromIDListW` limita la salida a MAX_PATH. La variante que
        // devuelve memoria del sistema sí conserva rutas largas, que también
        // deben poder llegar a una operación de mover/cortar.
        PWSTR rutaLarga = nullptr;
        if (SUCCEEDED(SHGetNameFromIDList(carpeta, SIGDN_FILESYSPATH, &rutaLarga)) && rutaLarga) {
            std::wstring resultado(rutaLarga);
            CoTaskMemFree(rutaLarga);
            return resultado;
        }

        wchar_t ruta[MAX_PATH] = { 0 };
        if (!SHGetPathFromIDListW(carpeta, ruta))
            return {};
        return ruta;
    }

    std::wstring carpetaDe(const std::wstring &archivo)
    {
        const size_t separador = archivo.find_last_of(L"\\/");
        if (separador == std::wstring::npos)
            return {};
        if (separador == 0)
            return archivo.substr(0, 1);
        // Conserva la barra de la raíz de una unidad: «C:\archivo» → «C:\».
        if (separador == 2 && archivo.size() > 1 && archivo[1] == L':')
            return archivo.substr(0, 3);
        return archivo.substr(0, separador);
    }

    bool iguales(const std::wstring &una, const std::wstring &otra)
    {
        return !una.empty() && StrCmpIW(una.c_str(), otra.c_str()) == 0;
    }

    DWORD efectoVisible(DWORD teclas, ipc::Operacion preferida)
    {
        if (teclas & MK_SHIFT)
            return DROPEFFECT_MOVE;
        if (teclas & MK_CONTROL)
            return DROPEFFECT_COPY;
        return preferida == ipc::Operacion::Mover ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
    }

} // namespace

Extension::Extension()
    : m_referencias(1)
{
    retenerModulo();
}

Extension::~Extension()
{
    soltarModulo();
}

IFACEMETHODIMP Extension::QueryInterface(REFIID riid, void **objeto)
{
    if (!objeto)
        return E_POINTER;

    if (riid == IID_IUnknown || riid == IID_IContextMenu)
        *objeto = static_cast<IContextMenu *>(this);
    else if (riid == IID_IShellExtInit)
        *objeto = static_cast<IShellExtInit *>(this);
    else if (riid == IID_IPersistFile || riid == IID_IPersist)
        *objeto = static_cast<IPersistFile *>(this);
    else if (riid == IID_IDropTarget)
        *objeto = static_cast<IDropTarget *>(this);
    else {
        *objeto = nullptr;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

IFACEMETHODIMP_(ULONG) Extension::AddRef()
{
    return InterlockedIncrement(&m_referencias);
}

IFACEMETHODIMP_(ULONG) Extension::Release()
{
    const LONG quedan = InterlockedDecrement(&m_referencias);
    if (quedan == 0)
        delete this;
    return ULONG(quedan);
}

IFACEMETHODIMP Extension::Initialize(PCIDLIST_ABSOLUTE carpeta, IDataObject *datos, HKEY)
{
    m_comandos.clear();
    m_operacionDeDatos = ipc::Operacion::Copiar;
    m_arrastrandoAceptado = false;
    m_destino = rutaDe(carpeta);
    m_origenes = rutasDe(datos);
    m_operacionDeDatos = operacionDe(datos);
    if (m_origenes.empty() && !m_destino.empty() && hayArchivosEnElPortapapeles()) {
        // Algunas variantes del menú de fondo no entregan el IDataObject a la
        // extensión. En ese camino leemos la misma marca directamente del
        // portapapeles para no convertir un cortar+pegar en una copia.
        m_operacionDeDatos = operacionDelPortapapeles();
    }

    // Sin archivos y sin carpeta no hay nada que ofrecer; con carpeta sola
    // todavía queda la opción de pegar lo que haya en el portapapeles.
    if (m_origenes.empty() && m_destino.empty())
        return E_INVALIDARG;
    return S_OK;
}

Extension::Entrada Extension::entradaQueToca() const
{
    if (!m_origenes.empty()) {
        // Al pulsar el botón derecho sobre unos archivos, el Explorador nos
        // pasa también la carpeta en la que están: eso no es un destino (sería
        // copiarlos sobre sí mismos), así que ahí toca preguntar adónde van.
        if (m_destino.empty() || iguales(carpetaDe(m_origenes.front()), m_destino))
            return Entrada::CopiarPreguntando;
        return Entrada::CopiarAqui;
    }
    if (!m_destino.empty() && hayArchivosEnElPortapapeles())
        return Entrada::Pegar;
    return Entrada::Ninguna;
}

IFACEMETHODIMP Extension::QueryContextMenu(HMENU menu, UINT posicion, UINT primerId, UINT, UINT banderas)
{
    if (banderas & CMF_DEFAULTONLY)
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    m_comandos.clear();
    const Entrada entrada = entradaQueToca();

    switch (entrada) {
    case Entrada::CopiarPreguntando:
        m_comandos.push_back({ kTextoCopiarPreguntando, ipc::Operacion::Copiar, {} });
        m_comandos.push_back({ kTextoMoverPreguntando, ipc::Operacion::Mover, {} });
        break;
    case Entrada::CopiarAqui:
        m_comandos.push_back({ kTextoCopiarAqui, ipc::Operacion::Copiar, m_destino });
        m_comandos.push_back({ kTextoMoverAqui, ipc::Operacion::Mover, m_destino });
        break;
    case Entrada::Pegar:
        m_comandos.push_back({ kTextoPegar, m_operacionDeDatos, m_destino, true });
        break;
    case Entrada::Ninguna:
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }

    for (size_t i = 0; i < m_comandos.size(); ++i) {
        if (!InsertMenuW(menu, posicion + UINT(i), MF_STRING | MF_BYPOSITION,
                primerId + UINT(i), m_comandos[i].texto.c_str())) {
            m_comandos.clear();
            return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
        }
    }

    // DragDropHandlers recibe el objeto de datos que Explorer va a pegar o
    // arrastrar. El menú trae Copy (1) o Move (2) como acción predeterminada;
    // sustituimos ese identificador por el comando equivalente de MaxCopier.
    // En el fondo de una carpeta solo hay un comando Pegar, que se convierte
    // directamente en la acción predeterminada para Ctrl+V/Shift+Insert.
    const UINT predeterminado = GetMenuDefaultItem(menu, FALSE, 0);
    if (entrada == Entrada::CopiarAqui) {
        if (predeterminado == 1)
            SetMenuDefaultItem(menu, primerId, FALSE);
        else if (predeterminado == 2)
            SetMenuDefaultItem(menu, primerId + 1, FALSE);
        else if (predeterminado == UINT(-1))
            SetMenuDefaultItem(menu,
                m_operacionDeDatos == ipc::Operacion::Mover ? primerId + 1 : primerId, FALSE);
    } else if (entrada == Entrada::Pegar) {
        SetMenuDefaultItem(menu, primerId, FALSE);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, USHORT(m_comandos.size()));
}

IFACEMETHODIMP Extension::InvokeCommand(CMINVOKECOMMANDINFO *informacion)
{
    if (!informacion)
        return E_INVALIDARG;
    const int indice = indiceDelVerbo(informacion);
    if (indice < 0 || size_t(indice) >= m_comandos.size())
        return E_INVALIDARG;

    // El envío va en otro hilo. Si ni siquiera se pudo crear ese hilo, dejamos
    // que Explorer sepa que el verbo no se ejecutó.
    return mandarAMaxCopier(m_comandos[size_t(indice)]) ? S_OK : E_FAIL;
}

IFACEMETHODIMP Extension::GetCommandString(UINT_PTR id, UINT tipo, UINT *, LPSTR nombre, UINT tamano)
{
    if (!nombre || tamano == 0 || id >= m_comandos.size())
        return E_INVALIDARG;

    // Nombre canónico del verbo; el resto de tipos no nos hacen falta.
    if (tipo == GCS_VERBW)
        return StringCchCopyW(reinterpret_cast<LPWSTR>(nombre), tamano,
            m_comandos[id].operacion == ipc::Operacion::Mover ? L"maxcopier-move" : L"maxcopier-copy");
    if (tipo == GCS_VERBA)
        return StringCchCopyA(nombre, tamano,
            m_comandos[id].operacion == ipc::Operacion::Mover ? "maxcopier-move" : "maxcopier-copy");
    return E_NOTIMPL;
}

bool Extension::mandarAMaxCopier(const Comando &comando)
{
    const std::vector<std::wstring> origenes
        = m_origenes.empty() ? rutasDelPortapapeles() : m_origenes;
    return enviarPeticion(comando.operacion, origenes, comando.destino, comando.desdePortapapeles);
}

IFACEMETHODIMP Extension::GetClassID(CLSID *clase)
{
    if (!clase)
        return E_POINTER;
    *clase = CLSID_ExtensionMaxCopier;
    return S_OK;
}

IFACEMETHODIMP Extension::IsDirty()
{
    return S_FALSE;
}

IFACEMETHODIMP Extension::Load(LPCOLESTR archivo, DWORD)
{
    // Como controlador de soltado, el Explorador nos dice aquí sobre qué
    // carpeta o unidad se está soltando.
    if (!archivo)
        return E_INVALIDARG;
    m_destino = archivo;
    m_origenes.clear();
    m_comandos.clear();
    m_operacionDeDatos = ipc::Operacion::Copiar;
    return S_OK;
}

IFACEMETHODIMP Extension::Save(LPCOLESTR, BOOL)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP Extension::SaveCompleted(LPCOLESTR)
{
    return E_NOTIMPL;
}

IFACEMETHODIMP Extension::GetCurFile(LPOLESTR *archivo)
{
    if (!archivo)
        return E_POINTER;
    *archivo = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP Extension::DragEnter(IDataObject *datos, DWORD teclas, POINTL, DWORD *efecto)
{
    if (!efecto)
        return E_POINTER;

    // Solo aceptamos archivos de verdad; lo demás que lo mire el Explorador.
    m_origenes = rutasDe(datos);
    m_operacionDeDatos = operacionDe(datos);
    m_arrastrandoAceptado = !m_destino.empty() && !m_origenes.empty();
    *efecto = m_arrastrandoAceptado ? efectoVisible(teclas, m_operacionDeDatos) : DROPEFFECT_NONE;
    return S_OK;
}

IFACEMETHODIMP Extension::DragOver(DWORD teclas, POINTL, DWORD *efecto)
{
    if (!efecto)
        return E_POINTER;
    // Solo es una indicación visual. En Drop devolvemos COPY incluso para un
    // mover, porque el borrado real lo hace MaxCopier después de completar el
    // archivo; así Explorer no puede adelantarse a la copia.
    *efecto = m_arrastrandoAceptado ? efectoVisible(teclas, m_operacionDeDatos) : DROPEFFECT_NONE;
    return S_OK;
}

IFACEMETHODIMP Extension::DragLeave()
{
    m_arrastrandoAceptado = false;
    m_origenes.clear();
    return S_OK;
}

IFACEMETHODIMP Extension::Drop(IDataObject *datos, DWORD teclas, POINTL, DWORD *efecto)
{
    if (!efecto)
        return E_POINTER;

    m_arrastrandoAceptado = false;
    m_origenes = rutasDe(datos);
    if (m_origenes.empty() || m_destino.empty()) {
        *efecto = DROPEFFECT_NONE;
        return S_OK;
    }

    const ipc::Operacion operacion = teclas & MK_SHIFT ? ipc::Operacion::Mover
        : teclas & MK_CONTROL ? ipc::Operacion::Copiar : operacionDe(datos);
    if (!enviarPeticion(operacion, m_origenes, m_destino)) {
        // Si el canal no pudo ni siquiera encolar la petición, no reclamamos
        // el Drop: Explorer aún puede ejecutar su operación normal.
        *efecto = DROPEFFECT_NONE;
        return S_OK;
    }

    // Siempre devolvemos COPY aunque la operación sea mover: el origen no debe
    // borrarse al salir de Drop, porque MaxCopier lo hará después de verificar
    // cada archivo. Devolver MOVE aquí provocaría un borrado prematuro por parte
    // de Explorer.
    *efecto = DROPEFFECT_COPY;
    return S_OK;
}

} // namespace maxcopier::shell
