#include "instanciaunica.h"

#include "diagnostico.h"

#ifdef _WIN32
#include "ipc/tuberia.h"
#else
#include <QLocalServer>
#include <QLocalSocket>
#endif

namespace maxcopier {
namespace {

    QString desdeUtf16(const std::u16string &texto)
    {
        return QString::fromStdU16String(texto);
    }

    std::string mensajeDe(Operacion operacion, const QStringList &origenes,
        const QString &carpetaDestino, bool desdePortapapeles)
    {
        ipc::Peticion peticion;
        peticion.operacion = operacion;
        peticion.origenes.reserve(size_t(origenes.size()));
        for (const QString &origen : origenes)
            peticion.origenes.push_back(origen.toStdU16String());
        peticion.destino = carpetaDestino.toStdU16String();
        peticion.desdePortapapeles = desdePortapapeles;
        return ipc::serializar(peticion);
    }

#ifndef _WIN32
    constexpr int kMsEspera = 1000;
#endif

} // namespace

#ifdef _WIN32

InstanciaUnica::InstanciaUnica(QObject *parent)
    : QObject(parent)
    , m_nombre(QString::fromStdString(ipc::nombreDeCanal()))
{
    // El hilo del canal entrega los mensajes desde fuera de Qt: se saltan al
    // hilo de la interfaz con una conexión en cola.
    m_servidor = std::make_unique<ipc::Servidor>([this](const std::string &mensaje) {
        const QByteArray datos(mensaje.data(), qsizetype(mensaje.size()));
        QMetaObject::invokeMethod(
            this, [this, datos] { entregar(datos); }, Qt::QueuedConnection);
    });

    m_primera = m_servidor->escuchando();
    if (m_primera)
        anotar(QStringLiteral("canal abierto: \\\\.\\pipe\\%1").arg(m_nombre));
    else
        anotar(QStringLiteral("canal ya ocupado (error %1): esta instancia no escucha")
                   .arg(m_servidor->error()));
}

InstanciaUnica::~InstanciaUnica() = default;

bool InstanciaUnica::enviar(
    Operacion operacion, const QStringList &origenes, const QString &carpetaDestino,
    bool desdePortapapeles) const
{
    const std::string mensaje = mensajeDe(operacion, origenes, carpetaDestino, desdePortapapeles);
    unsigned long error = 0;
    const bool bien = ipc::escribirEnCanal(mensaje, error);
    if (!bien)
        anotar(QStringLiteral("no se ha podido pasar la petición a la otra instancia (error %1)")
                   .arg(error));
    return bien;
}

#else

InstanciaUnica::InstanciaUnica(QObject *parent)
    : QObject(parent)
    , m_nombre(QString::fromStdString(ipc::nombreDeCanal()))
{
    // Si nadie contesta en el socket, o bien no hay otra instancia o bien quedó
    // un socket huérfano de un cierre brusco: en los dos casos lo quitamos.
    QLocalSocket sonda;
    sonda.connectToServer(m_nombre);
    if (sonda.waitForConnected(kMsEspera)) {
        sonda.disconnectFromServer();
        return;
    }
    QLocalServer::removeServer(m_nombre);

    m_servidor = new QLocalServer(this);
    m_primera = m_servidor->listen(m_nombre);
    connect(m_servidor, &QLocalServer::newConnection, this, &InstanciaUnica::atenderConexion);
}

InstanciaUnica::~InstanciaUnica() = default;

bool InstanciaUnica::enviar(
    Operacion operacion, const QStringList &origenes, const QString &carpetaDestino,
    bool desdePortapapeles) const
{
    QLocalSocket socket;
    socket.connectToServer(m_nombre);
    if (!socket.waitForConnected(kMsEspera))
        return false;

    const std::string mensaje = mensajeDe(operacion, origenes, carpetaDestino, desdePortapapeles);
    socket.write(mensaje.data(), qint64(mensaje.size()));
    return socket.waitForBytesWritten(kMsEspera);
}

void InstanciaUnica::atenderConexion()
{
    QLocalSocket *socket = m_servidor->nextPendingConnection();
    if (!socket)
        return;
    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);

    // El mensaje puede llegar troceado, así que se acumula hasta tenerlo entero.
    auto *pendiente = new QByteArray;
    connect(socket, &QObject::destroyed, this, [pendiente] { delete pendiente; });
    connect(socket, &QLocalSocket::readyRead, this, [this, socket, pendiente] { leerDe(socket, pendiente); });
    leerDe(socket, pendiente);
}

void InstanciaUnica::leerDe(QLocalSocket *socket, QByteArray *pendiente)
{
    pendiente->append(socket->readAll());

    const long completo = ipc::tamanoDeMensaje(pendiente->constData(), size_t(pendiente->size()));
    if (completo < 0) { // no habla nuestro idioma
        socket->disconnectFromServer();
        return;
    }
    if (completo == 0 || pendiente->size() < completo)
        return;

    const QByteArray mensaje = *pendiente;
    pendiente->clear();
    socket->write(ipc::kConforme, sizeof(ipc::kConforme));
    socket->flush();
    socket->disconnectFromServer();
    entregar(mensaje);
}

#endif // _WIN32

void InstanciaUnica::entregar(const QByteArray &mensaje)
{
    ipc::Peticion peticion;
    if (!ipc::deserializar(mensaje.constData(), size_t(mensaje.size()), peticion)) {
        anotar(QStringLiteral("mensaje del canal ilegible (%1 bytes)").arg(mensaje.size()));
        return;
    }

    QStringList origenes;
    origenes.reserve(int(peticion.origenes.size()));
    for (const std::u16string &origen : peticion.origenes)
        origenes.append(desdeUtf16(origen));

    anotar(QStringLiteral("petición recibida: %1 origen(es), destino «%2»")
               .arg(origenes.size())
               .arg(desdeUtf16(peticion.destino)));

    emit peticionRecibida(
        peticion.operacion, origenes, desdeUtf16(peticion.destino), peticion.desdePortapapeles);
}

} // namespace maxcopier
