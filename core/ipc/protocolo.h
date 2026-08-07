#pragma once

/// Protocolo del canal local de MaxCopier (instancia única + extensión de
/// shell). Este archivo lo comparten la app (Qt) y la DLL del Explorador, que
/// es Win32 puro: aquí no puede entrar nada de Qt.
///
/// Mensaje: «MXC1» · tamaño del resto (u32) · operación (u32) · número de
/// orígenes (u32) · [longitud (u32) + texto UTF-16LE] por cada origen · destino
/// (longitud + texto, vacío = «pregúntale al usuario») · desde portapapeles
/// (u32, opcional para mantener compatibilidad con mensajes antiguos). Los
/// enteros van en little endian, que es lo único que hay en las máquinas donde
/// corre esto.
///
/// La app contesta «MXOK» cuando ha entendido la petición: así el Explorador
/// sabe si de verdad ha hablado con MaxCopier o tiene que tirar del respaldo.

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace maxcopier::ipc {

inline constexpr char kMarca[4] = { 'M', 'X', 'C', '1' };
inline constexpr char kConforme[4] = { 'M', 'X', 'O', 'K' };
inline constexpr uint32_t kTamanoMaximo = 64u * 1024u * 1024u;

enum class Operacion : uint32_t {
    Copiar = 0,
    Mover = 1, ///< copia y elimina el origen solo cuando el destino termina bien
};

struct Peticion {
    Operacion operacion = Operacion::Copiar;
    std::vector<std::u16string> origenes;
    std::u16string destino; ///< vacío: la app pregunta la carpeta de destino
    bool desdePortapapeles = false; ///< cortar+pegar: limpiar la marca al terminar
};

namespace detalle {

    /// FNV-1a de 32 bits: distingue dos usuarios cuyo nombre se sanea igual.
    inline uint32_t huella(const void *datos, size_t bytes)
    {
        const auto *octetos = static_cast<const unsigned char *>(datos);
        uint32_t valor = 2166136261u;
        for (size_t i = 0; i < bytes; ++i) {
            valor ^= uint32_t(octetos[i]);
            valor *= 16777619u;
        }
        return valor;
    }

    inline std::string hexadecimal(uint32_t valor)
    {
        static const char digitos[] = "0123456789abcdef";
        std::string texto(8, '0');
        for (int i = 7; i >= 0; --i) {
            texto[size_t(i)] = digitos[valor & 0xf];
            valor >>= 4;
        }
        return texto;
    }

    /// Solo ASCII simple: el nombre de la tubería tiene que salir byte a byte
    /// igual en la app (Qt, UTF-8) y en la DLL (Win32, UTF-16).
    inline char caracterSeguro(uint32_t caracter)
    {
        const bool corriente = (caracter >= 'a' && caracter <= 'z')
            || (caracter >= 'A' && caracter <= 'Z') || (caracter >= '0' && caracter <= '9')
            || caracter == '.' || caracter == '-' || caracter == '_';
        return corriente ? char(caracter) : '_';
    }

} // namespace detalle

/// Nombre del canal, uno por usuario: dos sesiones de Windows no se pisan.
/// Es siempre ASCII (`maxcopier-<usuario saneado>-<huella>`) porque el nombre
/// lo compone la app desde `std::string` y la DLL desde `std::wstring`: con
/// acentos o cirílico en el nombre de usuario, cualquier conversión suelta
/// entre los dos daba tuberías distintas y el Explorador no encontraba a nadie.
inline std::string nombreDeCanal()
{
    std::string saneado;
    uint32_t huella = 0;
#ifdef _WIN32
    wchar_t bufer[256] = { 0 };
    DWORD tamano = DWORD(sizeof(bufer) / sizeof(bufer[0]));
    std::wstring usuario;
    if (GetUserNameW(bufer, &tamano) && tamano > 1)
        usuario.assign(bufer, tamano - 1); // `tamano` incluye el cero final
    if (usuario.empty())
        usuario = L"usuario";
    huella = detalle::huella(usuario.data(), usuario.size() * sizeof(wchar_t));
    for (wchar_t caracter : usuario)
        saneado.push_back(detalle::caracterSeguro(uint32_t(caracter)));
#else
    std::string usuario;
    if (const char *nombre = std::getenv("USER"))
        usuario = nombre;
    if (usuario.empty())
        usuario = "usuario";
    huella = detalle::huella(usuario.data(), usuario.size());
    for (char caracter : usuario)
        saneado.push_back(detalle::caracterSeguro(uint32_t(static_cast<unsigned char>(caracter))));
#endif
    if (saneado.size() > 32)
        saneado.resize(32);
    return "maxcopier-" + saneado + "-" + detalle::hexadecimal(huella);
}

#ifdef _WIN32
/// Ruta completa de la tubería con nombre, tal cual la abren la app y la DLL.
inline std::wstring rutaDeCanal()
{
    const std::string nombre = nombreDeCanal(); // ASCII: ensanchar es exacto
    return L"\\\\.\\pipe\\" + std::wstring(nombre.begin(), nombre.end());
}
#endif

namespace detalle {

    inline void escribirU32(std::string &destino, uint32_t valor)
    {
        const char bytes[4] = { char(valor & 0xff), char((valor >> 8) & 0xff), char((valor >> 16) & 0xff),
            char((valor >> 24) & 0xff) };
        destino.append(bytes, 4);
    }

    inline bool leerU32(const char *datos, size_t tamano, size_t &posicion, uint32_t &valor)
    {
        if (posicion + 4 > tamano)
            return false;
        const auto *bytes = reinterpret_cast<const unsigned char *>(datos + posicion);
        valor = uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8) | (uint32_t(bytes[2]) << 16)
            | (uint32_t(bytes[3]) << 24);
        posicion += 4;
        return true;
    }

    inline void escribirTexto(std::string &destino, const std::u16string &texto)
    {
        escribirU32(destino, uint32_t(texto.size()));
        destino.append(reinterpret_cast<const char *>(texto.data()), texto.size() * 2);
    }

    inline bool leerTexto(const char *datos, size_t tamano, size_t &posicion, std::u16string &texto)
    {
        uint32_t caracteres = 0;
        if (!leerU32(datos, tamano, posicion, caracteres))
            return false;
        if (caracteres > kTamanoMaximo / 2 || posicion + size_t(caracteres) * 2 > tamano)
            return false;
        texto.assign(reinterpret_cast<const char16_t *>(datos + posicion), caracteres);
        posicion += size_t(caracteres) * 2;
        return true;
    }

} // namespace detalle

