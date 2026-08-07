#include "copia/motordecopia.h"

#include "util/atributos.h"
#include "util/rutas.h"
#include "util/velocimetro.h"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#include <cmath>
#include <memory>
#include <vector>

#ifdef Q_OS_WIN
#include "copia/backendwin32.h"

namespace {
constexpr int kBloquesEnVuelo = 8; // E/S asíncronas simultáneas por archivo
}
#endif

namespace maxcopier {
namespace {

constexpr qint64 kMiB = 1024 * 1024;
constexpr int kMsEntreAvisos = 150; // cada cuánto se emite `progreso`
constexpr int kMsDormidoEnPausa = 50;
// Archivo lateral de toda copia: se escribe ahí y se renombra al terminar. Si
// la copia se corta, el `.mcpart` queda para reanudarla la próxima vez.
const QString kSufijoParcial = QStringLiteral(".mcpart");

enum class ModoApertura {
    Nuevo,    ///< el archivo no debe existir todavía
    Reanudar, ///< continuar un `.mcpart` desde su tamaño actual
};

/// Abstracción mínima de archivo para el motor: QFile en Linux/macOS y las
/// APIs Win32 (con rutas largas `\\?\`) en Windows. El bucle de copia, la
/// pausa, el límite y el reanudado de `.mcpart` son comunes a los dos
/// backends.
class ArchivoIO {
public:
    virtual ~ArchivoIO() = default;
    virtual bool abrirLectura(const QString &ruta, QString *error) = 0;
    virtual bool abrirEscritura(const QString &ruta, ModoApertura modo, QString *error) = 0;
    virtual qint64 leer(char *datos, qint64 max, QString *error) = 0;
    virtual bool escribir(const char *datos, qint64 n, QString *error) = 0;
    virtual bool vaciar(QString *error) = 0;
    virtual void cerrar() = 0;
    virtual qint64 tamano() const = 0;
};

class ArchivoQFile : public ArchivoIO {
public:
    bool abrirLectura(const QString &ruta, QString *error) override
    {
        m_archivo.setFileName(ruta);
        if (m_archivo.open(QIODevice::ReadOnly))
            return true;
        if (error != nullptr)
            *error = m_archivo.errorString();
        return false;
    }

    bool abrirEscritura(const QString &ruta, ModoApertura modo, QString *error) override
    {
        m_archivo.setFileName(ruta);
        // Append escribe siempre al final sin truncar: exactamente lo que hace
        // falta al reanudar un parcial (con WriteOnly solo, QFile trunca).
        QIODevice::OpenMode flags = QIODevice::WriteOnly;
        if (modo == ModoApertura::Nuevo)
            flags |= QIODevice::NewOnly;
        else
            flags |= QIODevice::Append;
        if (m_archivo.open(flags))
            return true;
        if (error != nullptr)
            *error = m_archivo.errorString();
        return false;
    }

    qint64 leer(char *datos, qint64 max, QString *error) override
    {
        const qint64 leidos = m_archivo.read(datos, max);
        if (leidos < 0 && error != nullptr)
            *error = m_archivo.errorString();
        return leidos;
    }

    bool escribir(const char *datos, qint64 n, QString *error) override
    {
        if (m_archivo.write(datos, n) == n)
            return true;
        if (error != nullptr)
            *error = m_archivo.errorString();
        return false;
    }

    bool vaciar(QString *error) override
    {
        if (m_archivo.flush())
            return true;
        if (error != nullptr)
            *error = m_archivo.errorString();
        return false;
    }

    void cerrar() override
    {
        if (m_archivo.isOpen())
            m_archivo.close();
    }

    qint64 tamano() const override { return m_archivo.size(); }

private:
    QFile m_archivo;
};

#ifdef Q_OS_WIN
class ArchivoWin32 : public ArchivoIO {
public:
    ~ArchivoWin32() override { cerrar(); }

