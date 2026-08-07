# MaxCopier — Propuesta técnica y de UI (v3 · aprobada)

App de escritorio para **Windows** que copia y mueve archivos sustituyendo el diálogo del
Explorador. Inspirada en **SuperCopier 2**, **Ultracopier**, **TeraCopy** y **FastCopy**.

- **Licencia: MIT.** Nada de código copiado de Ultracopier/SuperCopier (son GPLv3): solo ideas
  de funcionalidad y de aspecto, con implementación propia.
- **Idioma del proyecto: español** (UI, comentarios, commits y documentación).
- **Stack: C++20 + Qt 6 Widgets + MinGW‑w64 + CMake.**
- **Motor secuencial: un archivo a la vez.**
- **Sin verificación por hash** (descartado por decisión del usuario).

---

## 1. Referentes

| Referente | Qué tomamos | Qué hacemos distinto |
|---|---|---|
| SuperCopier 2 | ventana compacta ancha y baja, `Desde`/`Hacia`, dos barras con texto encima, pestañas abajo, barra vertical de cola, icono en bandeja | temas oscuro/claro, lista con ruta de destino completa, diálogos más claros |
| Ultracopier | cola editable, pausa/reanudar, límite de velocidad, políticas de colisión y error | gratis y sin plugins; mucho más simple |
| TeraCopy | la lista de copia como centro de la app, modo desatendido (“hacer lo mismo para todo”) | sin checksums ni informes |
| FastCopy | E/S sin búfer para archivos grandes, bloque adaptativo | con UI usable |

## 2. Alcance funcional

### Núcleo
- Copiar y mover archivos y carpetas (escaneo recursivo).
- **Una sola lista de copia**, plana, con `Fuente · Tamaño · Destino` (ruta completa de llegada).
  Cada archivo **desaparece de la lista al terminar de copiarse**.
- **Un archivo a la vez**, en el orden de la lista (reordenable a mano).
- Progreso total + progreso del archivo actual, velocidad instantánea, media y máxima, ETA.
- Pausar / reanudar / cancelar; saltar el archivo en curso.
- **Colisión** → diálogo modal: **Sobrescribir · Renombrar · Saltar** + *Hacer lo mismo para todo*.
- **Error “no encontrado / inaccesible”** → diálogo modal: **Reintentar · Poner al final · Saltar**
  + *Hacer lo mismo para todo*. El resto de errores se irá definiendo más adelante.
- Al pedir una copia mientras hay una lista activa → diálogo:
  **Añadir a la lista actual · Abrir en una ventana nueva** (segunda instancia independiente) · Cancelar.
