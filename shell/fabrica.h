#pragma once

#include <unknwn.h>
#include <windows.h>

namespace maxcopier::shell {

/// Fábrica COM: lo único que sabe hacer es crear una `Extension`.
class Fabrica : public IClassFactory {
public:
    Fabrica();

    IFACEMETHODIMP QueryInterface(REFIID riid, void **objeto) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    IFACEMETHODIMP CreateInstance(IUnknown *agregador, REFIID riid, void **objeto) override;
    IFACEMETHODIMP LockServer(BOOL bloquear) override;

private:
    virtual ~Fabrica();

    LONG m_referencias;
};

} // namespace maxcopier::shell
