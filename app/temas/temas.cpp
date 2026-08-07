#include "temas/temas.h"

#include <QApplication>
#include <QFile>

namespace maxcopier {
namespace {

    Tema g_tema = Tema::Oscuro;

    const Paleta &paletaOscura()
    {
        static const Paleta p {
            QColor(0x1e, 0x9b, 0xf0), // acento
            QColor(0x0d, 0x78, 0xc4), // acento2
            QColor(30, 155, 240, 46), // acentoSuave
            QColor(0x19, 0x1c, 0x21), // fondo
            QColor(0x20, 0x24, 0x2a), // fondo2
            QColor(0x26, 0x2b, 0x32), // fondo3
            QColor(0x31, 0x36, 0x3e), // linea
            QColor(0xe9, 0xec, 0xf1), // texto
            QColor(0x98, 0xa1, 0xad), // atenuado
            QColor(0x11, 0x13, 0x18), // barra
            QColor(0x1e, 0x9b, 0xf0), // relleno1
            QColor(0x0d, 0x78, 0xc4), // relleno2
        };
        return p;
    }

    const Paleta &paletaClara()
    {
        static const Paleta p {
            QColor(0x12, 0x89, 0xe0), // acento
            QColor(0x0a, 0x6c, 0xb5), // acento2
            QColor(18, 137, 224, 36), // acentoSuave
            QColor(0xfb, 0xfc, 0xfe), // fondo
            QColor(0xf1, 0xf4, 0xf8), // fondo2
            QColor(0xe7, 0xec, 0xf3), // fondo3
            QColor(0xcc, 0xd5, 0xe0), // linea
            QColor(0x16, 0x1a, 0x1f), // texto
            QColor(0x5b, 0x64, 0x72), // atenuado
            QColor(0xe2, 0xe8, 0xf0), // barra
            QColor(0xa5, 0xd9, 0xfb), // relleno1
            QColor(0x71, 0xc3, 0xf3), // relleno2
        };
        return p;
    }

} // namespace

void aplicarTema(Tema tema)
{
    g_tema = tema;

    const QString ruta
        = tema == Tema::Oscuro ? QStringLiteral(":/temas/oscuro.qss") : QStringLiteral(":/temas/claro.qss");
    QFile hoja(ruta);
    if (!hoja.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    qApp->setStyleSheet(QString::fromUtf8(hoja.readAll()));
}

Tema temaActual()
{
    return g_tema;
}

const Paleta &paleta(Tema tema)
{
    return tema == Tema::Oscuro ? paletaOscura() : paletaClara();
}

const Paleta &paletaActual()
{
    return paleta(g_tema);
}

Tema temaContrario(Tema tema)
{
    return tema == Tema::Oscuro ? Tema::Claro : Tema::Oscuro;
}

QString nombreTema(Tema tema)
{
    return tema == Tema::Oscuro ? QObject::tr("oscuro") : QObject::tr("claro");
}

} // namespace maxcopier
