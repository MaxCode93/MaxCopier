# MaxCopier — Registro de handoff

Estado del proyecto para retomarlo sin contexto previo (por una persona o por otra IA).
**Se actualiza al terminar cada fase, justo antes de abrir el PR.**

---

## 1. Estado actual

- **Fase actual: parte 2 de Archivos a la vez hecha** (§4.8). El pool de motores por ventana ya
  aplica **Archivos a la vez** (1–4, por defecto 1): la ventana reparte la cola entre N motores en
  N hilos, con un **límite de velocidad compartido** (token bucket), diálogos de colisión/error
  uno a la vez, varias filas marcadas con ▶ en la lista y **barra de progreso segmentada** (hasta
  4 segmentos, cada uno con su % y su etiqueta de nombre+velocidad; clic en un segmento selecciona
  el archivo para **Saltar**). Con 1 archivo a la vez el comportamiento y la barra son los de
  siempre. Todo esto se suma a lo anterior: **Método de copia** (Compatible síncrono / Rápido
  asíncrono overlapped), cola ordenable por Fuente/Tamaño/Destino y **guardar/cargar lista**
  (`.mclist`). Las pruebas automáticas de Linux pasan (`configuracion`, `movimiento`, `f8` —con
  el limitador compartido y las múltiples anclas—), la app arranca y el backend Win32 compila
  cruzado con MinGW. **CI restaurado** (§4.9): los workflows `compilar.yml` y `canal.yml` vuelven
  al repositorio, así que el próximo push debe generar el artefacto `.exe` para el retest en
  Windows. Además, dos mejoras nuevas (§4.9): **pausa automática si se desconecta el dispositivo
  de destino** (USB, HDD portable…) con reanudación automática al reconectarlo, y un **overlay de
  carga que bloquea la ventana mientras se enumera la lista** (con botón Cancelar), que se quita
  al terminar la enumeración justo antes de arrancar la copia. Pendientes: retest funcional/
  visual en Windows de F8 + métodos + cola + pool + estas dos, y el retest visual de los chips y
  Opciones de F10. Después sigue **F9** (pestañas Errores y Registro; guardar/cargar `.mclist`
  ya está hecho). **Auditoría de la sesión 28 (§4.10):** sin leaks críticos en el núcleo, el IPC
  ni la DLL; se acotó el drenaje de E/S asíncronas del motor y las esperas de hilos del
  destructor (antes la salida podía colgarse), y **Salir de MaxCopier ahora confirma si hay
  copias en curso**, cancela todo y no deja el proceso en segundo plano. Se conserva la
  arquitectura de bandeja estilo SuperCopier de F7: controlador
  global sin UI principal visible, una ventana y un icono de bandeja por transferencia, menús
  separados, cierre completo desde «Salir de MaxCopier» y redirección de avisos.
- **FS.1**: el menú contextual ya no se queda en «No se ha podido hablar con MaxCopier»; el canal
  con el Explorador se rehizo de arriba abajo (§4.3) y tiene prueba automática.
- Antes: **FS terminada** (integración con el Explorador: extensión de shell, §4.2), con la que
  MaxCopier ya puede ponerse en el sitio del copiador de Windows para el arrastrar y soltar y añade
  «Copiar con MaxCopier…» y «Pegar con MaxCopier» al menú contextual; **F6** (instancia única +
  diálogo «Ya hay una copia en curso» + varias ventanas copiando a la vez); **F5** (diálogo de error
  de acceso: Reintentar / Poner al final / Saltar + *Hacer lo mismo para todo*).
- **CI confirmado en `main` tras F10**: los workflows de Linux, Windows (Qt/MinGW, pruebas,
  empaquetado) y Canal con el Explorador terminaron correctamente tras integrar la auditoría y la
  corrección de enumeración/UI. El artefacto esperado
  sigue trayendo `MaxCopier.exe`, `MaxCopierShell.dll`, las DLL de Qt y los dos `.cmd` de instalación;
  la validación funcional de bandeja, Explorer y Cortar/Mover la hace el usuario en Windows.
- **Rama de trabajo:** `main` es la base (ya es la rama por defecto en GitHub); cada fase va en su
  rama `devin/fase-Fx-...` con PR. La rama `devin/ajustes-ui-funcionales` ya fue integrada mediante
  los PR #4, #5 y #6; no hay cambios pendientes fuera de `main`.
- **Cómo probar F3:** abrir el `.exe` y pulsar **+** (pregunta si añadir *Archivos…* o *Una
  carpeta…* y luego la carpeta de destino), o desde la consola:
  `MaxCopier.exe C:\ruta\carpeta C:\otro\archivo.iso D:\destino` (el último argumento es el
  destino). Con **Detalles** (o **Ctrl+D**) la ventana crece y muestra la lista de copia: filtro,
  barra de cola para reordenar/quitar/vaciar y barra de estado. La fila que se está copiando lleva
  **▶** y desaparece al terminar. **Pausar/Reanudar**, **Saltar archivo** y **Cancelar** siguen
  funcionando (cancelar vacía la lista y corta el escaneo). El tema se cambia con el botón **◑** de
  la barra de título o con **Ctrl+T**.
- **Cómo probar F4:** copiar algo cuyo destino ya exista. Antes de arrancar ese archivo sale el
  diálogo **El archivo ya existe** con el tamaño y la fecha de origen y destino: **Sobrescribir**
  (reemplaza), **Renombrar** (copia como `nombre (2).ext`) o **Saltar** (cerrar el diálogo también
  salta). Con *Hacer lo mismo para todo* la elección vale para el resto de la lista y vuelve a
  `preguntar` al empezar una tanda nueva o al cancelar. La política no se ve ni se toca en la
  ventana: su ajuste irá en **Opciones** (F10) y por defecto es `preguntar`.
- **Bordes redondeados:** la ventana y los diálogos tienen las esquinas curvas (10 px). Como no hay
  marco del sistema, el `border-radius` del `.qss` solo curva el borde pintado: `redondearEsquinas`
  (`app/vistas/esquinas.h`) recorta la ventana con una máscara en cada `resizeEvent` para que los
  widgets de dentro no tapen las esquinas.
- **Cómo probar F5:** meter en la lista un archivo y hacerlo inaccesible antes de que le toque el
  turno (borrarlo, desconectar la unidad o quitarle permisos de lectura); también vale una carpeta
  grande de una memoria USB que se saca a mitad de la copia. Sale el diálogo **No se encuentra el
  archivo** con la ruta y el motivo: **Reintentar** (vuelve a intentar el mismo archivo), **Poner al
  final** (lo manda al final de la lista y sigue con el siguiente) o **Saltar** (lo quita; cerrar el
  diálogo también salta). Con *Hacer lo mismo para todo* la elección vale para el resto de la lista,
  salvo que un archivo ya reintentado vuelva a fallar: entonces se pregunta otra vez para no dar
  vueltas sobre él.

## 2. Qué hay en el repo

