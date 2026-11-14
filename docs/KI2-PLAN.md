# ComboBreaker 2: plan

The arcade Killer Instinct 2 (MAME `kinst2`, L1.4), run by the same program
as ComboBreaker, with the menus and modes of its console version
(Killer Instinct Gold, Nintendo 64). Branch `ki2-gold`.

Everything in `home.c` (console menu, modes, pause, Practice and its dummy,
EASY COMBO BREAKER, MOVE LIST, netplay's two-player mode) is shared code
that turns on per game once `game_ki2.c` knows the RAM it needs. So the work
is mostly finding KI2's RAM, then what is new in the console version.

## 1. RAM to find (L1.4)

Same method as KI1: deterministic headless scripts, `KI2_DUMP_AT`,
`KI2_WATCH`, `KI2_POKE`, diffs with numpy.

| Field | KI1 (L1.5D) | Used for |
|---|---|---|
| attract, title_timer | 0x8f2da, 0x8f2c0 | menu on the title, high scores |
| in_game, picked, player_step | 0x8f326, 0x8f334, 0x68 | modes, pause, picks |
| match_over, winner, fight_scene, after_match | 0x86230, 0x86204, 0x884fd, 0x40e50 | rematch, tournament |
| stage table/current, music | 0xe4a4, 0x86238, 0x86240 | stage codes, music per stage |
| cpu_level, level_table, difficulty | 0x8dc2c, 0x55c, 0x3a168 | fixed difficulty, Practice CPU |
| opponents, cursor | 0x8f428, 0x8f314 | Practice against the CPU |
| fighter_x, fighter_state | 0x8bc68, 0x8bc1c | dummy block, crouch |
| fighter_combo, fighter_strength, fighter_slot | 0x8bc8d, 0x8bc62, 0x8f33b | easy breaker |
| fighter_shadow | 0x8bcbb | shadow indicator |
| breakers, combos, moves | per fighter | easy breaker, COMBO dummy, MOVE LIST |

Already known for KI2: health 0x7fc105 (step 0x100), timer 0x7f7a10..12,
credits 0x7ff124, the adjustments (difficulty) on the disk.

Found (2026-11-14): attract 0x7ff29c (0 in the attract loop; the title
stays once Start is pressed, no countdown), in_game 0x7ff148, picked
0x7ff149, player_step 0x130, fight_scene 0x7fc0d3 (a pointer's top byte in
player 1's fighter), after_match = match_over 0x7ff136 (continue screen),
opponents 0x7ff428 (the one-player order, 11 bytes). No winner byte found
yet: the one with more health is taken. With these the console menu opens
on KI2, a one-player game starts from it and pauses.

## 2. Console menus (Killer Instinct Gold)

To confirm from the manual and the game: main menu (Arcade, Team, Tournament,
Practice, Training, Options), Options items, the Team mode (a team of
fighters, the next one comes in when one falls), knockout tournament.
Arcade, versus, tournament and Practice come almost straight from
ComboBreaker; Team mode is new and is built on top, as the tournament was.

## 3. Then

Move lists and breakers for KI2's fighters (11, Gargos too), the dummy
combos, tests on every platform, packages and a release.

## Game files

Never in the repository or the packages. Victor puts his own `kinst2` set in
`Instinct\ComboBreaker2\run` on his PC for testing.
