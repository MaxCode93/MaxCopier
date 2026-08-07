#include "util/atributos.h"

#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include "copia/backendwin32.h"
#endif

namespace maxcopier {

bool copiarMetadatos(const QString &origen, const QString &destino, QString *error)
{
#ifdef Q_OS_WIN
    std::wstring detalle;
    if (win32::copiarMetadatos(origen.toStdWString(), destino.toStdWString(), &detalle))
        return true;
    if (error != nullptr)
        *error = QString::fromStdWString(detalle);
    return false;
#else
    const QFileInfo info(origen);

    // Abrir en lectura basta para tener el archivo y poder tocar sus fechas;
    // no se modifica el contenido.
    QFile archivo(destino);
    if (!archivo.open(QIODevice::ReadOnly))
        return false;

    bool bien = true;
    const QDateTime modificado = info.lastModified();
    if (modificado.isValid()
        && !archivo.setFileTime(modificado, QFileDevice::FileModificationTime))
        bien = false;

    const QDateTime acceso = info.lastRead();
    if (acceso.isValid() && !archivo.setFileTime(acceso, QFileDevice::FileAccessTime))
        bien = false;
    archivo.close();

    // Conservar los permisos del origen (en particular el de solo lectura).
    const QFile::Permissions permisos = info.permissions();
    if (!QFile::setPermissions(destino, permisos))
        bien = false;

    if (!bien && error != nullptr)
        *error = QStringLiteral("No se pudieron copiar todas las fechas y atributos.");
    return bien;
#endif
}

} // namespace maxcopier
