@echo off
rem Quita la extensión del Explorador del usuario actual y deja el arrastrar y
rem soltar como estaba. Puede hacer falta cerrar y volver a abrir el Explorador
rem (o reiniciar) para que suelte la DLL.
setlocal
set DLL=%~dp0MaxCopierShell.dll
if not exist "%DLL%" (
    echo No se encuentra MaxCopierShell.dll junto a este archivo.
    exit /b 1
)

regsvr32 /n /u /i:todo "%DLL%"
