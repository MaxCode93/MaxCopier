#pragma once

#include "acciones.h"
#include "copia/metododecopia.h"
#include "politicas/acceso.h"
#include "politicas/colision.h"
#include "temas/temas.h"

#include <QObject>
#include <QString>

class QEvent;

namespace maxcopier {

/// Acción que se ejecuta al terminar correctamente una tanda completa.
/// Los valores se serializan con tokens ingleses en `config.mc`.
enum class AccionAlTerminar {
    Nada,
    Cerrar,
    Suspender,
    Apagar,
};

/// Preferencia persistente de apariencia. `Tema` sigue representando el tema
/// efectivo que se aplica a la interfaz.
enum class TemaPreferido {
    Oscuro,
    Claro,
    Sistema,
};

/// Ajustes globales de MaxCopier.
///
/// Se guardan en formato INI, en un fichero portable junto al ejecutable,
/// independientemente de la plataforma de configuración nativa de Qt:
/// `<directorio del ejecutable>/config.mc`.
class Configuracion : public QObject {
    Q_OBJECT

public:
    explicit Configuracion(QObject *parent = nullptr);
    explicit Configuracion(const QString &rutaArchivo, QObject *parent = nullptr);

    QString rutaArchivo() const { return m_rutaArchivo; }

    /// Límite por transferencia, en bytes por segundo. Cero significa sin límite.
    qint64 limiteVelocidad() const { return m_limiteVelocidad; }
    AccionAlTerminar accionAlTerminar() const { return m_accionAlTerminar; }
    AccionColision accionColision() const { return m_accionColision; }
    AccionError accionError() const { return m_accionError; }
    AccionListaActiva accionListaActiva() const;
    MetodoDeCopia metodoDeCopia() const { return m_metodoDeCopia; }
    int archivosALaVez() const { return m_archivosALaVez; }
    bool comprobarEspacioLibre() const { return m_comprobarEspacioLibre; }
    TemaPreferido temaPreferido() const { return m_temaPreferido; }

    /// Tema que debe aplicarse ahora, resolviendo `Sistema` si hace falta.
    Tema temaEfectivo() const;

    void establecerLimiteVelocidad(qint64 bytesPorSegundo);
    void establecerAccionAlTerminar(AccionAlTerminar accion);
    void establecerAccionColision(AccionColision accion);
    void establecerAccionError(AccionError accion);
    void establecerAccionListaActiva(AccionListaActiva accion);
    void establecerMetodoDeCopia(MetodoDeCopia metodo);
    void establecerArchivosALaVez(int archivos);
    void establecerComprobarEspacioLibre(bool comprobar);
    void establecerTemaPreferido(TemaPreferido tema);

    /// Alterna entre claro y oscuro y deja de seguir al sistema.
    void alternarTema();

    static QString token(AccionAlTerminar accion);
    static QString token(AccionColision accion);
    static QString token(AccionError accion);
    static QString token(AccionListaActiva accion);
    static QString token(MetodoDeCopia metodo);
    static QString token(TemaPreferido tema);

    static QString nombre(AccionAlTerminar accion);
    static QString nombre(MetodoDeCopia metodo);
    static QString nombre(TemaPreferido tema);

signals:
    /// Se emite después de guardar cualquier ajuste.
    void configuracionCambiada();

    /// Se emite cuando el tema efectivo puede haber cambiado.
    void temaCambiado();

    /// La ruta portable no se pudo escribir. No se usa una ruta alternativa.
    void errorAlGuardar(const QString &ruta);

protected:
    bool eventFilter(QObject *objeto, QEvent *evento) override;

private:
    void cargar();
    void guardar();
    void actualizarTemaSistema();
    void emitirCambio();

    static AccionAlTerminar accionAlTerminarDesdeToken(const QString &valor);
    static AccionColision accionColisionDesdeToken(const QString &valor);
    static AccionError accionErrorDesdeToken(const QString &valor);
    static AccionListaActiva accionListaActivaDesdeToken(const QString &valor);
    static MetodoDeCopia metodoDesdeToken(const QString &valor);
    static TemaPreferido temaDesdeToken(const QString &valor);
    static Tema detectarTemaSistema();

    QString m_rutaArchivo;
    qint64 m_limiteVelocidad = 0;
    AccionAlTerminar m_accionAlTerminar = AccionAlTerminar::Nada;
    AccionColision m_accionColision = AccionColision::Preguntar;
    AccionError m_accionError = AccionError::Preguntar;
    TemaPreferido m_temaPreferido = TemaPreferido::Oscuro;
    QString m_accionListaActivaToken = QStringLiteral("ask");
    MetodoDeCopia m_metodoDeCopia = MetodoDeCopia::Sincrono;
    int m_archivosALaVez = 1;
    bool m_comprobarEspacioLibre = true;
    Tema m_temaAplicado = Tema::Oscuro;
    bool m_avisoGuardadoMostrado = false;
};

} // namespace maxcopier
