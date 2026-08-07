#pragma once

#include <QList>
#include <QMetaType>
#include <QString>

namespace maxcopier {

/// Una fila de la lista de copia: un archivo con su ruta de llegada completa.
/// La lista es plana (las carpetas se resuelven al escanear, no se guardan).
struct ElementoDeCopia {
    QString fuente;  ///< ruta absoluta del archivo de origen
    QString destino; ///< ruta absoluta del archivo de llegada
    qint64 tamano = 0;
};

using ElementosDeCopia = QList<ElementoDeCopia>;

} // namespace maxcopier

// Los lotes del escáner viajan por señales en cola entre hilos.
Q_DECLARE_METATYPE(maxcopier::ElementoDeCopia)
Q_DECLARE_METATYPE(maxcopier::ElementosDeCopia)
