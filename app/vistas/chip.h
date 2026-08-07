#pragma once

#include <QLabel>

class QMouseEvent;

namespace maxcopier {

/// Etiqueta redondeada de la fila de metadatos: `Colisión: **preguntar**`.
/// Cuando está destacada usa el color de acento (por ejemplo, límite activo).
class Chip : public QLabel {
    Q_OBJECT

public:
    Chip(const QString &etiqueta, const QString &valor, QWidget *parent = nullptr);

    void establecerValor(const QString &valor);
    void establecerDestacado(bool destacado);

    /// Vuelve a componer el texto con los colores del tema actual.
    void refrescar();

signals:
    void clicado();

protected:
    void mousePressEvent(QMouseEvent *evento) override;

private:
    QString m_etiqueta;
    QString m_valor;
    bool m_destacado = false;
};

} // namespace maxcopier
