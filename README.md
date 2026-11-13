# ComboBreaker

**The arcade Killer Instinct (1994), with the menus and modes of the Super
Nintendo version, as a native program for PC and Android.**

**El Killer Instinct de arcade (1994) con los menús y modos de la versión de
Super Nintendo, como programa nativo para PC y Android.**

[English](#why-i-made-this) · [Español](#en-español) ·
**Guide: [docs/GUIDE.md](docs/GUIDE.md)** · **Guía: [docs/GUIA.md](docs/GUIA.md)**

## Why I made this

Killer Instinct was my first game, and it's still my favourite. I wanted to
play it with the arcade's graphics and animations, but with the Super
Nintendo version's fuller modes, like Practice, Tournament and the rematch in
2 Player. The arcade has the best picture and sound, but no modes: you put in
a coin and you fight. ComboBreaker puts the console's menus on top of the
arcade game itself.

It is an unofficial fan project, not licensed by the game's owners, and it
stands on other people's work: almost all of it is
[**UltraCombo**](https://github.com/thefixinhixon/UltraCombo) by
thefixinhixon, which runs the arcade game's own code natively (translated to
C, with the sound board and hard disk emulated) and already brings shaders,
gamepads, save states, rewind, netplay and more. All of that is UltraCombo's
work. On top of it I added, with the help of AI, the console interface and
the other adaptations, trying to make them feel seamless with the original
game. Nothing changes in how the arcade game itself runs.

I hope you like it.

## What ComboBreaker adds

Press Start in the attract loop for the arcade's own title screen, then Start
again for the SNES menu:

- **1 Player Game**: the arcade game.
- **2 Player Game**: both players go straight to the character screen; after
  each match both come back to it, instead of the arcade's winner-stays-on.
- **Tournament Game**: up to eight players with three-letter names and
  win/loss records, and the SNES NEXT FIGHT screen laid over the arcade's own
  character screen. The winner stays on and the loser goes to the back of the
  line.
- **Practice Game**: player 1 picks both fighters and nobody loses health.
  No health bars or clock, as on the SNES. The pause menu's DUMMY sets what
  the other fighter does: stand, crouch, jump, block (standing or low,
  whenever you attack), COMBO (walks up and does a combo again and again, to
  practise breakers), or CPU: the game's own CPU fighter, attacking and
  doing combos, at its own CPU LEVEL (one to five stars). The same setting is in F1 >
  Practice, beside the recorded dummy.
- **Options**: difficulty (five stars), fixed difficulty, random select,
  timer, blood, finishing moves, and the SNES controls table to remap each
  player's buttons. The arcade raises the CPU's level fight by fight; with
  FIXED DIFFICULTY: YES it stays at the chosen difficulty's first level all
  game. EASY COMBO BREAKER: ON makes any attack button break a combo, in
  every mode.

Also:

- **Pause at any time** with Start in these modes (Continue / Main Menu).
- **The SNES codes**: stage select (hold a direction and pick your fighter
  with a button), speed on the VS screen (slow, fast, faster, fastest; the
  music keeps its pitch), random fighter (Up + Start).
- **Each stage's own music** in two-player fights.
- Left alone on the menu, the arcade's attract loop rolls (fighter profiles,
  high scores) and Start brings the menu back.

The menu is navigated with the pad's buttons by position, as on a Nintendo
pad: A (right) chooses, B (bottom) goes back, Start also chooses. On the
keyboard: Enter and Backspace. The gamepad's default layout is the SNES one:
L Y X are weak, medium and fierce punch, R B A weak, medium and fierce kick.
`home_menu = no` in `ki1.ini` gives the plain arcade game.

## What you need: the game files

**No game files are included, and none ever will be.** You need your own,
legally obtained copy of the arcade game's files: MAME's `kinst` set,
version L1.5D. Ten files, all with these exact names:

| File | What it is | Size | MD5 |
|---|---|---|---|
| `ki-l15d.u98` | boot ROM, version L1.5D | 524,288 bytes | `6a63e4bbdef8ea9f0d26f7244341ca08` |
| `u10-l1` | sound ROM | 524,288 bytes | `9ee2b2cdda7ede8811a57507ef61e07b` |
| `u11-l1` | sound ROM | 524,288 bytes | `bce2d8cd20e94d4406c19fd55d798946` |
| `u12-l1` | sound ROM | 524,288 bytes | `14bb46cef4884f8df5df1fa985e6d3bf` |
| `u13-l1` | sound ROM | 524,288 bytes | `ca0904ee517845eed5509426cf5d8560` |
| `u33-l1` | sound ROM | 524,288 bytes | `da9e7f5f70320b51e6fdc23e9337484c` |
| `u34-l1` | sound ROM | 524,288 bytes | `207395792342705bea307ecb321a95bf` |
| `u35-l1` | sound ROM | 524,288 bytes | `a1758b43b42b32c25c770dc3ac7badc3` |
| `u36-l1` | sound ROM | 524,288 bytes | `57b73bec5d0e8c90d8898e102ce4066b` |
| `kinst.chd` | the hard disk (MAME CHD) | about 95 MB | SHA1 inside the CHD (`chdman info`): `81d833236e994528d1482979261401b198d1ca53` |

The CHD file's own MD5 depends on the chdman version that made it, so check
the SHA1 that `chdman info -i kinst.chd` prints instead. A raw disk image
(`kinst.raw`, from `chdman extractraw`) works too.

Put them all in the `run` folder next to the program:

```
ComboBreaker/
├── ComboBreaker.exe
└── run/
    ├── ki-l15d.u98
    ├── u10-l1 … u13-l1
    ├── u33-l1 … u36-l1
    └── kinst.chd
```

A MAME zip of the set (`kinst.zip`, with `kinst/kinst.chd` beside it) can be
opened directly instead: `ComboBreaker.exe path\to\kinst.zip`. At startup
the program compares your files with the known good dumps and names any that
does not match. The optional `ki-l14.u98` (version L1.4, the original release
with its infinite combos) is accepted too, but the console menu only runs on
L1.5D; on L1.4 the game plays as the plain arcade game.

## System requirements

The game's own code runs natively, so almost any PC from the last ten years
is enough. These are approximate:

| | Minimum |
|---|---|
| System | Windows 10 or 11 (64-bit), or Linux x86-64 from about 2020 on (glibc 2.31) |
| Processor | any 64-bit x86 processor with two cores |
| Memory | 1 GB free (rewind keeps 256 MB of history by default; less in `ki1.ini`) |
| Graphics | OpenGL 2.1 (the Lola Retro CRT shader needs OpenGL 3.3) |
| Disk | about 300 MB for the program and its bundled compiler, plus about 100 MB of game files |
| Controls | keyboard, or one or two gamepads (Xbox, PlayStation, Switch and most others) |

The first time you play, the program builds the game's code for your PC,
which takes about a minute, once.

