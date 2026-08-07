#include "diagnostico.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

namespace maxcopier {
namespace {

    constexpr qint64 kTamanoMaximo = 256 * 1024;

    QMutex &cerrojo()
    {
        static QMutex cerrojo;
        return cerrojo;
    }

} // namespace

QString rutaDelRegistro()
{
    QString carpeta = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (carpeta.isEmpty())
        carpeta = QDir::tempPath();
    return QDir(carpeta).filePath(QStringLiteral("maxcopier.log"));
}

void anotar(const QString &texto)
{
    const QMutexLocker cierre(&cerrojo());

    const QString ruta = rutaDelRegistro();
    QDir().mkpath(QFileInfo(ruta).absolutePath());

    if (QFileInfo(ruta).size() > kTamanoMaximo)
        QFile::remove(ruta);

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream salida(&archivo);
    salida << QDateTime::currentDateTime().toString(Qt::ISODate) << ' ' << texto << '\n';
}

} // namespace maxcopier
