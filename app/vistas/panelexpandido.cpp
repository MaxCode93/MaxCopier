#include "vistas/panelexpandido.h"

#include "lista/listadecopia.h"
#include "util/formatos.h"
#include "vistas/opcionespanel.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>

namespace maxcopier {
namespace {

    QLabel *crearEtiqueta(const QString &nombreObjeto, QWidget *padre)
    {
        auto *etiqueta = new QLabel(padre);
        etiqueta->setObjectName(nombreObjeto);
        return etiqueta;
    }

} // namespace

PanelExpandido::PanelExpandido(Configuracion *configuracion, QWidget *parent)
    : QWidget(parent)
    , m_configuracion(configuracion)
{
    auto *columna = new QVBoxLayout(this);
    columna->setContentsMargins(0, 0, 0, 0);
    columna->setSpacing(0);

    m_pestanas = new QTabWidget(this);
    m_pestanas->setObjectName(QStringLiteral("pestanas"));
    m_pestanas->setDocumentMode(true);
    m_pestanas->addTab(construirPestanaLista(), tr("\u2630 Lista de copia"));
    m_pestanas->addTab(construirPestanaErrores(), tr("\u2717 Errores"));
    m_pestanas->addTab(construirPestanaRegistro(), tr("\u25F7 Registro"));
    m_pestanas->addTab(construirPestanaOpciones(), tr("\u2699 Opciones"));

    columna->addWidget(m_pestanas);
    mostrarResumen(0, 0);
    mostrarEstado(formatearDuracion(0), tr("0 B/s"), tr("0 B/s"), formatearDuracion(-1));
}

QWidget *PanelExpandido::construirPestanaLista()
{
    auto *pestana = new QWidget(this);
    auto *columna = new QVBoxLayout(pestana);
    columna->setContentsMargins(8, 8, 8, 6);
    columna->setSpacing(6);

    auto *superior = new QHBoxLayout;
    superior->setSpacing(8);

    m_buscador = new QLineEdit(pestana);
    m_buscador->setObjectName(QStringLiteral("buscador"));
    m_buscador->setPlaceholderText(tr("\u2315 filtrar por nombre…"));
    m_buscador->setClearButtonEnabled(true);
    m_buscador->setFixedWidth(220);

    m_resumen = crearEtiqueta(QStringLiteral("resumenLista"), pestana);
    m_resumen->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    superior->addWidget(m_buscador);
    superior->addStretch();
    superior->addWidget(m_resumen);
    columna->addLayout(superior);

    m_filtro = new QSortFilterProxyModel(this);
    m_filtro->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filtro->setFilterKeyColumn(ListaDeCopia::ColumnaFuente);
    connect(m_buscador, &QLineEdit::textChanged, m_filtro, &QSortFilterProxyModel::setFilterFixedString);

    m_tabla = new QTableView(pestana);
    m_tabla->setObjectName(QStringLiteral("tablaLista"));
    m_tabla->setModel(m_filtro);
    m_tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabla->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabla->setAlternatingRowColors(false);
    m_tabla->setShowGrid(false);
    m_tabla->verticalHeader()->setVisible(false);
    m_tabla->verticalHeader()->setDefaultSectionSize(24);
    m_tabla->horizontalHeader()->setHighlightSections(false);
    // Pulsar las cabeceras reorganiza la cola (Fuente, Tamaño o Destino).
    m_tabla->horizontalHeader()->setSortIndicatorShown(true);
    connect(m_tabla->horizontalHeader(), &QHeaderView::sectionClicked, this,
        &PanelExpandido::ordenarColumna);
    m_tabla->setWordWrap(false);
    // Las rutas comparten una ventana relativamente estrecha: conservar el
    // inicio (unidad/carpeta) y el nombre final resulta más útil que el
    // truncado derecho por defecto, que suele dejar solo «D:\\...». El
    // tooltip sigue mostrando siempre la ruta completa.
    m_tabla->setTextElideMode(Qt::ElideMiddle);

    auto *cuerpo = new QHBoxLayout;
    cuerpo->setSpacing(6);
    cuerpo->addWidget(construirBarraDeCola());
    cuerpo->addWidget(m_tabla, 1);
    columna->addLayout(cuerpo, 1);

    auto *estado = new QHBoxLayout;
    estado->setSpacing(14);
    m_transcurrido = crearEtiqueta(QStringLiteral("estadoLista"), pestana);
    m_media = crearEtiqueta(QStringLiteral("estadoLista"), pestana);
    m_maxima = crearEtiqueta(QStringLiteral("estadoLista"), pestana);
    m_restante = crearEtiqueta(QStringLiteral("estadoLista"), pestana);
    //auto *pista = crearEtiqueta(QStringLiteral("estadoLista"), pestana);
    //pista->setText(tr("Los archivos copiados desaparecen de la lista"));

    for (QLabel *etiqueta : { m_transcurrido, m_media, m_maxima, m_restante })
        estado->addWidget(etiqueta);
    estado->addStretch();
    //estado->addWidget(pista);
    columna->addLayout(estado);

    return pestana;
}

QWidget *PanelExpandido::construirPestanaErrores()
{
    auto *pestana = new QWidget(this);
    auto *columna = new QVBoxLayout(pestana);
    columna->setContentsMargins(8, 8, 8, 6);
    columna->setSpacing(6);

    auto *superior = new QHBoxLayout;
    auto *limpiar = new QPushButton(tr("Limpiar"), pestana);
    limpiar->setObjectName(QStringLiteral("fantasma"));
    limpiar->setCursor(Qt::PointingHandCursor);
    superior->addStretch();
    superior->addWidget(limpiar);
    columna->addLayout(superior);

    m_modeloErrores = new QStandardItemModel(0, 4, this);
    m_modeloErrores->setHeaderData(0, Qt::Horizontal, tr("Hora"));
    m_modeloErrores->setHeaderData(1, Qt::Horizontal, tr("Acción"));
    m_modeloErrores->setHeaderData(2, Qt::Horizontal, tr("Archivo"));
    m_modeloErrores->setHeaderData(3, Qt::Horizontal, tr("Motivo"));

    auto *tabla = new QTableView(pestana);
    tabla->setObjectName(QStringLiteral("tablaErrores"));
    tabla->setModel(m_modeloErrores);
    tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabla->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabla->setAlternatingRowColors(false);
    tabla->setShowGrid(false);
    tabla->verticalHeader()->setVisible(false);
    tabla->verticalHeader()->setDefaultSectionSize(24);
    tabla->horizontalHeader()->setHighlightSections(false);
    tabla->horizontalHeader()->setStretchLastSection(true);
    tabla->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tabla->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tabla->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    tabla->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    tabla->setWordWrap(false);
    columna->addWidget(tabla, 1);

    connect(limpiar, &QPushButton::clicked, this, &PanelExpandido::limpiarErrores);
    return pestana;
}

QWidget *PanelExpandido::construirPestanaRegistro()
{
    auto *pestana = new QWidget(this);
    auto *columna = new QVBoxLayout(pestana);
    columna->setContentsMargins(8, 8, 8, 6);
    columna->setSpacing(6);

    auto *superior = new QHBoxLayout;
    auto *guardar = new QPushButton(tr("Guardar…"), pestana);
    guardar->setObjectName(QStringLiteral("fantasma"));
    guardar->setCursor(Qt::PointingHandCursor);
    superior->addStretch();
    superior->addWidget(guardar);
    columna->addLayout(superior);

    m_registro = new QPlainTextEdit(pestana);
    m_registro->setObjectName(QStringLiteral("textoRegistro"));
    m_registro->setReadOnly(true);
    m_registro->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_registro->setPlaceholderText(tr("El registro de la sesión aparece aquí."));
    columna->addWidget(m_registro, 1);

    connect(guardar, &QPushButton::clicked, this, &PanelExpandido::guardarRegistro);
    return pestana;
}

void PanelExpandido::anadirError(const QString &hora, const QString &accion,
    const QString &archivo, const QString &motivo)
{
    if (!m_modeloErrores)
        return;
    const int fila = m_modeloErrores->rowCount();
    m_modeloErrores->insertRow(fila);
    m_modeloErrores->setData(m_modeloErrores->index(fila, 0), hora);
    m_modeloErrores->setData(m_modeloErrores->index(fila, 1), accion);
    m_modeloErrores->setData(m_modeloErrores->index(fila, 2), archivo);
    m_modeloErrores->setData(m_modeloErrores->index(fila, 3), motivo);
    m_modeloErrores->setData(m_modeloErrores->index(fila, 2), archivo, Qt::ToolTipRole);
    m_modeloErrores->setData(m_modeloErrores->index(fila, 3), motivo, Qt::ToolTipRole);
    actualizarContadorErrores();
}

void PanelExpandido::limpiarErrores()
{
    if (m_modeloErrores)
        m_modeloErrores->removeRows(0, m_modeloErrores->rowCount());
    actualizarContadorErrores();
}

void PanelExpandido::actualizarContadorErrores()
{
    if (!m_pestanas)
        return;
    const int errores = m_modeloErrores ? m_modeloErrores->rowCount() : 0;
    m_pestanas->setTabText(1,
        errores == 0 ? tr("\u2717 Errores") : tr("\u2717 Errores (%1)").arg(errores));
}

void PanelExpandido::anadirRegistro(const QString &linea)
{
    if (m_registro)
        m_registro->appendPlainText(linea);
}

void PanelExpandido::guardarRegistro()
{
    if (!m_registro || m_registro->toPlainText().isEmpty())
        return;
    const QString ruta = QFileDialog::getSaveFileName(this, tr("Guardar el registro"),
        QDir::homePath() + QStringLiteral("/registro MaxCopier.log"),
        tr("Registro (*.log *.txt)"));
    if (ruta.isEmpty())
        return;

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("No se pudo guardar"),
            tr("No se pudo escribir el registro en %1.").arg(ruta));
        return;
    }
    archivo.write(m_registro->toPlainText().toUtf8());
}

