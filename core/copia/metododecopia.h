#pragma once

namespace maxcopier {

/// Estrategia de E/S que usa el motor de copia para un archivo.
enum class MetodoDeCopia {
    Sincrono,  ///< lecturas/escrituras en bucle; funciona en cualquier sistema
    Asincrono, ///< overlapped con varias E/S en vuelo (solo Windows; fuera de
               ///< Windows el motor cae al síncrono automáticamente)
};

} // namespace maxcopier
