#include "vistas/panelcompacto.h"

#include "vistas/barraprogreso.h"
#include "vistas/chip.h"
#include "vistas/etiquetaruta.h"
#include "vistas/minigrafica.h"
#include "vistas/unidaddestino.h"

#include <QActionGroup>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace maxcopier {
namespace {

    QPushButton *crearBoton(const QString &texto, const QString &nombreObjeto, QWidget *padre)
    {
        auto *boton = new QPushButton(texto, padre);
        if (!nombreObjeto.isEmpty())
            boton->setObjectName(nombreObjeto);
        boton->setCursor(Qt::PointingHandCursor);
        return boton;
    }

} // namespace

PanelCompacto::PanelCompacto(QWidget *parent)
    : QWidget(parent)
{
    auto *columna = new QVBoxLayout(this);
    columna->setContentsMargins(0, 0, 0, 0);
    columna->setSpacing(0);
    columna->addWidget(construirCabecera());
    columna->addWidget(construirAcciones());
}

QWidget *PanelCompacto::construirCabecera()
{
    auto *cabecera = new QFrame(this);
    cabecera->setObjectName(QStringLiteral("cabecera"));

    auto *columna = new QVBoxLayout(cabecera);
    columna->setContentsMargins(10, 8, 10, 6);
    columna->setSpacing(4);

    auto *rutas = new QGridLayout;
    rutas->setContentsMargins(0, 0, 0, 3);
    rutas->setHorizontalSpacing(8);
    rutas->setVerticalSpacing(2);
    rutas->setColumnMinimumWidth(0, 46);
    rutas->setColumnStretch(1, 1);

    const QStringList claves { tr("Origen"), tr("Destino") };
    for (int fila = 0; fila < claves.size(); ++fila) {
        auto *clave = new QLabel(claves.at(fila), cabecera);
        clave->setObjectName(QStringLiteral("claveRuta"));

        auto *valor = new EtiquetaRuta(cabecera);
        auto *abrir = crearBoton(tr("Abrir"), QStringLiteral("enlace"), cabecera);
        abrir->setFlat(true);

        rutas->addWidget(clave, fila, 0);
        rutas->addWidget(valor, fila, 1);
        rutas->addWidget(abrir, fila, 2);

        if (fila == 0) {
            m_desde = valor;
            connect(abrir, &QPushButton::clicked, this, &PanelCompacto::abrirOrigenPedido);
        } else {
            m_hacia = valor;
            connect(abrir, &QPushButton::clicked, this, &PanelCompacto::abrirDestinoPedido);
        }
    }
    columna->addLayout(rutas);

    m_total = new BarraProgreso(BarraProgreso::Variante::Total, cabecera);
    m_archivos = new BarraArchivos(cabecera);
    columna->addWidget(m_total);
    columna->addWidget(m_archivos);

    auto *metadatos = new QHBoxLayout;
    metadatos->setContentsMargins(0, 2, 0, 0);
    metadatos->setSpacing(8);

    m_unidad = new UnidadDestino(cabecera);
    m_alTerminar = new Chip(tr("Al Terminar:"), QString(), cabecera);
    m_alTerminar->setToolTip(tr("Cambiar la acción al terminar esta copia"));

    metadatos->addWidget(m_unidad);
    metadatos->addStretch();
    metadatos->addWidget(m_alTerminar);
    columna->addLayout(metadatos);

    connect(m_alTerminar, &Chip::clicado, this, &PanelCompacto::abrirMenuAlTerminar);
    connect(m_archivos, &BarraArchivos::segmentoClicado, this, &PanelCompacto::segmentoClicado);

    return cabecera;
}

