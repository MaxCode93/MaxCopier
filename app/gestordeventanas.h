#pragma once

#include "configuracion.h"
#include "dialogos/dialogolistaactiva.h"
#include "instanciaunica.h"

#include <QList>
#include <QObject>
#include <QPointer>

namespace maxcopier {

class Bandeja;
class DialogoOpciones;
class VentanaPrincipal;

/// Dueño de las ventanas de MaxCopier. Como el proceso es único, todas las
/// peticiones de copia (línea de órdenes, botón **+** o segunda instancia)
/// pasan por aquí. Cada transferencia tiene su propia ventana, lista, motor,
/// escáner e icono de bandeja.
class GestorDeVentanas : public QObject {
    Q_OBJECT

public:
    explicit GestorDeVentanas(Configuracion *configuracion, QObject *parent = nullptr);

    /// Abre una ventana independiente y, si hay orígenes, arranca la tanda.
    VentanaPrincipal *arrancar(const QStringList &origenes, const QString &carpetaDestino,
        maxcopier::Operacion operacion = maxcopier::Operacion::Copiar,
        bool desdePortapapeles = false);

    bool hayVentanas() const { return !m_ventanas.isEmpty(); }
    bool tieneBandeja() const { return m_bandeja != nullptr; }

    /// Con bandeja, el proceso puede vivir sin ventanas visibles; sin bandeja
    /// necesita al menos una ventana para seguir siendo utilizable.
    bool puedeSeguirEjecutando() const { return tieneBandeja() || hayVentanas(); }

public slots:
    /// Atiende una petición venga de donde venga (línea de órdenes, botón **+**,
    /// segunda instancia o Explorador). Sin `origenes`, con bandeja, no crea
    /// una UI principal: el controlador global ya vive en el área de
    /// notificación.
    void atender(maxcopier::Operacion operacion, const QStringList &origenes,
        const QString &carpetaDestino, bool desdePortapapeles = false);

    /// Acción «Nueva copia» / «Nuevo movimiento» del menú global.
    void crearCopia(maxcopier::Operacion operacion);

    /// Abre el editor global de ajustes, incluso cuando no hay ventanas de
    /// transferencia visibles.
    void mostrarOpciones();

private:
    VentanaPrincipal *crearVentana(maxcopier::Operacion operacion);
    void cancelarTodas();
    void salir();
    void ejecutarAccionFinal(VentanaPrincipal *ventana, AccionAlTerminar accion);
    void alTerminarTanda(VentanaPrincipal *ventana, bool completa);
    void intentarAccionDeEnergia();

    /// Carpeta de destino de una petición que no la trae (menú «Copiar con
    /// MaxCopier» del Explorador, que no sabe adónde). Vacía si se cancela.
    QString preguntarDestino(const QStringList &origenes) const;

    /// Ventana a la que le toca la petición: una libre y, si no, una ocupada
    /// con el mismo destino para conservar el diálogo de lista activa.
    VentanaPrincipal *ventanaParaDestino(const QString &carpetaDestino, Operacion operacion) const;

    /// Pregunta qué hacer cuando `ocupada` está copiando. Devuelve la acción y
    /// se salta el diálogo si el usuario ya pidió recordar su elección.
    AccionListaActiva decidir(VentanaPrincipal *ocupada, Operacion operacion,
        const QStringList &origenes, const QString &carpetaDestino, bool permitirAnadir);

    QList<QPointer<VentanaPrincipal>> m_ventanas;
    Configuracion *m_configuracion = nullptr;
    Bandeja *m_bandeja = nullptr;
    QPointer<DialogoOpciones> m_dialogoOpciones;
    bool m_saliendo = false;

    bool m_hayAccionDeEnergia = false;
    bool m_accionDeEnergiaLanzada = false;
    AccionAlTerminar m_accionDeEnergia = AccionAlTerminar::Nada;

    /// Elección recordada con la casilla del diálogo; vale mientras la app siga
    /// abierta (el ajuste persistente irá en Opciones, F10).
    AccionListaActiva m_recordada = AccionListaActiva::Cancelar;
    bool m_hayRecordada = false;
};

} // namespace maxcopier
