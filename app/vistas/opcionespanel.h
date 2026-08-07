#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QListWidget;
class QSpinBox;
class QStackedWidget;

namespace maxcopier {

class Configuracion;

/// Editor reutilizable de las preferencias de MaxCopier, con el menú lateral de
/// categorías del mockup aprobado (ui-v3): General, Motor de copia, Colisiones,
/// Errores y Apariencia y temas. Solo hay páginas con ajustes reales: no se
/// inventan opciones que la app no tiene. Se muestra como pestaña dentro de una
/// transferencia y también desde la opción global de la bandeja.
class OpcionesPanel : public QWidget {
    Q_OBJECT

public:
    explicit OpcionesPanel(Configuracion *configuracion, QWidget *parent = nullptr);

private slots:
    void sincronizar();
    void cambiarCategoria(int fila);
    void limiteCambiado(int mebibytes);
    void accionFinalCambiada(int indice);
    void colisionCambiada(int indice);
    void errorCambiado(int indice);
    void listaActivaCambiada(int indice);
    void metodoCambiado(int indice);
    void archivosCambiados(int archivos);
    void espacioCambiado(bool comprobar);
    void temaCambiado(int indice);

private:
    QWidget *construirPaginaGeneral();
    QWidget *construirPaginaMotor();
    QWidget *construirPaginaColisiones();
    QWidget *construirPaginaErrores();
    QWidget *construirPaginaApariencia();
    static void seleccionarDato(QComboBox *combo, int dato);

    Configuracion *m_configuracion = nullptr;
    QListWidget *m_menu = nullptr;
    QStackedWidget *m_paginas = nullptr;

    QSpinBox *m_limite = nullptr;
    QComboBox *m_accionFinal = nullptr;
    QComboBox *m_colision = nullptr;
    QComboBox *m_error = nullptr;
    QComboBox *m_listaActiva = nullptr;
    QComboBox *m_metodo = nullptr;
    QSpinBox *m_archivos = nullptr;
    QCheckBox *m_comprobarEspacio = nullptr;
    QComboBox *m_tema = nullptr;
    QCheckBox *m_siempreFechas = nullptr;
    QCheckBox *m_siempreRutasLargas = nullptr;
    QCheckBox *m_siempreReanudar = nullptr;
};

} // namespace maxcopier