QWidget *PanelCompacto::construirAcciones()
{
    auto *acciones = new QFrame(this);
    acciones->setObjectName(QStringLiteral("acciones"));

    auto *fila = new QHBoxLayout(acciones);
    fila->setContentsMargins(10, 7, 10, 7);
    fila->setSpacing(6);

    m_pausar = crearBoton(tr("\u23F8 Pausar"), QStringLiteral("primario"), acciones);
    m_saltar = crearBoton(tr("\u23ED Saltar"), QString(), acciones);
    m_cancelar = crearBoton(tr("\u2715 Cancelar"), QString(), acciones);
    auto *anadir = crearBoton(QStringLiteral("+"), QStringLiteral("botonIcono"), acciones);
    anadir->setToolTip(tr("Añadir archivos a la lista"));
    auto *detalles = crearBoton(tr("\u25BE Detalles"), QStringLiteral("fantasma"), acciones);
    detalles->setToolTip(tr("Ver la lista de copia (F3)"));

    fila->addWidget(detalles);
    fila->addStretch();
    fila->addWidget(m_pausar);
    fila->addWidget(m_saltar);
    fila->addWidget(m_cancelar);
    fila->addWidget(anadir);

    connect(m_pausar, &QPushButton::clicked, this, &PanelCompacto::pausarPedido);
    connect(m_saltar, &QPushButton::clicked, this, &PanelCompacto::saltarPedido);
    connect(m_cancelar, &QPushButton::clicked, this, &PanelCompacto::cancelarPedido);
    connect(anadir, &QPushButton::clicked, this, &PanelCompacto::anadirPedido);
    connect(detalles, &QPushButton::clicked, this, &PanelCompacto::detallesPedido);

    return acciones;
}

void PanelCompacto::mostrarRutas(const QString &desde, const QString &hacia)
{
    m_desde->establecerRuta(desde);
    m_hacia->establecerRuta(hacia);
}

void PanelCompacto::mostrarTotal(int porcentajeTotal, const QString &detalle, const QString &velocidad)
{
    m_total->establecerPorcentaje(porcentajeTotal);
    m_total->establecerTextoIzquierda(detalle);
    m_total->establecerTextoDerecha(velocidad);
}

void PanelCompacto::mostrarArchivos(const QList<ArchivoEnCurso> &archivos)
{
    m_archivos->establecerArchivos(archivos);
}

void PanelCompacto::mostrarSinArchivo(const QString &detalle)
{
    m_archivos->establecerMensajeVacio(detalle);
}

void PanelCompacto::mostrarVelocidades(const QList<double> &muestras)
{
    m_total->grafica()->establecerMuestras(muestras);
}

void PanelCompacto::mostrarUnidad(
    const QString &tipo, const QString &nombre, const QString &libre, double ocupado)
{
    m_unidad->establecerUnidad(tipo, nombre, libre, ocupado);
}

void PanelCompacto::mostrarAlTerminar(const QString &accion, AccionAlTerminar accionConfigurada)
{
    m_accionFinal = accionConfigurada;
    m_alTerminar->establecerValor(accion + QStringLiteral(" \u25BE"));
}

void PanelCompacto::abrirMenuAlTerminar()
{
    QMenu menu(this);
    QActionGroup grupo(&menu);
    grupo.setExclusive(true);

    const QList<AccionAlTerminar> opciones {
        AccionAlTerminar::Nada,
        AccionAlTerminar::Cerrar,
        AccionAlTerminar::Suspender,
        AccionAlTerminar::Apagar,
    };
    for (const AccionAlTerminar opcion : opciones) {
        auto *accion = menu.addAction(Configuracion::nombre(opcion));
        accion->setCheckable(true);
        accion->setChecked(m_accionFinal == opcion);
        accion->setData(int(opcion));
        grupo.addAction(accion);
    }

    QAction *seleccion = menu.exec(
        m_alTerminar->mapToGlobal(QPoint(0, m_alTerminar->height())));
    if (seleccion)
        emit accionFinalPedida(AccionAlTerminar(seleccion->data().toInt()));
}

void PanelCompacto::mostrarPausado(bool pausado)
{
    m_pausar->setText(pausado ? tr("\u25B6 Reanudar") : tr("\u23F8 Pausar"));
}

void PanelCompacto::habilitarControles(bool copiando, bool escaneando)
{
    m_pausar->setEnabled(copiando || escaneando);
    m_saltar->setEnabled(copiando);
    m_cancelar->setEnabled(copiando || escaneando);
    m_pausar->setToolTip(copiando
            ? tr("Pausar o reanudar la transferencia")
            : escaneando ? tr("Pausar o reanudar la enumeración")
                         : tr("Disponible cuando termine la enumeración"));
    m_cancelar->setToolTip(escaneando && !copiando
            ? tr("Cancelar la enumeración")
            : tr("Cancelar la transferencia"));
}

void PanelCompacto::refrescarTema()
{
    for (Chip *chip : { m_alTerminar })
        chip->refrescar();
    for (QWidget *hijo : findChildren<QWidget *>())
        hijo->update();
    update();
}

} // namespace maxcopier