QWidget *PanelExpandido::construirPestanaOpciones()
{
    return new OpcionesPanel(m_configuracion, this);
}

void PanelExpandido::refrescarTema()
{
    for (QWidget *hijo : findChildren<QWidget *>())
        hijo->update();
    update();
}

QWidget *PanelExpandido::construirBarraDeCola()
{
    auto *barra = new QFrame(this);
    barra->setObjectName(QStringLiteral("barraCola"));

    auto *columna = new QVBoxLayout(barra);
    columna->setContentsMargins(0, 0, 0, 0);
    columna->setSpacing(2);

    struct Accion {
        QString glifo;
        QString ayuda;
        void (PanelExpandido::*ranura)();
    };

    const QList<Accion> acciones {
        { QStringLiteral("\u2912"), tr("Mover al principio"), &PanelExpandido::moverAlPrincipio },
        { QStringLiteral("\u25B2"), tr("Subir"), &PanelExpandido::moverArriba },
        { QStringLiteral("\u25BC"), tr("Bajar"), &PanelExpandido::moverAbajo },
        { QStringLiteral("\u2913"), tr("Mover al final"), &PanelExpandido::moverAlFinal },
        { QStringLiteral("\u2212"), tr("Quitar de la lista"), &PanelExpandido::quitarSeleccion },
    };

    const auto crearBoton = [barra](const QString &glifo, const QString &ayuda) {
        auto *boton = new QPushButton(glifo, barra);
        boton->setObjectName(QStringLiteral("botonCola"));
        boton->setToolTip(ayuda);
        boton->setFixedSize(22, 22);
        boton->setFocusPolicy(Qt::NoFocus);
        boton->setCursor(Qt::PointingHandCursor);
        return boton;
    };

    for (int i = 0; i < acciones.size(); ++i) {
        const Accion &accion = acciones.at(i);
        auto *boton = crearBoton(accion.glifo, accion.ayuda);
        connect(boton, &QPushButton::clicked, this, accion.ranura);
        columna->addWidget(boton);
        if (i == 3) { // el «+» separa reordenar de gestionar la lista
            auto *anadir = crearBoton(QStringLiteral("+"), tr("Añadir archivos a la lista"));
            connect(anadir, &QPushButton::clicked, this, &PanelExpandido::anadirPedido);
            columna->addWidget(anadir);
        }
    }
    auto *cargar = crearBoton(QStringLiteral("🗀"), tr("Cargar lista de copia…"));
    connect(cargar, &QPushButton::clicked, this, &PanelExpandido::cargarListaPedida);
    columna->addWidget(cargar);
    auto *guardar = crearBoton(QStringLiteral("💾"), tr("Guardar la lista de copia actual…"));
    connect(guardar, &QPushButton::clicked, this, &PanelExpandido::guardarListaPedida);
    columna->addWidget(guardar);
    columna->addStretch();

    return barra;
}