| Ruta | Qué es |
|---|---|
| `docs/PROPUESTA.md` | documento maestro: alcance, stack, arquitectura, UI y roadmap |
| `docs/mockups/ui-v3.html` / `ui-v3.png` | **diseño aprobado** de la UI (fuente editable + render) |
| `docs/mockups/v3-*.png` | recortes: compacta oscuro/claro, expandida, opciones, 3 diálogos |
| `docs/mockups/referencia-supercopier2*.png` | capturas de referencia que pasó el usuario |
| `docs/mockups/ui-v2.*`, `classic.*`, `main.png`, `dialogs.png` | iteraciones descartadas (histórico) |
| `README.md`, `LICENSE` | presentación y licencia MIT |
| `handoff.md` | este registro |
| `CMakeLists.txt` | proyecto raíz: C++20, Qt 6 Core/Widgets/Test, subdirectorios, `MAXCOPIER_VERSION` |
| `core/util/formatos.{h,cpp}` | `formatearTamano`, `formatearVelocidad`, `formatearDuracion`, `porcentaje` |
| `core/util/velocimetro.{h,cpp}` | velocidad instantánea (suavizada), media, máxima y ETA a partir de muestras; el instante se le pasa, así se prueba sin esperas |
| `core/escaneo/escaner.{h,cpp}` | escaneo recursivo (`QDirIterator`) que emite lotes de archivos con su ruta de llegada; `cancelar/reiniciar` son banderas atómicas; en modo mover emite la estructura de directorios para crearla solo al terminar bien y bloquea destinos solapados |
| `core/lista/elementodecopia.h` | `ElementoDeCopia` (`fuente`, `destino`, `tamano`) y `ElementosDeCopia`, registrados como metatipos para las señales entre hilos |
| `core/lista/listadecopia.{h,cpp}` | `QAbstractTableModel` de la lista: columnas `▶ · Fuente · Tamaño · Destino`, añadir/quitar/vaciar, reordenar, `ordenarPor` (cabeceras), **varias filas en curso** (▶) que anclan la cola, `quitarTerminada` y `remapearDestinos` (cuando el volumen vuelve con otra letra) |
| `core/copia/metododecopia.h` | enum `MetodoDeCopia` (Sincrono/Asincrono), compartido por el motor y la configuración |
| `core/copia/limitadorvelocidad.{h,cpp}` | límite de velocidad compartido por ventana (token bucket atómico): N motores gastan de un mismo presupuesto; sin límite no acumula deuda |
| `core/politicas/acceso.{h,cpp}` | `AccionError` (Preguntar/Reintentar/PonerAlFinal/Saltar), `motivoInaccesible` (texto del porqué: no existe, no es un archivo, no se puede leer) y el nombre en español de cada política |
| `app/dialogos/dialogoerror.{h,cpp}` | diálogo modal de error de acceso del mockup: barra de título propia, tarjeta con la ruta y el motivo, tres botones y casilla *Hacer lo mismo para todo* |
| `core/politicas/colision.{h,cpp}` | `AccionColision` (Preguntar/Sobrescribir/Renombrar/Saltar), `rutaLibre` (intercala « (2)», « (3)»… antes de la extensión) y el nombre en español de cada política |
| `core/copia/motordecopia.{h,cpp}` | motor de un archivo por bloques (bloque adaptativo 64 KB/1 MB/4 MB): señales `iniciada`, `progreso`, `pausaCambiada`, `terminada`; señales de control atómicas; `copiar(origen, destino, sobrescribir, mover)` rechaza el mismo archivo, copia a un `.mcpart` reanudable, replica metadatos al terminar y elimina el origen solo después de copiarlo |
| `core/copia/backendwin32.{h,cpp}` | backend de E/S Win32 del motor (F8), Qt-libre para poder compilarse cruzado: rutas largas con `\\?\`/`\\?\UNC\`, sin búferes de tamaño fijo, apertura al final para reanudar, reemplazo con `MoveFileExW`, metadatos con `SetFileTime`/`SetFileAttributesW` y **E/S asíncrona overlapped** (ranuras con OVERLAPPED+evento, varias lecturas/escrituras en vuelo) |
| `core/util/atributos.{h,cpp}` | `copiarMetadatos`: replica fechas (modificación/acceso/creación) y atributos (solo lectura, oculto, sistema…) del origen al destino; Win32 en Windows, Qt en el resto |
| `core/util/espaciolibre.{h,cpp}` | comprobación de espacio previa: `volumenDe` (raíz del volumen de una ruta, aunque el destino aún no exista) y `faltasDeEspacio` (agrupa lo pendiente por volumen y calcula cuánto falta; disponibilidad inyectable para pruebas) |
| `pruebas/pruebaf8.cpp` | pruebas de F8: cancelar a media copia deja un `.mcpart` reanudable, parcial completo y parcial obsoleto, metadatos y ruta profunda |
| `app/dialogos/dialogocolision.{h,cpp}` | diálogo modal de colisión del mockup: barra de título propia, tarjetas Origen/Destino con tamaño y fecha, tres botones y casilla *Hacer lo mismo para todo* |
| `app/main.cpp`, `app/ventanaprincipal.{h,cpp}` | arranque (acepta `origen... destino` y `--move`/`--mover`) y ventana sin marco de 580 px: barra de título propia + panel compacto + vista expandida, motor y escáner en sendos `QThread`, copia secuencial, resolución de colisiones/errores, validación de rutas solapadas, limpieza segura de carpetas y modificadores Shift/Ctrl en arrastre |
| `app/vistas/panelexpandido.{h,cpp}` | vista expandida: pestañas **Lista de copia · Errores · Registro · Opciones**, con **Errores** (tabla Hora/Acción/Archivo/Motivo con contador y tooltips) y **Registro** (log de la sesión en vivo con Guardar…) |
| `app/vistas/panelcompacto.{h,cpp}` | cuerpo de la compacta: rutas, dos barras, fila de metadatos, botones y chips interactivos de Límite/Al terminar; los menús emiten cambios por transferencia |
| `app/vistas/barratitulo.{h,cpp}` | barra de título propia: icono de la app, `%`, título, tema, bandeja, minimizar, cerrar (no hay maximizar); arrastre de la ventana |
| `app/bandeja.{h,cpp}` | `QSystemTrayIcon` global sin ventana asociada: Nueva copia/Mover, lista de copias activas, Opciones, Cancelar todas, Salir y avisos globales |
| `app/bandejacopia.{h,cpp}` | `QSystemTrayIcon` individual de cada transferencia: porcentaje numérico, pausa, OK al terminar correctamente, tooltip, restauración, Pausar/Reanudar y Cancelar |
| `app/portapapeles.{h,cpp}` | en Windows, limpia el estado de Cortar solo si el portapapeles aún contiene exactamente los orígenes movidos y la tanda terminó completa |
| `app/vistas/iconos.{h,cpp}` | `iconoDeLaApp()` (los PNG de `:/iconos`) e `iconoDeBandeja()` (el mismo icono con porcentaje, pausa o OK superpuestos) |
| `app/bandejatarea.{h,cpp}` | progreso y superposición de pausa en el botón de la barra de tareas de Windows (`ITaskbarList3`): cada ventana de copia muestra su avance y ⏸ al pausar; fuera de Windows no hace nada |
| `app/recursos/icono.svg`, `icono-pequeno.svg` | fuente del icono: dos hojas y una flecha en el azul de la paleta; la versión «pequeña» (solo hoja y flecha) es la que se rasteriza a 16‑24 px |
| `app/recursos/iconos/maxcopier-*.png` | el icono rasterizado a 16/20/24/32/48/64/128/256 px, que es lo que va en el `.qrc` (así no hace falta el módulo Qt Svg) |
| `app/recursos/maxcopier.ico`, `maxcopier.rc` | icono del `.exe` en Windows; el `.rc` solo se compila en Windows (`enable_language(RC)`) |
| `app/vistas/barraprogreso.{h,cpp}` | barra con relleno en degradado, texto izquierdo, `%` centrado y texto derecho (variantes total y archivo) |
| `app/vistas/minigrafica.{h,cpp}`, `app/vistas/barralibre.{h,cpp}` | polilínea de velocidad y barrita de espacio de la unidad |
| `app/vistas/chip.{h,cpp}`, `app/vistas/unidaddestino.{h,cpp}`, `app/vistas/etiquetaruta.{h,cpp}` | chips `Límite/Al terminar`, insignia de unidad y etiqueta de ruta recortada por el medio |
| `app/vistas/esquinas.{h,cpp}` | `redondearEsquinas`: máscara de esquinas redondeadas (`kRadioVentana` = 10 px) para la ventana y los diálogos, que no tienen marco del sistema |
| `core/ipc/protocolo.h` | formato de los mensajes del canal local (marca `MXC1`, operación copiar/mover, orígenes/destino UTF‑16 y marca opcional de petición desde portapapeles): cabecera **sin Qt**, compartida por la app y la DLL |
| `core/ipc/tuberia.h`, `core/ipc/servidor.h` | la tubería con nombre en Win32 puro: descriptor de seguridad que deja escribir al Explorador (aunque MaxCopier esté elevado), envío con confirmación `MXOK` y bucle de escucha. Lo usan la app y la DLL |
| `app/instanciaunica.{h,cpp}` | instancia única: en Windows sobre `ipc::Servidor`, en Linux con `QLocalServer`/`QLocalSocket`. El segundo proceso (o el Explorador) manda su petición y se cierra |
| `app/diagnostico.{h,cpp}` | registro en `%LOCALAPPDATA%\MaxCopier\maxcopier.log` (la DLL escribe el suyo en `shell.log`) |
| `app/configuracion.{h,cpp}` | ajustes globales persistentes en `<directorio del ejecutable>\config.mc` (INI), con tokens en inglés: límite, acciones, **método de copia** (`sync`/`overlapped`) y **archivos a la vez** |
| `app/vistas/opcionespanel.{h,cpp}`, `app/dialogos/dialogoopciones.{h,cpp}` | editor de Opciones con **menú lateral de categorías** (mockup ui-v3): General, Motor de copia, Colisiones, Errores y Apariencia y temas; incluye **Método de copia**, **Archivos a la vez** y las capacidades «siempre activo» |
| `app/vistas/barraarchivos.{h,cpp}` | barra de progreso del archivo que se divide en hasta 4 segmentos (uno por archivo en curso), cada uno con su % centrado y una mini-etiqueta de nombre+velocidad debajo; el clic en un segmento selecciona el archivo |
| `app/accionfinal.{h,cpp}` | suspensión/apagado nativos de Windows para la acción final |
| `pruebas/pruebacanal.cpp` | prueba del canal de punta a punta (servidor real + cliente como el de la DLL) |
| `pruebas/pruebamovimiento.cpp` | prueba del motor, rechazo del mismo archivo, rutas solapadas, carpetas vacías y cancelación segura del escaneo |
| `shell/` | extensión del Explorador: DLL COM de Win32 puro (`extension.*` con `IShellExtInit`/`IContextMenu`/`IPersistFile`/`IDropTarget`, `fabrica.*`, `portapapeles.*`, `registro.*` en `HKCU`, `cliente.*` que habla con la app y la arranca si hace falta, `dllmain.cpp` e `instalar/desinstalar.cmd`) |
| `.github/workflows/compilar.yml` | CI restaurado (sesión 27): Linux compila y corre `ctest`; Windows (Qt 6.6/MinGW) publica artefacto con `MaxCopier.exe`, `MaxCopierShell.dll`, `windeployqt` y los `.cmd` |
| `.github/workflows/canal.yml` | CI restaurado: Windows/MSVC compila la app, ejecuta `pruebacanal`/`pruebacomandos` y compila la DLL del Explorador |
| `app/vistas/cargando.{h,cpp}` | overlay que bloquea la ventana mientras se enumera la lista: fondo translúcido, tarjeta centrada con barra indeterminada, texto «Enumerando archivos…» y botón **Cancelar** |
| `app/gestordeventanas.{h,cpp}` | dueño de todas las ventanas y del controlador global; crea cada transferencia independiente, reparte peticiones, registra iconos y coordina la salida limpia |
| `app/dialogos/dialogolistaactiva.{h,cpp}` | diálogo «Ya hay una copia en curso»: añadir a la lista / ventana nueva / cancelar + recordar |
| `app/temas/temas.{h,cpp}`, `app/temas/{oscuro,claro}.qss`, `app/recursos.qrc` | temas con la paleta del mockup: `.qss` para los widgets estándar y `Paleta` en C++ para lo que se dibuja a mano |
| `.clang-format`, `.gitignore` | estilo de código y exclusiones |

## 3. Decisiones cerradas (no volver a discutirlas)

- **Stack:** C++20 · Qt 6 Widgets · MinGW‑w64 · CMake + Ninja.
- **Licencia:** MIT. **Idioma del proyecto:** español (UI, comentarios, commits, docs).
- **UI:** estilo SuperCopier 2 redibujado, temas oscuro, claro y del sistema, una sola ventana que se expande.
- **Ajustes F10 en la UI de copia:** los chips **Límite** y **Al terminar** son controles interactivos,
  no simples indicadores. **Límite** abre valores rápidos y **Personalizado** (0–10000 MiB/s);
  **Al terminar** ofrece no hacer nada, cerrar esta copia, suspender o apagar. La selección se aplica
  a la transferencia activa y se guarda también como valor predeterminado en `config.mc`.
- **Panel de Opciones:** comparte el mismo editor entre la pestaña expandida y el diálogo de bandeja;
  usa tarjetas con encabezado, subtítulo, filas descriptivas y desplazamiento. No muestra la ruta del
  fichero ni el aviso «Los cambios se guardan automáticamente…».
- **Sin verificación por hash / checksums.** Sin favoritos de destino.
- El **destino no se puede cambiar** desde la UI (solo abrirlo).
- **Una única lista de copia por ventana**, plana, columnas `Fuente · Tamaño · Destino`; el archivo
  copiado **se borra de la lista**. Sin columnas de progreso, estado ni acciones por fila.
- **Copia secuencial: un archivo a la vez.**
- **Método de copia y concurrencia:** Opciones tiene **Método de copia** (Compatible síncrono por
  defecto / Rápido asíncrono overlapped) y **Archivos a la vez** (1–4, por defecto 1). El método
  asíncrono solo existe en Windows; fuera de él el motor cae al síncrono. El pool real de N
  archivos por ventana (aplicar `Archivos a la vez > 1`) queda para la parte 2, con su UI de
  varias filas en progreso.
- **Colisión** → diálogo: Sobrescribir / Renombrar / Saltar + *Hacer lo mismo para todo*.
- **Error “no encontrado / inaccesible”** → diálogo: Reintentar / Poner al final / Saltar +
  *Hacer lo mismo para todo*. Los demás errores se definirán más adelante.
- **Instancia única de proceso**: abrir MaxCopier otra vez **no hace nada** si no trae orígenes (no
  sale una segunda app ni se duplica la ventana); si trae orígenes, se los pasa a la app que ya está
  corriendo.
- **Varias copias a la vez, cada una en su ventana** del mismo proceso: cada ventana tiene su lista,
  su motor, su escáner, sus botones y su icono propio, y copia en paralelo con las demás (dentro de
  cada ventana sigue siendo un archivo a la vez). No existe una UI principal visible cuando hay
  bandeja: el icono global solo controla nuevas copias, la lista de activas, la cancelación global y
  la salida. Abrir MaxCopier sin argumentos deja el proceso residente en la bandeja.
- Copia pedida con una copia ya en curso → diálogo: **Añadir a la lista actual** / **Abrir en una
  ventana nueva** / Cancelar, con *Recordar mi elección*. Como una lista = un destino, si el destino
  pedido no es el de la ventana ocupada, «añadir a la lista actual» no se ofrece.
- **Bandeja estilo SuperCopier:** hay un icono global sin ventana principal visible y un icono
  individual por cada transferencia minimizada a la bandeja. El menú global contiene **Nueva copia**
  (Copiar/Mover), **Copias activas** (una entrada por transferencia, con **Cancelar**), **Cancelar
  todas**, **Opciones…** y **Salir de MaxCopier**. El menú de cada icono individual contiene
  **Pausar/Reanudar** y **Cancelar**; un clic restaura esa ventana concreta. Acerca de queda fuera de
  F10 y no se inventa un interruptor Activar/Desactivar porque MaxCopier no tiene todavía el
  equivalente de interceptación configurable de SuperCopier.
- **Minimizar una copia:** el botón **↘** de la barra de título esconde la ventana, la retira de la
  barra de tareas y muestra su propio icono con porcentaje numérico, `⏸` al pausar, `OK` al terminar
  correctamente y velocidad en el tooltip. El botón **—** conserva el minimizar convencional de la
  barra de tareas; sin bandeja, **↘** usa ese mismo fallback.
- **Cierre explícito:** el botón **×** de cualquier copia cancela escáner/motor y destruye solo esa UI.
  **Cancelar** desde cualquiera de sus menús hace lo mismo para esa transferencia; las demás siguen
  vivas. «Salir de MaxCopier» marca un cierre definitivo, destruye todas las ventanas (visibles,
  minimizadas o inactivas) y espera a que sus hilos terminen antes de dejar el proceso.
- **Ventana sin marco del sistema** con barra de título propia (como en el mockup): arrastre desde
  la barra. **Ancho fijo de 580 px**; no se puede maximizar (no hay botón de maximizar ni doble
  clic) ni redimensionar (la vista expandida cambia el tamaño desde el código en F3).
- **Proceso de trabajo:** fases cortas; al final de cada fase → compilar (Linux + CI Windows),
  actualizar este handoff, abrir PR. **Devin no hace pruebas funcionales, solo compila**; las
  pruebas las hace el usuario con el `.exe` que publica el CI.

## 4. Roadmap (estado por fase)

| Fase | Contenido | Estado |
|---|---|---|
| F0 | CMake + estructura + CI Windows (Qt/MinGW, artefacto `.exe`) + ventana vacía con tema | **hecha** |
| F1 | Ventana compacta estática completa (oscuro y claro) | **hecha** |
| F2 | Motor secuencial real de un archivo: progreso, velocidad, ETA, pausar/reanudar/cancelar | **hecha** |
| F3 | Escaneo recursivo + lista de copia (vista expandida) + borrado de fila al terminar | **hecha** |
| F4 | Diálogo de colisión (Sobrescribir/Renombrar/Saltar + hacer para todo) | **hecha** |
| F5 | Diálogo de error de acceso (Reintentar/Poner al final/Saltar + hacer para todo) | **hecha** |
| F6 | Instancia única + diálogo “ya hay una copia en curso” + varias ventanas copiando a la vez | **hecha** |
| F7 | Bandeja global estilo SuperCopier, un icono por copia, minimizar a bandeja, arrastrar y soltar, notificación final, icono de la app | **hecha; retest Windows funcional confirmado** |
| F8 | Fechas/atributos, rutas largas, reanudar `.mcpart` | **implementada; retest Windows pendiente** |
| F9 | Pestañas Errores y Registro, guardar/cargar `.mclist` | **hecha** (Errores y Registro en la sesión 34; `.mclist` en la 25) |
| F10 | Opciones persistentes en `config.mc` (INI), límite de velocidad, acción al terminar y tema del sistema; sin i18n | **hecha; CI Linux/Windows/Explorer verde; retest visual de UI pendiente** |
| FS | Integración con el Explorador: menú contextual, pegar y arrastrar y soltar (§4.2) | **hecha** |
| FS.2 | Handler predeterminado de pegar/cortar + modo mover de la app (§4.4) | **hecha; Windows funcional** |
| FS.3 | Auditoría transversal de Cortar/Mover: seguridad de rutas, portapapeles, UI, CLI, carpetas y cobertura (§4.5) | **hecha; CI y Windows funcionales** |
| FS.4 | Enumeración completa antes de copiar, pausa/cancelación durante preparación y rutas legibles en la lista (§4.6) | **hecha; CI y Windows funcionales** |
| F11 | Instalador Inno Setup + portable + menú corto de Windows 11 (`IExplorerCommand` + MSIX) | pendiente |
| F12 | Interceptar **Ctrl+V** del Explorador si los handlers de FS.2 no bastan | cubierto por FS.2; pendiente de validación |
 
## 4.2 FS — Integración con el Explorador (hecha)

Windows **no ofrece ninguna forma soportada de sustituir el copiador**; lo que hacen TeraCopy y
SuperCopier son extensiones de shell. Aquí se ha hecho lo mismo, mirando el código público de
SuperCopier 2 (`SC2C++`, `DragDropHandlers`) y del plugin `catchcopy` de Ultracopier: una **DLL COM
de Win32 puro** (`shell/`) que el Explorador carga en su proceso, junta *qué* archivos y *adónde*, y
se lo manda a MaxCopier por el canal local. **La DLL no copia nada y no enlaza Qt**: dentro de
`explorer.exe` solo hay Win32.

Cuatro enganches, todos en el mismo objeto COM:

1. **Arrastrar y soltar normal** (`IPersistFile` + `IDropTarget`, registrado como `DropHandler` de
   `Directory`, `Folder` y `Drive`). **Esta es la vía que sustituye al copiador de Windows**: al
   soltar sobre una carpeta o una unidad, la transferencia la hace MaxCopier. `Drop` siempre
   devuelve `DROPEFFECT_COPY`, incluso para mover: el origen no debe borrarse hasta que MaxCopier
   confirme que cada archivo ha terminado.
2. **Arrastre con el botón derecho** (`DragDropHandlers`) → «Copiar aquí con MaxCopier».
3. **Menú contextual de archivos, carpetas y unidades** (`ContextMenuHandlers`) → «Copiar con
   MaxCopier…», sin destino: lo pregunta la app.
4. **Fondo de una carpeta** (`Directory\Background`) → «Pegar con MaxCopier», leyendo del
   portapapeles `CF_HDROP` (las rutas) y `Preferred DropEffect` (copiar o cortar).

Detalles que conviene no re-descubrir:

- **Registro por usuario**, todo en `HKCU\Software\Classes`: no hace falta administrador.
  `regsvr32 /n /i:todo` da de alta menús + `DropHandler`; `/i:menus`, solo los menús (por si el
  usuario quiere probar sin tocar el arrastrar y soltar). `instalar.cmd` y `desinstalar.cmd` lo
  envuelven. `DllUnregisterServer` borra también el CLSID: no quedan claves huérfanas.
- **Canal**: en Windows **ya no es `QLocalServer`** (ver §4.3), sino la misma tubería con nombre
  abierta a mano por las dos puntas: `core/ipc/tuberia.h` (crear/escribir) y `core/ipc/servidor.h`
  (escuchar), con el formato de `core/ipc/protocolo.h`. La app lee acumulando hasta tener el mensaje
  entero, contesta `MXOK` y tira la conexión si la marca no cuadra. En Linux (donde solo hay
  instancia única) se sigue usando `QLocalServer`.
- **Nunca se bloquea el Explorador**: el envío va en un hilo suelto que retiene el módulo
  (`DllCanUnloadNow` devuelve `S_FALSE` mientras haya trabajo). Si no contesta nadie, la petición se
  guarda en un archivo y se arranca `MaxCopier.exe --peticion <archivo>` (la DLL lo busca en
  `HKCU\Software\MaxCopier\Aplicacion` y, si no, **a su lado**).
- Las peticiones de mover (cortar y pegar, o arrastre con Shift/efecto mover) llegan con
  `Operacion::Mover`. MaxCopier copia cada archivo, lo verifica por cierre correcto y borra el
  origen después; al terminar elimina solo las carpetas de origen que han quedado vacías. La DLL
  obtiene la carpeta desde el PIDL con la variante que soporta rutas largas y ya no trunca el padre
  de un archivo a `MAX_PATH`; la copia/reanudación de rutas largas en el backend y las
  fechas/atributos siguen siendo F8.
- En **Windows 11** las entradas salen en «Mostrar más opciones»; el menú corto exige
  `IExplorerCommand` + un paquete MSIX firmado y queda para F11.
- **Ctrl+V**: el mecanismo no es `DropHandler`, sino `DragDropHandlers` + `IContextMenu`. La DLL
  recibe la lista y el destino desde `IShellExtInit`, conserva `Preferred DropEffect` y pone su
  comando como acción predeterminada del menú de transferencia; así Ctrl+V, Shift+Insert y el
  pegar del menú llegan a `InvokeCommand` sin inyectarse en `explorer.exe`. La marca
  `MayChangeDefaultMenu` se registra junto al CLSID. Sigue pendiente validar el comportamiento en
  las versiones de Windows del usuario.

**Cómo probar FS** (solo en Windows, con el artefacto del CI): descomprimirlo entero y ejecutar
`instalar.cmd`; reiniciar el Explorador (o la sesión) para que cargue la DLL. Luego: Ctrl+C y Ctrl+V
(la copia debe abrirse en MaxCopier), Ctrl+X y Ctrl+V (debe mover y borrar el origen), Shift+Insert,
pegar desde el menú, arrastrar con Ctrl/Shift, arrastrar con el botón derecho, clic derecho sobre
archivos/carpetas/unidades, MaxCopier cerrado (se debe arrancar solo), una copia ya en curso (debe
salir el diálogo de F6), nombres con acentos y rutas largas, y app elevada frente a Explorer normal.
Comprobar también que pegar una carpeta sobre sí misma o dentro de sí misma se rechaza sin crear
archivos, que una selección con carpeta y un archivo contenido no duplica el movimiento, que una
carpeta vacía llega al destino, y que cancelar o provocar un error conserva el origen y no deja una
estructura vacía creada por la tanda.
Por último ejecutar `desinstalar.cmd` + reiniciar Explorer para comprobar que el arrastrar y soltar
vuelve a ser el de Windows.

## 4.3 FS.1 — El canal que no hablaba (arreglado)

Con FS instalada, el menú salía pero al pulsarlo aparecía «No se ha podido hablar con MaxCopier».
Las causas, por orden de gravedad, y cómo quedan:

1. **Dos nombres de tubería distintos.** La app la abría con `QLocalServer` desde un `std::string`
   (UTF‑8) y la DLL desde un `std::wstring` (UTF‑16) ensanchando byte a byte: en cuanto el nombre de
   usuario de Windows tenía un acento, eran dos tuberías diferentes. Ahora el nombre es siempre
   ASCII (`maxcopier-<usuario saneado>-<huella FNV‑1a>`, `core/ipc/protocolo.h`) y las dos puntas lo
   componen igual.
2. **La ACL de la tubería de Qt.** `QLocalServer` la crea con el descriptor por omisión, que no
   deja escribir a un proceso de integridad más baja ni a otro contexto: con MaxCopier elevado, el
   Explorador se comía un `ERROR_ACCESS_DENIED`. La tubería la abre ahora `core/ipc/tuberia.h` con
   su descriptor (SYSTEM, administradores y el usuario, más la etiqueta de integridad baja
   `S:(ML;;NW;;;LW)`).
3. **Huecos en los que el canal no existía.** El servidor cerraba la instancia atendida antes de
   crear la siguiente, y quien llamaba justo entonces recibía `ERROR_FILE_NOT_FOUND` (o sea, «no
   está abierto»). `core/ipc/servidor.h` crea la siguiente instancia **antes** de atender a la
   actual, y el cliente reintenta un par de veces.
4. **Nadie sabía si el mensaje había llegado.** Escribir en la tubería podía salir bien sin que la
   app entendiera nada. Ahora la app contesta `MXOK` y el cliente solo se da por servido con esa
   respuesta.
5. **El verbo del menú.** `InvokeCommand` solo aceptaba el verbo por índice y devolvía
   `E_INVALIDARG` cuando el Explorador lo manda por nombre (`lpVerbW`, «Mostrar más opciones»), que
   es de donde salía parte del error.
6. **Respaldo.** Si aun así el canal no responde, la DLL escribe la petición en
   `%LOCALAPPDATA%\MaxCopier\peticion-*.mxc` y arranca `MaxCopier.exe --peticion <archivo>`: la
   copia se hace igual. Y todo lo que pasa queda en `shell.log` y `maxcopier.log`, en esa misma
   carpeta (`app/diagnostico.*`).

**Prueba automática**: `pruebas/pruebacanal.cpp` levanta el servidor de verdad y le escribe como lo
hace la DLL (nombre ASCII, cuatro peticiones seguidas, rutas con acentos y cirílico, segunda
instancia que no se queda el canal, y fallo rápido cuando no hay app). Corre en el CI
(`.github/workflows/canal.yml`, MSVC) y aquí se ha comprobado con MinGW bajo Wine.

## 4.4 FS.2 — Handler predeterminado y mover (implementada, retest funcional Windows confirmado)

1. La extensión usa `DragDropHandlers` como punto de entrada del pegar estándar. `QueryContextMenu`
   añade el comando de MaxCopier y llama a `SetMenuDefaultItem` cuando hay una transferencia de
   archivos; Explorer conserva así sus atajos Ctrl+V/Shift+Insert, pero invoca el comando de la DLL.
2. `Preferred DropEffect` distingue copiar de cortar: cortar+pegar se serializa como `Mover` y no
   como `Copiar`. Si el proveedor no publica la marca, un destino distinto de la carpeta de origen
   se trata como transferencia y se usa copiar por defecto.
3. El `IDropTarget` del arrastre normal devuelve siempre `DROPEFFECT_COPY` para impedir que Explorer
   borre el origen antes de tiempo. MaxCopier realiza el borrado después de completar cada archivo;
   si el envío ni siquiera puede encolarse, el `Drop` devuelve `DROPEFFECT_NONE` y no reclama la
   operación de Explorer.
4. El motor acepta `mover`; la validación rechaza el mismo destino y cualquier destino dentro de
   una carpeta de origen, y deduplica selecciones redundantes (carpeta + elemento contenido). Para
   carpetas, el escáner emite la estructura y solo se crean las carpetas vacías al final de una
   tanda completamente correcta. Se comprueba además que cada raíz de origen haya desaparecido;
   si hay saltos, errores, cancelación o elementos inaccesibles, no se declaran éxito ni se eliminan
   carpetas de origen.
5. En Cortar/Pegar la DLL marca la petición como procedente del portapapeles. Tras un movimiento
   completo, la app vacía el portapapeles únicamente si sigue conteniendo exactamente los mismos
   orígenes con `Preferred DropEffect=MOVE`; si el usuario lo cambió, lo deja intacto.
6. La UI respeta Shift para mover y Ctrl para copiar al soltar directamente sobre MaxCopier; el
   arrastre Qt también devuelve COPY para que el borrado lo haga el motor. La CLI admite
   `MaxCopier.exe --move origen... destino` (`--mover` y `-m` son alias).
7. La instalación por defecto (`instalar.cmd` sin argumentos) registra el handler predeterminado;
   `instalar.cmd menus` deja solo los menús y no sustituye el arrastre normal.
8. `InvokeCommand` recibe el primer verbo como `MAKEINTRESOURCE(0)`, que en C++ es un puntero nulo.
   El parser compartido de `shell/comandos.h` comprueba `IS_INTRESOURCE` antes de validar el puntero;
   así «Copiar» y «Pegar» ya no se rechazan con `E_INVALIDARG`. `pruebacomandos` cubre los verbos
   ANSI/Unicode de offset 0, el segundo comando de movimiento y el verbo Unicode por nombre.

**Retest funcional confirmado por el usuario en Windows:** reinstalar la DLL, reiniciar Explorer y
probar copiar/mover, Cortar/Pegar, la bandeja y múltiples transferencias. Esta matriz detallada queda
como guía de regresión: Ctrl+C/Ctrl+V, Ctrl+X/Ctrl+V, Shift+Insert, pegar desde el menú, arrastre
normal con Ctrl/Shift, rutas con acentos, app cerrada y app elevada; un mover no debe dejar el origen
y un error/cancelación sí debe conservarlo.

## 4.5 FS.3 — Auditoría transversal de Cortar/Mover (implementada, CI verde, Windows funcional confirmado)

La revisión cubrió todas las entradas actuales —menú de bandeja, botón **+**, arrastre directo sobre
la UI, arrastre normal y derecho del Explorador, menú contextual, Ctrl+V/Shift+Insert, segunda
instancia, respaldo `--peticion` y línea de órdenes— hasta el escáner, la cola, el motor, la
cancelación y la limpieza final. Se corrigieron estos huecos:

1. **Seguridad de rutas:** antes de escanear se normalizan y deduplican los orígenes; se rechaza
   copiar o mover un archivo sobre sí mismo y una carpeta dentro de sí misma. El escáner y el motor
   repiten la defensa para que una entrada externa no pueda autorrecorrerse ni borrar el destino.
2. **Errores parciales:** el mover ya no precrea toda la estructura del destino. Registra los padres
   creados para un archivo y los elimina si la tanda se cancela o queda incompleta; las carpetas
   vacías se crean solo después de completar todos los archivos. Al final se verifica que las raíces
   realmente hayan desaparecido, de modo que una subcarpeta inaccesible no se reporte como éxito.
3. **Cortar/Pegar:** `Preferred DropEffect` considera mover prioritario aunque el proveedor publique
   también COPY. La marca de origen del portapapeles viaja por IPC, y el estado cortado se limpia de
   forma condicional solo tras un movimiento exitoso. Se corrigió además la lectura de `CF_HDROP`
   directo del portapapeles: no se debe aplicar `GlobalLock` al `HDROP` que devuelve Explorer.
4. **UI y CLI:** Shift/Ctrl al soltar en la ventana seleccionan mover/copiar respectivamente, y
   `--move`, `--mover` y `-m` permiten iniciar un movimiento desde consola. La extensión usa
   `SHGetNameFromIDList(SIGDN_FILESYSPATH)` y un cálculo de padre sin búfer fijo para no truncar la
   carpeta de destino al construir una petición de ruta larga; el soporte completo del backend de
   rutas largas sigue planificado en F8.
5. **Cobertura:** `pruebacanal` comprueba `Mover` y la marca de portapapeles; `pruebacomandos`
   cubre combinaciones de `Preferred DropEffect`; `pruebamovimiento` cubre borrado posterior al
   éxito, rechazo del mismo archivo, estructura de carpetas vacías y cancelación por destino
   solapado. El CI las compila y ejecuta correctamente en Windows.

La única validación que no puede hacerse en este entorno es la ejecución funcional de Qt/Windows:
localmente faltan Qt 6 y Ninja. La compilación y las pruebas automáticas de Linux, Windows y el canal
terminaron correctamente en `main`; el usuario confirmó el retest funcional de Windows descrito en
§4.4. Los casos nuevos enumerados en §4.2 quedan como guía de regresión.

## 4.6 FS.4 — Enumeración y legibilidad de la lista (implementada, CI verde, Windows funcional confirmado)

Se corrigió el arranque prematuro de la transferencia: `VentanaPrincipal` ya no entrega al motor el
primer lote que llega. El escáner termina de enumerar archivos y directorios, emite su resumen final y
solo entonces comienza `copiarSiguiente()`. La lista se sigue llenando y muestra durante ese tiempo
cuántos archivos y carpetas se han encontrado.

Durante la enumeración, **Cancelar** queda disponible y detiene el escáner; **Pausar** pausa y reanuda
la enumeración mediante una bandera atómica, y el menú de la bandeja individual refleja ambos estados.
Cuando el escaneo termina, los tres controles vuelven a corresponder a la transferencia: Pausar,
Saltar y Cancelar.

La tabla expandida desactiva el salto de línea y usa elipsis central para Fuente y Destino, de modo que
conserva la unidad/inicio y el nombre final del archivo (`D:\Users\Max...\lolo.exe`); el tooltip
continúa mostrando la ruta completa.

## 4.7 F8 — Fechas/atributos, rutas largas y reanudar `.mcpart` (implementada, retest Windows pendiente)

1. **Reanudar `.mcpart`.** Toda copia escribe ahora a `<destino>.mcpart` y lo renombra al destino
   solo cuando el archivo está completo. Esto sustituye al lateral `.mcnuevo` del sobrescribir: el
   archivo anterior sigue intacto hasta el final y la reanudación es la misma copiando o
   sobrescribiendo. Al empezar, si el `.mcpart` existe y su tamaño es menor o igual que el del
   origen, la copia continúa desde ahí; si es mayor (el origen cambió o es otro archivo), se
   descarta y se empieza de cero; si es igual, solo se renombra. Cancelar, saltar o un error
   conservan el parcial; al terminar desaparece. Sin verificación por hash (decisión cerrada), el
   tamaño es el criterio de reanudación.
2. **Fechas y atributos.** Al terminar, el motor replica modificación/acceso (y creación en
   Windows) y los atributos: solo lectura, oculto, sistema… (`core/util/atributos.*`). Un fallo
   parcial no fracasa la copia del archivo.
3. **Rutas largas.** En Windows el motor usa un backend Win32 propio (`core/copia/backendwin32.*`,
   Qt-libre): `CreateFileW` con el prefijo `\\?\` (y `\\?\UNC\`), sin búferes de tamaño fijo (los
   bloques los pone el motor), apertura al final para reanudar, reemplazo final con `MoveFileExW`
   (quitando antes el bit de solo lectura del destino) y `SetFileTime`/`SetFileAttributesW` para
   los metadatos. En Linux/macOS se sigue con `QFile`. La E/S queda detrás de una pequeña
   abstracción (`ArchivoIO` dentro del motor) para que pausa, límite y reanudación sean comunes a
   los dos backends.

## 4.8 Métodos de copia, cola y pool de archivos (implementada)

1. **Método de copia.** `core/copia/metododecopia.h` define `MetodoDeCopia` (Sincrono/Asincrono).
   El ajuste vive en `config.mc` como `Transfer/copyMethod` (`sync`/`overlapped`). El síncrono es
   el motor clásico por bloques (QFile en Linux, Win32 en Windows). El asíncrono (`copiarAsincrono`
   en el motor, solo `Q_OS_WIN`) usa el backend Win32 con `FILE_FLAG_OVERLAPPED`: 8 ranuras con su
   OVERLAPPED y evento, varias lecturas/escrituras en vuelo, anillo de buffers, y pausa/límite/
   progreso equivalentes al síncrono. Comparte con él la preparación y finalización (`copiar()`:
   `.mcpart`, reanudación, renombrado, metadatos, mover). Fuera de Windows el asíncrono cae al
   síncrono y copia igual.
2. **Archivos a la vez (parte 2, hecha).** `Transfer/parallelFiles` (1–4, por defecto 1) en
   Opciones. La ventana crea N motores en N hilos (`QMetaObject::invokeMethod` con cola por
   motor); un repartidor (`asignarSiguiente`/`rellenarMotores`) asigna el siguiente pendiente al
   motor libre y resuelve colisiones/errores por archivo (los diálogos se muestran uno a la vez y
   solo bloquean ese archivo). El límite es compartido (`LimitadorVelocidad`, token bucket) y
   **Saltar** apunta al archivo seleccionado (clic en su segmento o en su fila) o al primero en
   curso. La lista admite varias filas en curso (▶) que anclan la cola al reordenar/ordenar;
   `quitarTerminada` retira la fila que acaba de copiar un motor. Con 1 el comportamiento es el
   de siempre.
3. **Ordenar la cola.** Pulsar las cabeceras **Fuente**, **Tamaño** o **Destino** reorganiza la
   cola (`ListaDeCopia::ordenarPor`), con la fila en curso anclada al principio; el segundo clic
   invierte el orden. El orden inicial es el de llegada (escaneo).
4. **Guardar/cargar lista (`.mclist`).** La barra vertical sustituyó «Abrir carpeta de destino» y
   «Vaciar la lista» por **Cargar lista de copia…** y **Guardar la lista de copia actual…**. El
   formato es INI (`MaxCopierLista`: versión, carpetaDestino, archivos y por archivo fuente/
   destino/tamaño). Cargar solo se permite sin copia en curso: sustituye la lista, arranca la
   tanda como copia y los `.mcpart` se reanudan automáticamente, de modo que cancelar, guardar y
   cargar luego continúa donde se quedó.
5. **Barra segmentada.** `BarraArchivos` sustituye a la barra de archivo: con un solo archivo
   pinta como la clásica (izquierda `nombre · tamaño`, % centro, derecha restante, y debajo el
   nombre completo recortado); con 2–4 divide la barra en segmentos iguales con su % centrado y
   debajo una fila de mini-etiquetas `nombre · velocidad` (tooltip con el restante). El clic en
   un segmento selecciona ese archivo para **Saltar**. La fila en pausa se ve atenuada.

## 4.9 CI restaurado, pausa por dispositivo y overlay de enumeración

1. **CI restaurado.** Vuelven `.github/workflows/compilar.yml` (Linux: build + `ctest`; Windows:
   Qt 6.6/MinGW con `jurplel/install-qt-action`, `windeployqt` y artefacto con `MaxCopier.exe`,
   `MaxCopierShell.dll` y los `.cmd` de instalación) y `.github/workflows/canal.yml` (Windows/MSVC:
   compila la app y la DLL y ejecuta `pruebacanal` + `pruebacomandos`). Así el próximo push vuelve
   a generar el `.exe` para el retest del usuario.
2. **Pausa automática por dispositivo desconectado.** Un temporizador de 1 s vigila el destino
   (`QStorageInfo`): si la unidad desaparece durante la copia/enumeración, la ventana pausa los
   motores activos (sin tocar la pausa manual), avisa por la bandeja («Dispositivo desconectado»)
   y bloquea nuevas asignaciones; al reconectarla, reanuda los motores, avisa y reparte la cola.
   Si un error de escritura llega antes que la detección, ese archivo vuelve a la cola sin diálogo
   y la copia se pausa igual; al volver el dispositivo se reanuda desde su `.mcpart` (F8).
3. **Overlay de carga durante la enumeración.** Al iniciar una copia (y mientras el escáner
   arma la lista) la ventana se bloquea con `Cargando`: fondo translúcido, tarjeta centrada con
   barra indeterminada, «Enumerando archivos…» y botón **Cancelar** (cancela la enumeración). Al
   terminar el escaneo se quita el overlay, se desbloquea la UI y arranca la copia (FS.4 sigue
   intacto: la transferencia solo empieza con la lista completa).

## 4.10 Auditoría de la aplicación y salida robusta (sesión 28)

Revisión intensiva de posibles casos no manejados, bugs y fugas de memoria, empezando por el
motor de copia:

1. **Motor asíncrono:** el drenaje de E/S en vuelo tras cancelar podía quedarse colgado si una
   operación a un dispositivo desaparecido no terminaba nunca (el hilo del motor no salía, la
   ventana no se destruía y el proceso quedaba en segundo plano al salir). Ahora el drenaje está
   acotado (~10 s máximo, y corta enseguida si la cancelación está pedida) y cerrar los manejos
   cancela las pendientes.
2. **Destructor de la ventana:** las esperas a los hilos eran ilimitadas (`wait()`); ahora son
   acotadas (`wait(3000)`), de modo que una E/S atascada no impide destruir la ventana ni salir.
3. **Salir de MaxCopier:** con copias en curso (una o varias) se muestra un diálogo de
   confirmación («¿Cancelarlas todas y salir?»); al aceptar se cancela todo, se cierran todas las
   ventanas y, como red de seguridad, un temporizador de 10 s llama a `quit()` si alguna ventana
   no llegara a destruirse. El proceso no queda activo en segundo plano.
4. **Limpieza general:** en la DLL del Explorador, el IPC y las utilidades no se encontraron
   fugas (handles cerrados, `CoTaskMemFree`/`LocalFree` en su sitio, refcounts COM equilibrados).
   Se quitó una llamada deprecada (`QDropEvent::keyboardModifiers()`), que además ensuciaba el CI.
5. **Notas aceptadas (documentadas):** el motor asíncrono reserva hasta 8 bloques × 4 MiB por
   motor (~32 MiB; con 4 motores, ~128 MiB); los fallos de metadatos no fracasan la copia (F8);
   el tamaño del pool («Archivos a la vez») es fijo por tanda (cambia al arrancar la siguiente).

## 4.0 F7 tal como quedó

1. **Controlador global** (`app/bandeja.{h,cpp}`), creado por `GestorDeVentanas`, no representa una
   transferencia y no crea una ventana principal visible. Su menú contextual queda así:

   ```text
   MaxCopier · N copia(s) activa(s)
   ──────────────────────────────
   Nueva copia
       Copiar…
       Mover…
   Copias activas (N)
       [copia activa]
           Cancelar
   Cancelar todas
   ──────────────────────────────
   Salir de MaxCopier
   ```

   Las entradas de **Copias activas** se generan solo mientras la transferencia está escaneando o
   copiando. Clic y doble clic en este icono no intentan mostrar una UI inexistente; cada copia se
   restaura desde su propio icono.
2. **Una ventana por transferencia** (`app/gestordeventanas.{h,cpp}`): cada `VentanaPrincipal` se
   construye con su operación (Copiar/Mover), lista, escáner, motor y `BandejaCopia`. La acción
   **Nueva copia** abre una ventana vacía independiente; las peticiones desde el Explorador, el botón
   **+**, arrastrar y soltar y otra instancia conservan la operación y siguen usando el diálogo de F6
   cuando corresponde.
3. **Icono individual** (`app/bandejacopia.{h,cpp}`): se crea oculto junto con la ventana. Al pulsar
   **↘**, la ventana se oculta, se retira de la barra de tareas y se muestra el icono propio con el
   porcentaje numérico dibujado sobre la marca de MaxCopier. En pausa superpone `⏸`; al terminar
   correctamente superpone `OK`. El tooltip contiene la tanda y la velocidad. El clic izquierdo o
   doble clic restaura solo esa copia. Su menú contextual contiene únicamente **Pausar/Reanudar** y
   **Cancelar**, como el icono de una copia minimizada en SuperCopier. **—** sigue siendo el minimizar
   convencional de la barra de tareas.
4. **Ciclo de cierre:** el botón **×** de cualquier copia cancela escáner/motor, acepta el cierre y
   destruye solo esa ventana. **Cancelar** desde el menú global o desde el icono individual usa el
   mismo cierre de esa transferencia; las demás continúan. `QPointer` retira las ventanas destruidas
   de la lista global sin invalidar acciones antiguas.
5. **Salida real:** **Salir de MaxCopier** marca un cierre definitivo, cancela y oculta todas las
   ventanas visibles o minimizadas (también las inactivas), y programa explícitamente `deleteLater()`.
   El gestor espera a la señal `destroyed` de la última y a que los destructores detengan sus hilos
   antes de llamar a `quit`. Con bandeja, que no queden ventanas no provoca una salida accidental;
   solo la acción explícita de salir termina el proceso.
6. **Avisos:** una tanda que termina minimizada notifica desde su propio `BandejaCopia`; si la ventana
   está visible, el gestor usa el icono global. Así el aviso no se confunde con otra transferencia.
7. **Arranque sin UI principal:** con `QSystemTrayIcon::isSystemTrayAvailable()` el arranque sin
   argumentos deja únicamente el icono global. Si no hay bandeja, se conserva una ventana vacía como
   fallback y **↘** se convierte en minimizar convencional. `setQuitOnLastWindowClosed(false)` permite
   que las copias minimizadas y el modo solo-bandeja sigan vivos.
8. **Icono de la app** (`app/recursos/icono.svg`): dos hojas y una flecha, en el azul de la paleta.
   Va en la ventana y la barra de tareas (`setWindowIcon`), en la marca de cada barra de título, en
   el icono global y como base de los iconos individuales. Los PNG del `.qrc` están generados con
   `rsvg-convert -w N -h N app/recursos/icono.svg -o app/recursos/iconos/maxcopier-N.png` (16, 20 y
   24 px desde `icono-pequeno.svg`) y el `.ico` con `convert` a partir de esos PNG: si se retoca el
   SVG hay que volver a generarlos.

**Cómo probar F7** (en Windows, con el artefacto del CI):

1. Arrancar `MaxCopier.exe` sin argumentos: debe quedar solo un icono global, sin ventana principal.
2. Desde **Nueva copia → Copiar…** y **Mover…**, abrir dos ventanas y comprobar que cada una tiene
   lista/estado independientes y que dos transferencias avanzan en paralelo.
3. Minimizar cada copia con **↘**: deben desaparecer de la barra de tareas y aparecer dos iconos
   individuales. Restaurar cada una desde su propio icono; el menú debe ofrecer solo Pausar/Reanudar
   y Cancelar, y el porcentaje/tooltip deben cambiar durante una copia. Al pausar debe verse `⏸` y
   al terminar correctamente debe verse `OK`.
4. Abrir el menú global: debe mostrar Nueva copia, solo las copias activas con Cancelar, Cancelar
   todas y Salir; no debe aparecer Mostrar/Ocultar MaxCopier ni una “copia principal”. Cancelar una
   copia debe retirar solo su ventana/icono; Cancelar todas debe afectar solo a las activas.
5. Cerrar una copia visible con **×** y comprobar que cancela y desaparece sin afectar a las demás.
   Después usar **Salir de MaxCopier** con copias visibles, minimizadas y en reposo; comprobar en el
   Administrador de tareas que el proceso termina y que no quedan iconos.
6. Completar una copia minimizada y comprobar que el aviso aparece en su icono; completar una visible
   y comprobar el aviso global. Repetir con peticiones del Explorador y con copiar/mover.

## 4.1 F6 tal como quedó

1. **Instancia única** (`core/app/instanciaunica.{h,cpp}`, `QLocalServer`/`QLocalSocket` con un
   nombre por usuario): el primer proceso escucha; el segundo envía sus argumentos (`origenes` +
   `destino`) y **se cierra sin abrir nada**. Sin argumentos, el segundo arranque no duplica el
   proceso ni la bandeja; con bandeja tampoco hay una ventana principal que traer al frente.
   En un escritorio sin bandeja se mantiene el fallback de una ventana vacía.
2. **Gestor de ventanas** (`app/gestordeventanas.{h,cpp}`): dueño de todas las `VentanaPrincipal`.
   Recibe cada petición (línea de órdenes, botón **+** o segunda instancia) y decide:
   - una ventana libre → se usa la que hay;
   - una ventana ocupada con el mismo destino y operación → diálogo **Ya hay una copia en curso**;
   - «ventana nueva» → `new VentanaPrincipal` en el mismo proceso, con su lista, su motor, su
     escáner y su icono, copiando a la vez que las demás;
   - copiar y mover no se mezclan en una misma tanda: la operación se conserva al repartir desde
     el botón **+**, arrastrar y soltar o el Explorador.
3. **Diálogo** (`app/dialogos/dialogolistaactiva.{h,cpp}`, mockup
   `docs/mockups/v3-dlg-lista-activa.png`): dice qué se quiere copiar y qué se está copiando ya
   (origen → destino y `%`), con **Añadir a la lista actual** (solo si el destino coincide),
   **Abrir en una ventana nueva**, **Cancelar** y *Recordar mi elección* (vale mientras la app siga
   abierta; su ajuste persistente irá en Opciones, F10).
4. **Ciclo de vida**: todas las ventanas se destruyen al pulsar cerrar, después de cancelar su
   escáner y motor; el proceso solo permanece vivo si todavía hay bandeja o alguna ventana. El menú
   global tiene «Salir de MaxCopier», que marca el cierre definitivo, cierra todas las ventanas y
   espera sus hilos antes de terminar el proceso.

**Cómo probar F6:** copiar algo grande y, mientras va, lanzar otra copia (con el `.exe` desde la
consola, con el botón **+**, desde **Nueva copia** o abriendo MaxCopier otra vez). Abrir la app sin
argumentos no debe sacar una ventana principal; con bandeja solo debe permanecer el icono global.
Con una copia en curso sale el diálogo: **Añadir a la lista actual** (gris si el destino u operación
son otros), **Abrir en una ventana nueva** (las dos ventanas copian a la vez, en cascada) o
**Cancelar**. Cerrar una ventana cancela y destruye solo esa copia; con bandeja la app sigue viva
hasta **Salir**.

## 5. Notas de entorno

- La máquina de desarrollo de Devin es **Linux**: no puede ejecutar la app ni el código Win32.
  El core y la parte portable de la app se compilan con Qt 6 en Linux; el `.exe` lo genera el CI en
  `windows-latest`. Todo lo específico de Windows va tras `#ifdef Q_OS_WIN`.
