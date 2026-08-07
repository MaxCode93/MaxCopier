#pragma once

// Backend Win32 de E/S para el motor de copia (F8): maneja rutas largas con el
// prefijo \\?\ y no impone búferes de tamaño fijo (los bloques los pone quien
// llama). Es Qt-libre a propósito: así se puede compilar con MinGW/MSVC sin Qt
// (o cruzado desde Linux) igual que la extensión del Explorador. Fuera de
// Windows el archivo .cpp no compila nada y estas funciones no se usan.

#include <cstdint>
#include <string>

namespace maxcopier {
namespace win32 {

/// Ruta lista para las APIs Win32: separadores de Windows, prefijo `\\?\`
/// (o `\\?\UNC\` para rutas UNC). La ruta debe ser absoluta, como las que
/// maneja el motor.
std::wstring rutaLarga(const std::wstring &ruta);

enum class ModoEscritura {
    Nuevo,    ///< falla si el archivo ya existe (CREATE_NEW)
    Reanudar, ///< abre lo que haya y escribe desde el final (OPEN_ALWAYS)
};

/// Abre `ruta` en lectura. Devuelve false y deja el motivo en `error`.
bool abrirLectura(const std::wstring &ruta, void **manejo, std::uint64_t *tamano,
    std::wstring *error);

/// Abre `ruta` en escritura con la política de `modo`.
bool abrirEscritura(const std::wstring &ruta, ModoEscritura modo, void **manejo,
    std::wstring *error);

/// Igual que `abrirLectura`/`abrirEscritura` pero con FILE_FLAG_OVERLAPPED para
/// poder lanzar E/S asíncrona sobre el manejo.
bool abrirLecturaAsincrona(const std::wstring &ruta, void **manejo, std::uint64_t *tamano,
    std::wstring *error);
bool abrirEscrituraAsincrona(const std::wstring &ruta, ModoEscritura modo, void **manejo,
    std::wstring *error);

/// E/S asíncrona por ranura: cada ranura tiene su OVERLAPPED y su evento, así
/// el motor puede tener varias lecturas/escrituras en vuelo a la vez.
struct IoAsincrono;
IoAsincrono *crearIo(std::wstring *error);
void liberarIo(IoAsincrono *io);
void *eventoDeIo(IoAsincrono *io);

/// Espera hasta `timeoutMs` a que ocurra alguno de los eventos (o todos, con
/// `todos`). Devuelve true si al menos uno estaba señalado al volver.
bool esperarEventos(void *const *eventos, int cantidad, int timeoutMs, bool todos);

/// Lanza una lectura/escritura sobre un manejo abierto con FILE_FLAG_OVERLAPPED.
/// Devuelve 0 si terminó al instante, 1 si quedó pendiente y -1 con error.
std::int64_t lanzarLectura(IoAsincrono *io, void *manejo, void *datos, std::int64_t n,
    std::uint64_t offset, std::wstring *error);
std::int64_t lanzarEscritura(IoAsincrono *io, void *manejo, const void *datos, std::int64_t n,
    std::uint64_t offset, std::wstring *error);

/// ¿La operación lanzada sobre `io` ya terminó (con éxito o con error)?
bool ioCompletado(IoAsincrono *io);

/// Bytes de la operación terminada; -1 con error.
std::int64_t resultadoIo(IoAsincrono *io, std::wstring *error);

/// Lee hasta `max` bytes. Devuelve -1 con error y 0 al llegar al final.
std::int64_t leer(void *manejo, void *datos, std::int64_t max, std::wstring *error);

/// Escribe `n` bytes (puede tardar varios WriteFile si el sistema entrega
/// menos de una vez).
bool escribir(void *manejo, const void *datos, std::int64_t n, std::wstring *error);

/// Mueve el puntero de lectura/escritura a `posicion`.
bool buscar(void *manejo, std::uint64_t posicion, std::wstring *error);

/// Volca los búferes del sistema al disco.
bool vaciar(void *manejo, std::wstring *error);

void cerrar(void *manejo);

/// Tamaño de un archivo sin abrirlo. Falso si no existe o no se puede leer.
bool tamanoDe(const std::wstring &ruta, std::uint64_t *tamano, std::wstring *error);

/// Borra un archivo; no es error que ya no exista.
bool eliminar(const std::wstring &ruta, std::wstring *error);

/// Deja `origen` en `destino`. Con `reemplazar` sustituye el destino (tras
/// quitarle el bit de solo lectura); sin él, falla si el destino existe.
bool renombrar(const std::wstring &origen, const std::wstring &destino, bool reemplazar,
    std::wstring *error);

bool existe(const std::wstring &ruta);

/// Quita el atributo de solo lectura de `ruta`, para poder sobrescribirla.
void quitarSoloLectura(const std::wstring &ruta);

/// Copia fechas (creación, acceso, modificación) y atributos (solo lectura,
/// oculto, sistema…) de `origen` a `destino`. Falso si no se pudo replicar
/// nada; los fallos parciales se toleran.
bool copiarMetadatos(const std::wstring &origen, const std::wstring &destino,
    std::wstring *error);

} // namespace win32
} // namespace maxcopier