void PanelExpandido::establecerLista(ListaDeCopia *lista)
{
    m_lista = lista;
    m_filtro->setSourceModel(lista);

    // Los anchos de columna solo se pueden fijar cuando la tabla ya tiene modelo.
    QHeaderView *cabecera = m_tabla->horizontalHeader();
    cabecera->setSectionResizeMode(ListaDeCopia::ColumnaMarca, QHeaderView::Fixed);
    cabecera->resizeSection(ListaDeCopia::ColumnaMarca, 20);
    cabecera->setSectionResizeMode(ListaDeCopia::ColumnaFuente, QHeaderView::Stretch);
    cabecera->setSectionResizeMode(ListaDeCopia::ColumnaTamano, QHeaderView::ResizeToContents);
    cabecera->setSectionResizeMode(ListaDeCopia::ColumnaDestino, QHeaderView::Stretch);

    connect(lista, &ListaDeCopia::cambiada, this,
        [this] { mostrarResumen(m_lista->archivos(), m_lista->bytes()); });
    connect(m_tabla->selectionModel(), &QItemSelectionModel::selectionChanged, this,
        [this] {
            const QList<int> filas = filasSeleccionadas();
            if (m_lista && !filas.isEmpty())
                emit archivoSeleccionado(m_lista->elemento(filas.first()).fuente);
        });
    mostrarResumen(lista->archivos(), lista->bytes());
}