- Aun así, la ventana **sí se puede ver en Linux** para comprobar el aspecto: `./build/bin/MaxCopier`
  con `DISPLAY` y captura con `import -window`. En F2 se usó además para copiar archivos de verdad
  (1 y 4,8 GB) y ver progreso, pausa y cancelación; las pruebas en Windows siguen siendo del usuario.
- El entorno de Devin ya trae el toolchain en el blueprint: `cmake`, `ninja-build`, `qt6-base-dev`,
  `libgl1-mesa-dev`, `clang-format` y `g++-mingw-w64-x86-64`.
- La DLL del Explorador **sí se puede compilar aquí** aunque no se pueda ejecutar, cruzando con
  MinGW (así se comprobó en FS):
  `x86_64-w64-mingw32-g++ -std=c++20 -Wall -Wextra -Wpedantic -shared -DUNICODE -D_UNICODE
  -DWIN32_LEAN_AND_MEAN -Ishell -Icore -o /tmp/MaxCopierShell.dll shell/*.cpp
  shell/MaxCopierShell.def -static -static-libgcc -static-libstdc++ -lshlwapi -lole32 -luuid`
  (con `x86_64-w64-mingw32-objdump -p` se comprueba que exporta las cinco funciones `Dll*`).
- Los mockups se editan en HTML (`docs/mockups/*.html`) y se renderizan a PNG con Chrome/Playwright.
- La máquina que retomó el proyecto instaló el toolchain (Qt 6.4, Ninja, clang-format y MinGW
  cruzado; se reinstaló tras un reinicio del entorno). El `ctest` local pasa con las tres pruebas
  (`configuracion`, `movimiento`, `f8`), y el backend Win32 se compila cruzado con
  `x86_64-w64-mingw32-g++ -c core/copia/backendwin32.cpp` sin Qt. Los workflows de CI se
  restauraron en la sesión 27 (§4.9); la validación en Windows (build real de Qt/MinGW/MSVC y
  artefacto `.exe`) la hace el propio GitHub Actions.

