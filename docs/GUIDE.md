# ComboBreaker guide

ComboBreaker is the arcade Killer Instinct (1994) with the menus and modes of
the Super Nintendo version. This guide covers installing it, where the game
files go, and how to play.

*[Versión en español](GUIA.md).*

## 1. What you need

- A PC with Windows 10 or 11 (64-bit), Linux, or a Mac with Apple silicon
  (M1 or later). Almost any PC from the last ten years is enough.
- **Your own game files.** ComboBreaker includes none and never will: they
  belong to the original game's owners. You need MAME's `kinst` set, version
  **L1.5D**, legally obtained. Ten files:

| File | What it is |
|---|---|
| `ki-l15d.u98` | boot ROM, version L1.5D |
| `u10-l1`, `u11-l1`, `u12-l1`, `u13-l1` | sound ROMs |
| `u33-l1`, `u34-l1`, `u35-l1`, `u36-l1` | sound ROMs |
| `kinst.chd` | the machine's hard disk (about 95 MB) |

Exact sizes and checksums are in the [README](../README.md#what-you-need-the-game-files).
A MAME `kinst.zip` (with the `kinst` folder and `kinst.chd` beside it) works
too: see step 3.

## 2. Download

1. Go to the [Releases](https://github.com/Vito2492/ComboBreaker/releases)
   page and download your system's package:
   - Windows: `ComboBreaker-Windows.zip`
   - Linux (Steam Deck too): `ComboBreaker-Linux.tar.xz`
   - Mac: `ComboBreaker-macOS.tar.xz` (**not tested** on a Mac yet: reports
     welcome)
   - Android: see section 7.
2. Unpack it anywhere. Nothing to install.

## 3. Add the game files

Inside the ComboBreaker folder there is a `run` folder. Copy the ten files
there, with exactly these names:

```
ComboBreaker/
├── ComboBreaker.exe        (Linux and Mac: combobreaker)
└── run/
    ├── ki-l15d.u98
    ├── u10-l1  u11-l1  u12-l1  u13-l1
    ├── u33-l1  u34-l1  u35-l1  u36-l1
    └── kinst.chd
```

If one is missing, the program names it, shows this list and offers to open
the `run` folder. A wrong file (another version, a bad dump) is named too.

With a MAME `kinst.zip`: drop it on `ComboBreaker.exe`, or run
`ComboBreaker.exe path\to\kinst.zip`.

## 4. First launch

- **Windows:** double-click `ComboBreaker.exe`. The program isn't signed, so
  Windows may say "Windows protected your PC": click *More info*, then *Run
  anyway*.
- **Linux:** run `./combobreaker` from its folder.
- **Mac:** the first time, right-click `combobreaker` → *Open*. If macOS says
  it is damaged or can't be verified, run `xattr -dr com.apple.quarantine .`
  in Terminal inside the folder and try again.

**The first launch takes about a minute** before the game shows: the program
prepares the game's code for your computer, once, and keeps it in `cache`.

## 5. Playing

The game starts with the arcade's intro and attract demo, as the cabinet
does. Press **Start** for the title screen and **Start** again for the Super
Nintendo menu.

### Controls

| | Keyboard, player 1 | Keyboard, player 2 | Gamepad |
|---|---|---|---|
| Move | Arrow keys | I J K L | D-pad or left stick |
| Weak / medium / fierce punch | A S D | T Y U | L, Y, X |
| Weak / medium / fierce kick | Z X C | G H B | R, B, A |
| Start | 1 | 2 | Start |

The gamepad uses the Super Nintendo layout (button names as on a Nintendo
pad: Y left, B bottom, A right, X top). Xbox, PlayStation, Switch, 8BitDo
and most other pads work. The first pad connected is player 1, the second
player 2.

**In the menus:** A (or Start) chooses and B goes back, always by the
button's position, whatever the game's controls are set to. Keyboard: Enter
and Backspace.

### Modes

- **1 Player Game:** the arcade game, against the CPU up to Eyedol.
- **2 Player Game:** one on one. After each match both players pick again
  (the console's rematch), with no continue or high-score screens.
- **Tournament Game:** 2 to 8 players, each with a three-letter name. Before
  every fight, **NEXT FIGHT** shows the two fighters with their win/loss
  records and the waiting line below. The winner stays on against the next
  in line; the loser goes to the back. The victory screen (supreme, awesome,
  perfect) plays in full; the arcade's continue and high scores don't.
- **Practice Game:** player 1 picks both fighters (his own first) and nobody
  loses health. No health bars or clock, as on the Super Nintendo. In the
  pause menu, **DUMMY** chooses what the other fighter does:
  - STAND, CROUCH, JUMP: just that.
  - BLOCK, LOW BLOCK: blocks standing or crouching whenever you attack.
  - COMBO: walks up and does a combo of its own (three to five hits), rests,
    and again: practice for combo breakers.
  - CPU: the game's own CPU fighter, which moves, attacks and does combos.
    **CPU LEVEL**, just below in the pause menu, sets how good it is (one to
    five stars, separate from the arcade's difficulty). Changing to or from
    CPU starts the fight again with the same fighters (the program picks
    them for you); the other changes apply at once.

  The same setting is in F1 > Practice, beside the recorded dummy.
- **Options:** difficulty (one to five stars), fixed difficulty, random
  select, timer, blood, finishing moves, and the controls table to remap
  each player's buttons. The arcade game gets harder fight by fight; with
  **FIXED DIFFICULTY: YES** the CPU stays at the chosen difficulty's first
  level for the whole game. With **EASY COMBO BREAKER: ON**, any attack
  button breaks a combo you are taking, in every mode: the program does your
  fighter's breaker move at the strength that breaks the hit coming in (if
  it doesn't take, press again and it tries the next strength).

### Pause

Start pauses any fight in these modes: continue, or back to the main menu.

### The Super Nintendo codes

- **Stage select:** on the character screen, hold a direction and pick your
  fighter with a button. The first player to pick chooses.

| | Down + | Up + |
|---|---|---|
| L (weak punch) | Canyon Bridge | Ice Temple |
| Y (medium punch) | Bloody Arena | Skull Room |
| X (fierce punch) | Lava Pit | Ice Sculpture |
| R (weak kick) | City Street | Castle Roof |
| B (medium kick) | Fireplace | City Roof |
| A (fierce kick) | Factory | Desert Roof |

  Sky Rooftop: both players pick with Down + B.
- **Speed**, on the VS screen, held until the fight starts (both players in
  a two-player game):
  - Left + all three kicks: slow
  - Right + all three punches: fast
  - Right + all three kicks: faster
  - Left + all three punches: fastest

  The music keeps its pitch.
- **Random fighter:** Up + Start on the character screen.

### More settings

**F1** (or the pad's Guide button) opens the program's own menu: fullscreen
and resolution, picture filters (CRT and others), controls, volume, save
states, rewind and netplay, all from UltraCombo and covered in the
[UltraCombo manual](UltraCombo-manual.md).

For the plain arcade game without the console menus, set `home_menu = no`
in `ki1.ini`.

## 6. Troubleshooting

- **One pad moves both players.** Some pads (certain 8BitDo models, for
  instance) show up twice on a PC. ComboBreaker notices within a few seconds
  of play and drops the duplicate, with a message on screen.
- **"Missing" or "does not match".** Check the names are exact (no added
  `.bin` or `.zip`) and that the set is version L1.5D.
- **The first launch is slow.** That's the one-time preparation. Deleting
  `cache` makes it run again.
- **Back to default settings.** Close the program and delete `ki1.ini`; it
  is written again at the next start.
- **Something went wrong.** `ki1.log` in the program's folder says what
  happened. Attach it to an
  [issue on GitHub](https://github.com/Vito2492/ComboBreaker/issues).

## 7. Android

Tested on a phone. If something doesn't work on yours, please report it on
[GitHub](https://github.com/Vito2492/ComboBreaker/issues).

1. Download `ComboBreaker-Android.apk` from
   [Releases](https://github.com/Vito2492/ComboBreaker/releases) on the phone
   and open it. Android asks to allow installing apps from that source (the
   browser or the file manager): allow it.
2. Copy your ten game files (or your MAME `kinst.zip`) to a folder on the
   phone, for example `Download/kinst`.
3. Open ComboBreaker. The first time it asks for the folder: tap *Choose
   folder*, pick that folder and tap *Use this folder*. The app copies the
   files into its own folder
   (`Android/data/io.github.vito2492.combobreaker/files/run`) and won't ask
   again.
4. The first game takes a little longer while it prepares the game for the
   phone.

**Controls:** a Bluetooth or USB gamepad is recommended (Xbox, PlayStation,
8BitDo and most others work). Without one, on-screen controls in the Super
Nintendo layout show: d-pad and Start on the left, Y X A B and L R on the
right. The corner button, or the phone's Back button, opens the program's
menu. `touch_controls` in `ki1.ini` (`auto`, `on`, `off`) turns them off or
keeps them on.

Android 7 or later, 64-bit. The APK includes no game files.
