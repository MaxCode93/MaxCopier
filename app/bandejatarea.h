#pragma once

#include <QtGlobal>

class QWidget;

namespace maxcopier {

/// Progreso y superposición de pausa en el botón de la barra de tareas de
/// Windows (ITaskbarList3): cada ventana de copia muestra su avance en el
/// botón y el símbolo ⏸ cuando está en pausa. Fuera de Windows no hace nada.
class BandejaDeTarea {
public:
    BandejaDeTarea();
    ~BandejaDeTarea();

    /// Actualiza el botón de `ventana`. `hecho`/`total` en las mismas
    /// unidades (se puede pasar el porcentaje con total 100).
    void fijar(QWidget *ventana, qint64 hecho, qint64 total, bool pausada, bool activa);

private:
    class Impl;
    Impl *m_impl = nullptr;
};

} // namespace maxcopier
