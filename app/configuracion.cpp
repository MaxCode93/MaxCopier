#include "configuracion.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QEvent>
#include <QPalette>
#include <QSettings>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif

namespace maxcopier {
namespace {

constexpr qint64 kMiB = 1024 * 1024;
constexpr qint64 kLimiteMaximoMiB = 10000;
constexpr int kArchivosALaVezMinimo = 1;
constexpr int kArchivosALaVezMaximo = 4;

QString normalizarToken(const QString &valor)
{
    return valor.trimmed().toLower();
}

qint64 leerLimite(const QVariant &valor)
{
    bool correcto = false;
    const qint64 mebibytes = valor.toLongLong(&correcto);
    if (!correcto || mebibytes < 0)
        return 0;
    return qMin(mebibytes, kLimiteMaximoMiB) * kMiB;
}

} // namespace

Configuracion::Configuracion(QObject *parent)
    : QObject(parent)
    , m_rutaArchivo(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config.mc")))
{
    cargar();
    m_temaAplicado = temaEfectivo();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this,
        [this](Qt::ColorScheme) { actualizarTemaSistema(); });
#else
    qApp->installEventFilter(this);
#endif
}

Configuracion::Configuracion(const QString &rutaArchivo, QObject *parent)
    : QObject(parent)
    , m_rutaArchivo(rutaArchivo)
{
    cargar();
    m_temaAplicado = temaEfectivo();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this,
        [this](Qt::ColorScheme) { actualizarTemaSistema(); });
#else
    qApp->installEventFilter(this);
#endif
}

bool Configuracion::eventFilter(QObject *objeto, QEvent *evento)
{
    if (objeto == qApp && evento && evento->type() == QEvent::ApplicationPaletteChange)
        actualizarTemaSistema();
    return QObject::eventFilter(objeto, evento);
}

void Configuracion::cargar()
{
    QSettings ajustes(m_rutaArchivo, QSettings::IniFormat);
    ajustes.setFallbacksEnabled(false);

    m_limiteVelocidad = leerLimite(ajustes.value(QStringLiteral("Transfer/speedLimitMiB"), 0));
    m_accionAlTerminar = accionAlTerminarDesdeToken(
        ajustes.value(QStringLiteral("Transfer/finishAction"), QStringLiteral("nothing")).toString());
    m_accionColision = accionColisionDesdeToken(
        ajustes.value(QStringLiteral("Transfer/collisionAction"), QStringLiteral("ask")).toString());
    m_accionError = accionErrorDesdeToken(
        ajustes.value(QStringLiteral("Transfer/errorAction"), QStringLiteral("ask")).toString());
    m_accionListaActivaToken = token(accionListaActivaDesdeToken(
        ajustes.value(QStringLiteral("Transfer/activeCopyAction"), QStringLiteral("ask")).toString()));
    m_metodoDeCopia = metodoDesdeToken(
        ajustes.value(QStringLiteral("Transfer/copyMethod"), QStringLiteral("sync")).toString());
    m_archivosALaVez = qBound(kArchivosALaVezMinimo,
        ajustes.value(QStringLiteral("Transfer/parallelFiles"), 1).toInt(),
        kArchivosALaVezMaximo);
    m_comprobarEspacioLibre = ajustes.value(
        QStringLiteral("Transfer/checkFreeSpace"), true).toBool();
    m_temaPreferido = temaDesdeToken(
        ajustes.value(QStringLiteral("Appearance/theme"), QStringLiteral("dark")).toString());
}

void Configuracion::guardar()
{
    QSettings ajustes(m_rutaArchivo, QSettings::IniFormat);
    ajustes.setFallbacksEnabled(false);
    ajustes.setAtomicSyncRequired(true);

    ajustes.setValue(QStringLiteral("version"), 1);
    ajustes.setValue(QStringLiteral("Transfer/speedLimitMiB"), m_limiteVelocidad / kMiB);
    ajustes.setValue(QStringLiteral("Transfer/finishAction"), token(m_accionAlTerminar));
    ajustes.setValue(QStringLiteral("Transfer/collisionAction"), token(m_accionColision));
    ajustes.setValue(QStringLiteral("Transfer/errorAction"), token(m_accionError));
    ajustes.setValue(QStringLiteral("Transfer/activeCopyAction"), m_accionListaActivaToken);
    ajustes.setValue(QStringLiteral("Transfer/copyMethod"), token(m_metodoDeCopia));
    ajustes.setValue(QStringLiteral("Transfer/parallelFiles"), m_archivosALaVez);
    ajustes.setValue(QStringLiteral("Transfer/checkFreeSpace"), m_comprobarEspacioLibre);
    ajustes.setValue(QStringLiteral("Appearance/theme"), token(m_temaPreferido));
    ajustes.sync();

    if (ajustes.status() != QSettings::NoError) {
        // La aplicación sigue funcionando con los valores en memoria. El
        // diagnóstico explica por qué no se pudo guardar junto al ejecutable.
        qWarning("No se pudo guardar la configuración en %s", qPrintable(m_rutaArchivo));
        if (!m_avisoGuardadoMostrado) {
            m_avisoGuardadoMostrado = true;
            emit errorAlGuardar(m_rutaArchivo);
        }
    } else {
        m_avisoGuardadoMostrado = false;
    }
}

