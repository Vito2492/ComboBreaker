# ComboBreaker

**The arcade Killer Instinct (1994), with the menus and modes of the Super
Nintendo version, as a native PC program.**

The arcade game has the best graphics and sound, but it has no modes: you
put in a coin and you fight. The SNES version had the menus: 1 Player Game,
2 Player Game, Tournament Game, Practice Game and Options. ComboBreaker puts
those menus on top of the arcade game itself.

ComboBreaker is built almost entirely on
[**UltraCombo**](https://github.com/thefixinhixon/UltraCombo) by
thefixinhixon, which runs the arcade game's own code natively on a PC
(translated to C, with the sound board and hard disk emulated) and already
brings shaders, gamepads, save states, rewind, netplay and more. All of that
work is UltraCombo's. ComboBreaker adds the home-version menus on top, and
changes nothing in how the arcade game runs.

*[En español, más abajo](#en-español).*

**New here? Read the [guide](docs/GUIDE.md)** · **¿Primera vez? Leé la [guía](docs/GUIA.md)**: installing, where the game files go, controls, modes and codes.

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
- **Practice Game**: player 1 picks both fighters; the dummy stands still and
  nobody loses health. No health bars or clock, as on the SNES.
- **Options**: difficulty (five stars), random select, timer, blood,
  finishing moves, and the SNES controls table to remap each player's
  buttons.

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
| Linux x86-64 (Steam Deck too) | `ComboBreaker-Linux.tar.xz` | **built, not tested on a real machine yet**: reports welcome |
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

**El Killer Instinct de arcade (1994), con los menús y modos de la versión de
Super Nintendo, como programa nativo para PC.**

El arcade tiene los mejores gráficos y sonido, pero no tiene modos: ponés una
ficha y peleás. La versión de SNES tenía los menús: 1 Player Game, 2 Player
Game, Tournament Game, Practice Game y Options. ComboBreaker pone esos menús
encima del propio juego de arcade.

ComboBreaker está construido casi por completo sobre
[**UltraCombo**](https://github.com/thefixinhixon/UltraCombo), de
thefixinhixon, que corre el código original del arcade de forma nativa en la
PC y ya trae filtros, mandos, guardado de partida, rebobinado y juego en red.
Todo ese trabajo es de UltraCombo. ComboBreaker agrega los menús de la
versión de consola y no cambia nada de cómo corre el juego.

**Qué agrega:** el menú del SNES al apretar Start en la pantalla de título;
2 jugadores con revancha; torneo de hasta 8 jugadores; práctica sin barras de
vida; Options con estrellas de dificultad y tabla de controles; pausa en
cualquier momento; los códigos del SNES (escenario, velocidad, personaje al
azar); la música de cada escenario.

**Requisitos:** Windows 10/11 de 64 bits, Linux, Mac con procesador Apple o Android 7 (64 bits), casi cualquier PC de los
últimos diez años, 1 GB de RAM libre, una placa de video con OpenGL 2.1 y unos
400 MB de disco. La primera vez el programa tarda un minuto en prepararse.

**Guía completa paso a paso:** [docs/GUIA.md](docs/GUIA.md).

**Qué necesitás:** tus propios archivos del juego, obtenidos legalmente: el
set `kinst` de MAME, versión L1.5D. Son 10 archivos: `ki-l15d.u98` (ROM de
arranque), las 8 ROMs de sonido `u10-l1` … `u13-l1` y `u33-l1` … `u36-l1`, y
el disco `kinst.chd`. La tabla de arriba tiene sus tamaños y checksums para
comprobar que sean los correctos. No se incluye ningún archivo del juego.
Ponelos en la carpeta `run`, al lado de `ComboBreaker.exe`, y abrí el
programa.

Killer Instinct es una marca registrada de sus dueños. Este es un proyecto de
fans, sin relación con ellos.
