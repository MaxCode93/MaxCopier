#pragma once

#include "lista/elementodecopia.h"

#include <QList>
#include <QString>

#include <functional>

namespace maxcopier {

/// Un volumen de destino donde no cabe todo lo pendiente.
struct FaltaDeEspacio {
    QString volumen;   ///< raíz del volumen (p. ej. «E:/»)
    qint64 necesitado = 0;
    qint64 disponible = 0;
    qint64 falta() const { return qMax<qint64>(0, necesitado - disponible); }
};

/// Raíz del volumen al que pertenece una ruta de destino.
QString volumenDe(const QString &ruta);

/// Identidad estable de un volumen (número de serie en Windows; dispositivo +
/// etiqueta en el resto). Sirve para reconocer un dispositivo que se reconecta
/// con otra letra de unidad. Si no se puede obtener, devuelve la raíz (equivale
/// a comportarse por letra).
QString identidadDeVolumen(const QString &ruta);

/// Raíz del volumen montado cuya identidad coincide con `identidad` (vacía si
/// no está conectado).
QString raizConIdentidad(const QString &identidad);

/// Volúmenes donde no cabe el total de `pendientes`, usando `QStorageInfo`.
QList<FaltaDeEspacio> faltasDeEspacio(const ElementosDeCopia &pendientes);

/// Igual, pero con el espacio disponible inyectado (para poder probarlo sin
/// tocar discos reales). `disponibleDe` recibe el volumen y devuelve los bytes
/// disponibles.
QList<FaltaDeEspacio> faltasDeEspacio(const ElementosDeCopia &pendientes,
    const std::function<qint64(const QString &volumen)> &disponibleDe);

} // namespace maxcopier

Q_DECLARE_METATYPE(maxcopier::FaltaDeEspacio)
Q_DECLARE_METATYPE(QList<maxcopier::FaltaDeEspacio>)
