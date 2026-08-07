@echo off
rem Da de alta la extensión del Explorador para el usuario actual (no hace
rem falta ser administrador). Con «instalar.cmd menus» solo se añaden las
rem entradas del menú contextual; sin argumentos también se registra el
rem DragDropHandler predeterminado: pegar (Ctrl+V/Shift+Insert), cortar+pegar y
rem arrastrar y soltar de carpetas y unidades pasan por MaxCopier.
setlocal
set DLL=%~dp0MaxCopierShell.dll
if not exist "%DLL%" (
    echo No se encuentra MaxCopierShell.dll junto a este archivo.
    exit /b 1
)

if /i "%~1"=="menus" (
    set MODO=menus
) else (
    set MODO=todo
)

regsvr32 /n /i:%MODO% "%DLL%"
if errorlevel 1 (
    echo No se pudo registrar la extension. Usa el regsvr32 de la misma arquitectura que Explorer.
    exit /b 1
)
echo Registro completado. Si Explorer ya estaba abierto, reinicialo para cargar la extension.