inline std::string serializar(const Peticion &peticion)
{
    std::string cuerpo;
    detalle::escribirU32(cuerpo, uint32_t(peticion.operacion));
    detalle::escribirU32(cuerpo, uint32_t(peticion.origenes.size()));
    for (const std::u16string &origen : peticion.origenes)
        detalle::escribirTexto(cuerpo, origen);
    detalle::escribirTexto(cuerpo, peticion.destino);
    // Va al final para que una app nueva siga entendiendo peticiones de una
    // extensión antigua y una extensión antigua pueda ignorar este dato.
    detalle::escribirU32(cuerpo, peticion.desdePortapapeles ? 1u : 0u);

    std::string mensaje(kMarca, sizeof(kMarca));
    detalle::escribirU32(mensaje, uint32_t(cuerpo.size()));
    mensaje += cuerpo;
    return mensaje;
}

/// Longitud total del mensaje que empieza en `datos`, o 0 si todavía no han
/// llegado bytes suficientes para saberlo. Devuelve -1 si no es un mensaje
/// nuestro o si dice ser absurdamente grande.
inline long tamanoDeMensaje(const char *datos, size_t tamano)
{
    if (tamano < sizeof(kMarca))
        return 0;
    if (std::memcmp(datos, kMarca, sizeof(kMarca)) != 0)
        return -1;
    size_t posicion = sizeof(kMarca);
    uint32_t cuerpo = 0;
    if (!detalle::leerU32(datos, tamano, posicion, cuerpo))
        return 0;
    if (cuerpo > kTamanoMaximo)
        return -1;
    return long(posicion + cuerpo);
}

inline bool deserializar(const char *datos, size_t tamano, Peticion &peticion)
{
    const long completo = tamanoDeMensaje(datos, tamano);
    if (completo <= 0 || size_t(completo) > tamano)
        return false;

    size_t posicion = sizeof(kMarca) + 4;
    uint32_t operacion = 0;
    uint32_t cuantos = 0;
    if (!detalle::leerU32(datos, tamano, posicion, operacion)
        || !detalle::leerU32(datos, tamano, posicion, cuantos))
        return false;
    if (operacion > uint32_t(Operacion::Mover) || cuantos > kTamanoMaximo / 4)
        return false;

    peticion.operacion = Operacion(operacion);
    peticion.origenes.clear();
    peticion.origenes.reserve(cuantos);
    for (uint32_t i = 0; i < cuantos; ++i) {
        std::u16string origen;
        if (!detalle::leerTexto(datos, tamano, posicion, origen))
            return false;
        peticion.origenes.push_back(std::move(origen));
    }
    if (!detalle::leerTexto(datos, tamano, posicion, peticion.destino))
        return false;

    // El campo se añadió al final del mensaje. Si no está, es una petición
    // antigua y equivale a una transferencia que no viene del portapapeles.
    peticion.desdePortapapeles = false;
    if (posicion < size_t(completo)) {
        uint32_t desdePortapapeles = 0;
        if (!detalle::leerU32(datos, size_t(completo), posicion, desdePortapapeles)
            || desdePortapapeles > 1)
            return false;
        peticion.desdePortapapeles = desdePortapapeles != 0;
    }
    return posicion == size_t(completo);
}

} // namespace maxcopier::ipc
