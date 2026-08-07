#pragma once

#include <QColor>
#include <QString>

namespace maxcopier {

enum class Tema { Oscuro, Claro };

/// Colores del tema para los widgets que se dibujan a mano (barras, gráfica).
/// Los widgets estándar se visten con las hojas `.qss` de los recursos.
struct Paleta {
    QColor acento;
    QColor acento2;
    QColor acentoSuave;
    QColor fondo;
    QColor fondo2;
    QColor fondo3;
    QColor linea;
    QColor texto;
    QColor atenuado;
    QColor barra;
    QColor relleno1;
    QColor relleno2;
};

/// Aplica la hoja de estilos del tema a toda la aplicación y lo fija como tema actual.
void aplicarTema(Tema tema);

/// Tema aplicado ahora mismo.
Tema temaActual();

/// Paleta del tema indicado.
const Paleta &paleta(Tema tema);

/// Paleta del tema aplicado ahora mismo.
const Paleta &paletaActual();

/// Tema contrario al indicado (para el botón de alternar).
Tema temaContrario(Tema tema);

/// Nombre visible del tema, en español.
QString nombreTema(Tema tema);

} // namespace maxcopier