    bool abrirLectura(const QString &ruta, QString *error) override
    {
        std::wstring detalle;
        if (win32::abrirLectura(ruta.toStdWString(), &m_manejo, &m_tamano, &detalle))
            return true;
        if (error != nullptr)
            *error = QString::fromStdWString(detalle);
        return false;
    }

    bool abrirEscritura(const QString &ruta, ModoApertura modo, QString *error) override
    {
        const win32::ModoEscritura modoWin = modo == ModoApertura::Nuevo
            ? win32::ModoEscritura::Nuevo
            : win32::ModoEscritura::Reanudar;
        std::wstring detalle;
        if (win32::abrirEscritura(ruta.toStdWString(), modoWin, &m_manejo, &detalle))
            return true;
        if (error != nullptr)
            *error = QString::fromStdWString(detalle);
        return false;
    }

    qint64 leer(char *datos, qint64 max, QString *error) override
    {
        std::wstring detalle;
        const std::int64_t leidos = win32::leer(m_manejo, datos, max, &detalle);
        if (leidos >= 0)
            return leidos;
        if (error != nullptr)
            *error = QString::fromStdWString(detalle);
        return -1;
    }

    bool escribir(const char *datos, qint64 n, QString *error) override
    {
        std::wstring detalle;
        if (win32::escribir(m_manejo, datos, n, &detalle))
            return true;
        if (error != nullptr)
            *error = QString::fromStdWString(detalle);
        return false;
    }

    bool vaciar(QString *error) override
    {
        std::wstring detalle;
        if (win32::vaciar(m_manejo, &detalle))
            return true;
        if (error != nullptr)
            *error = QString::fromStdWString(detalle);
        return false;
    }

    void cerrar() override
    {
        win32::cerrar(m_manejo);
        m_manejo = nullptr;
    }

