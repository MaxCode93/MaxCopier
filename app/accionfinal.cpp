#include "accionfinal.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <powrprof.h>
#endif

namespace maxcopier {

bool ejecutarAccionDeEnergia(AccionAlTerminar accion, QString *error)
{
    if (accion != AccionAlTerminar::Suspender && accion != AccionAlTerminar::Apagar)
        return true;

#ifdef Q_OS_WIN
    if (accion == AccionAlTerminar::Suspender) {
        if (SetSuspendState(FALSE, FALSE, FALSE) != FALSE)
            return true;
        if (error)
            *error = QObject::tr("Windows no ha podido suspender el equipo.");
        return false;
    }

    if (ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_MAJOR_APPLICATION
            | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) != FALSE)
        return true;
    if (error)
        *error = QObject::tr("Windows no ha podido apagar el equipo.");
    return false;
#else
    if (error)
        *error = QObject::tr("Esta acción de energía solo está disponible en Windows.");
    return false;
#endif
}

} // namespace maxcopier