## 6. Historial de sesiones

### Sesión 1 — 2026-08-03 · investigación y primera propuesta
Repo vacío. Análisis de Ultracopier/SuperCopier/TeraCopy/FastCopy y del menú contextual de
Windows 11. Primera propuesta (entonces .NET/WPF) y primeros mockups.

### Sesión 2 — 2026-08-03 · stack y estilo
El usuario fija **Qt 6 + MinGW** y pasa la captura de la ventana compacta de SuperCopier 2.
Propuesta reescrita a C++/Qt/CMake; mockup `classic.*` en oscuro y claro.

### Sesión 3 — 2026-08-03 · rediseño UI v2
El usuario pasa la vista expandida de SuperCopier 2 y pide rediseño libre. Mockup `ui-v2.*`
(chips, agrupación por trabajos, acciones en fila, buscador, opciones como pestaña).

### Sesión 4 — 2026-08-03 · F0
Andamiaje del proyecto: CMake raíz + `core` (librería estática con utilidades de formato) + `app`
(Qt Widgets, ventana con temas oscuro/claro desde `.qrc`) + `tests` (Qt Test) + CI en GitHub Actions
que compila en Windows con Qt 6.8/MinGW y publica el `.exe` empaquetado con `windeployqt`.
Compilado en Linux; las pruebas funcionales las hace el usuario con el artefacto del CI.

