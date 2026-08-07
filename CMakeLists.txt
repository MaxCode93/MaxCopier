cmake_minimum_required(VERSION 3.21)

project(MaxCopier
    VERSION 0.1.0
    DESCRIPTION "Copiador de archivos para Windows"
    LANGUAGES CXX)

# El icono del .exe entra por un .rc (app/recursos/maxcopier.rc).
if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilación" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Network Widgets)

add_compile_definitions(MAXCOPIER_VERSION="${PROJECT_VERSION}")

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(core)
add_subdirectory(app)

enable_testing()
add_subdirectory(pruebas)

# La extensión del Explorador y el canal con el que habla solo existen en
# Windows, y con ellos la prueba del canal.
if(WIN32)
    add_subdirectory(shell)
endif()
