#pragma once

class QWidget;

namespace maxcopier {

/// Radio de las esquinas de las ventanas de MaxCopier.
constexpr int kRadioVentana = 10;

/// Recorta `ventana` con esquinas redondeadas. Como las ventanas no tienen
/// marco del sistema, el `border-radius` del `.qss` solo curva el borde
/// pintado: hay que recortar la ventana con una máscara para que las esquinas
/// no las tapen los widgets de dentro. Se llama en cada `resizeEvent`.
void redondearEsquinas(QWidget *ventana, int radio = kRadioVentana);

} // namespace maxcopier