### Sesión 5 — 2026-08-03 · F1
Ventana compacta estática completa: barra de título propia (marca, `%`, título, botones), rutas
`Desde`/`Hacia` con enlace *Abrir*, barra total con mini-gráfica de velocidad, barra del archivo
actual con tiempo restante, insignia de unidad de destino, chips `Límite`/`Al terminar`
y fila de botones **Pausar · Saltar archivo · Cancelar · + · Detalles**. Widgets propios en
`app/vistas/`; `.qss` de los dos temas reescritos y paleta en C++ para los widgets dibujados a mano.
Cambio de tema con el botón **◑** o **Ctrl+T**. Datos de ejemplo del mockup formateados con el
núcleo. Sin motor de copia todavía: los botones no hacen nada (emiten señales para F2).

### Sesión 6 — 2026-08-03 · F2
Motor de copia real de un archivo: `core/copia/MotorDeCopia` copia por bloques en su propio hilo y
emite progreso cada ~150 ms; `core/util/Velocimetro` calcula velocidad suavizada, media, máxima y
ETA. La ventana ya no muestra datos de ejemplo: arranca en «Sin copia en curso», se alimenta de las
señales del motor y pinta rutas reales, bytes, `%`, velocidad, mini-gráfica, tiempo restante y la
unidad de destino con su espacio libre (`QStorageInfo`). **Pausar/Reanudar** cambia el botón,
**Saltar** y **Cancelar** abortan y borran el archivo a medias; los tres botones se deshabilitan
cuando no hay copia. La copia se pide con **+** (dos diálogos: archivo y carpeta) o con
`MaxCopier.exe origen destino`. Decisiones de esta fase: si el destino existe, la copia falla con
aviso (el diálogo de colisión es F4, y así no se sobrescribe nada por accidente); no se dejan
parciales (los `.mcpart` son F8); el backend Win32 sin búfer también queda para F8, de momento
`QFile` en los dos sistemas. Probado en Linux con archivos de 1 y 4,8 GB (progreso, pausa, cancelar
y colisión) además de `ctest`.

### Sesión 7 — 2026-08-03 · ancho de ventana y F3
Primero un ajuste pedido por el usuario: la ventana pasa a **700 px de ancho fijo** y deja de poder
maximizarse (fuera el botón **□** y el doble clic en la barra) y de redimensionarse.

Después **F3**: `core/escaneo/Escaner` recorre carpetas recursivamente en su propio hilo y emite
lotes (200 archivos o 150 ms) que la ventana va metiendo en `core/lista/ListaDeCopia`, el modelo de
tabla de la única lista de copia. La copia dejó de ser de un archivo: la ventana toma siempre la
fila 0, crea las subcarpetas del destino con `mkpath` y, cuando el motor termina, **borra la fila** y
pasa a la siguiente. Saltar y los errores también quitan la fila y descuentan su tamaño del total
(el diálogo de error es F5); cancelar vacía la lista y corta el escaneo. Nueva vista expandida
(`app/vistas/PanelExpandido`) con las cuatro pestañas del mockup —Errores, Registro y Opciones son
marcadores—, tabla `▶ · Fuente · Tamaño · Destino`, filtro por nombre, barra vertical de cola
(al principio/subir/bajar/al final, añadir, quitar, abrir carpeta, vaciar) y barra de estado con
transcurrido, media, máxima y restante; se abre y cierra con **Detalles** o **Ctrl+D** y solo cambia
el alto (700 px de ancho siempre). El botón **+** pregunta si añadir archivos o una carpeta y la
línea de órdenes acepta varios orígenes (el último argumento es el destino). Decisiones de la fase:
las carpetas vacías no se replican (la lista es solo de archivos) y la fila en curso ancla el
principio de la lista (no se mueve ni se quita mientras se copia). Probado en Linux copiando una
carpeta de 58 archivos y 14,9 GB (progreso, lista vaciándose, fila en curso marcada) más `ctest`.

