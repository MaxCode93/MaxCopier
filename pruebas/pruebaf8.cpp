/// Pruebas de F8 que no necesitan ventana ni Explorer: reanudación de `.mcpart`
/// (parcial cortado a media copia, parcial ya completo y parcial obsoleto),
/// fechas y atributos del origen, y copia hacia rutas profundas (el motor no
/// usa búferes de tamaño fijo para rutas).

#include "copia/motordecopia.h"
#include "copia/limitadorvelocidad.h"
#include "lista/listadecopia.h"
#include "util/espaciolibre.h"
#include "util/titulos.h"

#include <QDate>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTime>
#include <QTimeZone>

#include <cstdio>
#include <thread>

using namespace maxcopier;

namespace {

int fallos = 0;

void comprobar(bool bien, const char *que)
{
    std::printf("%s %s\n", bien ? "[ok]  " : "[FALLA]", que);
    if (!bien)
        ++fallos;
}

bool escribir(const QString &ruta, const QByteArray &contenido)
{
    QFile archivo(ruta);
    return archivo.open(QIODevice::WriteOnly) && archivo.write(contenido) == contenido.size();
}

QByteArray leer(const QString &ruta)
{
    QFile archivo(ruta);
    return archivo.open(QIODevice::ReadOnly) ? archivo.readAll() : QByteArray();
}

QByteArray contenidoPatron(qint64 bytes)
{
    QByteArray contenido;
    contenido.resize(int(bytes));
    for (int i = 0; i < contenido.size(); ++i)
        contenido[i] = char((i * 31 + 7) & 0xFF);
    return contenido;
}

/// Copia `origen` → `destino` con un tope de velocidad y la cancela en cuanto
/// el motor confirma que ya ha escrito (primera señal de progreso). Se ejecuta
/// en un hilo suelto porque `copiar()` bloquea hasta terminar y la cancelación
/// necesita que el hilo principal siga atendiendo señales.
MotorDeCopia::Resultado copiarYCancelar(MotorDeCopia &motor, const QString &origen,
    const QString &destino)
{
    MotorDeCopia::Resultado resultado = MotorDeCopia::Resultado::Error;
    bool cancelado = false;
    QEventLoop bucle;

    QObject::connect(&motor, &MotorDeCopia::terminada, &bucle,
        [&](MotorDeCopia::Resultado r, const QString &) {
            resultado = r;
            bucle.quit();
        });
    QObject::connect(&motor, &MotorDeCopia::progreso, &bucle,
        [&](qint64, qint64, double, qint64) {
            // En este punto el motor ya ha escrito al menos un bloque: la
            // cancelación deja un `.mcpart` con contenido, no un archivo vacío.
            if (!cancelado) {
                cancelado = true;
                motor.cancelar();
            }
        });

    motor.establecerLimiteVelocidad(2LL * 1024 * 1024);
    motor.reiniciar();

    std::thread hilo([&] { motor.copiar(origen, destino, false, false); });
    bucle.exec();
    hilo.join();

    QObject::disconnect(&motor, nullptr, nullptr, nullptr);
    motor.establecerLimiteVelocidad(0);
    return resultado;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication aplicacion(argc, argv);
    Q_UNUSED(aplicacion);

    QTemporaryDir temporal;
    comprobar(temporal.isValid(), "se crea el directorio temporal");
    if (!temporal.isValid())
        return 1;

    const QString base = temporal.path();

    // --- Reanudación: cancelar a media copia deja un `.mcpart` recuperable.
    const QString origenGrande = QDir(base).filePath(QStringLiteral("grande.bin"));
    const QString destinoGrande = QDir(base).filePath(QStringLiteral("salida/grande.bin"));
    const QByteArray grande = contenidoPatron(16 * 1024 * 1024);
    comprobar(escribir(origenGrande, grande), "se prepara el archivo grande");
    comprobar(QDir().mkpath(QFileInfo(destinoGrande).absolutePath()),
        "se crea la carpeta de destino");

    MotorDeCopia motor;
    const MotorDeCopia::Resultado cancelado
        = copiarYCancelar(motor, origenGrande, destinoGrande);
    comprobar(cancelado == MotorDeCopia::Resultado::Cancelada,
        "cancelar a media copia devuelve Cancelada");

    const QString parcialGrande = destinoGrande + QStringLiteral(".mcpart");
    const qint64 tamanoParcial = QFileInfo(parcialGrande).size();
    comprobar(QFileInfo::exists(parcialGrande) && tamanoParcial > 0
            && tamanoParcial < grande.size(),
        "el `.mcpart` queda a medias y es reanudable");
    comprobar(!QFileInfo::exists(destinoGrande),
        "el destino final no aparece mientras la copia está cortada");

    MotorDeCopia::Resultado resultado = MotorDeCopia::Resultado::Error;
    QObject::connect(&motor, &MotorDeCopia::terminada,
        [&resultado](MotorDeCopia::Resultado r, const QString &) { resultado = r; });
    motor.reiniciar();
    motor.copiar(origenGrande, destinoGrande, false, false);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada,
        "reanudar el parcial termina la copia");
    comprobar(leer(destinoGrande) == grande,
        "el destino reanudado tiene el contenido completo");
    comprobar(!QFileInfo::exists(parcialGrande),
        "el `.mcpart` desaparece al terminar");