| Platform | Package | State |
|---|---|---|
| Windows 10/11 (64-bit) | `ComboBreaker-Windows.zip` | tested |
| Linux x86-64 (Steam Deck too) | `ComboBreaker-Linux.tar.xz` | tested on a Steam Deck |
| macOS on Apple silicon (M1 and later) | `ComboBreaker-macOS.tar.xz` | **built, not tested on a Mac yet**: reports welcome |
| Android 7 or later (64-bit) | `ComboBreaker-Android.apk` | tested on a phone: see the [guide](docs/GUIDE.md#7-android) |

On macOS the program is not signed: the first time, right-click it and choose
Open. If it does not start, `brew install sdl2` and try again. On Android the app asks for the folder holding your game files the first time and copies them in; on-screen controls show when no pad is connected. There is no iOS version.

## Getting started

1. Download your platform's package from the
   [Releases page](https://github.com/Vito2492/ComboBreaker/releases) and
   unpack it anywhere.
2. Put your ten game files in the `run` folder inside it (see the table
   above). They can stay where you keep them, too: drop a MAME `kinst.zip`
   on the program, or set the paths in `ki1.ini`.
3. Start `ComboBreaker.exe` (Windows) or `combobreaker` (Linux, macOS). The
   first time it prepares the game for your PC, about a minute, once.

If a file is missing, the program says which, lists what goes where, and
offers to open the `run` folder for you.

F1 opens UltraCombo's own menu: graphics, fullscreen and resolution, shaders,
controls, volume, save states and netplay. See the
[UltraCombo manual](docs/UltraCombo-manual.md) for all of it.

## Building

The same as UltraCombo: `make` for Linux (needs gcc, SDL2 and python3),
`make windows` to cross-compile for Windows with MinGW. See the
[manual](docs/UltraCombo-manual.md#building).

The menu's code is in `src/home.c`. The RAM locations it reads (game state,
stage, music, win counters) are in the game profile, `src/game_ki1.c`, with
how each was found.

## Credits and licences

- [UltraCombo](https://github.com/thefixinhixon/UltraCombo) by thefixinhixon,
  MIT licence (`LICENSE`): the program this is built on.
- ComboBreaker's additions are under the same MIT licence.
- The menus' lettering is drawn from
  [Press Start 2P](https://fonts.google.com/specimen/Press+Start+2P) by
  CodeMan38, SIL Open Font Licence (`tools/fonts/OFL-PressStart2P.txt`).
- Bundled shaders and libraries keep their own licences (see the manual).
- The Android app carries [TinyCC](https://github.com/TinyCC/tinycc) (LGPL)
  as `libtcc.so`, to build the game's code on the phone, and SDL2 (zlib).

Killer Instinct is a trademark of its owners. This is a fan project, not
affiliated with them. The game, its code and its data belong to their owners.

---

## En español

### Por qué lo hice

Killer Instinct fue mi primer juego y sigue siendo mi favorito. Quería
jugarlo con los gráficos y las animaciones del arcade, pero con los modos más
completos de la versión de Super Nintendo, como la práctica, el torneo y la
revancha en 2 jugadores. El arcade tiene la mejor imagen y el mejor sonido,
pero no tiene modos: pones una ficha y peleas. ComboBreaker pone los menús de
la consola encima del propio juego de arcade.

Es un proyecto de fans no oficial, sin licencia de los dueños del juego, y se
construye sobre el trabajo de otros: casi todo es
[**UltraCombo**](https://github.com/thefixinhixon/UltraCombo), de
thefixinhixon, que corre el código original del arcade de forma nativa
(traducido a C, con la placa de sonido y el disco emulados) y ya trae
filtros de imagen, mandos, guardado de partida, rebobinado, juego en red y
más. Todo eso es trabajo de UltraCombo. Encima agregué, con ayuda de
inteligencia artificial, la interfaz de la consola y las demás adaptaciones,
buscando que se sientan parte del juego original. El juego de arcade en sí
corre exactamente igual.

Espero que les guste.

### Qué agrega ComboBreaker

En la demo del arcade, presiona Start para ir a la pantalla de título del
arcade, y Start otra vez para abrir el menú de la Super Nintendo:

- **1 Player Game:** el arcade de siempre.
- **2 Player Game:** los dos jugadores van directo a elegir personaje; al
  terminar cada pelea vuelven los dos a elegir, en vez de quedarse el ganador
  como en el arcade.
- **Tournament Game:** hasta 8 jugadores con nombre de tres letras y récord de
  victorias y derrotas, con la pantalla NEXT FIGHT de la Super Nintendo sobre
  la selección de personajes del arcade. El ganador se queda y el que pierde
  va al final de la fila. La pantalla de victoria (Supreme, Awesome, Perfect)
  se ve completa; el continue y los récords del arcade no aparecen.
- **Practice Game:** el jugador 1 elige los dos personajes y nadie pierde
  vida. Sin barras de vida ni reloj, como en la Super Nintendo. En la pausa,
  DUMMY elige qué hace el rival: quedarse parado, agachado, saltar, cubrirse
  (parado o agachado, cada vez que atacas), COMBO (se acerca y hace un combo una
  y otra vez, para practicar breakers) o CPU: el luchador de la máquina,
  que ataca y hace combos, con su propio CPU LEVEL (de una a cinco estrellas). La misma opción está en
  F1 > Practice, junto al rival grabado.
- **Options:** dificultad (cinco estrellas), dificultad fija, personaje al
  azar, reloj, sangre, finishing moves y la tabla de controles de la Super
  Nintendo para cambiar los botones de cada jugador. El arcade sube el nivel
  de la máquina pelea a pelea; con FIXED DIFFICULTY: YES se queda en el nivel
  inicial de la dificultad elegida toda la partida. Con EASY COMBO BREAKER: ON
  cualquier botón de ataque rompe un combo, en todos los modos.

Además:

- **Pausa en cualquier momento** con Start en estos modos (Continue / Main
  Menu).
- **Los códigos de la Super Nintendo:** elegir escenario (mantén una dirección
  y elige tu personaje con un botón), velocidad en la pantalla VS (lenta,
  rápida, más rápida, la más rápida; la música no cambia de tono) y personaje
  al azar (Arriba + Start).
- **La música propia de cada escenario** en las peleas de dos jugadores.
- Si dejas el menú quieto, corre la demo del arcade (fichas de los
  personajes, récords) y Start vuelve al menú.

En el menú se navega con los botones del mando por su posición, como en un
mando de Nintendo: A (derecha) elige, B (abajo) vuelve, Start también elige.
En el teclado: Enter y Retroceso. La distribución del mando es la de la Super
Nintendo: L Y X son puño débil, medio y fuerte; R B A, patada débil, media y
fuerte. Con `home_menu = no` en `ki1.ini` se juega el arcade puro.

### Lo que necesitas: los archivos del juego

**No se incluye ningún archivo del juego, y nunca se va a incluir.** Necesitas
tu propia copia, obtenida legalmente, de los archivos del arcade: el set
`kinst` de MAME, versión L1.5D. Son 10 archivos, con estos nombres exactos
(los tamaños y checksums están en la tabla en inglés, más arriba):

| Archivo | Qué es |
|---|---|
| `ki-l15d.u98` | ROM de arranque, versión L1.5D |
| `u10-l1`, `u11-l1`, `u12-l1`, `u13-l1` | ROMs de sonido |
| `u33-l1`, `u34-l1`, `u35-l1`, `u36-l1` | ROMs de sonido |
| `kinst.chd` | el disco duro de la máquina (CHD de MAME, unos 95 MB) |

Van en la carpeta `run`, al lado del programa. También puedes abrir
directamente un `kinst.zip` de MAME. Al arrancar, el programa compara tus
archivos con los volcados conocidos y te dice si alguno no coincide.

### Requisitos

El código del juego corre de forma nativa, así que alcanza casi cualquier PC
de los últimos diez años: Windows 10 u 11 de 64 bits, o Linux x86-64; un
procesador de 64 bits con dos núcleos; 1 GB de memoria libre; una tarjeta de
video con OpenGL 2.1; unos 300 MB de disco más los 100 MB de los archivos del
juego; teclado o uno o dos mandos (Xbox, PlayStation, Switch y la mayoría de
los demás). La primera vez, el programa prepara el código del juego para tu
computadora: tarda alrededor de un minuto, una sola vez.

| Sistema | Paquete | Estado |
|---|---|---|
| Windows 10/11 (64 bits) | `ComboBreaker-Windows.zip` | probado |
| Linux x86-64 (también Steam Deck) | `ComboBreaker-Linux.tar.xz` | probado en una Steam Deck |
| macOS con procesador Apple (M1 o posterior) | `ComboBreaker-macOS.tar.xz` | **compilado, todavía sin probar en una Mac** |
| Android 7 o posterior (64 bits) | `ComboBreaker-Android.apk` | probado en un teléfono |

No hay versión para iOS.

### Cómo empezar

1. Descarga el paquete de tu sistema desde la
   [página de Releases](https://github.com/Vito2492/ComboBreaker/releases) y
   descomprímelo donde quieras.
2. Pon tus 10 archivos del juego en la carpeta `run` que viene dentro.
3. Abre `ComboBreaker.exe` (Windows) o `combobreaker` (Linux, macOS). La
   primera vez prepara el juego, alrededor de un minuto.

Si falta un archivo, el programa te dice cuál, te muestra qué va dónde y te
ofrece abrir la carpeta `run`. En Android, la app te pide la carpeta del
teléfono donde tienes los archivos y los copia. La
[guía](docs/GUIA.md) explica todo paso a paso, con los controles y los
códigos.

F1 abre el menú propio de UltraCombo: gráficos, pantalla completa y
resolución, filtros, controles, volumen, guardado de partida y juego en red
(ver el [manual de UltraCombo](docs/UltraCombo-manual.md), en inglés).

### Compilar

Igual que UltraCombo: `make` para Linux (necesita gcc, SDL2 y python3) y
`make windows` para compilar la versión de Windows con MinGW. El código del
menú está en `src/home.c`; las direcciones de memoria del juego que usa
están en `src/game_ki1.c`, con cómo se encontró cada una.

### Créditos y licencias

- [UltraCombo](https://github.com/thefixinhixon/UltraCombo), de
  thefixinhixon, licencia MIT (`LICENSE`): el programa sobre el que se
  construye este.
- Lo que agrega ComboBreaker usa la misma licencia MIT.
- Las letras del menú son de
  [Press Start 2P](https://fonts.google.com/specimen/Press+Start+2P), de
  CodeMan38, SIL Open Font Licence.
- La app de Android incluye [TinyCC](https://github.com/TinyCC/tinycc) (LGPL)
  y SDL2 (zlib).

Killer Instinct es una marca registrada de sus dueños. Este es un proyecto de
fans, sin relación con ellos. El juego, su código y sus datos pertenecen a
sus dueños.