void Configuracion::establecerLimiteVelocidad(qint64 bytesPorSegundo)
{
    const qint64 limite = qBound<qint64>(0, bytesPorSegundo, kLimiteMaximoMiB * kMiB);
    // Los valores persistidos son enteros de MiB/s; redondear aquí evita
    // guardar una fracción que la UI no puede representar.
    const qint64 normalizado = (limite / kMiB) * kMiB;
    if (m_limiteVelocidad == normalizado)
        return;
    m_limiteVelocidad = normalizado;
    emitirCambio();
}

void Configuracion::establecerAccionAlTerminar(AccionAlTerminar accion)
{
    if (m_accionAlTerminar == accion)
        return;
    m_accionAlTerminar = accion;
    emitirCambio();
}

void Configuracion::establecerAccionColision(AccionColision accion)
{
    if (m_accionColision == accion)
        return;
    m_accionColision = accion;
    emitirCambio();
}

void Configuracion::establecerAccionError(AccionError accion)
{
    if (m_accionError == accion)
        return;
    m_accionError = accion;
    emitirCambio();
}

void Configuracion::establecerAccionListaActiva(AccionListaActiva accion)
{
    const QString nuevo = token(accion);
    if (m_accionListaActivaToken == nuevo)
        return;
    m_accionListaActivaToken = nuevo;
    emitirCambio();
}

void Configuracion::establecerMetodoDeCopia(MetodoDeCopia metodo)
{
    if (m_metodoDeCopia == metodo)
        return;
    m_metodoDeCopia = metodo;
    emitirCambio();
}

void Configuracion::establecerArchivosALaVez(int archivos)
{
    const int normalizado = qBound(kArchivosALaVezMinimo, archivos, kArchivosALaVezMaximo);
    if (m_archivosALaVez == normalizado)
        return;
    m_archivosALaVez = normalizado;
    emitirCambio();
}

void Configuracion::establecerComprobarEspacioLibre(bool comprobar)
{
    if (m_comprobarEspacioLibre == comprobar)
        return;
    m_comprobarEspacioLibre = comprobar;
    emitirCambio();
}

void Configuracion::establecerTemaPreferido(TemaPreferido tema)
{
    const Tema antes = temaEfectivo();
    if (m_temaPreferido == tema)
        return;
    m_temaPreferido = tema;
    m_temaAplicado = temaEfectivo();
    guardar();
    emit configuracionCambiada();
    if (antes != m_temaAplicado)
        emit temaCambiado();
}

void Configuracion::alternarTema()
{
    establecerTemaPreferido(temaEfectivo() == Tema::Oscuro ? TemaPreferido::Claro : TemaPreferido::Oscuro);
}

AccionListaActiva Configuracion::accionListaActiva() const
{
    return accionListaActivaDesdeToken(m_accionListaActivaToken);
}

Tema Configuracion::temaEfectivo() const
{
    switch (m_temaPreferido) {
    case TemaPreferido::Oscuro:
        return Tema::Oscuro;
    case TemaPreferido::Claro:
        return Tema::Claro;
    case TemaPreferido::Sistema:
        return detectarTemaSistema();
    }
    return Tema::Oscuro;
}

void Configuracion::actualizarTemaSistema()
{
    if (m_temaPreferido != TemaPreferido::Sistema)
        return;
    const Tema actual = temaEfectivo();
    if (actual == m_temaAplicado)
        return;
    m_temaAplicado = actual;
    emit configuracionCambiada();
    emit temaCambiado();
}

void Configuracion::emitirCambio()
{
    guardar();
    emit configuracionCambiada();
}

Tema Configuracion::detectarTemaSistema()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    switch (qApp->styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
        return Tema::Oscuro;
    case Qt::ColorScheme::Light:
        return Tema::Claro;
    case Qt::ColorScheme::Unknown:
        break;
    }
#endif
    return qApp->palette().color(QPalette::Window).lightness() < 128 ? Tema::Oscuro : Tema::Claro;
}

QString Configuracion::token(AccionAlTerminar accion)
{
    switch (accion) {
    case AccionAlTerminar::Nada:
        return QStringLiteral("nothing");
    case AccionAlTerminar::Cerrar:
        return QStringLiteral("close");
    case AccionAlTerminar::Suspender:
        return QStringLiteral("suspend");
    case AccionAlTerminar::Apagar:
        return QStringLiteral("shutdown");
    }
    return QStringLiteral("nothing");
}

QString Configuracion::token(AccionColision accion)
{
    switch (accion) {
    case AccionColision::Preguntar:
        return QStringLiteral("ask");
    case AccionColision::Sobrescribir:
        return QStringLiteral("overwrite");
    case AccionColision::Renombrar:
        return QStringLiteral("rename");
    case AccionColision::Saltar:
        return QStringLiteral("skip");
    }
    return QStringLiteral("ask");
}

