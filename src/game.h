// What differs between the games UltraCombo plays. Both run on the same
// board (MAME's kinst driver). The program holds both profiles (game_ki1.c,
// game_ki2.c) and picks one at startup; `game` is the one in use.
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *label;          // as shown to the player, e.g. "1.5d"
    const char *rom;            // boot ROM file name
    uint32_t crc;               // CRC32 of the good dump (MAME's)
} game_version_t;

typedef struct {
    const char *id;             // "ki1" / "ki2"
    const char *title;          // the game it plays, e.g. "Killer Instinct 2"
    const char *product;        // window and dialog titles, e.g. "UltraCombo 2"
    const char *icon_text;      // up to 3 characters; a final digit is drawn glowing

    game_version_t versions[4]; // [0] is the default; label NULL ends the list
    const char *sound_roms[8];  // U10-U13, U33-U36
    uint32_t sound_crc[8];
    // Sound DSP program images to translate (adsprec.c): where U10 holds them
    // (byte offset, 4 bytes per word), the program address they run at and
    // their length; boot: a boot page, its length in its header. Found by
    // logging what the DSP runs; anything else it runs is interpreted.
    struct { uint32_t rom; uint16_t pm, words; bool boot; } sound_code[6];   // words 0, !boot: end
    const char *set;            // MAME's name for the game (its CHD sits in a folder of that name)
    const char *disk;           // raw disk image file name
    const char *chd;            // or the MAME CHD itself
    const char *chd_sha1;       // the CHD's SHA1 as chdman shows it (MAME's)

    // drive geometry (from the CHD metadata) and the identify word where the
    // game checks the drive model
    uint32_t cyls, heads, secs;
    int serial_offset;

    // I/O ports (physical addresses). Each write port is paired with a read
    // port the same way on both boards: P1/video control, P2/sound reset,
    // volume/sound control, "unused"/sound data.
    struct { uint32_t p1, p2, volume, unused, dsw, coin, nop; } io;   // nop: written, ignored

    int snapshot_frames;        // the game has loaded all of its code by then
    int input_lag;              // frames the game itself waits before acting on input
    bool has_blood_colour;      // DIP switch 6: red or white blood (KI1)

    // Game RAM locations, as offsets into ram2; 0 = not known for this game,
    // and the features using them are off.
    uint32_t credits;           // credit count (32-bit)
    int credits_needed;         // credits to start with the default coinage
    uint32_t health_p1;         // current health bar, one byte per player...
    uint32_t health_step;       // ...this far apart
    int health_max;
    uint32_t health_frac;       // the low byte of the 16-bit health, if any (0 = not known)
    uint32_t health_shown;      // the 16-bit health the bar shows, easing toward the real one
    uint32_t timer_tick, timer_ones, timer_tens;   // round clock
    uint32_t vars, vars_len;    // range of game variables for netplay's quick check
    // For the console-style menu (home.c): 1 while the attract loop runs; per
    // player (player_step apart) 1 once in a game and once a fighter is picked.
    uint32_t attract, in_game, picked, player_step;
    bool attract_zero;          // the attract flag is 0 while the attract loop runs instead
    // Stage choice: the game draws two-player stages from stage_pool (32
    // bytes) at random, never the one in stage_last; the result lands in stage.
    uint32_t stage_pool, stage_last, stage;
    // Fight music: with music_set non-zero the game plays track `music` (by
    // stage number, as the one-player game does) instead of drawing one.
    uint32_t music_set, music;
    uint32_t title_timer;       // 16 bits: frames until the title screen gives way to the attract loop
    uint32_t match_over;        // non-zero from a lost match through the continue countdown
    // the side that won a match: 1 player 1, 2 player 2, 0 a draw. Set when
    // the game decides it, which can be well before the match's end shows
    // (an ultimate combo); read it once fight_scene has gone.
    uint32_t match_result;      // 0: not known; the one with more health won
    // non-zero while a fight's scene is up: from the round's start through
    // the winner's moment on the stage (finishing move, ultimate); zero from
    // the victory screen (supreme or awesome victory, perfect) if there is
    // one, else from the screens after the match (continue, high scores)
    uint32_t fight_scene;
    // non-zero while the screens after a match (continue, high scores) are up
    uint32_t after_match;
    // The CPU's skill, 1 to 50, set as each one-player fight starts from a
    // table (level_table: 16 bytes per difficulty, a level per fight, then
    // the bounds of the adjustment for wins and losses) and read by the
    // fighters' AI all through it. difficulty: the setting the game booted
    // with, 0 to 4.
    uint32_t cpu_level, level_table, difficulty;
    // The one-player game's opponents, one byte each in fight order (11,
    // drawn as a game starts), by their place on the character screen;
    // player 1's cursor there (16 bits, the place), moved by the game alone.
    uint32_t opponents, cursor;
    uint32_t fighter_x;         // 32 bits per fighter, health_step apart: position across the screen, 16.16
    uint32_t fighter_state;     // a byte per fighter, health_step apart: 0 standing still
    // Combo breakers: per fighter (health_step apart) the hits of the combo
    // it is landing, and the strength of the attack it is doing (1 quick,
    // 2 medium, 4 fierce); per player (player_step apart) the fighter, by
    // its place on the character screen; and each fighter's breaker move,
    // by that place: directions relative to the way it faces (F, B, D, U,
    // DF...), then P or K.
    uint32_t fighter_combo, fighter_strength, fighter_slot;
    // per fighter: shadow moves left from its last combo breaker (3, one
    // used by each; the arcade flashes a line at the end of the health bar)
    uint32_t fighter_shadow;
    const char *breakers[11];
    // A combo each fighter can do on any other, for the training dummy to
    // break: from this far (pixels between the fighters), "C" (back for 40
    // frames, then forward) or "Q" (down, down-forward, forward), the
    // opening button (HQ HM HF LQ LM LF), and the button repeated after it.
    const char *combos[11];
    // Each fighter's moves for the console menu's MOVE LIST, by the same
    // place: "NAME|INPUT" lines, '#' for headings, '!' for notes
    const char *const *moves;
    const char *codes;              // the CODES page, the same way
    const char *fighter_names[11];   // by the same place, for the menus

    // Operator adjustments on the disk: adj_len bytes at adj_data, the first
    // set once they were edited, with the game's checksum of them at adj_sum.
    // adj_len 0 = not known. KI1 keeps that checksum in two header sectors
    // instead (at +4), each with a generation count (+0, +0x1fc) and a
    // checksum of its own first 0x1f8 bytes (+0x1f8).
    uint32_t adj_sum, adj_data, adj_len, adj_difficulty;
    uint32_t adj_headers[2];
} game_t;

// The program as a whole: its name and its data folder.
#define PRODUCT "UltraCombo"

extern game_t game;                          // the profile in use (game_select)
extern const game_t game_ki1, game_ki2;
extern const game_t *const game_profiles[]; // NULL-terminated
void game_select(const game_t *g);
const game_t *game_find(const char *id);    // by id ("ki1", "ki2"), NULL if unknown
