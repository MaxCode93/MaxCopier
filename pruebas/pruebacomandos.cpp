/// Regresión de IContextMenu::InvokeCommand: Explorer usa un puntero nulo para
/// MAKEINTRESOURCE(0), que es el primer comando («Copiar»/«Pegar»).

#include "comandos.h"
#include "portapapeles.h"

#include <cstdio>

namespace {

int fallos = 0;

void comprobar(bool bien, const char *que)
{
    std::printf("%s %s\n", bien ? "[ok]  " : "[FALLA]", que);
    if (!bien)
        ++fallos;
}

} // namespace

int main()
{
    CMINVOKECOMMANDINFO ansi = {};
    ansi.cbSize = sizeof(ansi);
    ansi.lpVerb = MAKEINTRESOURCEA(0);
    comprobar(maxcopier::shell::indiceDelVerbo(&ansi) == 0,
        "el verbo ANSI con offset cero se reconoce");

    CMINVOKECOMMANDINFOEX unicode = {};
    unicode.cbSize = sizeof(unicode);
    unicode.fMask = CMIC_MASK_UNICODE;
    unicode.lpVerbW = MAKEINTRESOURCEW(0);
    comprobar(maxcopier::shell::indiceDelVerbo(
                  reinterpret_cast<const CMINVOKECOMMANDINFO *>(&unicode))
            == 0,
        "el verbo Unicode con offset cero se reconoce");

    ansi.lpVerb = MAKEINTRESOURCEA(1);
    comprobar(maxcopier::shell::indiceDelVerbo(&ansi) == 1,
        "el segundo comando conserva su offset");

    unicode.lpVerbW = L"maxcopier-move";
    comprobar(maxcopier::shell::indiceDelVerbo(
                  reinterpret_cast<const CMINVOKECOMMANDINFO *>(&unicode))
            == 1,
        "el verbo Unicode por nombre se reconoce");

    comprobar(maxcopier::shell::operacionDeEfectoPreferido(DROPEFFECT_MOVE)
            == maxcopier::ipc::Operacion::Mover,
        "Preferred DropEffect MOVE se traduce a mover");
    comprobar(maxcopier::shell::operacionDeEfectoPreferido(DROPEFFECT_MOVE | DROPEFFECT_COPY)
            == maxcopier::ipc::Operacion::Mover,
        "MOVE conserva prioridad cuando también aparece COPY");
    comprobar(maxcopier::shell::operacionDeEfectoPreferido(DROPEFFECT_COPY)
            == maxcopier::ipc::Operacion::Copiar,
        "Preferred DropEffect COPY se traduce a copiar");

    return fallos == 0 ? 0 : 1;
}
