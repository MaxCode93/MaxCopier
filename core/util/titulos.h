#pragma once

#include "ipc/protocolo.h"

#include <QString>

namespace maxcopier {

/// Título de la ventana según cuántas transferencias estén en curso:
/// con una, «Copia · origen → destino»; con varias, «Copia · N archivos en
/// curso». `origen` y `destino` solo se usan con una transferencia.
QString tituloDeTransferencia(ipc::Operacion operacion, int enCurso,
    const QString &origen = QString(), const QString &destino = QString());

} // namespace maxcopier
