#pragma once

#include "ipc/protocolo.h"

/// El objeto COM que carga el Explorador. Una sola clase cubre los cuatro
/// enganches, porque en todos hace lo mismo (juntar orígenes + destino y
/// mandárselos a MaxCopier), solo cambia por dónde le llegan los datos:
///
/// - `IShellExtInit` + `IContextMenu`: menú contextual de archivos y carpetas
///   («Copiar con MaxCopier…», sin destino: lo pregunta la app), menú del fondo
///   de una carpeta («Pegar con MaxCopier», leyendo el portapapeles) y arrastre
///   con el botón derecho («Copiar aquí con MaxCopier»).
/// - `IPersistFile` + `IDropTarget`: arrastrar y soltar normal sobre una
///   carpeta o una unidad, que es lo que sustituye al copiador de Windows.

#include <shlobj.h>
#include <windows.h>

#include <string>
#include <vector>

namespace maxcopier::shell {

class Extension : public IShellExtInit, public IContextMenu, public IPersistFile, public IDropTarget {
public:
    Extension();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **objeto) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IShellExtInit
    IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE carpeta, IDataObject *datos, HKEY clave) override;

    // IContextMenu
    IFACEMETHODIMP QueryContextMenu(
        HMENU menu, UINT posicion, UINT primerId, UINT ultimoId, UINT banderas) override;
    IFACEMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO *informacion) override;
    IFACEMETHODIMP GetCommandString(
        UINT_PTR id, UINT tipo, UINT *reservado, LPSTR nombre, UINT tamano) override;

    // IPersistFile (solo para que el Explorador nos diga sobre qué carpeta se suelta)
    IFACEMETHODIMP GetClassID(CLSID *clase) override;
    IFACEMETHODIMP IsDirty() override;
    IFACEMETHODIMP Load(LPCOLESTR archivo, DWORD modo) override;
    IFACEMETHODIMP Save(LPCOLESTR archivo, BOOL recordar) override;
    IFACEMETHODIMP SaveCompleted(LPCOLESTR archivo) override;
    IFACEMETHODIMP GetCurFile(LPOLESTR *archivo) override;

    // IDropTarget
    IFACEMETHODIMP DragEnter(IDataObject *datos, DWORD teclas, POINTL punto, DWORD *efecto) override;
    IFACEMETHODIMP DragOver(DWORD teclas, POINTL punto, DWORD *efecto) override;
    IFACEMETHODIMP DragLeave() override;
    IFACEMETHODIMP Drop(IDataObject *datos, DWORD teclas, POINTL punto, DWORD *efecto) override;

private:
    virtual ~Extension();

    /// Qué entrada se pinta en el menú, según lo que trajo `Initialize`.
    enum class Entrada {
        Ninguna,
        CopiarPreguntando, ///< clic derecho sobre archivos: la app pregunta el destino
        CopiarAqui, ///< arrastre con el botón derecho sobre una carpeta
        Pegar, ///< pegar/cortar en una carpeta: conserva Preferred DropEffect
    };

    Entrada entradaQueToca() const;

    struct Comando {
        std::wstring texto;
        ipc::Operacion operacion = ipc::Operacion::Copiar;
        std::wstring destino;
        bool desdePortapapeles = false;
    };

    bool mandarAMaxCopier(const Comando &comando);

    LONG m_referencias;
    std::vector<std::wstring> m_origenes;
    std::wstring m_destino;
    ipc::Operacion m_operacionDeDatos = ipc::Operacion::Copiar;
    std::vector<Comando> m_comandos;
    bool m_arrastrandoAceptado = false;
};

} // namespace maxcopier::shell