QString Configuracion::token(AccionError accion)
{
    switch (accion) {
    case AccionError::Preguntar:
        return QStringLiteral("ask");
    case AccionError::Reintentar:
        return QStringLiteral("retry");
    case AccionError::PonerAlFinal:
        return QStringLiteral("moveToEnd");
    case AccionError::Saltar:
        return QStringLiteral("skip");
    }
    return QStringLiteral("ask");
}

QString Configuracion::token(AccionListaActiva accion)
{
    switch (accion) {
    case AccionListaActiva::Preguntar:
        return QStringLiteral("ask");
    case AccionListaActiva::AnadirALaActual:
        return QStringLiteral("addToCurrent");
    case AccionListaActiva::VentanaNueva:
        return QStringLiteral("newWindow");
    case AccionListaActiva::Cancelar:
        return QStringLiteral("cancel");
    }
    return QStringLiteral("ask");
}

QString Configuracion::token(MetodoDeCopia metodo)
{
    switch (metodo) {
    case MetodoDeCopia::Sincrono:
        return QStringLiteral("sync");
    case MetodoDeCopia::Asincrono:
        return QStringLiteral("overlapped");
    }
    return QStringLiteral("sync");
}

QString Configuracion::token(TemaPreferido tema)
{
    switch (tema) {
    case TemaPreferido::Oscuro:
        return QStringLiteral("dark");
    case TemaPreferido::Claro:
        return QStringLiteral("light");
    case TemaPreferido::Sistema:
        return QStringLiteral("system");
    }
    return QStringLiteral("dark");
}

QString Configuracion::nombre(AccionAlTerminar accion)
{
    switch (accion) {
    case AccionAlTerminar::Nada:
        return QObject::tr("No hacer nada");
    case AccionAlTerminar::Cerrar:
        return QObject::tr("Cerrar la copia");
    case AccionAlTerminar::Suspender:
        return QObject::tr("Suspender PC");
    case AccionAlTerminar::Apagar:
        return QObject::tr("Apagar PC");
    }
    return QObject::tr("No hacer nada");
}

QString Configuracion::nombre(MetodoDeCopia metodo)
{
    switch (metodo) {
    case MetodoDeCopia::Sincrono:
        return QObject::tr("Compatible (síncrono)");
    case MetodoDeCopia::Asincrono:
        return QObject::tr("Rápido (asíncrono)");
    }
    return QObject::tr("Compatible (síncrono)");
}

QString Configuracion::nombre(TemaPreferido tema)
{
    switch (tema) {
    case TemaPreferido::Oscuro:
        return QObject::tr("Oscuro");
    case TemaPreferido::Claro:
        return QObject::tr("Claro");
    case TemaPreferido::Sistema:
        return QObject::tr("Sistema");
    }
    return QObject::tr("Oscuro");
}

AccionAlTerminar Configuracion::accionAlTerminarDesdeToken(const QString &valor)
{
    const QString token = normalizarToken(valor);
    if (token == QStringLiteral("close"))
        return AccionAlTerminar::Cerrar;
    if (token == QStringLiteral("suspend"))
        return AccionAlTerminar::Suspender;
    if (token == QStringLiteral("shutdown"))
        return AccionAlTerminar::Apagar;
    return AccionAlTerminar::Nada;
}

AccionColision Configuracion::accionColisionDesdeToken(const QString &valor)
{
    const QString token = normalizarToken(valor);
    if (token == QStringLiteral("overwrite"))
        return AccionColision::Sobrescribir;
    if (token == QStringLiteral("rename"))
        return AccionColision::Renombrar;
    if (token == QStringLiteral("skip"))
        return AccionColision::Saltar;
    return AccionColision::Preguntar;
}

AccionError Configuracion::accionErrorDesdeToken(const QString &valor)
{
    const QString token = normalizarToken(valor);
    if (token == QStringLiteral("retry"))
        return AccionError::Reintentar;
    if (token == QStringLiteral("movetoend"))
        return AccionError::PonerAlFinal;
    if (token == QStringLiteral("skip"))
        return AccionError::Saltar;
    return AccionError::Preguntar;
}

AccionListaActiva Configuracion::accionListaActivaDesdeToken(const QString &valor)
{
    const QString token = normalizarToken(valor);
    if (token == QStringLiteral("addtocurrent"))
        return AccionListaActiva::AnadirALaActual;
    if (token == QStringLiteral("newwindow"))
        return AccionListaActiva::VentanaNueva;
    if (token == QStringLiteral("cancel"))
        return AccionListaActiva::Cancelar;
    return AccionListaActiva::Preguntar;
}

MetodoDeCopia Configuracion::metodoDesdeToken(const QString &valor)
{
    const QString token = normalizarToken(valor);
    if (token == QStringLiteral("overlapped"))
        return MetodoDeCopia::Asincrono;
    return MetodoDeCopia::Sincrono;
}

TemaPreferido Configuracion::temaDesdeToken(const QString &valor)
{
    const QString token = normalizarToken(valor);
    if (token == QStringLiteral("light"))
        return TemaPreferido::Claro;
    if (token == QStringLiteral("system"))
        return TemaPreferido::Sistema;
    return TemaPreferido::Oscuro;
}

} // namespace maxcopier
