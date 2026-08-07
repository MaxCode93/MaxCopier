add_library(maxcopier_core STATIC
    escaneo/escaner.h
    escaneo/escaner.cpp
    lista/elementodecopia.h
    lista/listadecopia.h
    lista/listadecopia.cpp
    politicas/acceso.h
    politicas/acceso.cpp
    politicas/colision.h
    politicas/colision.cpp
    copia/motordecopia.h
    copia/motordecopia.cpp
    copia/metododecopia.h
    copia/backendwin32.h
    copia/backendwin32.cpp
    copia/limitadorvelocidad.h
    copia/limitadorvelocidad.cpp
    util/atributos.h
    util/atributos.cpp
    util/espaciolibre.h
    util/espaciolibre.cpp
    util/formatos.h
    util/formatos.cpp
    util/rutas.h
    util/rutas.cpp
    util/titulos.h
    util/titulos.cpp
    util/velocimetro.h
    util/velocimetro.cpp
)

target_include_directories(maxcopier_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(maxcopier_core PUBLIC Qt6::Core)