    // --- Reanudación: un parcial ya completo solo se deja en su sitio.
    const QString destinoCompleto = QDir(base).filePath(QStringLiteral("completo.bin"));
    comprobar(escribir(destinoCompleto + QStringLiteral(".mcpart"), grande),
        "se prepara un `.mcpart` completo");
    resultado = MotorDeCopia::Resultado::Error;
    motor.reiniciar();
    motor.copiar(origenGrande, destinoCompleto, false, false);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada
            && leer(destinoCompleto) == grande,
        "un parcial completo solo se renombra al destino");
    comprobar(!QFileInfo::exists(destinoCompleto + QStringLiteral(".mcpart")),
        "el parcial completo no se duplica");

    // --- Reanudación: un parcial más grande que el origen se descarta.
    const QString origenCorto = QDir(base).filePath(QStringLiteral("corto.bin"));
    const QString destinoCorto = QDir(base).filePath(QStringLiteral("corto-salida.bin"));
    const QByteArray corto = contenidoPatron(64 * 1024);
    comprobar(escribir(origenCorto, corto), "se prepara el archivo corto");
    comprobar(escribir(destinoCorto + QStringLiteral(".mcpart"), QByteArray(256 * 1024, 'x')),
        "se prepara un `.mcpart` obsoleto más grande que el origen");
    resultado = MotorDeCopia::Resultado::Error;
    motor.reiniciar();
    motor.copiar(origenCorto, destinoCorto, false, false);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada
            && leer(destinoCorto) == corto,
        "un parcial obsoleto se descarta y la copia empieza de cero");
    comprobar(!QFileInfo::exists(destinoCorto + QStringLiteral(".mcpart")),
        "el parcial obsoleto no queda al terminar");

    // --- Metadatos: fecha de modificación y solo lectura.
    const QString origenMeta = QDir(base).filePath(QStringLiteral("metadatos.txt"));
    const QString destinoMeta = QDir(base).filePath(QStringLiteral("metadatos-salida.txt"));
    comprobar(escribir(origenMeta, "contenido"), "se prepara el archivo de metadatos");
    QFile meta(origenMeta);
    const QDateTime fecha(QDate(2021, 3, 4), QTime(5, 6, 7), QTimeZone::utc());
    const bool abierto = meta.open(QIODevice::ReadOnly);
    const bool fechaFijada = abierto && meta.setFileTime(fecha, QFileDevice::FileModificationTime);
    meta.close();
    comprobar(fechaFijada,
        "se fija la fecha de modificación del origen");
    comprobar(QFile::setPermissions(origenMeta,
                  QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther),
        "se deja el origen de solo lectura");
    resultado = MotorDeCopia::Resultado::Error;
    motor.reiniciar();
    motor.copiar(origenMeta, destinoMeta, false, false);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada,
        "la copia de un archivo de solo lectura termina");
    const QFileInfo destinoInfo(destinoMeta);
    comprobar(destinoInfo.lastModified() == fecha,
        "el destino conserva la fecha de modificación del origen");
    comprobar(!destinoInfo.isWritable(),
        "el destino conserva el atributo de solo lectura");

    // --- Rutas largas: el motor no usa búferes de tamaño fijo para rutas.
    QString profundo = base;
    const QString componente = QString(38, QChar(u'c'));
    for (int nivel = 0; nivel < 8; ++nivel)
        profundo += QChar(u'/') + componente + QString::number(nivel);
    comprobar(QDir().mkpath(profundo), "se crea una estructura de rutas profundas");
    const QString destinoProfundo = QDir(profundo).filePath(QStringLiteral("profundo.bin"));
    comprobar(destinoProfundo.size() > 260,
        "la ruta profunda supera la longitud clásica de Windows");
    resultado = MotorDeCopia::Resultado::Error;
    motor.reiniciar();
    motor.copiar(origenCorto, destinoProfundo, false, false);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada
            && leer(destinoProfundo) == corto,
        "se copia hacia una ruta profunda sin truncar la ruta");

    // --- Método asíncrono: fuera de Windows cae al síncrono y copia igual.
    const QString origenMetodo = QDir(base).filePath(QStringLiteral("metodo.bin"));
    const QString destinoMetodo = QDir(base).filePath(QStringLiteral("metodo-salida.bin"));
    comprobar(escribir(origenMetodo, "contenido"), "se prepara el archivo del método");
    motor.establecerMetodo(MetodoDeCopia::Asincrono);
    comprobar(motor.metodo() == MetodoDeCopia::Asincrono,
        "el motor recuerda el método pedido");
    resultado = MotorDeCopia::Resultado::Error;
    motor.reiniciar();
    motor.copiar(origenMetodo, destinoMetodo, false, false);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada
            && leer(destinoMetodo) == "contenido",
        "el método asíncrono cae al síncrono fuera de Windows y copia igual");

    // --- Archivo vacío: se deja un parcial vacío y se renombra al destino.
    const QString origenVacio = QDir(base).filePath(QStringLiteral("vacio.txt"));
    const QString destinoVacio = QDir(base).filePath(QStringLiteral("vacio-salida.txt"));
    comprobar(escribir(origenVacio, QByteArray()), "se prepara un archivo vacío");
    resultado = MotorDeCopia::Resultado::Error;
    motor.reiniciar();
    motor.copiar(origenVacio, destinoVacio, false, false);
    comprobar(resultado == MotorDeCopia::Resultado::Terminada
            && QFileInfo(destinoVacio).exists() && leer(destinoVacio).isEmpty(),
        "un archivo vacío se copia y queda en su sitio");

    // --- Ordenación de la cola por cabeceras (Fuente/Tamaño/Destino).
    {
        ListaDeCopia lista;
        lista.anadir({
            { QDir(base).filePath(QStringLiteral("z.txt")),
                QDir(base).filePath(QStringLiteral("salida/z.txt")), 10 },
            { QDir(base).filePath(QStringLiteral("a.txt")),
                QDir(base).filePath(QStringLiteral("salida/a.txt")), 30 },
            { QDir(base).filePath(QStringLiteral("m.txt")),
                QDir(base).filePath(QStringLiteral("salida/m.txt")), 20 },
        });
        lista.ordenarPor(ListaDeCopia::ColumnaFuente, Qt::AscendingOrder);
        comprobar(lista.elemento(0).fuente.endsWith(QStringLiteral("/a.txt"))
                && lista.elemento(2).fuente.endsWith(QStringLiteral("/z.txt")),
            "ordenar por Fuente reorganiza la cola");
        lista.ordenarPor(ListaDeCopia::ColumnaTamano, Qt::DescendingOrder);
        comprobar(lista.elemento(0).tamano == 30 && lista.elemento(2).tamano == 10,
            "ordenar por Tamaño reorganiza la cola");
        lista.ordenarPor(ListaDeCopia::ColumnaDestino, Qt::AscendingOrder);
        comprobar(lista.elemento(0).destino.endsWith(QStringLiteral("/a.txt")),
            "ordenar por Destino reorganiza la cola");

        // La fila en curso queda anclada al principio al reorganizar.
        lista.marcarEnCurso(0, true);
        lista.ordenarPor(ListaDeCopia::ColumnaTamano, Qt::AscendingOrder);
        comprobar(lista.filasEnCurso().contains(0)
                && lista.elemento(0).fuente.endsWith(QStringLiteral("/a.txt")),
            "la fila en curso ancla el principio al ordenar");

        // Varias filas en curso: quedan ancladas todas juntas al principio.
        lista.anadir({
            { QDir(base).filePath(QStringLiteral("b.txt")),
                QDir(base).filePath(QStringLiteral("salida/b.txt")), 5 },
            { QDir(base).filePath(QStringLiteral("c.txt")),
                QDir(base).filePath(QStringLiteral("salida/c.txt")), 40 },
        });
        lista.marcarEnCurso(1, true);
        comprobar(lista.filasEnCurso().size() == 2,
            "se marcan dos filas en curso a la vez");
        lista.ordenarPor(ListaDeCopia::ColumnaTamano, Qt::AscendingOrder);
        comprobar(lista.esEnCurso(0) && lista.esEnCurso(1)
                && lista.elemento(0).fuente.endsWith(QStringLiteral("/a.txt"))
                && lista.elemento(1).fuente.endsWith(QStringLiteral("/z.txt")),
            "las dos filas en curso quedan ancladas al principio al ordenar");
        lista.quitarTerminada(0);
        comprobar(lista.filasEnCurso().size() == 1
                && lista.esEnCurso(0),
            "quitar la fila terminada mantiene la otra ancla en su sitio");
    }

    // --- Límite de velocidad compartido entre varios motores.
    {
        LimitadorVelocidad limitador;
        QAtomicInt pausa { 0 };
        QAtomicInt cancelar { 0 };
        limitador.establecerLimite(0);
        QElapsedTimer reloj;
        reloj.start();
        limitador.gastar(16LL * 1024 * 1024, pausa, cancelar);
        comprobar(reloj.elapsed() < 500,
            "sin límite, gastar no espera");

        // 128 KiB/s compartido: gastar 256 KiB desde dos «motores» cuesta
        // unos 2 segundos en total (el presupuesto es común).
        limitador.establecerLimite(128LL * 1024);
        reloj.restart();
        limitador.gastar(128LL * 1024, pausa, cancelar);
        limitador.gastar(128LL * 1024, pausa, cancelar);
        const qint64 tardado = reloj.elapsed();
        comprobar(tardado >= 1800 && tardado < 6000,
            "el límite compartido reparte el presupuesto entre los motores");
    }

    // --- Espacio libre: cálculo de faltas por volumen.
    {
        ElementosDeCopia pendientes;
        pendientes.append({ QDir(base).filePath(QStringLiteral("a.bin")),
            QDir(base).filePath(QStringLiteral("salida/a.bin")), 60 });
        pendientes.append({ QDir(base).filePath(QStringLiteral("b.bin")),
            QDir(base).filePath(QStringLiteral("salida/b.bin")), 70 });
        comprobar(!volumenDe(pendientes.first().destino).isEmpty(),
            "se identifica el volumen de un destino");

        const auto disponible100 = [](const QString &) { return qint64(100); };
        const QList<FaltaDeEspacio> faltas = faltasDeEspacio(pendientes, disponible100);
        comprobar(faltas.size() == 1 && faltas.first().necesitado == 130
                && faltas.first().falta() == 30,
            "se calcula la falta de espacio del volumen");

        const auto disponible200 = [](const QString &) { return qint64(200); };
        comprobar(faltasDeEspacio(pendientes, disponible200).isEmpty(),
            "si cabe todo, no hay faltas de espacio");
    }

    // --- Identidad de volumen: reconocer el mismo volumen (para reconectar
    // un dispositivo que vuelve con otra letra de unidad).
    {
        const QString identidad = identidadDeVolumen(base);
        comprobar(!identidad.isEmpty(), "se obtiene la identidad del volumen");
        const QString raiz = raizConIdentidad(identidad);
        comprobar(!raiz.isEmpty() && volumenDe(base) == raiz,
            "se encuentra el volumen montado por su identidad");
    }

    // --- Título de la ventana según las copias en curso (se actualiza al
    // saltar o terminar un archivo).
    comprobar(tituloDeTransferencia(ipc::Operacion::Copiar, 1,
                QStringLiteral("C:/a"), QStringLiteral("D:/b"))
            == QStringLiteral("Copia · C:/a → D:/b"),
        "el título con una copia muestra origen y destino");
    comprobar(tituloDeTransferencia(ipc::Operacion::Mover, 3)
            == QStringLiteral("Movimiento · 3 archivos en curso"),
        "el título con varias copias muestra el número en curso");
    comprobar(tituloDeTransferencia(ipc::Operacion::Copiar, 0)
            == QStringLiteral("Copia"),
        "el título sin copias no inventa contenido");

    std::printf(fallos == 0 ? "\nTodo bien.\n" : "\n%d comprobaciones falladas.\n", fallos);
    return fallos == 0 ? 0 : 1;
}
