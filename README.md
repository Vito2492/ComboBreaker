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

## What ComboBreaker adds

Press Start in the attract loop for the arcade's own title screen, then Start
again for the SNES menu:

- **1 Player Game**: the arcade game.
- **2 Player Game**: both players go straight to the character screen; after
  each match both come back to it, instead of the arcade's winner-stays-on.
- **Tournament Game**: up to eight players with three-letter names and
  win/loss records. The winner stays on and the loser goes to the back of the
  line, with a NEXT FIGHT screen between matches.
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

## What you need

**No game files are included, and none ever will be.** You need your own,
legally obtained copy of the arcade game's files (MAME's `kinst` set):

- `ki-l15d.u98`: boot ROM, version L1.5D
- `u10-l1` … `u13-l1`, `u33-l1` … `u36-l1`: sound ROMs
- `kinst.chd`: the hard disk

The menu works with version L1.5D. The program checks your files against the
known good dumps at startup.

## Getting started (Windows)

1. Download ComboBreaker from the Releases page and unzip it.
2. Put your game files in the `run` folder next to `ComboBreaker.exe`.
3. Start `ComboBreaker.exe`. The first time it prepares the game for your PC,
   which takes about a minute, once.

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

**Qué necesitás:** tus propios archivos del juego, obtenidos legalmente (set
`kinst` de MAME, versión L1.5D). No se incluye ningún archivo del juego.
Ponelos en la carpeta `run`, al lado de `ComboBreaker.exe`, y abrí el
programa.

Killer Instinct es una marca registrada de sus dueños. Este es un proyecto de
fans, sin relación con ellos.
