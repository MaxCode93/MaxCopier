#include "fabrica.h"

#include "extension.h"
#include "modulo.h"

#include <new>

namespace maxcopier::shell {

Fabrica::Fabrica()
    : m_referencias(1)
{
    retenerModulo();
}

Fabrica::~Fabrica()
{
    soltarModulo();
}

IFACEMETHODIMP Fabrica::QueryInterface(REFIID riid, void **objeto)
{
    if (!objeto)
        return E_POINTER;
    if (riid != IID_IUnknown && riid != IID_IClassFactory) {
        *objeto = nullptr;
        return E_NOINTERFACE;
    }
    *objeto = static_cast<IClassFactory *>(this);
    AddRef();
    return S_OK;
}

IFACEMETHODIMP_(ULONG) Fabrica::AddRef()
{
    return InterlockedIncrement(&m_referencias);
}

IFACEMETHODIMP_(ULONG) Fabrica::Release()
{
    const LONG quedan = InterlockedDecrement(&m_referencias);
    if (quedan == 0)
        delete this;
    return ULONG(quedan);
}

IFACEMETHODIMP Fabrica::CreateInstance(IUnknown *agregador, REFIID riid, void **objeto)
{
    if (agregador)
        return CLASS_E_NOAGGREGATION;
    if (!objeto)
        return E_POINTER;

    auto *extension = new (std::nothrow) Extension;
    if (!extension) {
        *objeto = nullptr;
        return E_OUTOFMEMORY;
    }
    const HRESULT resultado = extension->QueryInterface(riid, objeto);
    extension->Release();
    return resultado;
}

IFACEMETHODIMP Fabrica::LockServer(BOOL bloquear)
{
    if (bloquear)
        retenerModulo();
    else
        soltarModulo();
    return S_OK;
}

} // namespace maxcopier::shell
