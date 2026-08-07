#include "lista/listadecopia.h"

#include "util/formatos.h"

#include <QDir>

#include <algorithm>
#include <utility>

namespace maxcopier {

ListaDeCopia::ListaDeCopia(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ListaDeCopia::rowCount(const QModelIndex &padre) const
{
    return padre.isValid() ? 0 : int(m_elementos.size());
}

int ListaDeCopia::columnCount(const QModelIndex &padre) const
{
    return padre.isValid() ? 0 : NumeroDeColumnas;
}

QVariant ListaDeCopia::data(const QModelIndex &indice, int rol) const
{
    if (!indice.isValid() || indice.row() >= m_elementos.size())
        return {};

    const ElementoDeCopia &elemento = m_elementos.at(indice.row());

    if (rol == Qt::DisplayRole) {
        switch (indice.column()) {
        case ColumnaMarca:
            return m_enCurso.contains(indice.row()) ? QStringLiteral("\u25B6") : QString();
        case ColumnaFuente:
            return QDir::toNativeSeparators(elemento.fuente);
        case ColumnaTamano:
            return formatearTamano(elemento.tamano);
        case ColumnaDestino:
            return QDir::toNativeSeparators(elemento.destino);
        default:
            return {};
        }
    }

    if (rol == Qt::ToolTipRole)
        return QDir::toNativeSeparators(indice.column() == ColumnaDestino ? elemento.destino : elemento.fuente);

    if (rol == Qt::TextAlignmentRole && indice.column() == ColumnaTamano)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    if (rol == Qt::TextAlignmentRole && indice.column() == ColumnaMarca)
        return int(Qt::AlignCenter);

    return {};
}

QVariant ListaDeCopia::headerData(int seccion, Qt::Orientation orientacion, int rol) const
{
    if (orientacion != Qt::Horizontal || rol != Qt::DisplayRole)
        return {};

    switch (seccion) {
    case ColumnaFuente:
        return tr("Fuente");
    case ColumnaTamano:
        return tr("Tamaño");
    case ColumnaDestino:
        return tr("Destino");
    default:
        return QString();
    }
}

void ListaDeCopia::anadir(const ElementosDeCopia &elementos)
{
    if (elementos.isEmpty())
        return;

    beginInsertRows(QModelIndex(), int(m_elementos.size()), int(m_elementos.size() + elementos.size()) - 1);
    m_elementos.append(elementos);
    for (const ElementoDeCopia &elemento : elementos)
        m_bytes += elemento.tamano;
    endInsertRows();
    emit cambiada();
}

void ListaDeCopia::quitar(QList<int> filas)
{
    std::sort(filas.begin(), filas.end());
    filas.erase(std::unique(filas.begin(), filas.end()), filas.end());

    for (int i = int(filas.size()) - 1; i >= 0; --i) {
        const int fila = filas.at(i);
        if (fila < 0 || fila >= m_elementos.size() || m_enCurso.contains(fila))
            continue;
        beginRemoveRows(QModelIndex(), fila, fila);
        m_elementos.removeAt(fila);
        for (int &enCurso : m_enCurso) {
            if (enCurso > fila)
                --enCurso;
        }
        endRemoveRows();
    }
    recalcularBytes();
    emit cambiada();
}

void ListaDeCopia::quitarTerminada(int fila)
{
    if (fila < 0 || fila >= m_elementos.size())
        return;

    beginRemoveRows(QModelIndex(), fila, fila);
    m_elementos.removeAt(fila);
    m_enCurso.removeAll(fila);
    for (int &enCurso : m_enCurso) {
        if (enCurso > fila)
            --enCurso;
    }
    endRemoveRows();
    recalcularBytes();
    emit cambiada();
}

void ListaDeCopia::remapearDestinos(const QString &raizVieja, const QString &raizNueva)
{
    if (raizVieja.isEmpty() || raizVieja.size() < 2 || raizVieja == raizNueva)
        return;

    bool cambio = false;
    for (ElementoDeCopia &elemento : m_elementos) {
        QString destino = elemento.destino;
        if (destino == raizVieja) {
            destino = raizNueva;
        } else if (destino.startsWith(raizVieja + QLatin1Char('/'))) {
            destino = raizNueva + destino.mid(raizVieja.size());
        } else {
            continue;
        }
        if (destino != elemento.destino) {
            elemento.destino = destino;
            cambio = true;
        }
    }
    if (!cambio)
        return;

    beginResetModel();
    endResetModel();
    emit cambiada();
}

void ListaDeCopia::vaciar()
{
    // Las filas en curso se quedan: sus archivos se siguen copiando (o se
    // reanudan al volver a arrancar la tanda).
    ElementosDeCopia enCurso;
    for (int fila : m_enCurso)
        enCurso.append(m_elementos.at(fila));

    beginResetModel();
    m_elementos.clear();
    m_elementos = enCurso;
    m_enCurso.clear();
    for (int i = 0; i < enCurso.size(); ++i)
        m_enCurso.append(i);
    endResetModel();
    recalcularBytes();
    emit cambiada();
}

void ListaDeCopia::marcarEnCurso(int fila, bool activa)
{
    const bool estaba = m_enCurso.contains(fila);
    if (fila < 0 || fila >= m_elementos.size() || estaba == activa)
        return;

    if (activa)
        m_enCurso.append(fila);
    else
        m_enCurso.removeAll(fila);

    const QModelIndex indice = index(fila, ColumnaMarca);
    emit dataChanged(indice, indice, { Qt::DisplayRole });
}

void ListaDeCopia::desmarcarTodas()
{
    if (m_enCurso.isEmpty())
        return;
    const QModelIndex primero = index(0, ColumnaMarca);
    const QModelIndex ultimo = index(int(m_elementos.size()) - 1, ColumnaMarca);
    m_enCurso.clear();
    emit dataChanged(primero, ultimo, { Qt::DisplayRole });
}

QList<int> ListaDeCopia::moverAlPrincipio(QList<int> filas)
{
    return reubicar(std::move(filas), false, true);
}

QList<int> ListaDeCopia::moverArriba(QList<int> filas)
{
    return reubicar(std::move(filas), false, false);
}

QList<int> ListaDeCopia::moverAbajo(QList<int> filas)
{
    return reubicar(std::move(filas), true, false);
}

QList<int> ListaDeCopia::moverAlFinal(QList<int> filas)
{
    return reubicar(std::move(filas), true, true);
}

void ListaDeCopia::ordenarPor(Columna columna, Qt::SortOrder orden)
{
    if (columna != ColumnaFuente && columna != ColumnaTamano && columna != ColumnaDestino)
        return;

    // No se debe tocar el orden de las filas activas: los motores guardan el
    // elemento que están copiando y esas filas son las anclas de la cola.
    // Precalcular la clave evita convertir y plegar cada ruta en cada
    // comparación de `stable_sort`; con una carpeta grande esa diferencia
    // bloquea menos tiempo el hilo de la interfaz mientras siguen llegando
    // señales de progreso de los motores.
    struct PendienteOrdenado {
        ElementoDeCopia elemento;
        QString clave;
        int filaOriginal = -1;
    };

    const QModelIndexList indicesPersistentes = persistentIndexList();

    // Las filas en curso anclan el principio: la ordenación solo reorganiza lo
    // pendiente, igual que el reordenado a mano.
    ElementosDeCopia anclas;
    QList<int> ordenNuevo;
    ordenNuevo.reserve(m_elementos.size());
    for (int fila : m_enCurso)
        anclas.append(m_elementos.at(fila));
    for (int fila : m_enCurso)
        ordenNuevo.append(fila);
    QList<PendienteOrdenado> pendientes;
    for (int i = 0; i < m_elementos.size(); ++i) {
        if (m_enCurso.contains(i))
            continue;

        PendienteOrdenado pendiente;
        pendiente.elemento = m_elementos.at(i);
        pendiente.filaOriginal = i;
        if (columna != ColumnaTamano) {
            const QString &ruta = columna == ColumnaDestino
                ? pendiente.elemento.destino
                : pendiente.elemento.fuente;
            pendiente.clave = QDir::fromNativeSeparators(ruta).toCaseFolded();
        }
        pendientes.append(std::move(pendiente));
    }

    const auto criterio = [columna](const PendienteOrdenado &izquierda,
                             const PendienteOrdenado &derecha) {
        if (columna == ColumnaTamano)
            return izquierda.elemento.tamano < derecha.elemento.tamano;
        return QString::compare(izquierda.clave, derecha.clave) < 0;
    };
    const auto antiCriterio = [&criterio](const PendienteOrdenado &izquierda,
                                  const PendienteOrdenado &derecha) {
        return criterio(derecha, izquierda);
    };

    if (pendientes.size() > 1) {
        if (orden == Qt::AscendingOrder)
            std::stable_sort(pendientes.begin(), pendientes.end(), criterio);
        else
            std::stable_sort(pendientes.begin(), pendientes.end(), antiCriterio);
    }

    ElementosDeCopia ordenados;
    ordenados.reserve(pendientes.size());
    for (const PendienteOrdenado &pendiente : pendientes) {
        ordenados.append(pendiente.elemento);
        ordenNuevo.append(pendiente.filaOriginal);
    }

    // Es una reordenación de las mismas filas, no un cambio de contenido. Un
    // reset completo obliga al proxy y a la tabla a reconstruir toda la vista
    // y puede dejar en cola las señales de progreso mientras se copia. El
    // cambio de layout conserva el estado visual y las filas activas.
    emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
    m_elementos = anclas + ordenados;
    m_enCurso.clear();
    for (int i = 0; i < anclas.size(); ++i)
        m_enCurso.append(i);

    QList<int> filaNueva(m_elementos.size(), -1);
    for (int i = 0; i < ordenNuevo.size(); ++i)
        filaNueva[ordenNuevo.at(i)] = i;

    QModelIndexList indicesNuevos;
    indicesNuevos.reserve(indicesPersistentes.size());
    for (const QModelIndex &indice : indicesPersistentes) {
        const int fila = indice.row();
        if (fila >= 0 && fila < filaNueva.size() && filaNueva.at(fila) >= 0)
            indicesNuevos.append(index(filaNueva.at(fila), indice.column()));
        else
            indicesNuevos.append(QModelIndex());
    }
    changePersistentIndexList(indicesPersistentes, indicesNuevos);
    emit layoutChanged({}, QAbstractItemModel::VerticalSortHint);
    emit cambiada();
}

QList<int> ListaDeCopia::reubicar(QList<int> filas, bool haciaAbajo, bool alExtremo)
{
    std::sort(filas.begin(), filas.end());
    filas.erase(std::unique(filas.begin(), filas.end()), filas.end());
    filas.removeIf([this](int fila) {
        return fila < 0 || fila >= m_elementos.size() || m_enCurso.contains(fila);
    });
    if (filas.isEmpty())
        return {};

    // Las filas en curso anclan el principio de la lista: nada se mueve por
    // delante de ellas.
    const int primeraLibre = int(m_enCurso.size());

    if (!alExtremo) {
        QList<int> nuevas;
        if (haciaAbajo) {
            for (int i = int(filas.size()) - 1; i >= 0; --i) {
                const int fila = filas.at(i);
                const int siguiente = fila + 1;
                if (siguiente >= m_elementos.size() || filas.contains(siguiente)) {
                    nuevas.prepend(fila);
                    continue;
                }
                m_elementos.swapItemsAt(fila, siguiente);
                nuevas.prepend(siguiente);
            }
        } else {
            for (int i = 0; i < filas.size(); ++i) {
                const int fila = filas.at(i);
                const int anterior = fila - 1;
                if (anterior < primeraLibre || filas.contains(anterior)) {
                    nuevas.append(fila);
                    continue;
                }
                m_elementos.swapItemsAt(fila, anterior);
                nuevas.append(anterior);
            }
        }
        emit dataChanged(index(0, 0), index(int(m_elementos.size()) - 1, NumeroDeColumnas - 1));
        return nuevas;
    }

    ElementosDeCopia movidos;
    movidos.reserve(filas.size());
    beginResetModel();
    for (int i = int(filas.size()) - 1; i >= 0; --i)
        movidos.prepend(m_elementos.takeAt(filas.at(i)));
    const int insercion = haciaAbajo ? int(m_elementos.size()) : primeraLibre;
    for (int i = 0; i < movidos.size(); ++i)
        m_elementos.insert(insercion + i, movidos.at(i));
    // Las anclas ocupan 0..primeraLibre-1 y no se han movido: sus índices
    // siguen siendo los mismos.
    endResetModel();

    QList<int> nuevas;
    for (int i = 0; i < movidos.size(); ++i)
        nuevas.append(insercion + i);
    return nuevas;
}

void ListaDeCopia::recalcularBytes()
{
    m_bytes = 0;
    for (const ElementoDeCopia &elemento : m_elementos)
        m_bytes += elemento.tamano;
}

} // namespace maxcopier
