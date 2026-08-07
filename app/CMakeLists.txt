set(MAXCOPIER_APP_SOURCES
    acciones.h
    accionfinal.h
    accionfinal.cpp
    bandeja.h
    bandeja.cpp
    bandejatarea.h
    bandejatarea.cpp
    bandejacopia.h
    bandejacopia.cpp
    diagnostico.h
    diagnostico.cpp
    configuracion.h
    configuracion.cpp
    portapapeles.h
    portapapeles.cpp
    gestordeventanas.h
    gestordeventanas.cpp
    instanciaunica.h
    instanciaunica.cpp
    ventanaprincipal.h
    ventanaprincipal.cpp
    dialogos/dialogocolision.h
    dialogos/dialogocolision.cpp
    dialogos/dialogoerror.h
    dialogos/dialogoerror.cpp
    dialogos/dialogolistaactiva.h
    dialogos/dialogolistaactiva.cpp
    dialogos/dialogoopciones.h
    dialogos/dialogoopciones.cpp
    temas/temas.h
    temas/temas.cpp
    vistas/barralibre.h
    vistas/barralibre.cpp
    vistas/barraarchivos.h
    vistas/barraarchivos.cpp
    vistas/barraprogreso.h
    vistas/barraprogreso.cpp
    vistas/barratitulo.h
    vistas/barratitulo.cpp
    vistas/cargando.h
    vistas/cargando.cpp
    vistas/chip.h
    vistas/chip.cpp
    vistas/esquinas.h
    vistas/esquinas.cpp
    vistas/etiquetaruta.h
    vistas/etiquetaruta.cpp
    vistas/iconos.h
    vistas/iconos.cpp
    vistas/minigrafica.h
    vistas/minigrafica.cpp
    vistas/panelcompacto.h
    vistas/panelcompacto.cpp
    vistas/panelexpandido.h
    vistas/panelexpandido.cpp
    vistas/opcionespanel.h
    vistas/opcionespanel.cpp
    vistas/unidaddestino.h
    vistas/unidaddestino.cpp
    recursos.qrc
)

# La UI vive en una biblioteca para que las pruebas puedan ejercer la ventana
# real (incluido su pool de motores y el overlay) sin duplicar todos sus
# fuentes en cada ejecutable de prueba.
add_library(maxcopier_app STATIC ${MAXCOPIER_APP_SOURCES})
target_include_directories(maxcopier_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(maxcopier_app PUBLIC maxcopier_core Qt6::Network Qt6::Widgets)

# `accionfinal.cpp` pertenece a esta biblioteca estática y usa
# `SetSuspendState`. La dependencia debe viajar con la biblioteca para que
# también la reciban las pruebas que la enlazan (por ejemplo,
# `pruebainterfaz`). Además, al ser una dependencia transitiva de la
# biblioteca, CMake la coloca después de `libmaxcopier_app.a` en MinGW; si se
# añade solo al ejecutable principal puede quedar antes de la biblioteca y el
# enlazador no vuelve a resolverla.
if(WIN32)
    target_link_libraries(maxcopier_app PUBLIC powrprof)
endif()

qt_add_executable(maxcopier WIN32 main.cpp)

# Icono del ejecutable: solo Windows mira el .rc. `advapi32` trae el descriptor
# de seguridad de la tubería del canal (`core/ipc/tuberia.h`).
if(WIN32)
    target_sources(maxcopier PRIVATE recursos/maxcopier.rc)
    target_link_libraries(maxcopier PRIVATE advapi32 user32 ole32)
    target_compile_definitions(maxcopier_app PRIVATE _WIN32_WINNT=0x0601 NTDDI_VERSION=0x06010000)
    target_compile_definitions(maxcopier PRIVATE _WIN32_WINNT=0x0601 NTDDI_VERSION=0x06010000)
endif()

set_target_properties(maxcopier PROPERTIES OUTPUT_NAME "MaxCopier")
target_link_libraries(maxcopier PRIVATE maxcopier_app)
