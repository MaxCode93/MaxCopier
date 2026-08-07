#pragma once

#include <QFrame>

class QLabel;

namespace maxcopier {

class BarraLibre;

/// Insignia de la unidad de destino: tipo (HD/SSD), etiqueta, barra de espacio
/// ocupado y espacio libre.
class UnidadDestino : public QFrame {
    Q_OBJECT

public:
    explicit UnidadDestino(QWidget *parent = nullptr);

    void establecerUnidad(const QString &tipo, const QString &nombre, const QString &libre, double ocupado);

private:
    QLabel *m_tipo = nullptr;
    QLabel *m_nombre = nullptr;
    QLabel *m_libre = nullptr;
    BarraLibre *m_barra = nullptr;
};

} // namespace maxcopier
