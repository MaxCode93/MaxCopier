add_executable(pruebaconfiguracion
    pruebaconfiguracion.cpp
    ${CMAKE_SOURCE_DIR}/app/configuracion.h
    ${CMAKE_SOURCE_DIR}/app/configuracion.cpp)

target_include_directories(pruebaconfiguracion PRIVATE
    ${CMAKE_SOURCE_DIR}/app ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(pruebaconfiguracion PRIVATE Qt6::Widgets)

add_test(NAME configuracion COMMAND pruebaconfiguracion)
set_tests_properties(configuracion PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(pruebamovimiento pruebamovimiento.cpp)

target_include_directories(pruebamovimiento PRIVATE ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(pruebamovimiento PRIVATE maxcopier_core Qt6::Core)

add_test(NAME movimiento COMMAND pruebamovimiento)

add_executable(pruebaf8 pruebaf8.cpp)

target_include_directories(pruebaf8 PRIVATE ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(pruebaf8 PRIVATE maxcopier_core Qt6::Core)

add_test(NAME f8 COMMAND pruebaf8)

add_executable(pruebainterfaz pruebainterfaz.cpp)
target_include_directories(pruebainterfaz PRIVATE
    ${CMAKE_SOURCE_DIR}/app ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(pruebainterfaz PRIVATE maxcopier_app)

add_test(NAME interfaz COMMAND pruebainterfaz)
set_tests_properties(interfaz PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

if(WIN32)
    # Prueba del canal local (solo Windows: la tubería con nombre es de allí).
    add_executable(pruebacanal pruebacanal.cpp)

    target_include_directories(pruebacanal PRIVATE ${CMAKE_SOURCE_DIR}/core)
    target_compile_definitions(pruebacanal PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601 NTDDI_VERSION=0x06010000)
    target_link_libraries(pruebacanal PRIVATE advapi32)

    add_test(NAME canal COMMAND pruebacanal)

    add_executable(pruebacomandos pruebacomandos.cpp)

    target_include_directories(pruebacomandos PRIVATE
        ${CMAKE_SOURCE_DIR}/shell ${CMAKE_SOURCE_DIR}/core)
    target_compile_definitions(pruebacomandos PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601 NTDDI_VERSION=0x06010000)
    target_link_libraries(pruebacomandos PRIVATE shlwapi)

    add_test(NAME comandos COMMAND pruebacomandos)
endif()