void PanelExpandido::mostrarResumen(int archivos, qint64 bytes)
{
    m_resumen->setText(archivos == 1
            ? tr("1 archivo pendiente · %1").arg(formatearTamano(bytes))
            : tr("%1 archivos pendientes · %2").arg(archivos).arg(formatearTamano(bytes)));
    if (m_pestanas)
        m_pestanas->setTabText(0, tr("\u2630 Lista de copia  %1").arg(archivos));
}

void PanelExpandido::mostrarEstado(
    const QString &transcurrido, const QString &media, const QString &maxima, const QString &restante)
{
    m_transcurrido->setText(tr("Copiando: %1").arg(transcurrido));
    m_media->setText(tr("Media: %1").arg(media));
    m_maxima->setText(tr("Máxima: %1").arg(maxima));
    m_restante->setText(tr("Restante: %1").arg(restante));
}

QList<int> PanelExpandido::filasSeleccionadas() const
{
    QList<int> filas;
    if (!m_lista)
        return filas;
    const QModelIndexList indices = m_tabla->selectionModel()->selectedRows();
    for (const QModelIndex &indice : indices)
        filas.append(m_filtro->mapToSource(indice).row());
    std::sort(filas.begin(), filas.end());
    return filas;
}

void PanelExpandido::seleccionar(const QList<int> &filas)
{
    QItemSelectionModel *seleccion = m_tabla->selectionModel();
    seleccion->clearSelection();
    for (int fila : filas) {
        const QModelIndex indice = m_filtro->mapFromSource(m_lista->index(fila, 0));
        if (indice.isValid())
            seleccion->select(indice, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void PanelExpandido::moverAlPrincipio()
{
    if (m_lista)
        seleccionar(m_lista->moverAlPrincipio(filasSeleccionadas()));
}

void PanelExpandido::moverArriba()
{
    if (m_lista)
        seleccionar(m_lista->moverArriba(filasSeleccionadas()));
}

void PanelExpandido::moverAbajo()
{
    if (m_lista)
        seleccionar(m_lista->moverAbajo(filasSeleccionadas()));
}

void PanelExpandido::moverAlFinal()
{
    if (m_lista)
        seleccionar(m_lista->moverAlFinal(filasSeleccionadas()));
}

void PanelExpandido::quitarSeleccion()
{
    if (m_lista)
        m_lista->quitar(filasSeleccionadas());
}

void PanelExpandido::ordenarColumna(int seccion)
{
    if (!m_lista
        || (seccion != ListaDeCopia::ColumnaFuente
            && seccion != ListaDeCopia::ColumnaTamano
            && seccion != ListaDeCopia::ColumnaDestino))
        return;

    if (m_columnaOrden == seccion) {
        m_orden = m_orden == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_columnaOrden = seccion;
        m_orden = Qt::AscendingOrder;
    }
    m_lista->ordenarPorEnSegundoPlano(ListaDeCopia::Columna(seccion), m_orden);
    m_tabla->horizontalHeader()->setSortIndicator(seccion, m_orden);
}

} // namespace maxcopier