    qint64 tamano() const override { return qint64(m_tamano); }

private:
    void *m_manejo = nullptr;
    std::uint64_t m_tamano = 0;
};
#endif

std::unique_ptr<ArchivoIO> crearArchivoIO()
{
#ifdef Q_OS_WIN
    return std::make_unique<ArchivoWin32>();
#else
    return std::make_unique<ArchivoQFile>();
#endif
}

bool existeArchivo(const QString &ruta)
{
#ifdef Q_OS_WIN
    return win32::existe(ruta.toStdWString());
#else
    return QFileInfo::exists(ruta);
#endif
}

bool tamanoArchivo(const QString &ruta, qint64 *tamano)
{
#ifdef Q_OS_WIN
    std::uint64_t tam = 0;
    std::wstring detalle;
    if (!win32::tamanoDe(ruta.toStdWString(), &tam, &detalle))
        return false;
    *tamano = qint64(tam);
    return true;
#else
    *tamano = QFileInfo(ruta).size();
    return true;
#endif
}

bool eliminarArchivo(const QString &ruta)
{
#ifdef Q_OS_WIN
    std::wstring detalle;
    return win32::eliminar(ruta.toStdWString(), &detalle);
#else
    if (QFile::remove(ruta))
        return true;
    return !QFileInfo::exists(ruta);
#endif
}

/// Deja el `.mcpart` en el sitio de `destino`, reemplazando lo que hubiera.
/// Al sobrescribir no se toca el destino hasta el final: si la copia se corta,
/// el archivo que ya estaba sigue intacto.
bool reemplazarCon(const QString &parcial, const QString &destino)
{
#ifdef Q_OS_WIN
    std::wstring detalle;
    return win32::renombrar(parcial.toStdWString(), destino.toStdWString(), true, &detalle);
#else
    if (QFile::exists(destino) && !QFile::remove(destino))
        return false;
    return QFile::rename(parcial, destino);
#endif
}

/// Renombra el `.mcpart` al destino sin sobrescribir (no debía existir).
bool renombrarFinal(const QString &parcial, const QString &destino)
{
#ifdef Q_OS_WIN
    std::wstring detalle;
    return win32::renombrar(parcial.toStdWString(), destino.toStdWString(), false, &detalle);
#else
    return QFile::rename(parcial, destino);
#endif
}

} // namespace

MotorDeCopia::MotorDeCopia(QObject *parent)
    : QObject(parent)
    , m_limitador(&m_limitadorPropio)
{
}

void MotorDeCopia::establecerLimiteVelocidad(qint64 bytesPorSegundo)
{
    if (m_limitador)
        m_limitador->establecerLimite(bytesPorSegundo);
}

void MotorDeCopia::establecerMetodo(MetodoDeCopia metodo)
{
    m_metodo.storeRelaxed(int(metodo));
}

void MotorDeCopia::establecerLimitadorCompartido(LimitadorVelocidad *limitador)
{
    m_limitador = limitador != nullptr ? limitador : &m_limitadorPropio;
}

qint64 MotorDeCopia::tamanoDeBloque(qint64 tamanoArchivo)
{
    if (tamanoArchivo < 8 * kMiB)
        return 64 * 1024;
    if (tamanoArchivo < 256 * kMiB)
        return kMiB;
    return 4 * kMiB;
}

void MotorDeCopia::reiniciar()
{
    m_pausa.storeRelaxed(0);
    m_saltar.storeRelaxed(0);
    m_cancelar.storeRelaxed(0);
}

void MotorDeCopia::establecerPausa(bool pausada)
{
    const int valor = pausada ? 1 : 0;
    if (m_pausa.fetchAndStoreRelaxed(valor) != valor)
        emit pausaCambiada(pausada);
}

void MotorDeCopia::alternarPausa()
{
    establecerPausa(!pausada());
}

void MotorDeCopia::saltar()
{
    m_saltar.storeRelaxed(1);
    m_pausa.storeRelaxed(0);
}

void MotorDeCopia::cancelar()
{
    m_cancelar.storeRelaxed(1);
    m_pausa.storeRelaxed(0);
}

void MotorDeCopia::copiar(const QString &origen, const QString &destino, bool sobrescribir, bool mover)
{
    // Defensa final para no truncar ni borrar un archivo si una petición llega
    // sin pasar por la UI (por ejemplo, desde una integración externa).
    if (rutasIguales(origen, destino)) {
        emit terminada(Resultado::Error,
            tr("El origen y el destino son el mismo archivo."));
        return;
    }

    // El destino final no se toca hasta el final: se copia a un `.mcpart`
    // lateral y se renombra al terminar. Así una copia cortada (cancelar,
    // error o caída) deja el archivo anterior intacto y el parcial queda para
    // reanudar.
    const QString parcial = destino + kSufijoParcial;

    if (!sobrescribir && existeArchivo(destino)) {
        emit terminada(Resultado::Error,
            tr("No se puede crear «%1»: ya existe.").arg(destino));
        return;
    }

    std::unique_ptr<ArchivoIO> origenIO = crearArchivoIO();
    QString motivo;
    if (!origenIO->abrirLectura(origen, &motivo)) {
        emit terminada(Resultado::Error,
            tr("No se puede leer «%1»: %2").arg(origen, motivo));
        return;
    }
    const qint64 total = origenIO->tamano();
    origenIO->cerrar();
    if (total < 0) {
        emit terminada(Resultado::Error,
            tr("No se puede obtener el tamaño de «%1».").arg(origen));
        return;
    }

    // Reanudación de parciales: si queda un `.mcpart` válido (mismo tamaño o
    // menor que el origen), se continúa desde donde se quedó. Un parcial más
    // grande que el origen ya no corresponde a este archivo: se descarta y se
    // empieza de cero. Sin verificación por hash (decisión cerrada del
    // proyecto), el tamaño es el criterio de reanudación.
    qint64 resumido = 0;
    if (existeArchivo(parcial)) {
        qint64 tamanoParcial = 0;
        if (!tamanoArchivo(parcial, &tamanoParcial) || tamanoParcial > total) {
            eliminarArchivo(parcial);
        } else {
            resumido = tamanoParcial;
        }
    }

    emit iniciada(origen, destino, total);

    Resultado resultado = Resultado::Terminada;
    QString error;
    qint64 copiado = resumido;

#ifdef Q_OS_WIN
    if (metodo() == MetodoDeCopia::Asincrono)
        copiarAsincrono(origen, parcial, total, resumido, &resultado, &error, &copiado);
    else
#endif
        copiarSincrono(origen, parcial, total, resumido, &resultado, &error, &copiado);

    // El tamaño inicial solo sirve para planificar la copia. El origen puede
    // haberse truncado, eliminado o haber crecido mientras se copiaba; nunca
    // se debe publicar como correcta una copia que ya no corresponde a la
    // instantánea con la que se hizo el preflight. Esta comprobación ocurre
    // antes de renombrar el parcial y antes de borrar el origen en modo mover.
    if (resultado == Resultado::Terminada) {
        qint64 tamanoFinal = -1;
        if (!tamanoArchivo(origen, &tamanoFinal) || tamanoFinal != total) {
            resultado = Resultado::Error;
            error = tr("El origen «%1» cambió de tamaño durante la copia; se conserva el parcial.")
                        .arg(origen);
        }
    }

    // Un archivo vacío no llega a crear el `.mcpart`; se deja un parcial vacío
    // para poder renombrarlo al destino igual que cualquier otro archivo.
    if (resultado == Resultado::Terminada && !existeArchivo(parcial)) {
        QFile vacio(parcial);
        if (!vacio.open(QIODevice::WriteOnly)) {
            resultado = Resultado::Error;
            error = tr("No se puede crear «%1»: %2").arg(destino, vacio.errorString());
        }
        vacio.close();
    }

    // Saltar un archivo lo descarta del todo: el `.mcpart` no debe quedar
    // residual para no reanudar algo que el usuario decidió saltar.
    if (resultado == Resultado::Saltada && existeArchivo(parcial))
        eliminarArchivo(parcial);

    if (resultado == Resultado::Terminada) {
        // Dejar el `.mcpart` en su sitio final. Al sobrescribir se reemplaza el
        // archivo que ya había; sin sobrescribir el destino no debía existir.
        const bool colocado = sobrescribir ? reemplazarCon(parcial, destino)
                                           : renombrarFinal(parcial, destino);
        if (!colocado) {
            resultado = Resultado::Error;
            error = tr("No se puede reemplazar «%1»").arg(destino);
        }
    }
    // Cualquier otro resultado conserva el `.mcpart`: se reanuda en la próxima
    // copia del mismo origen al mismo destino.

    if (resultado == Resultado::Terminada) {
        // Fechas y atributos del origen sobre el destino ya colocado. Un fallo
        // parcial no fracasa la copia del archivo.
        QString motivoMetadatos;
        copiarMetadatos(origen, destino, &motivoMetadatos);
    }

    // Explorer no debe borrar el origen: el DropHandler devuelve COPY para
    // evitar una carrera con esta copia. En modo mover, MaxCopier se encarga de
    // borrar cada archivo únicamente cuando su destino ya está completo.
    if (resultado == Resultado::Terminada && mover && !eliminarArchivo(origen)) {
        resultado = Resultado::Error;
        error = tr("No se puede borrar el origen «%1»").arg(origen);
    }

    emit terminada(resultado, error);
}

void MotorDeCopia::copiarSincrono(const QString &origen, const QString &parcial, qint64 total,
    qint64 resumido, Resultado *resultado, QString *error, qint64 *copiado)
{
    std::unique_ptr<ArchivoIO> origenIO = crearArchivoIO();
    std::unique_ptr<ArchivoIO> destinoIO = crearArchivoIO();

    QString motivo;
    if (!origenIO->abrirLectura(origen, &motivo)) {
        *resultado = Resultado::Error;
        *error = tr("No se puede leer «%1»: %2").arg(origen, motivo);
        return;
    }

    if (resumido < total) {
        const ModoApertura modo = resumido > 0 ? ModoApertura::Reanudar
                                               : ModoApertura::Nuevo;
        if (!destinoIO->abrirEscritura(parcial, modo, &motivo)) {
            *resultado = Resultado::Error;
            *error = tr("No se puede crear «%1»: %2").arg(parcial, motivo);
            return;
        }
    }

    QByteArray bloque;
    bloque.resize(int(tamanoDeBloque(total)));

    Velocimetro velocimetro;
    QElapsedTimer reloj;
    reloj.start();
    velocimetro.registrar(0, 0);

    qint64 copiadoLocal = resumido;
    qint64 msUltimoAviso = 0;

    while (copiadoLocal < total) {
        if (m_pausa.loadRelaxed() != 0) {
            while (m_pausa.loadRelaxed() != 0)
                QThread::msleep(kMsDormidoEnPausa);
            // La pausa no debe contar como copia lenta.
            velocimetro.reiniciar();
            velocimetro.registrar(copiadoLocal, reloj.elapsed());
        }
        if (m_cancelar.loadRelaxed() != 0) {
            *resultado = Resultado::Cancelada;
            break;
        }
        if (m_saltar.loadRelaxed() != 0) {
            *resultado = Resultado::Saltada;
            break;
        }

        QString motivoOperacion;
        const qint64 leidos = origenIO->leer(bloque.data(), bloque.size(), &motivoOperacion);
        if (leidos < 0) {
            *resultado = Resultado::Error;
            *error = tr("Error al leer «%1»: %2").arg(origen, motivoOperacion);
            break;
        }
        if (leidos == 0) {
            *resultado = Resultado::Error;
            *error = tr("El origen «%1» se truncó durante la copia.").arg(origen);
            break;
        }

        if (!destinoIO->escribir(bloque.constData(), leidos, &motivoOperacion)) {
            *resultado = Resultado::Error;
            *error = tr("Error al escribir «%1»: %2").arg(parcial, motivoOperacion);
            break;
        }
        copiadoLocal += leidos;

        // Límite de velocidad compartido: duerme en intervalos cortos para que
        // Pausar, Saltar y Cancelar sigan respondiendo.
        if (m_limitador)
            m_limitador->gastar(leidos, m_pausa, m_cancelar);

        const qint64 ms = reloj.elapsed();
        velocimetro.registrar(copiadoLocal, ms);
        if (ms - msUltimoAviso >= kMsEntreAvisos) {
            msUltimoAviso = ms;
            emit progreso(copiadoLocal, total, velocimetro.velocidad(),
                velocimetro.segundosRestantes(total - copiadoLocal));
        }
    }

    if (*resultado == Resultado::Terminada && copiadoLocal != total) {
        *resultado = Resultado::Error;
        *error = tr("El origen «%1» se truncó durante la copia.").arg(origen);
    }

    if (*resultado == Resultado::Terminada && resumido < total) {
        QString motivoVolcado;
        if (!destinoIO->vaciar(&motivoVolcado)) {
            *resultado = Resultado::Error;
            *error = tr("Error al volcar «%1»: %2").arg(parcial, motivoVolcado);
        }
    }
    destinoIO->cerrar();
    origenIO->cerrar();
    *copiado = copiadoLocal;

    if (*resultado == Resultado::Terminada)
        emit progreso(*copiado, total, velocimetro.media(), 0);
}

#ifdef Q_OS_WIN
void MotorDeCopia::copiarAsincrono(const QString &origen, const QString &parcial, qint64 total,
    qint64 resumido, Resultado *resultado, QString *error, qint64 *copiado)
{
    std::wstring detalle;
    void *hOrigen = nullptr;
    void *hDestino = nullptr;
    std::uint64_t tamanoAlAbrir = 0;

    if (!win32::abrirLecturaAsincrona(
            origen.toStdWString(), &hOrigen, &tamanoAlAbrir, &detalle)) {
        *resultado = Resultado::Error;
        *error = tr("No se puede leer «%1»: %2")
                     .arg(origen, QString::fromStdWString(detalle));
        return;
    }
    if (tamanoAlAbrir != static_cast<std::uint64_t>(total)) {
        win32::cerrar(hOrigen);
        *resultado = Resultado::Error;
        *error = tr("El origen «%1» cambió de tamaño antes de copiarse.").arg(origen);
        return;
    }

    if (resumido < total) {
        const win32::ModoEscritura modo = resumido > 0 ? win32::ModoEscritura::Reanudar
                                                       : win32::ModoEscritura::Nuevo;
        if (!win32::abrirEscrituraAsincrona(
                parcial.toStdWString(), modo, &hDestino, &detalle)) {
            win32::cerrar(hOrigen);
            *resultado = Resultado::Error;
            *error = tr("No se puede crear «%1»: %2")
                         .arg(parcial, QString::fromStdWString(detalle));
            return;
        }
    }

    const qint64 tamanoBloque = tamanoDeBloque(total);
    struct Ranura {
        enum Estado { Libre, Leyendo, Leido, Escribiendo };
        QByteArray datos;
        win32::IoAsincrono *io = nullptr;
        Estado estado = Libre;
        qint64 offset = 0;
        qint64 solicitado = 0;
        qint64 leidos = 0;
    };
    // Se redimensiona después de declarar: `std::vector<Ranura> ranuras(size_t(...))`
    // es un "most vexing parse" y algunos compiladores (MSVC, MinGW 8.1) lo
    // leen como una declaración de función.
    std::vector<Ranura> ranuras;
    ranuras.resize(size_t(kBloquesEnVuelo));
    for (Ranura &ranura : ranuras) {
        ranura.datos.resize(int(tamanoBloque));
        ranura.io = win32::crearIo(&detalle);
        if (ranura.io == nullptr) {
            for (Ranura &otra : ranuras)
                win32::liberarIo(otra.io);
            win32::cerrar(hOrigen);
            win32::cerrar(hDestino);
            *resultado = Resultado::Error;
            *error = tr("No se puede preparar la copia asíncrona: %1")
                         .arg(QString::fromStdWString(detalle));
            return;
        }
    }

    Velocimetro velocimetro;
    QElapsedTimer reloj;
    reloj.start();
    velocimetro.registrar(0, 0);

    qint64 msUltimoAviso = 0;

    qint64 posLectura = resumido;
    qint64 posEscritura = resumido;
    *resultado = Resultado::Terminada;

    const auto numeroPendientes = [&]() {
        int pendientes = 0;
        for (const Ranura &ranura : ranuras) {
            if (ranura.estado != Ranura::Libre)
                ++pendientes;
        }
        return pendientes;
    };

    const auto numeroEio = [&]() {
        int pendientes = 0;
        for (const Ranura &ranura : ranuras) {
            if (ranura.estado == Ranura::Leyendo || ranura.estado == Ranura::Escribiendo)
                ++pendientes;
        }
        return pendientes;
    };

    const auto esperar = [&]() {
        std::vector<void *> eventos;
        for (const Ranura &ranura : ranuras) {
            if (ranura.estado == Ranura::Leyendo || ranura.estado == Ranura::Escribiendo)
                eventos.push_back(win32::eventoDeIo(ranura.io));
        }
        if (!eventos.empty())
            win32::esperarEventos(eventos.data(), int(eventos.size()), 50, false);
        else
            QThread::msleep(1);
    };

    const auto avisarProgreso = [&]() {
        const qint64 ms = reloj.elapsed();
        if (ms - msUltimoAviso >= kMsEntreAvisos) {
            msUltimoAviso = ms;
            velocimetro.registrar(posEscritura, ms);
            emit progreso(posEscritura, total, velocimetro.velocidad(),
                velocimetro.segundosRestantes(total - posEscritura));
        }
    };

    const auto marcarOrigenInvalido = [&]() {
        *resultado = Resultado::Error;
        *error = tr("El origen «%1» se truncó o cambió durante la copia.").arg(origen);
    };

    const auto completarLectura = [&](Ranura &ranura) {
        while (true) {
            const std::int64_t leidos = win32::resultadoIo(ranura.io, &detalle);
            if (leidos < 0) {
                *resultado = Resultado::Error;
                *error = tr("Error al leer «%1»: %2")
                             .arg(origen, QString::fromStdWString(detalle));
                return false;
            }
            const qint64 restantes = ranura.solicitado - ranura.leidos;
            if (leidos <= 0 || leidos > restantes) {
                marcarOrigenInvalido();
                return false;
            }
            ranura.leidos += leidos;
            if (ranura.leidos == ranura.solicitado) {
                ranura.estado = Ranura::Leido;
                return true;
            }

            // Una lectura corta no se da por buena ni se pierde: se pide el
            // resto en el offset siguiente usando la misma ranura y buffer.
            const std::int64_t est = win32::lanzarLectura(ranura.io, hOrigen,
                ranura.datos.data() + ranura.leidos,
                ranura.solicitado - ranura.leidos,
                std::uint64_t(ranura.offset + ranura.leidos), &detalle);
            if (est < 0) {
                *resultado = Resultado::Error;
                *error = tr("Error al leer «%1»: %2")
                             .arg(origen, QString::fromStdWString(detalle));
                return false;
            }
            ranura.estado = Ranura::Leyendo;
            if (est != 0)
                return true;
            // E/S inmediata: el siguiente resultado se acumula en este mismo
            // bucle, hasta completar exactamente el bloque solicitado.
        }
    };

    const auto confirmarEscritura = [&](Ranura &ranura, std::int64_t escritos) {
        if (escritos < 0) {
            *resultado = Resultado::Error;
            *error = tr("Error al escribir «%1»: %2")
                         .arg(parcial, QString::fromStdWString(detalle));
            return false;
        }
        if (escritos != ranura.leidos) {
            *resultado = Resultado::Error;
            *error = tr("Escritura incompleta en «%1»: se esperaban %2 bytes y se escribieron %3.")
                         .arg(parcial).arg(ranura.leidos).arg(escritos);
            return false;
        }
        const qint64 confirmados = ranura.leidos;
        posEscritura += confirmados;
        ranura.leidos = 0;
        ranura.estado = Ranura::Libre;
        if (m_limitador)
            m_limitador->gastar(confirmados, m_pausa, m_cancelar);
        avisarProgreso();
        return true;
    };

    const auto procesar = [&]() {
        for (Ranura &ranura : ranuras) {
            if (ranura.estado == Ranura::Leyendo && win32::ioCompletado(ranura.io)
                && !completarLectura(ranura))
                return;
        }
        for (Ranura &ranura : ranuras) {
            if (ranura.estado == Ranura::Escribiendo && win32::ioCompletado(ranura.io)) {
                if (!confirmarEscritura(ranura, win32::resultadoIo(ranura.io, &detalle)))
                    return;
            }
        }
    };

    const auto lanzarLecturas = [&]() {
        for (Ranura &ranura : ranuras) {
            if (*resultado != Resultado::Terminada)
                return;
            if (ranura.estado != Ranura::Libre || posLectura >= total)
                continue;
            ranura.offset = posLectura;
            ranura.solicitado = qMin(tamanoBloque, total - posLectura);
            ranura.leidos = 0;
            const std::int64_t est = win32::lanzarLectura(ranura.io, hOrigen,
                ranura.datos.data(), ranura.solicitado, std::uint64_t(ranura.offset), &detalle);
            if (est < 0) {
                *resultado = Resultado::Error;
                *error = tr("Error al leer «%1»: %2")
                             .arg(origen, QString::fromStdWString(detalle));
                return;
            }
            // El cursor de lectura reserva el offset, pero nunca se usa como
            // cursor de escritura: cada ranura conserva su posición original.
            posLectura += ranura.solicitado;
            if (est == 0) {
                if (!completarLectura(ranura))
                    return;
            } else {
                ranura.estado = Ranura::Leyendo;
            }
        }
    };

    const auto lanzarEscrituras = [&]() {
        for (Ranura &ranura : ranuras) {
            if (*resultado != Resultado::Terminada)
                return;
            if (ranura.estado != Ranura::Leido || ranura.leidos <= 0 || hDestino == nullptr
                || ranura.offset != posEscritura)
                continue;
            const std::int64_t est = win32::lanzarEscritura(ranura.io, hDestino,
                ranura.datos.constData(), ranura.leidos, std::uint64_t(ranura.offset), &detalle);
            if (est < 0) {
                *resultado = Resultado::Error;
                *error = tr("Error al escribir «%1»: %2")
                             .arg(parcial, QString::fromStdWString(detalle));
                return;
            }
            if (est == 0) {
                if (!confirmarEscritura(ranura, win32::resultadoIo(ranura.io, &detalle)))
                    return;
            } else {
                ranura.estado = Ranura::Escribiendo;
            }
            // Solo se publica un commit a la vez y siempre para el siguiente
            // offset contiguo. Las lecturas pueden ir adelantadas, pero nunca
            // pueden hacer que el destino avance fuera de orden.
            return;
        }
    };

    while (*resultado == Resultado::Terminada) {
        // Pausa: no se lanzan E/S nuevas, solo se dejan terminar las que van.
        while (m_pausa.loadRelaxed() != 0 && *resultado == Resultado::Terminada) {
            esperar();
            procesar();
            if (*resultado != Resultado::Terminada)
                break;
            QThread::msleep(kMsDormidoEnPausa);
            if (m_cancelar.loadRelaxed() != 0) {
                *resultado = Resultado::Cancelada;
                break;
            }
        }
        if (*resultado != Resultado::Terminada)
            break;
        if (m_cancelar.loadRelaxed() != 0) {
            *resultado = Resultado::Cancelada;
            break;
        }
        if (m_saltar.loadRelaxed() != 0) {
            *resultado = Resultado::Saltada;
            break;
        }

        lanzarLecturas();
        lanzarEscrituras();
        if (*resultado != Resultado::Terminada)
            break;

        if (posLectura >= total && posEscritura >= total && numeroPendientes() == 0)
            break;

        esperar();
        procesar();
        if (*resultado != Resultado::Terminada)
            break;
        avisarProgreso();
    }

    // Dejar que terminen las E/S en vuelo antes de cerrar los manejos. Si la
    // cancelación ya está pedida (o un dispositivo desapareció y las E/S no
    // acaban nunca), no se espera indefinidamente: cerrar los manejos cancela
    // las pendientes y la salida del proceso no puede quedarse colgada.
    int drenaje = 0;
    while (numeroEio() > 0 && drenaje < 200) {
        ++drenaje;
        esperar();
        for (Ranura &ranura : ranuras) {
            if (ranura.estado == Ranura::Leyendo && win32::ioCompletado(ranura.io)) {
                win32::resultadoIo(ranura.io, nullptr);
                ranura.estado = Ranura::Libre;
            }
            if (ranura.estado == Ranura::Escribiendo && win32::ioCompletado(ranura.io)) {
                win32::resultadoIo(ranura.io, nullptr);
                ranura.estado = Ranura::Libre;
            }
        }
        if (m_cancelar.loadRelaxed() != 0)
            break;
    }

    if (*resultado == Resultado::Terminada && posEscritura != total) {
        *resultado = Resultado::Error;
        *error = tr("La copia asíncrona no alcanzó el tamaño esperado de «%1».").arg(parcial);
    }

    if (*resultado == Resultado::Terminada && resumido < total) {
        std::wstring detalleVolcado;
        if (!win32::vaciar(hDestino, &detalleVolcado)) {
            *resultado = Resultado::Error;
            *error = tr("Error al volcar «%1»: %2")
                         .arg(parcial, QString::fromStdWString(detalleVolcado));
        }
    }
    win32::cerrar(hDestino);
    win32::cerrar(hOrigen);
    for (Ranura &ranura : ranuras)
        win32::liberarIo(ranura.io);

    *copiado = posEscritura;
    if (*resultado == Resultado::Terminada)
        emit progreso(*copiado, total, velocimetro.media(), 0);
}
#endif

} // namespace maxcopier