### Sesión 8 — 2026-08-03 · F4
Diálogo de colisión. La política vive en `core/politicas/colision.h` (`AccionColision` y `rutaLibre`,
que busca `nombre (2).ext`, `(3)`… respetando la extensión) y la decisión la toma la **ventana**, no
el motor: antes de pasarle la fila 0 comprueba si el destino existe y, si existe, aplica la política
«para todo» o abre `app/dialogos/DialogoColision` (modal, con barra de título propia, tarjetas
Origen/Destino con tamaño y fecha, y la casilla *Hacer lo mismo para todo*). Saltar quita la fila y
descuenta su tamaño del total sin molestar al motor; renombrar cambia solo el destino de esa copia;
sobrescribir se le pide al motor con el nuevo parámetro `copiar(origen, destino, sobrescribir)`.
Decisiones de la fase: **sobrescribir no escribe encima del archivo bueno** — el motor copia en un
`destino.mcnuevo` lateral y lo pone en su sitio al terminar, así cancelar o un error dejan intacto lo
que ya había; cerrar el diálogo (Esc) equivale a **Saltar**; la política «para todo» se olvida al
empezar una tanda nueva o al cancelar. Como el diálogo es modal pero deja correr los eventos, la copia se marca como activa antes
de preguntar para que los lotes que llegue el escáner no arranquen una segunda copia. `ctest` con dos
pruebas nuevas del motor (sobrescritura y cancelación de una sobrescritura) y una batería para
`rutaLibre`.

### Sesión 9 — 2026-08-03 · ajustes de UI pedidos por el usuario
Tres cambios sobre F4, cada uno en su PR: el diálogo de colisión se **centra en la pantalla** (al no
tener marco del sistema salía en la esquina, y centrarlo sobre la ventana no vale porque cuando la
copia viene de la línea de órdenes la ventana aún no está colocada); **fuera el chip Colisión** de la
cabecera (la política sigue por dentro con `Preguntar` por defecto y su ajuste irá en Opciones, F10);
y la fila de botones se reordena — **Detalles** a la izquierda y **Pausar · Saltar archivo ·
Cancelar · +** a la derecha — con la ventana a **580 px** de ancho.

### Sesión 10 — 2026-08-04 · F5
Diálogo de error de acceso. La política vive en `core/politicas/acceso.h` (`AccionError` y
`motivoInaccesible`) y, como en F4, la decisión la toma la **ventana**: antes de pasarle la fila 0 al
motor comprueba que el origen se pueda leer y, si no, aplica la política «para todo» o abre
`app/dialogos/DialogoError`. Reintentar deja la fila donde está y vuelve a intentarlo; poner al final
la manda al final de la lista (con un solo archivo equivale a saltar, si no sería un bucle); saltar
la quita y descuenta su tamaño del total. El mismo diálogo sale ahora cuando el motor termina en
`Error` —antes solo había un aviso—: ahí la fila ya se ha quitado, así que reintentar o poner al
final la vuelven a meter en la lista. Decisiones de la fase: cerrar el diálogo (Esc) equivale a
**Saltar**; las políticas «para todo» se olvidan al empezar una tanda nueva o al cancelar; y
**reintentar para todo solo reintenta una vez por archivo** —si el mismo archivo vuelve a fallar se
pregunta otra vez— para no dejar la lista girando sobre un archivo que no vuelve. Compilado en Linux.
Nota de entorno: el repo clonado **ya no trae `tests/` ni `.github/workflows/compilar.yml`** (el
historial se reescribió a dos commits), así que en esta fase no hubo `ctest` ni artefacto `.exe`.

### Sesión 11 — 2026-08-04 · FS (extensión del Explorador)
El usuario pide que la shell «quede hecha» para que MaxCopier sustituya al copiador de Windows, y
deja las decisiones técnicas al asistente. Se estudiaron el SuperCopier 2 público (Delphi + la DLL
`SC2C++`, que registra `DragDropHandlers`) y el plugin `catchcopy` de Ultracopier: los dos hacen lo
mismo —una DLL COM en el Explorador que solo recolecta rutas y se las manda a la app—, y así se ha
hecho aquí (§4.2). Decisiones de la fase: **una sola DLL Win32 pura sin Qt** con los cuatro
enganches; **`DropHandler`** como la vía que de verdad sustituye al copiador (nada de inyectar en
`explorer.exe`, que es lo que SuperCopier acabó quitando); **registro en `HKCU`**, sin
administrador, con `/i:menus` para probar sin tocar el arrastrar y soltar; **protocolo binario
propio** (`core/ipc/protocolo.h`) en vez de `QDataStream`, porque la DLL no puede enlazar Qt, con la
operación copiar/mover metida en el mensaje y en `GestorDeVentanas`; **destino vacío = la app
pregunta**, para el «Copiar con MaxCopier…» del menú; **envío en un hilo aparte** que arranca la app
si hace falta, para no colgar el Explorador; y **`Drop` siempre responde `COPY`**, nunca `MOVE`, no
sea que el origen borre los archivos antes de que se hayan copiado. Se recupera el CI: Windows
(Qt 6.6/MinGW) publica `.exe` + `.dll` + `windeployqt` + los `.cmd`, y Linux compila la app.
Comprobado aquí: la app en Linux, la DLL con MinGW cruzado (exporta las cinco funciones) y el
protocolo con un programita de ida y vuelta. Lo funcional lo prueba el usuario en Windows.

### Sesión 12 — 2026-08-05 · FS.1 (el canal que no hablaba)
El usuario cuenta que el menú aparece pero al pulsarlo sale «No se ha podido hablar con MaxCopier».
Se repasa el camino entero (menú → DLL → tubería → app) y salen cinco fallos encadenados, más la
falta de respaldo y de diagnóstico: están en §4.3 con lo que se ha hecho en cada uno. Decisiones de
la sesión: **la tubería de Windows se abre a mano** y no con `QLocalServer` (es la única forma de
ponerle una ACL que deje escribir al Explorador con MaxCopier elevado); **el nombre del canal es
ASCII** con huella del usuario; **confirmación `MXOK`** para no dar por buena una petición que no ha
llegado; **respaldo por archivo + `--peticion`** para no perder nunca una copia; **registros en
`%LOCALAPPDATA%\MaxCopier`** porque dentro del Explorador no hay otra forma de ver qué pasa; y
**testigo de instancia única con un mutex con nombre**, que también vale cuando la otra instancia
está elevada. Comprobado aquí: la app en Linux (incluida una segunda instancia que le pasa la copia
a la primera), la DLL con MinGW cruzado y la prueba del canal bajo Wine. Lo funcional —menú,
arrastrar y soltar, MaxCopier elevado— lo prueba el usuario en Windows.

### Sesión 4 — 2026-08-03 · ajustes finales y aprobación
El usuario recorta el alcance: sin hash, sin favoritos, destino fijo, lista única plana que se vacía,
un archivo a la vez, diálogos modales de colisión y error, licencia MIT, español, fases cortas con
PR y handoff. Resultado: `ui-v3.*` (mockup aprobado), `PROPUESTA.md` v3, `README.md`, `LICENSE`.

### Sesión 13 — 2026-08-05 · FS.2 (handler predeterminado y mover)
El usuario informa de que la extensión anterior no estaba sustituyendo las acciones normales de
Explorer. La causa principal era que `QueryContextMenu` solo añadía un comando visible: no lo ponía
como acción predeterminada, y el código clasificaba el pegar con `m_origenes` como una copia aunque
el portapapeles indicara cortar. Se implementa el patrón de `DragDropHandlers` usado por
SuperCopier: conservar `Preferred DropEffect`, añadir el comando de transferencia, marcar
`MayChangeDefaultMenu` y llamar a `SetMenuDefaultItem` para redirigir Ctrl+V/Shift+Insert/pegar del
menú a `InvokeCommand`. El `IDropTarget` sigue devolviendo `COPY` para que Explorer no borre el
origen mientras MaxCopier trabaja.

La app deja de rechazar `Operacion::Mover`: copia cada archivo y elimina su origen después de un
resultado correcto; el escáner prepara directorios vacíos en modo mover y al final se quitan solo
los directorios de origen que han quedado vacíos. La compilación local queda pendiente porque esta
máquina no tiene Qt 6 ni el compilador MinGW; la validación funcional requiere el artefacto Windows
y reiniciar Explorer después de `instalar.cmd`.

### Sesión 14 — 2026-08-05 · ciclo de vida de ventanas y bandeja central
El usuario detecta que la ventana principal se cerraba visualmente pero dejaba el proceso en segundo
plano, y que las copias secundarias compartían un ciclo de cierre incorrecto. Se corrige la
arquitectura: `GestorDeVentanas` crea un solo `QSystemTrayIcon` y registra la primera ventana como
principal; las demás se registran como copias secundarias dentro del submenú **Copias secundarias**.
La principal sobrescribe `closeEvent()` para ocultarse y aceptar solo el cierre explícito desde
**Salir de MaxCopier**. Una secundaria acepta el botón de cierre después de cancelar escáner y motor,
por lo que se destruye solo esa UI y se elimina del menú. La salida global cierra todas las ventanas,
espera sus hilos y solo entonces termina el proceso. Los estados y avisos de cada ventana se envían
al icono central. El CI del PR confirmó la compilación Linux/Windows, el empaquetado y el canal Win32;
la prueba funcional de estos caminos sigue pendiente en Windows con el artefacto generado.

### Sesión 15 — 2026-08-05 · regresión del verbo Copiar/Pegar del shell
El usuario prueba FS.2 y observa que Explorer deja de ejecutar la copia/pega, mientras MaxCopier
tampoco recibe la petición; el menú contextual también queda sin efecto. La causa estaba en
`Extension::indiceDelVerbo`: Windows representa el primer comando con `MAKEINTRESOURCE(0)`, cuyo valor
es `nullptr`. El código nuevo comprobaba el puntero antes de `IS_INTRESOURCE`, por lo que devolvía
`E_INVALIDARG` para «Copiar» y «Pegar» (offset 0). Como FS.2 había convertido ese comando en la acción
predeterminada, tampoco quedaba un fallback visible de Explorer. Se extrae el parser a
`shell/comandos.h`, se corrige el orden de comprobación y se añade `pruebacomandos` al CI; cubre
ANSI/Unicode, offset 0, movimiento y verbo Unicode por nombre. La primera ejecución del CI detectó
además un orden incorrecto de headers Win32 en el test; se corrige en la sesión siguiente. Falta que
el usuario reinstale la DLL, reinicie Explorer y confirme la copia, pega, corta/pega y menú contextual
en su máquina.

### Sesión 16 — 2026-08-05 · orden de headers Win32 del test
La compilación del PR del parser falló en MSVC y MinGW porque `shell/comandos.h` incluía
`shellapi.h` antes de `windows.h`; los headers del SDK necesitan que `windows.h` defina primero
`EXTERN_C` y tipos base. Se mueve `windows.h` delante de `shellapi.h` y `shlwapi.h`. El cambio no
altera el comportamiento del parser, solo permite compilar de forma consistente la DLL y
`pruebacomandos`. El CI final de Windows, Linux y el canal Explorer queda verde; sigue pendiente el
retest funcional en Windows con la DLL instalada.

### Sesión 17 — 2026-08-05 · cierre completo desde el menú de la bandeja
El usuario detecta que **Salir de MaxCopier**, elegido desde el icono de la bandeja, oculta la
interfaz pero puede dejar el proceso vivo. La ventana principal ya llega a esta ruta escondida, por
lo que no se debe confiar en que `QWidget::close()` programe `WA_DeleteOnClose` en todos los casos.
`VentanaPrincipal::cerrarDefinitivamente()` ahora cancela explícitamente el escáner y el motor,
oculta la ventana y llama a `deleteLater()`. El gestor conserva la salida condicionada a la señal
`destroyed` de la última ventana, de modo que el proceso termina después de que los hilos hayan
quedado detenidos. Falta validar en Windows que **Salir de MaxCopier** cierra también el proceso con
la principal escondida y mientras hay una copia activa.

### Sesión 18 — 2026-08-05 · arquitectura de bandeja estilo SuperCopier
El usuario aprueba la comparación con `gligli/SuperCopier2` y pide implantar ese comportamiento en
MaxCopier. Se reemplaza el concepto de ventana principal por un controlador global de bandeja sin UI
visible (`app/bandeja.{h,cpp}`), cuyo menú crea Copiar/Mover, lista copias activas, cancela todas y
sale. Cada `VentanaPrincipal` se vuelve una transferencia independiente y recibe un
`BandejaCopia` (`app/bandejacopia.{h,cpp}`): al minimizar con **↘** desaparece de la barra de tareas,
su icono dibuja el progreso y su menú ofrece Pausar/Reanudar y Cancelar; el clic restaura solo esa
transferencia. Los avisos terminados se enrutan al icono individual si está minimizado y al global si
la ventana está visible. Se conserva el cierre explícito de la sesión 17: **Salir de MaxCopier**
destruye visibles, minimizadas e inactivas, y el proceso solo termina al destruirse la última ventana.
El arranque sin argumentos queda residente solo en la bandeja cuando el entorno la ofrece; sin
bandeja se conserva una ventana vacía de fallback. La compilación local no se puede ejecutar en esta
máquina porque faltan Qt 6 y Ninja; queda pendiente confirmar compilación CI y retest funcional en
Windows con dos copias simultáneas, iconos individuales y salida desde la bandeja.

### Sesión 19 — 2026-08-05 · auditoría completa de Cortar/Mover