- Límite de velocidad ajustable en caliente; acción al terminar (nada / cerrar / suspender / apagar).
- Preservar fechas y atributos; rutas largas (`\\?\`); reanudar transferencias incompletas (`.mcpart`);
  comprobar espacio libre antes de empezar.
- Icono en bandeja con progreso, minimizar a la bandeja, arrastrar y soltar.
- Guardar / cargar la lista de copia (`.mclist`).
- Integración con el Explorador: `Copiar con MaxCopier`, `Mover con MaxCopier`, `Pegar con MaxCopier`.
- Temas oscuro, claro y del sistema. Los ajustes se guardan en formato INI en
  `config.mc`, junto al ejecutable, para que el paquete siga siendo portable.

### Fuera de alcance
Verificación por hash y checksums, favoritos de destino, cambiar el destino de una lista en curso,
multihilo, varios trabajos simultáneos en la misma ventana, informes HTML/CSV, VSS, sincronización,
FTP/cloud.

## 3. Stack

| Capa | Elección |
|---|---|
| Lenguaje | **C++20** |
| UI | **Qt 6.6+ Widgets** (no QML): `QSystemTrayIcon`, hojas de estilo `.qss` por tema, `QTranslator` |
| Toolchain | **MinGW‑w64 (GCC 13+)** + **CMake ≥ 3.25** + Ninja; `windeployqt` para empaquetar |
| Motor | librería estática `maxcopier_core` (solo `QtCore`): Win32 `CreateFileW/ReadFile/WriteFile`, bloque adaptativo, `FILE_FLAG_NO_BUFFERING` en archivos grandes; en Linux se compila con un backend `QFile` solo para poder construir y probar el código no‑Win32 |
| IPC | named pipe Win32 (detectar instancia activa y decidir “añadir a la lista” o “ventana nueva”) |
| Instalador | Inno Setup + ZIP portable |
| Tests | Qt Test sobre el core (los ejecuta el usuario) |
| CI | GitHub Actions `windows-latest` (Qt + MinGW): compila y publica el `.exe` como artefacto; `ubuntu-latest` compila el core |

Notas MinGW: Qt publica builds oficiales MinGW; algunas cabeceras COM vienen incompletas
(`shobjidl_core.h`), por lo que la shell extension puede necesitar declaraciones a mano.

## 4. Arquitectura

```
MaxCopier/                  # CMake raíz
├─ core/                    # librería estática, sin GUI
│   ├─ escaneo/             # enumeración recursiva, tamaños, rutas largas
│   ├─ copia/               # motor secuencial, bloques, reanudación, límite de velocidad
│   ├─ politicas/           # colisiones, errores, renombrado
│   ├─ lista/               # ListaDeCopia (modelo de datos), orden, .mclist
│   └─ util/                # formatos, ETA, unidades
├─ app/                     # Qt Widgets: ventana, diálogos, bandeja, temas (.qss), i18n (.ts)
├─ cli/                     # maxcopier-cli.exe (opcional, para scripts)
├─ shellext/                # DLL IExplorerCommand + sparse package
├─ packaging/               # Inno Setup, iconos, windeployqt
└─ tests/                   # Qt Test
```

Flujo: `Escaner` (hilo propio) va llenando `ListaDeCopia` → `MotorDeCopia` (hilo propio) toma el
primer elemento pendiente, lo copia por bloques, emite progreso por señal Qt en cola y **quita el
elemento de la lista** al terminar → la UI solo pinta. Colisiones y errores detienen el motor y
piden decisión a la UI (o aplican la decisión recordada).

## 5. UI

Mockup aprobado: **`docs/mockups/ui-v3.png`** (fuente `ui-v3.html`); recortes `v3-*.png`
(compacta oscuro/claro, expandida oscuro, opciones claro, y los tres diálogos).
Referencias del usuario: `referencia-supercopier2.png` y `referencia-supercopier2-expandida.png`.
Iteraciones anteriores (`ui-v2`, `classic`, `main`, `dialogs`) quedan solo como histórico.

- **Ventana compacta** (~880×175): barra de título con `%` y `origen → destino`; líneas
  `Desde` / `Hacia` (cada una con enlace *Abrir*; el destino **no se puede cambiar**);
  barra total con `archivo N de M`, bytes, `%` centrado, mini-gráfica y velocidad; barra del archivo
  actual con `%` y tiempo restante; badge de la unidad destino con barra de espacio libre; chips de
  `Límite` y `Al terminar` (la política de colisión no es un chip: se ajusta en **Opciones**);
  botones **Pausar · Saltar archivo · Cancelar · +**.
- **Ventana expandida** (~880×470): la misma cabecera + pestañas
  **Lista de copia · Errores · Registro · Opciones**, barra vertical de cola
  (mover al principio/arriba/abajo/al final, añadir, quitar, abrir carpeta, vaciar), buscador y
  barra de estado con transcurrido, media, máxima y restante.
- **Lista de copia**: `Fuente · Tamaño · Destino`. Sin columnas de progreso, estado ni acciones.
  La fila en curso se marca con una flecha; al acabar, la fila se elimina.
- **Diálogos**: colisión (Sobrescribir/Renombrar/Saltar), error de acceso
  (Reintentar/Poner al final/Saltar) y copia con lista activa (Añadir/Ventana nueva/Cancelar),
  los tres con *Hacer lo mismo para todo* / *Recordar mi elección*.
- Detalles: minimizar a la bandeja como botón aparte, arrastrar y soltar en cualquier vista,
  navegación completa por teclado, notificación al terminar.

## 6. Roadmap por fases cortas

Cada fase termina con: **compilar** (Linux + CI Windows) → **actualizar `handoff.md`** → **PR**.
Las pruebas funcionales las hace el usuario con el `.exe` que publica el CI.

| Fase | Entrega | Cómo se prueba |
|---|---|---|
| **F0** | CMake + estructura de carpetas + CI (`windows-latest` con Qt/MinGW, artefacto `.exe`) + ventana vacía con tema | abre el exe: se ve una ventana |
| **F1** | Ventana compacta completa **estática** (cabecera, barras, chips, botones) en oscuro y claro | comprobar aspecto y cambio de tema |
| **F2** | Motor de copia secuencial real de **un archivo** con progreso, velocidad, ETA, pausar/reanudar/cancelar | copiar un archivo grande desde el CLI/botón de prueba |
| **F3** | Escaneo recursivo + **lista de copia** (vista expandida, tabla Fuente/Tamaño/Destino) y borrado de la fila al terminar | copiar una carpeta con muchos archivos |
| **F4** | **Diálogo de colisión** (Sobrescribir/Renombrar/Saltar + hacer para todo) | copiar sobre archivos existentes |
| **F5** | **Diálogo de error** de acceso/no encontrado (Reintentar/Poner al final/Saltar + hacer para todo) | desconectar un USB a mitad de copia |
| **F6** | Instancia única + diálogo **“ya hay una copia en curso”** (añadir / ventana nueva) | lanzar una segunda copia mientras corre otra |
| **F7** | Bandeja, minimizar a bandeja, arrastrar y soltar, notificación al terminar | usar la app minimizada |
| **F8** | **Mover** archivos, preservar fechas/atributos, rutas largas, reanudar `.mcpart` | mover carpetas y rutas muy largas |
| **F9** | Pestañas **Errores** y **Registro**, guardar/cargar `.mclist` | revisar el registro tras una copia con fallos |
| **F10** | **Opciones** persistentes en `config.mc` (INI), límite de velocidad, acción al terminar y tema del sistema; sin i18n | cambiar ajustes y reiniciar la app |
| **F11** | Integración con el Explorador (`IExplorerCommand` + sparse package) e instalador Inno Setup + portable | instalar y usar el menú contextual |

### Contrato de configuración de F10

La configuración de usuario se carga y guarda en `<directorio del ejecutable>/config.mc`.
El fichero usa sintaxis INI y los valores persistidos son tokens estables en inglés,
independientes del idioma visible de la interfaz. Por ejemplo:

```ini
[Transfer]
speedLimitMiB=0
finishAction=nothing
collisionAction=ask
errorAction=ask
activeCopyAction=ask

[Appearance]
theme=system
```

La internacionalización (`QTranslator`, ficheros `.ts` y selector de idioma) no forma parte de F10.
Si la carpeta del ejecutable no permite escritura, se conserva la configuración en memoria y se
muestra un aviso; no se usa una ubicación alternativa.

## 7. Riesgos
- No puedo ejecutar Windows: compilo el core y la app con Qt 6 en Linux (código Win32 tras
  `#ifdef Q_OS_WIN`) y el `.exe` lo produce el CI. **Las pruebas funcionales las hace el usuario.**
- MinGW: cabeceras COM incompletas para la shell extension (F11) y sin SDK de VSS (no se usará).
- El menú contextual de Windows 11 exige identidad de app (paquete MSIX sparse **firmado**):
  hará falta un certificado, autofirmado para desarrollo.

## 8. Decisiones cerradas
Stack Qt 6 + MinGW · UI estilo SuperCopier 2 con temas oscuro, claro y del sistema · licencia **MIT** ·
idioma **español** · **sin hash** · **sin favoritos** · destino no modificable ·
**una lista plana** que se va vaciando · **un archivo a la vez** · diálogos modales para colisión
y errores con *hacer para todo* · fases cortas con PR y handoff al final de cada una.
