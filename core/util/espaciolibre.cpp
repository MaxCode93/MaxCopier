#include "util/espaciolibre.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QStorageInfo>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace maxcopier {
namespace {

qint64 disponibleReal(const QString &volumen)
{
    const QStorageInfo unidad(volumen);
    return unidad.isValid() ? unidad.bytesAvailable() : 0;
}

/// Volúmenes montados en caché: enumerarlos por archivo sería caro con listas
/// de miles de elementos. Si un destino no casa con ninguno, se refresca una
/// vez (pudo conectarse un volumen nuevo).
QList<QStorageInfo> volumenesMontados(bool refrescar = false)
{
    static QMutex mutex;
    static QList<QStorageInfo> cache;
    static bool cargado = false;
    QMutexLocker bloqueo(&mutex);
    if (!cargado || refrescar) {
        cache = QStorageInfo::mountedVolumes();
        cargado = true;
    }
    return cache;
}

QString comparable(const QString &ruta)
{
    QString resultado = QDir::cleanPath(ruta);
#ifdef Q_OS_WIN
    resultado = resultado.toCaseFolded();
#endif
    return resultado;
}

QString volumenDeConLista(const QString &ruta, const QList<QStorageInfo> &volumenes)
{
    if (ruta.isEmpty())
        return {};

    const QString absoluta = comparable(QFileInfo(ruta).absoluteFilePath());
    QString mejor;
    for (const QStorageInfo &volumen : volumenes) {
        const QString raiz = comparable(volumen.rootPath());
        if (raiz.isEmpty())
            continue;
        if ((absoluta == raiz || absoluta.startsWith(raiz + QLatin1Char('/')))
            && raiz.size() > mejor.size())
            mejor = raiz;
    }
    return mejor;
}

QString volumenDeConListaOFallback(const QString &ruta, const QList<QStorageInfo> &volumenes,
    bool refrescar)
{
    QString raiz = volumenDeConLista(ruta, volumenes);
    if (!raiz.isEmpty())
        return raiz;

    // Un volumen puede haberse conectado después de tomar la instantánea.
    if (refrescar) {
        raiz = volumenDeConLista(ruta, volumenesMontados(true));
        if (!raiz.isEmpty())
            return raiz;
    }

    // Respaldo: subir hasta un directorio existente y preguntar ahí.
    QString candidato = QFileInfo(ruta).absolutePath();
    while (!QFileInfo(candidato).exists()) {
        const QString padre = QFileInfo(candidato).absolutePath();
        if (padre == candidato)
            break;
        candidato = padre;
    }
    return comparable(QStorageInfo(candidato).rootPath());
}

} // namespace

QString volumenDe(const QString &ruta)
{
    if (ruta.isEmpty())
        return {};

    // El destino puede no existir todavía (es lo normal): QStorageInfo sobre
    // una ruta inexistente no dice a qué volumen pertenece. Se busca el
    // volumen montado que es prefijo de la ruta, eligiendo el prefijo más
    // largo (en Linux los volúmenes se anidan: «/», «/home», «/mnt/…»).
    return volumenDeConListaOFallback(ruta, volumenesMontados(), true);
}

QString identidadDeVolumen(const QString &ruta)
{
    const QString raiz = volumenDe(ruta);
    if (raiz.isEmpty())
        return {};

#ifdef Q_OS_WIN
    QString nativa = QDir::toNativeSeparators(raiz);
    wchar_t nombre[MAX_PATH + 1] = {};
    DWORD serial = 0;
    DWORD longitud = 0;
    DWORD atributos = 0;
    if (GetVolumeInformationW(reinterpret_cast<const wchar_t *>(nativa.utf16()),
            nombre, MAX_PATH, &serial, &longitud, &atributos, nullptr, 0)) {
        return QString::number(serial) + QLatin1Char('-') + QString::fromWCharArray(nombre);
    }
#else
    const QStorageInfo unidad(raiz);
    const QString dispositivo = unidad.device();
    if (!dispositivo.isEmpty())
        return dispositivo + QLatin1Char('-') + unidad.name();
#endif

    // Sin identidad estable: comportarse por letra de unidad.
    return raiz;
}

QString raizConIdentidad(const QString &identidad)
{
    if (identidad.isEmpty())
        return {};
    const auto buscar = [&identidad](const QList<QStorageInfo> &lista) {
        for (const QStorageInfo &volumen : lista) {
            const QString raiz = comparable(volumen.rootPath());
            if (raiz.isEmpty())
                continue;
            if (identidadDeVolumen(raiz) == identidad)
                return raiz;
        }
        return QString();
    };

    QString raiz = buscar(volumenesMontados());
    if (raiz.isEmpty())
        raiz = buscar(volumenesMontados(true));
    return raiz;
}

QList<FaltaDeEspacio> faltasDeEspacio(const ElementosDeCopia &pendientes)
{
    return faltasDeEspacio(pendientes, disponibleReal);
}

QList<FaltaDeEspacio> faltasDeEspacio(const ElementosDeCopia &pendientes,
    const std::function<qint64(const QString &volumen)> &disponibleDe)
{
    QHash<QString, qint64> necesario;
    QList<QStorageInfo> montados = volumenesMontados();
    bool volumenesRefrescados = false;
    for (const ElementoDeCopia &elemento : pendientes) {
        QString volumen = volumenDeConListaOFallback(elemento.destino, montados, false);
        if (volumen.isEmpty() && !volumenesRefrescados) {
            montados = volumenesMontados(true);
            volumenesRefrescados = true;
            volumen = volumenDeConListaOFallback(elemento.destino, montados, false);
        }
        if (!volumen.isEmpty())
            necesario[volumen] += elemento.tamano;
    }

    QList<FaltaDeEspacio> faltas;
    const QStringList volumenesNecesarios = necesario.keys();
    for (const QString &volumen : volumenesNecesarios) {
        FaltaDeEspacio falta;
        falta.volumen = volumen;
        falta.necesitado = necesario.value(volumen);
        falta.disponible = disponibleDe ? disponibleDe(volumen) : 0;
        if (falta.falta() > 0)
            faltas.append(falta);
    }

    // Orden estable por nombre para que el diálogo no cambie de orden.
    std::sort(faltas.begin(), faltas.end(),
        [](const FaltaDeEspacio &izquierda, const FaltaDeEspacio &derecha) {
            return izquierda.volumen < derecha.volumen;
        });
    return faltas;
}

} // namespace maxcopier
