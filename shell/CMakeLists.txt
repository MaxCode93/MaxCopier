# Extensión del Explorador: DLL COM de Win32 puro. Aquí no entra Qt, porque
# esto lo carga `explorer.exe` dentro de su proceso.
add_library(maxcopier_shell SHARED
    cliente.h
    cliente.cpp
    comandos.h
    dllmain.cpp
    extension.h
    extension.cpp
    fabrica.h
    fabrica.cpp
    identificadores.h
    modulo.h
    portapapeles.h
    portapapeles.cpp
    registro.h
    registro.cpp
    MaxCopierShell.def
)

set_target_properties(maxcopier_shell PROPERTIES
    OUTPUT_NAME "MaxCopierShell"
    PREFIX ""
    AUTOMOC OFF
    AUTORCC OFF
)

# Solo el protocolo del canal, que es una cabecera sin Qt.
target_include_directories(maxcopier_shell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

# Windows 7 en adelante: hace falta para `SHGetKnownFolderPath` y para los
# verbos Unicode del menú contextual (`CMIC_MASK_UNICODE`).
target_compile_definitions(maxcopier_shell PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601 NTDDI_VERSION=0x06010000)
target_link_libraries(maxcopier_shell PRIVATE shell32 shlwapi ole32 uuid advapi32)

if(WIN32)
    target_sources(maxcopier_shell PRIVATE maxcopiershell.rc)
endif()

if(MINGW)
    # Sin dependencias del runtime de GCC: la DLL vive dentro del Explorador.
    target_link_options(maxcopier_shell PRIVATE -static -static-libgcc -static-libstdc++)
endif()