El usuario pide revisar Cortar/Mover «en todos lados» y corregir lo que falte. Se auditan la UI,
la bandeja, el arrastre Qt, todos los caminos de la extensión del Explorador, el portapapeles, la
segunda instancia, el respaldo por archivo, la CLI, el protocolo, el escáner, la cola, el motor,
las colisiones, los errores y la cancelación. Se crea la rama `devin/auditoria-mover` desde
`origin/main`, porque el PR anterior de F7 ya estaba fusionado.

Se implementan: validación y defensa contra rutas iguales o anidadas; deduplicación de orígenes;
limpieza de padres creados en movimientos incompletos; preservación transaccional de carpetas
vacías; verificación de que desaparezcan las raíces; respeto de Shift/Ctrl en el arrastre directo
de la UI; `--move`/`--mover`/`-m`; prioridad correcta de `Preferred DropEffect`; lectura correcta
de `CF_HDROP`; propagación IPC de «desde portapapeles» y limpieza condicional tras Cortar/Pegar;
mejor extracción de padres largos en la DLL; y devolución de `DROPEFFECT_NONE` si un Drop no pudo
encolarse. Se agregan `pruebamovimiento` y comprobaciones adicionales de canal/comandos.

La verificación local solo pudo ejecutar el round-trip C++ del protocolo y `git diff --check`:
este entorno no tiene Qt 6 ni Ninja. El CI posterior a la auditoría confirmó la compilación Qt/MinGW,
Linux, el canal MSVC, la DLL, el nuevo test de movimiento y el empaquetado. Queda el retest funcional
en Windows con Explorer reiniciado.

### Sesión 20 — 2026-08-05 · CI final y corrección de compilación Windows

El CI de FS.3 detectó primero que `app/portapapeles.cpp` usaba `CFSTR_PREFERREDDROPEFFECT` sin
incluir `shlobj.h`. Se añadió el header y se integró la corrección mediante el PR #5. La ejecución
posterior al merge confirmó Linux, Windows/Qt-MinGW, las pruebas del canal, la extensión de Explorer
y el empaquetado. El estado funcional pendiente es exclusivamente el retest manual en Windows:
reinstalar la DLL, reiniciar Explorer y recorrer los casos de §4.2.

### Sesión 21 — 2026-08-05 · enumeración completa y rutas legibles

El usuario detecta que la copia arrancaba con el primer lote del escáner, antes de terminar de
enumerar toda la lista, y que durante esa ventana Pausar/Cancelar podían aparecer deshabilitados.
También observa que Fuente y Destino se truncaban a `D:\...` en la lista expandida.

Se corrige FS.4: la UI acumula todos los lotes y espera `Escaner::terminado` antes de iniciar el
motor; durante la preparación muestra los contadores provisionales, habilita Cancelar y permite
pausar/reanudar la propia enumeración con una bandera atómica. La bandeja individual expone el mismo
estado. La tabla usa truncado central sin salto de línea y mantiene el tooltip completo para cada ruta.
El CI posterior al merge confirmó Linux, Windows/Qt-MinGW, el canal de Explorer y el empaquetado.
Queda pendiente únicamente el retest manual en Windows.

### Sesión 22 — 2026-08-05 · F10 (opciones persistentes, límite y tema)

El usuario confirma que el retest funcional de Windows de la bandeja, Explorer, copiar/mover,
Cortar/Pegar y múltiples transferencias ya está completado. Se implementa F10 sin i18n: una clase
`app/configuracion.{h,cpp}` carga y guarda automáticamente los ajustes en
`<directorio del ejecutable>/config.mc`, usando `QSettings::IniFormat` con grupos `[Transfer]` y
`[Appearance]`. Los valores serializados son tokens estables en inglés (`nothing`, `close`,
`suspend`, `shutdown`, `ask`, `overwrite`, `rename`, `skip`, `retry`, `moveToEnd`, `system`, etc.),
mientras la interfaz permanece en español. Si la carpeta del ejecutable no permite escritura, la
aplicación avisa y no cambia a una ruta alternativa; los valores siguen activos en memoria durante
la sesión.

La pestaña **Opciones** deja de ser un marcador y también se abre desde el menú global de la bandeja.
Persisten las políticas de colisión/error, la elección ante otra copia activa, el límite por
transferencia y la acción final. El límite se aplica en caliente desde el motor mediante una bandera
atómica, sin bloquear Pausar/Saltar/Cancelar. Se añaden los temas Oscuro/Claro/Sistema; el modo
Sistema reacciona a cambios de apariencia del sistema y `Ctrl+T` selecciona un tema explícito.

Las acciones finales solo se consideran tras una tanda completa: cerrar afecta a esa transferencia;
suspender/apagar se coordinan desde `GestorDeVentanas`, esperan a que no queden otras copias activas
y se cancelan si otra transferencia termina con error o cancelación. En Windows usan las APIs nativas;
en otros sistemas se informa de que no están disponibles. Se agregan pruebas de persistencia del INI
y del contrato del límite, y el workflow Linux ejecuta ahora `ctest`. La validación local queda
limitada porque esta máquina no tiene Qt 6 ni Ninja; falta confirmar el CI de la rama antes del PR.

### Sesión 23 — 2026-08-05 · F10: chips funcionales y rediseño de Opciones

El usuario aclara que los chips **Límite** y **Al terminar** ya existían en la UI de copia, pero no
respondían. Se convierten en controles interactivos dentro de `PanelCompacto`: el chip de límite abre
un menú con valores rápidos (`Sin límite`, 1, 5, 10, 25, 50, 100, 250, 500 y 1000 MiB/s) y
**Personalizado…** entre 0 y 10000 MiB/s; el chip de acción final ofrece no hacer nada, cerrar esta
copia, suspender o apagar. La elección actualiza la copia activa cuando la hay y se persiste mediante
`Configuracion` en el INI junto al ejecutable. Los tokens del fichero siguen siendo ingleses; solo se
mejoran los nombres visibles en español.

La clase `Chip` ahora emite `clicado()` y muestra cursor/estado hover. `VentanaPrincipal` conecta las
selecciones con el motor y con `config.mc`; al iniciar una tanda nueva se sigue capturando el valor
predeterminado, y durante una tanda el cambio de **Al terminar** actualiza `m_accionFinalTanda`.
La etiqueta de cierre es **cerrar esta copia**, porque no garantiza cerrar el proceso completo si la
bandeja global sigue viva.

`OpcionesPanel` se rediseña para la pestaña expandida y el diálogo de bandeja compartiendo exactamente
el mismo editor: tarjetas para **Copias y rendimiento** y **Apariencia**, encabezados y subtítulos,
filas con título/ayuda, controles de ancho consistente, scroll vertical y menús QSS propios para ambos
temas. Se elimina por completo `m_ruta` y el texto «Los cambios se guardan automáticamente…»; la
persistencia en `config.mc` no cambia. El diálogo de opciones pasa a 560×500 px para alojar el diseño.

La primera ejecución del PR #4 fue auto-mergeada antes de que terminara CI y dejó un error de Qt MOC:
los miembros de `PanelCompacto` habían quedado bajo `private slots:`. El PR #5 añadió la separación
correcta y fue validado en Linux y Windows; el PR #6 corrigió la etiqueta de cierre. Los tres PR fueron
integrados automáticamente. El estado final de `main` es `06dee69`; el workflow `Compilar` pasó Linux,
Windows/Qt-MinGW, pruebas y empaquetado, y `Canal con el Explorador` también pasó.

**Retest manual pendiente en Windows para esta sesión:** instalar el artefacto, abrir una copia y
pulsar ambos chips; comprobar que el menú abre, que el límite rápido y personalizado modifican la
velocidad, que la acción final se refleja en la copia activa, que `config.mc` conserva los tokens
ingleses, que F10/`Opciones…` muestra las tarjetas sin el texto de ruta y que los temas oscuro, claro
y sistema mantienen el diseño. El retest funcional anterior de bandeja, Explorer, copiar/mover,
Cortar/Pegar y múltiples transferencias sigue confirmado por el usuario.

