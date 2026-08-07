#include "vistas/opcionespanel.h"

#include "configuracion.h"

#include "politicas/acceso.h"
#include "politicas/colision.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace maxcopier {
namespace {

constexpr qint64 kMiB = 1024 * 1024;
constexpr int kAnchoControl = 230;

QString nombreAccionLista(AccionListaActiva accion)
{
    switch (accion) {
    case AccionListaActiva::Preguntar:
        return QObject::tr("Preguntar");
    case AccionListaActiva::AnadirALaActual:
        return QObject::tr("Añadir a la lista actual");
    case AccionListaActiva::VentanaNueva:
        return QObject::tr("Abrir una ventana nueva");
    case AccionListaActiva::Cancelar:
        return QObject::tr("Cancelar");
    }
    return QObject::tr("preguntar");
}

QWidget *etiquetaDeAjuste(const QString &titulo, const QString &detalle, QWidget *padre)
{
    auto *contenedor = new QWidget(padre);
    auto *columna = new QVBoxLayout(contenedor);
    columna->setContentsMargins(0, 0, 0, 0);
    columna->setSpacing(1);

    auto *texto = new QLabel(titulo, contenedor);
    texto->setObjectName(QStringLiteral("tituloAjuste"));
    auto *ayuda = new QLabel(detalle, contenedor);
    ayuda->setObjectName(QStringLiteral("detalleAjuste"));
    ayuda->setWordWrap(true);

    columna->addWidget(texto);
    columna->addWidget(ayuda);
    return contenedor;
}

/// Fila del estilo del mockup: etiqueta (título + ayuda) a la izquierda y el
/// control a la derecha.
QWidget *filaDeAjuste(const QString &titulo, const QString &detalle, QWidget *control)
{
    auto *fila = new QWidget;
    auto *caja = new QHBoxLayout(fila);
    caja->setContentsMargins(0, 0, 0, 0);
    caja->setSpacing(16);
    caja->addWidget(etiquetaDeAjuste(titulo, detalle, fila), 1);
    caja->addWidget(control, 0, Qt::AlignVCenter);
    return fila;
}

void prepararControl(QWidget *control)
{
    control->setMinimumWidth(kAnchoControl);
    control->setMaximumWidth(kAnchoControl);
    control->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

/// Página con desplazamiento para que las filas quepan en cualquier alto.
QWidget *paginaConScroll(QWidget *contenido)
{
    auto *desplazamiento = new QScrollArea;
    desplazamiento->setObjectName(QStringLiteral("areaOpciones"));
    desplazamiento->setFrameShape(QFrame::NoFrame);
    desplazamiento->setWidgetResizable(true);
    desplazamiento->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    desplazamiento->setWidget(contenido);
    return desplazamiento;
}

QWidget *cabeceraDePagina(const QString &titulo, const QString &subtitulo, QWidget *padre)
{
    auto *contenedor = new QWidget(padre);
    auto *columna = new QVBoxLayout(contenedor);
    columna->setContentsMargins(0, 0, 0, 0);
    columna->setSpacing(2);

    auto *encabezado = new QLabel(titulo, contenedor);
    encabezado->setObjectName(QStringLiteral("tituloSeccionOpciones"));
    auto *descripcion = new QLabel(subtitulo, contenedor);
    descripcion->setObjectName(QStringLiteral("subtituloSeccionOpciones"));
    descripcion->setWordWrap(true);

    columna->addWidget(encabezado);
    columna->addWidget(descripcion);
    return contenedor;
}

} // namespace

OpcionesPanel::OpcionesPanel(Configuracion *configuracion, QWidget *parent)
    : QWidget(parent)
    , m_configuracion(configuracion)
{
    setObjectName(QStringLiteral("opcionesPanel"));

    auto *fila = new QHBoxLayout(this);
    fila->setContentsMargins(0, 0, 0, 0);
    fila->setSpacing(0);

    // Menú lateral de categorías, como en el mockup aprobado.
    m_menu = new QListWidget(this);
    m_menu->setObjectName(QStringLiteral("menuOpciones"));
    m_menu->setFixedWidth(158);
    m_menu->setFrameShape(QFrame::NoFrame);
    m_menu->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_menu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_menu->addItem(tr("General"));
    m_menu->addItem(tr("Motor de copia"));
    m_menu->addItem(tr("Colisiones"));
    m_menu->addItem(tr("Errores"));
    m_menu->addItem(tr("Apariencia y temas"));
    for (int i = 0; i < m_menu->count(); ++i)
        m_menu->item(i)->setSizeHint(QSize(0, 36));
    connect(m_menu, &QListWidget::currentRowChanged, this, &OpcionesPanel::cambiarCategoria);
    fila->addWidget(m_menu);

    m_paginas = new QStackedWidget(this);
    m_paginas->addWidget(construirPaginaGeneral());
    m_paginas->addWidget(construirPaginaMotor());
    m_paginas->addWidget(construirPaginaColisiones());
    m_paginas->addWidget(construirPaginaErrores());
    m_paginas->addWidget(construirPaginaApariencia());
    fila->addWidget(m_paginas, 1);

    // Como en el mockup, la categoría destacada al abrir es «Motor de copia».
    m_menu->setCurrentRow(1);

    if (m_configuracion) {
        connect(m_configuracion, &Configuracion::configuracionCambiada, this,
            &OpcionesPanel::sincronizar);
    }
    sincronizar();
}

QWidget *OpcionesPanel::construirPaginaGeneral()
{
    auto *contenido = new QWidget;
    auto *columna = new QVBoxLayout(contenido);
    columna->setContentsMargins(14, 12, 14, 12);
    columna->setSpacing(12);
    columna->addWidget(cabeceraDePagina(
        tr("General"), tr("Qué hace MaxCopier al recibir otra petición."), contenido));

    m_listaActiva = new QComboBox(contenido);
    for (const AccionListaActiva accion : { AccionListaActiva::Preguntar,
             AccionListaActiva::AnadirALaActual, AccionListaActiva::VentanaNueva,
             AccionListaActiva::Cancelar }) {
        m_listaActiva->addItem(nombreAccionLista(accion), int(accion));
    }
    prepararControl(m_listaActiva);
    columna->addWidget(filaDeAjuste(tr("Copia en curso"),
        tr("Cuando llega una petición y ya hay una transferencia activa"), m_listaActiva));
    columna->addStretch();

    connect(m_listaActiva, qOverload<int>(&QComboBox::currentIndexChanged), this,
        &OpcionesPanel::listaActivaCambiada);
    return paginaConScroll(contenido);
}

QWidget *OpcionesPanel::construirPaginaMotor()
{
    auto *contenido = new QWidget;
    auto *columna = new QVBoxLayout(contenido);
    columna->setContentsMargins(14, 12, 14, 12);
    columna->setSpacing(12);
    columna->addWidget(cabeceraDePagina(
        tr("Motor de copia"), tr("Cómo copia MaxCopier cada transferencia."), contenido));

    m_limite = new QSpinBox(contenido);
    m_limite->setRange(0, 10000);
    m_limite->setSingleStep(5);
    m_limite->setSpecialValueText(tr("Sin Límite"));
    m_limite->setSuffix(tr(" MiB/s"));
    m_limite->setToolTip(tr("Límite independiente para cada transferencia"));
    prepararControl(m_limite);
    columna->addWidget(filaDeAjuste(tr("Límite de velocidad"),
        tr("Máximo por transferencia; cero significa sin límite"), m_limite));

    m_accionFinal = new QComboBox(contenido);
    m_accionFinal->addItem(
        Configuracion::nombre(AccionAlTerminar::Nada), int(AccionAlTerminar::Nada));
    m_accionFinal->addItem(
        Configuracion::nombre(AccionAlTerminar::Cerrar), int(AccionAlTerminar::Cerrar));
    m_accionFinal->addItem(
        Configuracion::nombre(AccionAlTerminar::Suspender), int(AccionAlTerminar::Suspender));
    m_accionFinal->addItem(
        Configuracion::nombre(AccionAlTerminar::Apagar), int(AccionAlTerminar::Apagar));
    prepararControl(m_accionFinal);
    columna->addWidget(filaDeAjuste(tr("Al Terminar"),
        tr("Acción tras una tanda completa y correcta"), m_accionFinal));

    m_metodo = new QComboBox(contenido);
    m_metodo->addItem(Configuracion::nombre(MetodoDeCopia::Sincrono),
        int(MetodoDeCopia::Sincrono));
    m_metodo->addItem(Configuracion::nombre(MetodoDeCopia::Asincrono),
        int(MetodoDeCopia::Asincrono));
    prepararControl(m_metodo);
    columna->addWidget(filaDeAjuste(tr("Método de copia"),
        tr("Rápido usa E/S asíncrona en Windows; el compatible funciona en todo"),
        m_metodo));

    m_archivos = new QSpinBox(contenido);
    m_archivos->setRange(1, 4);
    m_archivos->setSuffix(tr(" archivo(s)"));
    m_archivos->setToolTip(tr("Copias en paralelo dentro de cada transferencia"));
    connect(m_archivos, qOverload<int>(&QSpinBox::valueChanged), this,
        [this](int cantidad) {
            m_archivos->setSuffix(cantidad == 1 ? tr(" archivo") : tr(" archivos"));
        });
    prepararControl(m_archivos);
    columna->addWidget(filaDeAjuste(tr("Archivos a la vez"),
        tr("Cuántos archivos copia cada ventana al mismo tiempo"), m_archivos));

    m_comprobarEspacio = new QCheckBox(
        tr("Comprobar espacio libre antes de empezar"), contenido);
    m_comprobarEspacio->setToolTip(
        tr("Avisa si no cabe todo en el destino; al continuar, copia hasta donde quepa"));
    connect(m_comprobarEspacio, &QCheckBox::toggled, this, &OpcionesPanel::espacioCambiado);
    columna->addWidget(filaDeAjuste(tr("Espacio en el destino"),
        tr("Avisar antes de arrancar si el volumen no tiene sitio para la lista"),
        m_comprobarEspacio));

    auto *separador = new QFrame(contenido);
    separador->setObjectName(QStringLiteral("separadorOpciones"));
    separador->setFixedHeight(1);
    columna->addWidget(separador);

    auto *siempre = new QLabel(tr("Siempre activo en esta versión"), contenido);
    siempre->setObjectName(QStringLiteral("kickerOpciones"));
    columna->addWidget(siempre);

    m_siempreFechas = new QCheckBox(tr("Preservar fechas y atributos"), contenido);
    m_siempreRutasLargas = new QCheckBox(tr("Rutas largas (> 260 caracteres) con \\?\\"), contenido);
    m_siempreReanudar = new QCheckBox(tr("Reanudar transferencias incompletas (.mcpart)"), contenido);
    for (QCheckBox *casilla : { m_siempreFechas, m_siempreRutasLargas, m_siempreReanudar }) {
        casilla->setChecked(true);
        casilla->setEnabled(false);
        casilla->setToolTip(tr("Esta capacidad está siempre activa; no es configurable."));
        columna->addWidget(casilla);
    }
    columna->addStretch();

    connect(m_limite, qOverload<int>(&QSpinBox::valueChanged), this,
        &OpcionesPanel::limiteCambiado);
    connect(m_accionFinal, qOverload<int>(&QComboBox::currentIndexChanged), this,
        &OpcionesPanel::accionFinalCambiada);
    connect(m_metodo, qOverload<int>(&QComboBox::currentIndexChanged), this,
        &OpcionesPanel::metodoCambiado);
    connect(m_archivos, qOverload<int>(&QSpinBox::valueChanged), this,
        &OpcionesPanel::archivosCambiados);
    return paginaConScroll(contenido);
}

QWidget *OpcionesPanel::construirPaginaColisiones()
{
    auto *contenido = new QWidget;
    auto *columna = new QVBoxLayout(contenido);
    columna->setContentsMargins(14, 12, 14, 12);
    columna->setSpacing(12);
    columna->addWidget(cabeceraDePagina(
        tr("Colisiones"), tr("Qué hacer cuando el destino ya tiene un archivo."), contenido));

    m_colision = new QComboBox(contenido);
    for (const AccionColision accion : { AccionColision::Preguntar, AccionColision::Sobrescribir,
             AccionColision::Renombrar, AccionColision::Saltar }) {
        m_colision->addItem(nombreAccionColision(accion), int(accion));
    }
    prepararControl(m_colision);
    columna->addWidget(filaDeAjuste(
        tr("Destino existente"), tr("Se puede elegir por archivo en cada copia"), m_colision));
    columna->addStretch();

    connect(m_colision, qOverload<int>(&QComboBox::currentIndexChanged), this,
        &OpcionesPanel::colisionCambiada);
    return paginaConScroll(contenido);
}

QWidget *OpcionesPanel::construirPaginaErrores()
{
    auto *contenido = new QWidget;
    auto *columna = new QVBoxLayout(contenido);
    columna->setContentsMargins(14, 12, 14, 12);
    columna->setSpacing(12);
    columna->addWidget(cabeceraDePagina(
        tr("Errores"), tr("Qué hacer cuando un archivo no se puede leer."), contenido));

    m_error = new QComboBox(contenido);
    for (const AccionError accion : { AccionError::Preguntar, AccionError::Reintentar,
             AccionError::PonerAlFinal, AccionError::Saltar }) {
        m_error->addItem(nombreAccionError(accion), int(accion));
    }
    prepararControl(m_error);
    columna->addWidget(filaDeAjuste(
        tr("Origen no disponible"), tr("Si no existe, no se puede leer o desaparece a mitad"),
        m_error));
    columna->addStretch();

    connect(m_error, qOverload<int>(&QComboBox::currentIndexChanged), this,
        &OpcionesPanel::errorCambiado);
    return paginaConScroll(contenido);
}

QWidget *OpcionesPanel::construirPaginaApariencia()
{
    auto *contenido = new QWidget;
    auto *columna = new QVBoxLayout(contenido);
    columna->setContentsMargins(14, 12, 14, 12);
    columna->setSpacing(12);
    columna->addWidget(cabeceraDePagina(
        tr("Apariencia y temas"), tr("Elige cómo quieres ver MaxCopier."), contenido));

    m_tema = new QComboBox(contenido);
    m_tema->addItem(Configuracion::nombre(TemaPreferido::Oscuro), int(TemaPreferido::Oscuro));
    m_tema->addItem(Configuracion::nombre(TemaPreferido::Claro), int(TemaPreferido::Claro));
    m_tema->addItem(Configuracion::nombre(TemaPreferido::Sistema), int(TemaPreferido::Sistema));
    prepararControl(m_tema);
    columna->addWidget(filaDeAjuste(
        tr("Tema visual"), tr("Sigue el sistema o elige un modo"), m_tema));
    columna->addStretch();

    connect(m_tema, qOverload<int>(&QComboBox::currentIndexChanged), this,
        &OpcionesPanel::temaCambiado);
    return paginaConScroll(contenido);
}

void OpcionesPanel::cambiarCategoria(int fila)
{
    if (m_paginas && fila >= 0 && fila < m_paginas->count())
        m_paginas->setCurrentIndex(fila);
}

void OpcionesPanel::seleccionarDato(QComboBox *combo, int dato)
{
    if (!combo)
        return;
    const int indice = combo->findData(dato);
    combo->setCurrentIndex(indice >= 0 ? indice : 0);
}

void OpcionesPanel::sincronizar()
{
    if (!m_configuracion)
        return;

    const QSignalBlocker bloqueoLimite(m_limite);
    const QSignalBlocker bloqueoFinal(m_accionFinal);
    const QSignalBlocker bloqueoColision(m_colision);
    const QSignalBlocker bloqueoError(m_error);
    const QSignalBlocker bloqueoLista(m_listaActiva);
    const QSignalBlocker bloqueoMetodo(m_metodo);
    const QSignalBlocker bloqueoArchivos(m_archivos);
    const QSignalBlocker bloqueoEspacio(m_comprobarEspacio);
    const QSignalBlocker bloqueoTema(m_tema);

    m_limite->setValue(int(m_configuracion->limiteVelocidad() / kMiB));
    seleccionarDato(m_accionFinal, int(m_configuracion->accionAlTerminar()));
    seleccionarDato(m_colision, int(m_configuracion->accionColision()));
    seleccionarDato(m_error, int(m_configuracion->accionError()));
    seleccionarDato(m_listaActiva, int(m_configuracion->accionListaActiva()));
    seleccionarDato(m_metodo, int(m_configuracion->metodoDeCopia()));
    m_archivos->setValue(m_configuracion->archivosALaVez());
    m_comprobarEspacio->setChecked(m_configuracion->comprobarEspacioLibre());
    seleccionarDato(m_tema, int(m_configuracion->temaPreferido()));
}

void OpcionesPanel::limiteCambiado(int mebibytes)
{
    if (m_configuracion)
        m_configuracion->establecerLimiteVelocidad(qint64(mebibytes) * kMiB);
}

void OpcionesPanel::accionFinalCambiada(int indice)
{
    if (m_configuracion)
        m_configuracion->establecerAccionAlTerminar(
            AccionAlTerminar(m_accionFinal->itemData(indice).toInt()));
}

void OpcionesPanel::colisionCambiada(int indice)
{
    if (m_configuracion)
        m_configuracion->establecerAccionColision(
            AccionColision(m_colision->itemData(indice).toInt()));
}

void OpcionesPanel::errorCambiado(int indice)
{
    if (m_configuracion)
        m_configuracion->establecerAccionError(AccionError(m_error->itemData(indice).toInt()));
}

void OpcionesPanel::listaActivaCambiada(int indice)
{
    if (m_configuracion)
        m_configuracion->establecerAccionListaActiva(
            AccionListaActiva(m_listaActiva->itemData(indice).toInt()));
}

void OpcionesPanel::metodoCambiado(int indice)
{
    if (m_configuracion)
        m_configuracion->establecerMetodoDeCopia(
            MetodoDeCopia(m_metodo->itemData(indice).toInt()));
}

void OpcionesPanel::archivosCambiados(int archivos)
{
    if (m_configuracion)
        m_configuracion->establecerArchivosALaVez(archivos);
}

void OpcionesPanel::espacioCambiado(bool comprobar)
{
    if (m_configuracion)
        m_configuracion->establecerComprobarEspacioLibre(comprobar);
}

void OpcionesPanel::temaCambiado(int indice)
{
    if (m_configuracion)
        m_configuracion->establecerTemaPreferido(
            TemaPreferido(m_tema->itemData(indice).toInt()));
}

} // namespace maxcopier
