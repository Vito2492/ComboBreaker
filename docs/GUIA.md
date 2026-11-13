# Guía de ComboBreaker

ComboBreaker es el Killer Instinct de arcade (1994) con los menús y modos de
la versión de Super Nintendo. Esta guía explica cómo instalarlo, dónde van
los archivos del juego y cómo se juega.

*[English version](GUIDE.md).*

## 1. Lo que necesitas

- Una PC con Windows 10 u 11 (64 bits), Linux o una Mac con procesador Apple
  (M1 o posterior). Casi cualquier PC de los últimos diez años alcanza.
- **Tus propios archivos del juego.** ComboBreaker no trae ninguno, y nunca
  los va a traer: son del juego original y tienen dueño. Necesitas el set
  `kinst` de MAME, versión **L1.5D**, obtenido de forma legal. Son 10
  archivos:

| Archivo | Qué es |
|---|---|
| `ki-l15d.u98` | ROM de arranque, versión L1.5D |
| `u10-l1`, `u11-l1`, `u12-l1`, `u13-l1` | ROMs de sonido |
| `u33-l1`, `u34-l1`, `u35-l1`, `u36-l1` | ROMs de sonido |
| `kinst.chd` | el disco duro de la máquina (unos 95 MB) |

Los tamaños y checksums exactos están en el [README](../README.md#what-you-need-the-game-files).
Si tienes el set como un `kinst.zip` de MAME (con la carpeta `kinst` y el
`kinst.chd` al lado), también sirve: ver el paso 3.

## 2. Descargar

1. Entra a la página de
   [Releases](https://github.com/Vito2492/ComboBreaker/releases) y descarga el
   paquete de tu sistema:
   - Windows: `ComboBreaker-Windows.zip`
   - Linux (también Steam Deck): `ComboBreaker-Linux.tar.xz`
   - Mac: `ComboBreaker-macOS.tar.xz` (todavía **no probado** en una Mac:
     si lo pruebas, cuéntanos cómo te fue)
   - Android: ver la sección 7.
2. Descomprímelo donde quieras, por ejemplo en `Documentos\ComboBreaker`.
   No hace falta instalar nada.

## 3. Poner los archivos del juego

Dentro de la carpeta de ComboBreaker hay una carpeta `run`. Copia ahí los
10 archivos, con esos nombres exactos:

```
ComboBreaker/
├── ComboBreaker.exe        (en Linux y Mac: combobreaker)
└── run/
    ├── ki-l15d.u98
    ├── u10-l1  u11-l1  u12-l1  u13-l1
    ├── u33-l1  u34-l1  u35-l1  u36-l1
    └── kinst.chd
```

Si te falta alguno, el programa te dice cuál, te muestra esta lista y tiene
un botón para abrir la carpeta `run`. Si alguno está mal (otra versión, un
archivo dañado), también te lo dice.

Con un `kinst.zip` de MAME: arrástralo sobre `ComboBreaker.exe`, o ábrelo
desde la línea de comandos: `ComboBreaker.exe ruta\a\kinst.zip`.

## 4. Primer arranque

- **Windows:** doble clic en `ComboBreaker.exe`. Como el programa no está
  firmado, Windows puede mostrar "Windows protegió su PC": haz clic en *Más
  información* y después *Ejecutar de todas formas*.
- **Linux:** ejecuta `./combobreaker` desde la carpeta.
- **Mac:** la primera vez, clic derecho sobre `combobreaker` → *Abrir*. Si
  dice que está dañado o que no se puede verificar, en la Terminal, dentro de
  la carpeta: `xattr -dr com.apple.quarantine .` y prueba de nuevo.

**La primera vez tarda alrededor de un minuto** antes de mostrar el juego: el
programa prepara el código del juego para tu computadora. Lo hace una sola
vez y lo guarda en la carpeta `cache`.

## 5. Cómo se juega

El juego arranca con la introducción y la demo del arcade, como la máquina.
Presiona **Start** para ir a la pantalla de título y **Start** otra vez para
abrir el menú de la Super Nintendo.

### Controles

| | Teclado, jugador 1 | Teclado, jugador 2 | Joystick |
|---|---|---|---|
| Moverse | Flechas | I J K L | Cruz o palanca izquierda |
| Puño débil / medio / fuerte | A S D | T Y U | L, Y, X |
| Patada débil / media / fuerte | Z X C | G H B | R, B, A |
| Start | 1 | 2 | Start |

El joystick usa la distribución del Super Nintendo (los nombres son los de un
mando de Nintendo: Y a la izquierda, B abajo, A a la derecha, X arriba). Sirve
cualquier joystick: Xbox, PlayStation, Switch, 8BitDo y la mayoría de los
demás. El primero que conectes es el jugador 1 y el segundo, el jugador 2.

**En los menús:** A (o Start) elige y B vuelve atrás. Van siempre por la
posición del botón, aunque cambies los controles del juego. En el teclado:
Enter elige y Retroceso vuelve.

### Los modos

- **1 Player Game:** el arcade de siempre, contra la máquina hasta Eyedol.
- **2 Player Game:** uno contra uno. Al terminar cada pelea los dos vuelven a
  elegir personaje (la revancha de la consola), sin continue ni récords.
- **Tournament Game:** de 2 a 8 jugadores. Cada uno pone su nombre de tres
  letras. Antes de cada pelea aparece **NEXT FIGHT** con los dos que pelean,
  su récord de victorias y derrotas, y abajo la lista de los que esperan. El
  ganador se queda y pelea contra el siguiente de la lista; el que pierde va
  al final de la fila. La pantalla de victoria (Supreme, Awesome, Perfect) se
  ve completa; los récords y el continue del arcade no aparecen.
- **Practice Game:** el jugador 1 elige los dos personajes (primero el suyo)
  y nadie pierde vida. Sin barras ni reloj, como en la Super Nintendo. En la
  pausa, **DUMMY** elige qué hace el rival:
  - STAND, CROUCH, JUMP: quedarse parado, agachado o saltar.
  - BLOCK, LOW BLOCK: se cubre parado o agachado cada vez que atacas.
  - CPU: el luchador de la máquina, que se mueve, ataca y hace combos.
    **CPU LEVEL**, justo debajo en la pausa, elige qué tan bueno es (de una a
    cinco estrellas, aparte de la dificultad del arcade). Al pasar a CPU o
    salir de CPU vuelves a la pantalla de personajes.

  La misma opción está en F1 > Practice, junto al rival grabado.
- **Options:** dificultad (de una a cinco estrellas), dificultad fija,
  personaje al azar, reloj, sangre, finishing moves, y la tabla de controles
  para cambiar los botones de cada jugador. En el arcade la máquina se pone
  más difícil pelea a pelea; con **FIXED DIFFICULTY: YES** se queda en el
  nivel inicial de la dificultad elegida toda la partida.

### Pausa

En cualquier momento de una pelea de estos modos, **Start** pausa. Desde la
pausa puedes seguir o volver al menú principal.

### Los códigos de la Super Nintendo

- **Elegir escenario:** en la pantalla de personajes, mantén una dirección
  y elige tu personaje con un botón. Elige el primer jugador que confirma.

| | Abajo + | Arriba + |
|---|---|---|
| L (puño débil) | Canyon Bridge | Ice Temple |
| Y (puño medio) | Bloody Arena | Skull Room |
| X (puño fuerte) | Lava Pit | Ice Sculpture |
| R (patada débil) | City Street | Castle Roof |
| B (patada media) | Fireplace | City Roof |
| A (patada fuerte) | Factory | Desert Roof |

  Sky Rooftop: los dos jugadores eligen con Abajo + B.
- **Velocidad**, en la pantalla VS, manteniendo los botones hasta que empiece
  la pelea (en dos jugadores, los dos):
  - Izquierda + las tres patadas: lenta
  - Derecha + los tres puños: rápida
  - Derecha + las tres patadas: más rápida
  - Izquierda + los tres puños: la más rápida

  La música no cambia de tono.
- **Personaje al azar:** Arriba + Start en la pantalla de personajes.

### Más opciones

**F1** (o el botón Guide del joystick) abre el menú propio del programa:
pantalla completa y resolución, filtros de imagen (CRT y otros), controles,
volumen, guardar y cargar partida, rebobinar y juego en red. Todo eso viene
de UltraCombo y está explicado en el
[manual de UltraCombo](UltraCombo-manual.md).

Si prefieres el arcade puro, sin los menús de la consola, pon
`home_menu = no` en `ki1.ini`.

## 6. Problemas comunes

- **Un solo joystick mueve a los dos jugadores.** Algunos mandos (por ejemplo
  ciertos 8BitDo) aparecen dos veces en la PC. ComboBreaker lo detecta a los
  pocos segundos de jugar y descarta el duplicado, con un aviso en pantalla.
- **"Falta un archivo" o "no coincide".** Revisa que los nombres sean
  exactos (sin `.bin` ni `.zip` agregados) y que sea la versión L1.5D.
- **Tarda mucho la primera vez.** Es normal: prepara el juego una vez. Si
  borras la carpeta `cache`, lo vuelve a hacer.
- **Quiero volver a la configuración original.** Cierra el programa y borra
  `ki1.ini`; se crea de nuevo al abrirlo.
- **Algo falló.** En la carpeta del programa está `ki1.log`. Si abres un
  reporte en [GitHub](https://github.com/Vito2492/ComboBreaker/issues),
  adjúntalo.

## 7. Android

Probado en un teléfono. Si en el tuyo algo no funciona, cuéntalo en
[GitHub](https://github.com/Vito2492/ComboBreaker/issues).

1. Descarga `ComboBreaker-Android.apk` desde
   [Releases](https://github.com/Vito2492/ComboBreaker/releases) en el teléfono
   y ábrelo. Android te va a pedir permiso para instalar apps de esa fuente
   (el navegador o el administrador de archivos): acéptalo.
2. Copia tus 10 archivos del juego (o tu `kinst.zip` de MAME) a una carpeta
   del teléfono, por ejemplo `Descargas/kinst`.
3. Abre ComboBreaker. La primera vez te pide la carpeta: toca *Choose
   folder*, elige esa carpeta y toca *Usar esta carpeta*. La app copia los
   archivos a su carpeta (`Android/data/io.github.vito2492.combobreaker/files/run`)
   y ya no los vuelve a pedir.
4. La primera partida tarda un poco más mientras prepara el juego para el
   teléfono.

**Controles:** se recomienda un joystick Bluetooth o USB (los de Xbox,
PlayStation, 8BitDo y la mayoría funcionan). Sin joystick aparecen controles
en pantalla con la distribución de la Super Nintendo: cruz y Start a la
izquierda, Y X A B y L R a la derecha. El botón de la esquina, o el botón
Atrás del teléfono, abre el menú del programa. Se pueden apagar o dejar
siempre puestos con `touch_controls` (`auto`, `on`, `off`) en `ki1.ini`.

Se necesita Android 7 o posterior, de 64 bits. El APK no trae ningún archivo
del juego.
