#pragma once

#include "ipc/protocolo.h"

#include <QObject>
#include <QStringList>

#ifdef _WIN32
#include "ipc/servidor.h"

#include <memory>
#else
class QLocalServer;
class QLocalSocket;
#endif

namespace maxcopier {

using ipc::Operacion;

/// Instancia única del proceso: el primer MaxCopier escucha en un canal local
/// y los siguientes le mandan su petición de copia y se cierran, así abrir la
/// app otra vez no duplica el proceso ni la bandeja. Con bandeja no hay una UI
/// principal que traer al frente; las copias en paralelo se hacen con varias
/// ventanas del mismo proceso (ver `GestorDeVentanas`).
///
/// Por ese mismo canal habla la extensión del Explorador (`shell/`), que es
/// Win32 puro: el formato de los mensajes está en `core/ipc/protocolo.h`. En
/// Windows la tubería no la abre `QLocalServer` sino `core/ipc/{tuberia,
/// servidor}.h`, para que el Explorador pueda escribir en ella pase lo que
/// pase (ver allí el porqué).
class InstanciaUnica : public QObject {
    Q_OBJECT

public:
    explicit InstanciaUnica(QObject *parent = nullptr);
    ~InstanciaUnica() override;

    /// `false` si ya había otra instancia: quien construye esta clase debe
    /// llamar entonces a `enviar()` y salir sin abrir ninguna ventana.
    bool esPrimera() const { return m_primera; }

    /// Manda la petición a la instancia que ya está corriendo. Con `origenes`
    /// vacío solo le pide que mantenga activa la instancia (si no hay bandeja,
    /// el gestor muestra su ventana de fallback).
    bool enviar(Operacion operacion, const QStringList &origenes, const QString &carpetaDestino,
        bool desdePortapapeles = false) const;

signals:
    /// Petición recibida de otra instancia o del Explorador. `origenes` vacío
    /// significa «arranque sin una transferencia»; `carpetaDestino` vacío,
    /// «pregúntale al usuario dónde».
    void peticionRecibida(
        maxcopier::Operacion operacion, const QStringList &origenes, const QString &carpetaDestino,
        bool desdePortapapeles);

private:
    /// Convierte un mensaje completo en la señal de arriba. Se llama siempre
    /// desde el hilo de la interfaz.
    void entregar(const QByteArray &mensaje);

    QString m_nombre;
    bool m_primera = false;
#ifdef _WIN32
    std::unique_ptr<ipc::Servidor> m_servidor;
#else
    void atenderConexion();
    void leerDe(QLocalSocket *socket, QByteArray *pendiente);

    QLocalServer *m_servidor = nullptr;
#endif
};

} // namespace maxcopier