### Sesión 24 — 2026-08-06 · F8 (fechas/atributos, rutas largas y `.mcpart`)
Se implementan los tres bloques de F8 en el motor (§4.7). La copia pasa por un `.mcpart` lateral
que se reanuda por tamaño (el lateral `.mcnuevo` del sobrescribir desaparece; el destino solo se
toca al final, con la misma garantía de antes), el motor replica fechas y atributos al terminar, y
en Windows se sustituye `QFile` por un backend Win32 con rutas largas `\\?\`, sin búferes fijos,
reemplazo final con `MoveFileExW` y metadatos con `SetFileTime`/`SetFileAttributesW`. Nueva prueba
`pruebas/pruebaf8.cpp`: cancelar a media copia deja un parcial reanudable (con el mismo contenido
al reanudar), un parcial completo solo se renombra, un parcial más grande que el origen se descarta,
el destino conserva fecha y solo lectura, y se copia hacia una ruta de más de 260 caracteres sin
truncarla. Compilado y `ctest` verde en Linux con Qt 6.4; el backend Win32 compila cruzado con
MinGW. **Nota de entorno:** el repositorio de GitHub no trae `.github/workflows` (snapshot de un
solo commit), así que no hay CI ni artefacto `.exe` en esta fase; queda el retest funcional en
Windows y restaurar los workflows.

### Sesión 25 — 2026-08-06 · métodos de copia, ordenación de la cola y listas guardables
Se añade **Método de copia** (Compatible síncrono / Rápido asíncrono) y **Archivos a la vez**
(1–4, por defecto 1) a Opciones y a `config.mc` (`Transfer/copyMethod`, `Transfer/parallelFiles`).
El motor implementa el método asíncrono en Windows: E/S overlapped con 8 ranuras (OVERLAPPED +
evento por ranura, varias lecturas/escrituras en vuelo, anillo de buffers), mismo `.mcpart`,
pausa, límite y metadatos que el síncrono, y caída automática al síncrono fuera de Windows
(§4.8). La lista de copia se reorganiza pulsando las cabeceras Fuente/Tamaño/Destino
(`ListaDeCopia::ordenarPor`, fila en curso anclada). La barra vertical cambia «Abrir carpeta» y
«Vaciar» por **Cargar lista de copia…** y **Guardar la lista actual…** (`.mclist` en INI): permite
cancelar una copia, cargar la lista y continuar donde se quedó, con reanudación de `.mcpart`.
También se corrige el archivo vacío (se deja un parcial vacío para renombrar). `ctest` verde en
Linux (configuración ampliada, ordenación y fallback asíncrono); el backend Win32 compila cruzado
con MinGW. **Pendiente:** parte 2 de Archivos a la vez (pool de N motores por ventana con UI de
varias filas en progreso) y el retest Windows de todo lo anterior; el repo sigue sin workflows.

### Sesión 26 — 2026-08-06 · pool de motores, barra segmentada y límite compartido
Se implementa la **parte 2 de Archivos a la vez** (§4.8). La ventana crea N `MotorDeCopia` en N
hilos (N = `Transfer/parallelFiles`, 1–4) con un **`LimitadorVelocidad`** común (token bucket
atómico; sin límite no acumula deuda, así cambiar de «sin límite» a limitado a mitad no frena en
exceso). El repartidor `asignarSiguiente`/`rellenarMotores` asigna el siguiente pendiente al
motor libre, resuelve colisiones/errores por archivo (diálogos uno a la vez, solo bloquean ese
archivo) y el botón **Saltar** apunta al archivo seleccionado (clic en su segmento o en su fila)
o al primero en curso. La `ListaDeCopia` admite varias filas en curso (▶) que anclan la cola al
reordenar/ordenar, y `quitarTerminada` retira la fila que acaba de copiar un motor. La barra de
archivo es ahora **`BarraArchivos`**: con 1 archivo pinta como la clásica; con 2–4 se divide en
segmentos iguales con su % centrado y una fila de mini-etiquetas `nombre · velocidad` debajo
(verificada por render a PNG; la fila en pausa se atenúa). El límite de velocidad se aplica en
caliente al limitador compartido y el método de copia se propaga a todos los motores. `ctest`
verde en Linux (con el limitador compartido y las múltiples anclas), la app arranca y el backend
Win32 compila cruzado con MinGW. **Pendiente:** retest funcional/visual en Windows de todo lo
acumulado (F8, métodos, cola, pool y barra segmentada) y restaurar los workflows de CI.

### Sesión 27 — 2026-08-06 · CI restaurado, pausa por dispositivo y loading de enumeración
Se restauran `.github/workflows/compilar.yml` y `canal.yml` (§4.9): Linux compila y corre `ctest`,
Windows con Qt 6.6/MinGW publica el artefacto (`MaxCopier.exe`, `MaxCopierShell.dll`,
`windeployqt`, los `.cmd`) y Windows/MSVC valida el canal y la DLL. **Pausa por dispositivo
desconectado:** un temporizador de 1 s vigila el destino con `QStorageInfo`; si la unidad
desaparece durante la copia/enumeración pausa los motores activos, avisa por la bandeja y bloquea
nuevas asignaciones, y al reconectarla reanuda y reparte la cola. Si un error de escritura llega
antes que la detección, el archivo vuelve a la cola sin diálogo y se reanuda desde su `.mcpart`.
**Overlay de enumeración:** `app/vistas/cargando.{h,cpp}` bloquea la ventana con fondo
translúcido, tarjeta centrada (barra indeterminada + «Enumerando archivos…» + botón Cancelar)
mientras el escáner arma la lista; al terminar se quita y arranca la copia. Verificado el render
del overlay por PNG. `ctest` verde en Linux; la app arranca; el backend Win32 compila cruzado con
MinGW. **Pendiente:** el retest funcional en Windows con el artefacto que ahora sí genera el CI.

### Sesión 28 — 2026-08-06 · auditoría de la app y salida robusta
Auditoría intensiva empezando por el motor (§4.10). Se corrige el cierre: el drenaje de E/S
asíncronas tras cancelar podía quedarse colgado con un dispositivo desaparecido, y el destructor
esperaba sin límite a los hilos; ambos quedan acotados para que la salida nunca se cuelgue.
**Salir de MaxCopier** (menú de la bandeja) ahora pide confirmación si hay copias en curso (una o
varias) y, al aceptar, cancela y cierra todas las ventanas; un temporizador de seguridad de 10 s
fuerza `quit()` si alguna ventana no llega a destruirse, así el proceso no queda en segundo plano.
Se revisaron la DLL del Explorador, el IPC y las utilidades sin encontrar fugas (handles y
memorias liberadas, refcounts COM correctos), y se quitó la llamada deprecada
`QDropEvent::keyboardModifiers()`. `ctest` verde en Linux, sin warnings nuevos; la app arranca.

### Sesión 29 — 2026-08-06 · CI arreglado y README nuevo
Los workflows fallaban en los dos jobs de Windows por un *most vexing parse* en el motor
asíncrono (`std::vector<Ranura> ranuras(size_t(...))`, que MSVC y MinGW leían como declaración de
función) y porque el MinGW 8.1 por defecto es incompatible con Qt 6.6 (su `<filesystem>`
experimental). Se corrige la declaración del vector y el toolchain pasa a `tools_mingw1310`
(MinGW 13.1): el workflow **Compilar** queda verde (Linux con `ctest` + Windows/MinGW con
artefacto) y **Canal** (MSVC) pasa en el run del PR. Además se reescribe el **README** desde cero
(el anterior tenía basura binaria al final): corto, en español, con las características reales,
uso, instalación, compilación y licencia. Pendiente: mergear los PR #9 (CI) y #10 (README) y el
retest en Windows con el artefacto que ya vuelve a generar el CI.

### Sesión 30 — 2026-08-06 · pestaña de Opciones con el menú lateral del mockup
El usuario revisa el mockup aprobado (`ui-v3.html`, sección 4) y la pestaña de Opciones no se
parecía: faltaba el **menú lateral de categorías**. Se rediseña `OpcionesPanel` con el menú del
mockup como base visual, pero poblado solo con lo que la app tiene de verdad: **General** (Copia
en curso), **Motor de copia** (Límite de velocidad, Al terminar, Método de copia, Archivos a la
vez, y la sección «Siempre activo» con casillas marcadas y deshabilitadas para fechas/atributos,
rutas largas y reanudación `.mcpart`), **Colisiones** (Destino existente), **Errores** (Origen no
disponible) y **Apariencia y temas** (Tema visual). No se añaden ajustes que no existan (tamaño de
bloque, E/S sin búfer, comprobar espacio libre): quedan fuera hasta que se implementen. Estilos
del menú lateral para ambos temas; el diálogo de la bandeja comparte el mismo editor. Verificado
por render (oscuro y claro). `ctest` verde en Linux.

### Sesión 31 — 2026-08-06 · comprobación de espacio libre antes de empezar
El usuario marca la comprobación de espacio como esencial. Se implementa como en SuperCopier2/
TeraCopy: al terminar la enumeración (y al cargar una lista), se agrupa lo pendiente **por volumen
de destino** (`core/util/espaciolibre.*`) y, si algún volumen no alcanza, sale un diálogo con el
detalle por volumen («se necesitan X y hay Y libres») y **Continuar de todas formas / Cancelar**.
Al continuar, se fija un **presupuesto por volumen**: cada archivo que se lanza descuenta de lo que
quedaba y la tanda se detiene en cuanto el siguiente ya no cabe («Copia detenida · sin espacio»,
aviso en la bandeja); los restantes quedan en la lista para reintentar tras liberar espacio.
Nuevo ajuste real en Opciones → Motor de copia: casilla **«Comprobar espacio libre antes de
empezar»** (por defecto activada, `Transfer/checkFreeSpace`). Pruebas de configuración y del
cálculo de faltas (con disponibilidad inyectada). `ctest` verde en Linux.

### Sesión 32 — 2026-08-06 · re-auditoría de la app y casos no manejados
Segunda pasada intensiva tras las mejoras recientes (espacio libre, pool, dispositivo, salida):

**Corregido en esta sesión:**
1. `volumenDe` elegía el *primer* volumen montado que fuera prefijo de la ruta: en Linux, donde
   los volúmenes se anidan (`/`, `/home`, `/mnt/…`), podía agrupar todo bajo `/` y dar falsas
   faltas de espacio; además enumeraba volúmenes por archivo (caro con listas grandes). Ahora
   elige el **prefijo más largo** y mantiene los volúmenes montados en **caché** (refresca una
   vez si una ruta no casa con ninguno, p. ej. un USB recién conectado).
2. **Disco lleno a mitad de copia** (con la comprobación desactivada, o si otro proceso llena el
   disco y el presupuesto se desvía): antes abría el diálogo de error archivo por archivo; ahora
   la tanda se detiene igual que la falta de espacio inicial —el archivo vuelve a la cola y avisa
   por la bandeja—, sin martillear con diálogos.
3. Al **reconectar el dispositivo** tras la pausa automática, el presupuesto de «Continuar de
   todas formas» se **recalcula con el espacio real** del volumen.

**Pendientes identificados (no corregidos, documentados):**
- Si el dispositivo se reconecta con **otra letra de unidad**, la pausa automática no lo detecta
  (vigila la letra original).
- Añadir archivos a una copia **ya en curso** no re-comprueba el espacio (solo se comprueba al
  empezar o al cargar una lista).
- La comprobación previa es una instantánea: otro proceso puede llenar el disco a mitad (mitigado
  por el corte por disco lleno de esta sesión).
- El botón **Cancelar** del overlay durante un escaneo de «añadir a la copia activa» cancela toda
  la tanda (por diseño).

### Sesión 33 — 2026-08-06 · atacados todos los pendientes de la sesión 32
1. **Reconexión por volumen, no por letra.** Nueva `identidadDeVolumen` (número de serie en
   Windows vía `GetVolumeInformationW`; dispositivo + etiqueta en Linux) y `raizConIdentidad`.
   La ventana recuerda la identidad del destino mientras está conectado; si se desconecta y
   vuelve con **otra letra de unidad**, `reanudarConOtraLetra` re-mapea la carpeta de destino y
   todas las filas (`ListaDeCopia::remapearDestinos`), re-cola los archivos en curso con el
   destino nuevo (reanudan su `.mcpart`) y continúa la copia con aviso en la bandeja. Si no se
   puede obtener identidad, se comporta por letra (comportamiento anterior).
2. **Re-comprobar espacio al añadir a una copia activa.** Al terminar la enumeración se comprueba
   siempre (no solo al empezar); se cuentan únicamente los **pendientes** (los archivos en curso
   ya ocupan su sitio). Si no cabe y el usuario cancela, se retiran solo los añadidos y la copia
   activa sigue viva.
3. **Instantánea del espacio:** aceptado como mitigado por el corte por disco lleno (sesión 32).
4. **Cancelar del overlay:** si hay una copia en curso, el botón **Cancelar** de la enumeración
   cancela solo el escaneo añadido (la tanda sigue); si no hay copia, cancela la tanda como antes.

Pruebas de identidad de volumen añadidas. `ctest` verde en Linux; sin warnings; la app arranca.

### Sesión 34 — 2026-08-06 · F9: pestañas Errores y Registro
Se completa F9 (con `.mclist` ya hecho en la sesión 25):

- **Pestaña Errores**: tabla con Hora · Acción · Archivo · Motivo, contador en la pestaña
  («✗ Errores (N)», como el mockup), tooltips con el motivo completo y botón **Limpiar**. Se
  alimenta con los errores del motor (alTerminada) y con los «no se puede leer» previos a arrancar
  (asignarSiguiente); se vacía al empezar una tanda nueva.
- **Pestaña Registro**: log de la sesión de la ventana en vivo (marca de tiempo + evento):
  tanda nueva, enumeración, copiando/OK/saltado/error, pausas, cancelación, dispositivo
  desconectado/reconectado (incluida otra letra), falta de espacio y listas cargadas/guardadas.
  Solo lectura, con botón **Guardar…** para exportar a `.log`/`.txt`.

Ambas pestañas sustituyen a los marcadores «en F9» que había. Verificado por render (tabla con
dos errores y contador; registro con líneas y botón Guardar). `ctest` verde; sin warnings; la app
arranca. **Pendiente:** retest visual/funcional en Windows y mergear los PR encadenados
(#11 Opciones → #13 re-auditoría → #14 pendientes → este).

### Sesión 35 — 2026-08-06 · arreglos de Saltar, chip de Límite y título
El usuario reporta tres cosas al saltar archivos con «Archivos a la vez» > 1:

1. **`.mcpart` residual al saltar.** Antes, saltar un archivo dejaba su parcial para reanudar;
   el usuario lo considera un error (no quiere reanudar algo que saltó a propósito). Ahora el
   motor **borra el `.mcpart` del archivo saltado** (`Resultado::Saltada`), tanto copiando como
   en pausa. Verificado con una reproducción de 2 archivos: al saltar uno y terminar el otro, no
   queda ningún `.mcpart`.
2. **UI «confusa» al saltar con pausa.** La barra segmentada no marcaba la pausa y el título se
   quedaba en «N archivos en curso» aunque uno ya se hubiera saltado/terminado. Ahora cada
   segmento (y la barra de un solo archivo) se **atenúa cuando la transferencia está pausada**
   (`ArchivoEnCurso::pausado`), y el **título se actualiza** al terminar o saltar un archivo
   (vuelve a «Copia · origen → destino» con uno, o «N archivos en curso» con varios).
3. **Chip de Límite fuera de la ventana.** El chip **Límite** desaparece del panel compacto:
   el límite de velocidad se ajusta únicamente en **Opciones → Motor de copia** (como pedía el
   usuario; se conserva el chip «Al terminar»).

`ctest` verde; sin warnings; la app arranca.

### Sesión 36 — 2026-08-06 · iconos de bandeja, barra de tareas y menús de tray
1. **Iconos de bandeja recortados.** El porcentaje y el símbolo de pausa se dibujaban muy abajo
   y el área de notificación los recortaba. La placa se sube (ya no toca el borde inferior) y el
   texto se ajusta por **altura** (no solo por ancho): verificado por render a 16 y 32 px.
2. **Progreso en la barra de tareas de Windows** (`app/bandejatarea.*`, `ITaskbarList3`): cada
   ventana de copia muestra su avance en su botón (`SetProgressValue`/`TBPF_PAUSED`) y, al
   pausar, una superposición ⏸ (`SetOverlayIcon`). Solo Windows; en el resto no hace nada.
3. **Menú del icono global:** fuera la subopción **Copias activas** (cada copia se gestiona desde
   su propio icono o con Cancelar todas). Además, el icono principal ya no parpadea: el icono se
   fija una sola vez y el tooltip solo se toca cuando el texto cambia.
4. **Menú de cada copia minimizada:** ahora empieza con **Restaurar** (arriba), seguido de
   Pausar/Reanudar y Cancelar.
5. **Título de la ventana con prueba unitaria:** se extrae `tituloDeTransferencia`
   (`core/util/titulos.*`) usado por `alIniciada`/`alTerminada` y se cubre con pruebas
   (`pruebaf8`): una copia muestra «Copia · origen → destino», varias «Copia · N archivos en
   curso» y sin copias solo el verbo. (Responde a la revisión del harness: antes la verificación
   del título era manual por render; ahora es una prueba automatizada.)

Nota: además se incorpora al cherry-pick la sesión 35 (saltar borra `.mcpart`, chip de Límite
fuera, título que se actualiza y pausa visual en la barra), que quedó pendiente de main.
`ctest` verde; sin warnings; la app arranca.

### Sesión 37 — 2026-08-06 · pausa estable, loading circular y preparación sin bloqueo
Se corrigen los problemas reportados al pausar copias con varios archivos y al finalizar la
enumeración:

1. **Pausa sin perder barras.** La pausa se sincroniza con el estado real de cada motor y la UI
   vuelve a publicar las filas activas al pausar/reanudar. Las barras permanecen visibles también
   con varios archivos y cada fila refleja su pausa individual; la ventana no queda bloqueada.
2. **Loading rediseñado.** El overlay de preparación usa únicamente un indicador circular animado,
   sin la barra horizontal indeterminada.
3. **Final de enumeración sin congelar la UI.** La comprobación de espacio libre y la agrupación
   por volumen se ejecutan en el hilo de escaneo. El overlay permanece visible mientras termina
   esa preparación, se descartan resultados obsoletos por generación y la caché de volúmenes se
   protege para acceso concurrente.
4. **Regresión de interfaz.** Se añade `pruebainterfaz`: verifica el indicador circular, que no
   exista una `QProgressBar` en el overlay y que una copia de tres archivos conserve sus barras
   al pausar y pueda reanudarse.

Validación Linux: compilación correcta y `ctest --test-dir build --output-on-failure` verde (4/4).
Pendiente únicamente el retest visual/funcional en Windows con el artefacto del CI, especialmente
la pausa con más de dos archivos y la respuesta al terminar una enumeración grande.
