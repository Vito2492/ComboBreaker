// Console-style home menu (see home.h). The arcade game is never patched:
// the menu reads a few RAM flags from the game profile and plays the
// cabinet's Start buttons for the player, the way an attendant would.
#include "home.h"
#include "config.h"
#include "game.h"
#include "machine.h"
#include "mem.h"
#include "adjust.h"
#include "state.h"
#include "osd.h"
#include "training.h"
#include "menufont_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// The SNES pad's buttons, in the order of the cabinet's six: L Y X are
// weak, medium, fierce punch and R B A weak, medium, fierce kick.
static const char pad_letters[] = "LYXRBA";

/* ---------------------------------------------------------------- */
/* Lettering: a blocky pixel face like the SNES menus'               */
/* ---------------------------------------------------------------- */

enum { F_SMALL, F_BODY, F_BIG };

typedef struct { const uint8_t *px; const void *glyph; int h; } face_t;
typedef struct { uint32_t off; uint8_t w; uint8_t adv4; } gl_t;

static face_t face(int f)
{
    if (f == F_SMALL) return (face_t){ mf_small_px, mf_small_glyph, MF_SMALL_H };
    if (f == F_BIG) return (face_t){ mf_big_px, mf_big_glyph, MF_BIG_H };
    return (face_t){ mf_body_px, mf_body_glyph, MF_BODY_H };
}

static const gl_t *glyph_of(face_t fc, char c)
{
    if (c < 32 || c > 126) c = '?';
    return &((const gl_t *)fc.glyph)[c - 32];
}

// Straight-alpha "over" onto the overlay.
static void blend(uint32_t *d, uint32_t rgb, unsigned a)
{
    if (!a) return;
    unsigned da = *d >> 24;
    unsigned oa = a + da * (255 - a) / 255;
    if (!oa) return;
    uint32_t out = oa << 24;
    for (int s = 0; s < 24; s += 8) {
        unsigned sc = rgb >> s & 255, dc = *d >> s & 255;
        unsigned c = (sc * a + dc * da * (255 - a) / 255) / oa;
        out |= (c > 255 ? 255 : c) << s;
    }
    *d = out;
}

static int text_width(int f, const char *s)
{
    face_t fc = face(f);
    int w4 = 0;
    for (; *s; s++) w4 += glyph_of(fc, *s)->adv4;
    return (w4 + 3) / 4;
}

// Each row a little darker than the one above, as the SNES menus shade
// their letters (bevel = false for shadows).
static uint32_t row_tint(uint32_t rgb, int r, int h)
{
    unsigned k = 256 - (unsigned)(r * 80 / (h > 1 ? h - 1 : 1));
    return ((rgb >> 16 & 255) * k >> 8) << 16 | ((rgb >> 8 & 255) * k >> 8) << 8 | ((rgb & 255) * k >> 8);
}

static void text_layer(uint32_t *ov, int f, int x, int y, const char *s, uint32_t rgb, int alpha, bool bevel)
{
    face_t fc = face(f);
    int pen4 = x * 4;
    for (; *s; s++) {
        const gl_t *g = glyph_of(fc, *s);
        int gx = pen4 / 4 - MF_BODY_PAD;
        for (int r = 0; r < fc.h; r++) {
            int yy = y + r;
            if (yy < 0 || yy >= 240) continue;
            for (int c = 0; c < g->w; c++) {
                int xx = gx + c;
                if (xx < 0 || xx >= 320) continue;
                unsigned a = fc.px[g->off + (uint32_t)r * g->w + (uint32_t)c] * (unsigned)alpha / 255;
                blend(&ov[yy * 320 + xx], bevel ? row_tint(rgb, r, fc.h) : rgb, a);
            }
        }
        pen4 += g->adv4;
    }
}

// Text with a hard dark shadow, legible over the title's busy art.
static void text(uint32_t *ov, int f, int x, int y, const char *s, uint32_t rgb, int alpha)
{
    text_layer(ov, f, x + 1, y + 1, s, 0x000000, alpha, false);
    text_layer(ov, f, x, y, s, rgb, alpha, true);
}

static void text_c(uint32_t *ov, int f, int y, const char *s, uint32_t rgb, int alpha)
{
    text(ov, f, (320 - text_width(f, s)) / 2, y, s, rgb, alpha);
}

/* ---------------------------------------------------------------- */
/* Game state, from the profile's RAM flags                          */
/* ---------------------------------------------------------------- */

static int flag(uint32_t off) { return off ? ram2[off] : -1; }
static bool in_attract(void) { return flag(game.attract) == 1; }
static bool in_game(int p) { return flag(game.in_game + (uint32_t)p * game.player_step) == 1; }
static bool picked(int p) { return flag(game.picked + (uint32_t)p * game.player_step) == 1; }
static bool playing(void) { return in_game(0) || in_game(1); }
// Past the character screen: everyone in the game has picked a fighter.
static bool all_picked(void) { return in_game(0) && picked(0) && (!in_game(1) || picked(1)); }
static bool fighting(void) { return all_picked() && game_health(0) >= 0; }

static bool net;               // online: everything here must come out the same on both sides

bool home_enabled(void)
{
    // the RAM locations were found in the default version (L1.5D for
    // UltraCombo); a ROM given by path is taken to be that one
    return (cfg.home_menu || net) && game.attract && game.in_game && game.picked && machine_version() <= 0;
}

/* ---------------------------------------------------------------- */
/* Stage select: the home version's codes at the character screen    */
/* ---------------------------------------------------------------- */

// Hold a direction and pick a fighter with one of the buttons. The SNES
// pad's default layout is L Y X = weak, medium, fierce punch and R B A =
// weak, medium, fierce kick (its Options screen); those are the cabinet's
// quick/medium/fierce buttons. The first player to pick chooses.
static const struct { int dir, button, stage; const char *name; } stage_codes[] = {
    { BTN_DOWN, BTN_HM, 12, "BLOODY ARENA" },       // Down + Y
    { BTN_DOWN, BTN_HQ, 5,  "CANYON BRIDGE" },      // Down + L
    { BTN_UP,   BTN_LQ, 6,  "CASTLE ROOF" },        // Up + R
    { BTN_UP,   BTN_LM, 7,  "CITY ROOF" },          // Up + B
    { BTN_DOWN, BTN_LQ, 2,  "CITY STREET" },        // Down + R
    { BTN_UP,   BTN_LF, 8,  "DESERT ROOF" },        // Up + A
    { BTN_DOWN, BTN_LF, 15, "FACTORY" },            // Down + A
    { BTN_DOWN, BTN_LM, 1,  "FIREPLACE" },          // Down + B
    { BTN_UP,   BTN_HF, 11, "ICE SCULPTURE" },      // Up + X
    { BTN_UP,   BTN_HQ, 3,  "ICE TEMPLE" },         // Up + L
    { BTN_DOWN, BTN_HF, 20, "LAVA PIT" },           // Down + X
    { BTN_UP,   BTN_HM, 13, "SKULL ROOM" },         // Up + Y
};
#define SKY_ROOFTOP 9           // Down + B (medium kick) on both players' picks

static struct {
    int want;                   // stage to force, -1 none
    bool armed;                 // the pool is rewritten until the game draws
    uint8_t pool[32], last;     // the game's own values, put back afterwards
    bool was_picked[2];
    int first_code;             // the first picker's code, -1 none
    uint32_t hist[2][8];        // the last frames' buttons (a pick shows a frame late)
    unsigned at;
} stg = { .want = -1, .first_code = -1 };

static void poke(uint32_t off, uint8_t v)
{
    if (ram2[off] == v) return;
    ram2[off] = v;
    ram_touch(&ram2[off]);
}

static void stage_restore(void)
{
    if (!stg.armed) return;
    for (int i = 0; i < 32; i++) poke(game.stage_pool + (uint32_t)i, stg.pool[i]);
    poke(game.stage_last, stg.want >= 0 ? (uint8_t)stg.want : stg.last);
    stg.armed = false;
}

static void stage_clear(void)
{
    stage_restore();
    stg.want = stg.first_code = -1;
    stg.was_picked[0] = stg.was_picked[1] = false;
}

// Which code (index into stage_codes, or SKY_ROOFTOP's -2) a pick was made with.
static int pick_code(uint32_t held)
{
    bool down = held >> BTN_DOWN & 1, up = held >> BTN_UP & 1;
    if (!down && !up) return -1;
    for (size_t i = 0; i < sizeof stage_codes / sizeof stage_codes[0]; i++)
        if ((stage_codes[i].dir == BTN_DOWN ? down : up) && held >> stage_codes[i].button & 1) return (int)i;
    return -1;
}

// Called every frame of a two-player mode, with the inputs the game gets.
static void stage_frame(uint32_t held[2])
{
    if (!game.stage_pool) return;
    // the game shows a pick the frame after the press: look at the last few frames
    uint32_t recent[2] = { 0, 0 };
    stg.at = (stg.at + 1) % 8;
    for (int p = 0; p < 2; p++) {
        stg.hist[p][stg.at] = held[p];
        for (int i = 0; i < 8; i++) recent[p] |= stg.hist[p][i];
    }
    // The arcade has stage codes of its own (a direction held on the pick),
    // mapped differently: the direction stays ours until a fighter is picked.
    for (int p = 0; p < 2; p++)
        if (in_game(p) && !picked(p)) held[p] &= ~(1u << BTN_UP | 1u << BTN_DOWN);
    for (int p = 0; p < 2; p++) {
        bool now = picked(p);
        if (now && !stg.was_picked[p]) {
            int code = pick_code(recent[p]);
            if (stg.want < 0 && code >= 0 && stg.first_code < 0) {
                stg.first_code = code;
                stg.want = stage_codes[code].stage;
            }
            // Sky Rooftop: Down + B on both picks
            bool sky = recent[p] >> BTN_DOWN & 1 && recent[p] >> BTN_LM & 1;
            if (p == 1 && sky && stg.was_picked[0] && stg.first_code >= 0 && stage_codes[stg.first_code].stage == 1)
                stg.want = SKY_ROOFTOP;
            if (stg.want >= 0) osd_message("Stage: %s", stg.want == SKY_ROOFTOP ? "SKY ROOFTOP" : stage_codes[stg.first_code].name);
        }
        stg.was_picked[p] = now;
    }
    // From the first pick on: make every draw come out as the chosen stage,
    // until the game has drawn it (it draws as soon as both have picked).
    if (stg.want >= 0 && ram2[game.stage] != stg.want) {
        if (!stg.armed) {
            memcpy(stg.pool, &ram2[game.stage_pool], 32);
            stg.last = ram2[game.stage_last];
            stg.armed = true;
        }
        for (int i = 0; i < 32; i++) poke(game.stage_pool + (uint32_t)i, (uint8_t)stg.want);
        poke(game.stage_last, 0xff);        // its no-repeat check would loop forever
    } else if (stg.armed && ram2[game.stage] == stg.want) {
        stage_restore();
    }
    if (!picked(0) && !picked(1) && !stg.armed) stg.want = stg.first_code = -1;   // back at the character screen
}

/* ---------------------------------------------------------------- */
/* Menu model                                                        */
/* ---------------------------------------------------------------- */

enum { MODE_NONE, MODE_ARCADE, MODE_VS, MODE_TOURNAMENT, MODE_PRACTICE };
enum { PAGE_CLOSED, PAGE_MAIN, PAGE_OPTIONS, PAGE_PAUSE, PAGE_ROSTER, PAGE_NEXT };
enum { MAIN_ARCADE, MAIN_VS, MAIN_TOURNAMENT, MAIN_PRACTICE, MAIN_OPTIONS, MAIN_COUNT };
enum { OPT_DIFFICULTY, OPT_FIXED, OPT_RANDOM, OPT_TIMER, OPT_BLOOD, OPT_FINISH, OPT_BREAKER,
       OPT_WP, OPT_MP, OPT_FP, OPT_WK, OPT_MK, OPT_FK, OPT_RESET, OPT_EXIT, OPT_COUNT };
enum { PAUSE_CONTINUE, PAUSE_DUMMY, PAUSE_LEVEL, PAUSE_MENU, PAUSE_COUNT };   // DUMMY: Practice only, LEVEL: against the CPU

// as on the SNES title screen
static const char *const main_items[MAIN_COUNT] = { "1 PLAYER GAME", "2 PLAYER GAME", "TOURNAMENT GAME", "PRACTICE GAME", "OPTIONS" };
static const char *const blood_names[] = { "OFF", "LOW", "MEDIUM", "HIGH" };

// The end of a match being watched (match_ended): armed once the fight is
// on; clear: the after-match flag was down during it; t: frames since the
// fight's scene went.
typedef struct { bool armed, clear; int t; } match_end_t;

static struct {
    int page, sel, main_sel, mode;
    int want_players;           // players the chosen mode starts with
    int seq_wait;               // frames until the next Start press
    int press, press_player;    // frames left of a Start press being played
    bool starting;              // pressing Start until the game has started
    bool switches_changed;      // DIP switches changed in Options: reset on leaving
    bool title_saved;           // a state at the title screen to come back to
    bool restore;               // load that state now (from the pause panel)
    bool route_armed;           // practice: player 1 has let go since picking
    // Practice against the CPU: a one-player game where player 1 picks his
    // fighter (kept, not yet given to the game), then the CPU's, then the
    // game gets the first pick and the second becomes every opponent.
    bool cpu;
    int cpu_step;               // 0 picking his own, 1 the CPU's, 2 picked
    uint8_t own, opp;
    int cpu_wait;               // frames the cursor shows his fighter before it is picked
    uint32_t cpu_button;        // the button that picked the CPU's, to pick his
    int cpu_last;               // the cursor before the last step back, -1 none
    bool cpu_dir;               // stepping back to the left
    int pause_dummy;            // the dummy as the pause panel was opened
    uint32_t prev[2];           // last frame's buttons, for presses
    unsigned mprev;             // last frame's menu buttons (frontend_menu_buttons)
    int repeat, repeat_dir;     // held up/down auto-repeat
    int cool;                   // frames before Start may be pressed again
    int again;                  // mode to start over once back at the title (a match ended)
    bool live;                  // a fight of this mode has started (the end-of-match check is armed)
    match_end_t vs_end;         // the 2 Player match being fought
    bool title_wait;            // a Start in the attract loop, held until the sound board is up
    int col;                    // Options > CONTROLS: the player column chosen
    int idle;                   // frames the main menu has been left alone
    bool left_menu;             // the menu gave way to the attract loop
    bool menu_after_title;      // a Start from there: open the menu once the title is up
    unsigned anim;              // frames the menu has shown, for its fade-in
} h;

/* ---------------------------------------------------------------- */
/* Random fighter: Up + Start on the character screen                 */
/* ---------------------------------------------------------------- */

// The cursor is moved a random number of places, then the fighter is
// picked, all with the player's own inputs.
static struct { int steps[2], t[2], wait[2]; uint32_t seed; } rnd;
static uint32_t rnd_ticks(void) { static uint32_t n; return net ? 0 : ++n * 2654435761u ^ (uint32_t)clock(); }

static void random_frame(uint32_t held[2], const uint32_t pressed[2])
{
    rnd.seed = rnd.seed * 1103515245u + 12345u + rnd_ticks();
    for (int p = 0; p < 2; p++) {
        if (!in_game(p) || picked(p)) { rnd.steps[p] = rnd.t[p] = rnd.wait[p] = 0; continue; }
        // Options > RANDOM SELECT: every human player's fighter is drawn
        bool always = cfg.home_random && !net && ++rnd.wait[p] == 45;
        if (!rnd.steps[p] && (always || (pressed[p] >> BTN_START & 1 && held[p] >> BTN_UP & 1))) {
            rnd.steps[p] = 1 + (int)(rnd.seed >> 16) % 10;
            rnd.t[p] = 0;
            osd_message("Player %d: random fighter", p + 1);
        }
        if (!rnd.steps[p]) continue;
        held[p] &= ~(((1u << BTN_START) - 1) | 1u << BTN_START);
        int t = rnd.t[p]++;
        int n = rnd.steps[p];
        if (t < n * 8) { if (t % 8 < 3) held[p] |= 1u << BTN_RIGHT; }       // step along the row
        else if (t < n * 8 + 4) held[p] |= 1u << BTN_HQ;                   // and pick
        else rnd.steps[p] = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Music: each stage's own, as on the SNES                            */
/* ---------------------------------------------------------------- */

// The arcade draws a two-player fight's music at random; the home
// versions play the stage's. Once the game has drawn the stage (it never
// repeats the last one, so the stage number changes), it is told to play
// that stage's track, the way the one-player game does.
static struct { int before; bool done; } mus = { -1, false };

static void music_frame(void)
{
    if (!game.music_set || !game.stage) return;
    if (!all_picked()) { mus.before = -1; mus.done = false; return; }
    if (mus.done) return;
    if (mus.before < 0) mus.before = ram2[game.stage];
    int st = ram2[game.stage];
    if (fighting()) { mus.done = true; return; }        // too late: leave the game's own
    if (st != mus.before && st > 0 && st <= 20 && (st < 16 || st == 20)) {
        poke(game.music_set, 1);
        poke(game.music, (uint8_t)st);
        mus.done = true;
    }
}

/* ---------------------------------------------------------------- */
/* Speed: held on the VS screen, both players in a two-player game    */
/* ---------------------------------------------------------------- */

#define PUNCHES (1u << BTN_HQ | 1u << BTN_HM | 1u << BTN_HF)     // L Y X
#define KICKS   (1u << BTN_LQ | 1u << BTN_LM | 1u << BTN_LF)     // R B A
static const struct { uint32_t dir, buttons; double speed; const char *name; } speed_codes[] = {
    { 1u << BTN_LEFT,  KICKS,   0.80, "SLOW" },
    { 1u << BTN_RIGHT, PUNCHES, 1.15, "FAST" },
    { 1u << BTN_RIGHT, KICKS,   1.30, "FASTER" },
    { 1u << BTN_LEFT,  PUNCHES, 1.45, "FASTEST" },
};

static struct { double speed; bool chosen; int since; } spd = { 1, false, 0 };

static void speed_frame(const uint32_t held[2])
{
    // the VS screen: the first seconds after everyone in the game has picked
    bool all = all_picked();
    if (!picked(0) && !picked(1)) { spd.speed = 1; spd.chosen = false; spd.since = 0; }   // a new match
    if (!all) return;
    if (spd.since < 100000) spd.since++;
    if (spd.chosen || spd.since > 360) return;
    for (size_t i = 0; i < sizeof speed_codes / sizeof speed_codes[0]; i++) {
        bool every = true;
        for (int p = 0; p < 2; p++) {
            if (!in_game(p)) continue;
            uint32_t want = speed_codes[i].dir | speed_codes[i].buttons;
            if ((held[p] & want) != want) every = false;
        }
        if (every) {
            spd.speed = speed_codes[i].speed;
            spd.chosen = true;
            osd_message("Speed: %s", speed_codes[i].name);
        }
    }
}

double home_speed(void)
{
    if (net) return 1;
    return home_enabled() && h.mode != MODE_NONE && fighting() && h.page == PAGE_CLOSED ? spd.speed : 1;
}

/* ---------------------------------------------------------------- */
/* Tournament: up to eight named players, as on the SNES              */
/* ---------------------------------------------------------------- */

// Players take turns in pairs: the winner stays on and the loser goes to
// the back of the line. Each match is a two-player game on the arcade's
// own character screen; the winner is read from the game's win counters.
#define TN_MAX 8
static struct {
    char name[TN_MAX][4];
    int w[TN_MAX], l[TN_MAX];
    int queue[TN_MAX], qn;      // fighting order: [0] (player 1 side) and [1] fight next
    int row, letter;            // roster cursor and the letter being chosen
    match_end_t end;            // the match being fought
    int won;                    // side that won it
    bool next;                  // go on to the next fight (title screen, NEXT FIGHT)
} tn;

static int tn_named(void)
{
    int n = 0;
    for (int i = 0; i < TN_MAX; i++) n += tn.name[i][0] != 0;
    return n;
}

// The fighting order follows the roster, keeping the order already played.
static void tn_build_queue(void)
{
    int q[TN_MAX], n = 0;
    for (int k = 0; k < tn.qn; k++) if (tn.name[tn.queue[k]][0]) q[n++] = tn.queue[k];
    for (int i = 0; i < TN_MAX; i++) {
        bool in = false;
        for (int k = 0; k < n; k++) in |= q[k] == i;
        if (tn.name[i][0] && !in) q[n++] = i;
    }
    memcpy(tn.queue, q, sizeof q);
    tn.qn = n;
}

static int health_of(int p) { return ram2[game.health_p1 + (uint32_t)p * game.health_step]; }

static bool scene_up(void) { return ram2[game.fight_scene] != 0; }

// FIXED DIFFICULTY: the arcade raises the CPU's level fight by fight from a
// table, and adjusts it for wins and losses; this keeps it at the table's
// first level for the difficulty the game booted with. The game sets the
// level once as a fight starts and its fighters read it all through, so
// setting it back every frame is enough.
static void hold_level(void)
{
    if (!game.cpu_level || !game.level_table || !game.difficulty || !(in_game(0) ^ in_game(1))) return;
    unsigned d = ram2[game.difficulty];
    if (d > 4) return;
    uint8_t lv = ram2[game.level_table + 16 * d];
    if (lv >= 1 && lv <= 50) poke(game.cpu_level, lv);
}

// Practice against the CPU, on the one-player character screen: the first
// pick is held back and kept, the cursor then chooses the CPU's fighter, and
// with that pick the cursor goes back to the first and picks it. Every opponent of the game is the CPU's choice,
// so the same fighter comes back if the fight ever ends.
static void cpu_pick(uint32_t held[2], const uint32_t pressed[2])
{
    const uint32_t attacks = (1u << BTN_START) - 1 - (0xfu << BTN_UP);
    if (h.cpu_step == 2) {
        for (uint32_t i = 0; i < 11; i++) poke(game.opponents + i, h.opp);
        if (in_game(0) && !picked(0)) {             // until the game has taken it
            held[0] &= ~((0xfu << BTN_UP) | attacks);
            if (h.cpu_wait > 0) h.cpu_wait--;
            else if (ram2[game.cursor] != h.own) {
                // right, or left once right goes no further
                if (ram2[game.cursor] == h.cpu_last) h.cpu_dir = !h.cpu_dir;
                h.cpu_last = ram2[game.cursor];
                held[0] |= 1u << (h.cpu_dir ? BTN_LEFT : BTN_RIGHT);
                h.cpu_wait = 12;
            }
            else held[0] |= h.cpu_button;
        }
        return;
    }
    if (!in_game(0) || picked(0)) return;
    bool press = (pressed[0] & attacks) != 0;
    if (h.cpu_step == 0 && press) {
        h.own = ram2[game.cursor];
        h.cpu_step = 1;
    } else if (h.cpu_step == 1 && press) {
        h.opp = ram2[game.cursor];
        for (uint32_t i = 0; i < 11; i++) poke(game.opponents + i, h.opp);
        // the cursor goes back to his fighter, a place at a time as if
        // he moved it, and the same button picks it
        h.cpu_wait = 6;
        h.cpu_last = -1;
        h.cpu_dir = false;
        h.cpu_button = pressed[0] & attacks & (0u - (pressed[0] & attacks));
        h.cpu_step = 2;
        fprintf(stderr, "home: practice: player 1 fighter %d, CPU %d\n", h.own, h.opp);
    }
    held[0] &= ~attacks;
}

// A match is over once the game moves on to its own screens after it
// (continue, high scores): that waits for the winner's whole moment,
// finishing move, ultimate, and the victory screen (supreme or awesome
// victory, perfect) with the fight's music. Called every frame of a mode;
// true once, on the frame the match ends.
static bool match_ended(match_end_t *e)
{
    if (!game.fight_scene || !game.after_match) return false;
    bool after = ram2[game.after_match] != 0;
    if (all_picked() && scene_up()) {
        if (!e->armed) e->clear = false;
        e->armed = true;
        e->clear |= !after;
        e->t = 0;
        return false;
    }
    if (!e->armed || scene_up()) return false;
    // the scene is gone: a victory screen, or the screens after the match.
    // Should the flag never have been clear in the fight, end now, and give
    // up waiting for it after 15 seconds.
    if (e->clear && !after && ++e->t < 900) return false;
    e->armed = false;
    return true;
}

// The side that won the match just ended: the game's own record of it, or
// on a draw the one with more health left.
static int match_winner(void)
{
    int r = ram2[game.match_result];
    if (r == 1 || r == 2) return r - 1;
    return health_of(0) >= health_of(1) ? 0 : 1;
}

// Called each frame of a tournament match: when it ends, the winner stays
// on and the loser goes to the back of the line, and the tournament moves on
// to its next fight instead of the arcade's continue and high-score screens.
// How each match ends, in the log (for reports of an ending cut short).
static void match_log(void)
{
    static int pm = -1, pr = -1, ps = -1;
    if (!game.match_over || !game.match_result || !game.fight_scene) return;
    int m = ram2[game.match_over], r = ram2[game.match_result], sc = scene_up();
    if (m != pm || r != pr || sc != ps)
        fprintf(stderr, "home: frame %llu: match over %d, winner %d, fight scene %s\n",
                (unsigned long long)frontend_frames(), m, r, sc ? "up" : "down");
    pm = m; pr = r; ps = sc;
}

static void tn_frame(void)
{
    if (!game.match_result || !game.health_p1 || !match_ended(&tn.end)) return;
    tn.won = match_winner();
    if (tn.qn >= 2) {
        int winner = tn.queue[tn.won], loser = tn.queue[tn.won ^ 1];
        tn.w[winner]++;
        tn.l[loser]++;
        fprintf(stderr, "home: tournament: %s beat %s\n", tn.name[winner], tn.name[loser]);
        // the winner stays on; the loser goes to the back of the line
        int q[TN_MAX], n = 0;
        q[n++] = winner;
        for (int k = 2; k < tn.qn; k++) q[n++] = tn.queue[k];
        q[n++] = loser;
        memcpy(tn.queue, q, sizeof q);
    }
    tn.next = true;
}


static void title_state_path(char *buf, size_t n)
{
    snprintf(buf, n, "%scache/home-%s.state", data_dir(), game.id);
}

static void open_page(int page, int sel)
{
    h.idle = 0;
    h.page = page;
    h.sel = sel;
    h.anim = 0;
}

void home_reset(void)
{
    h.live = false;
    stage_clear();
    tn.end.armed = false;
    h.vs_end.armed = false;
    training_timer_off = false;
    spd.speed = 1;
    spd.chosen = false;
    training_forced = false;
    h.cpu = false;
    h.cpu_step = 0;
    h.mode = MODE_NONE;
    h.page = PAGE_CLOSED;
    h.starting = false;
    h.press = 0;
}

// A state was loaded: close any page, and carry on in the mode the state
// shows (one player: the arcade game; two: 2 Player), so the pause works.
void home_state_loaded(void)
{
    home_reset();
    h.seq_wait = 0;
    h.title_wait = h.menu_after_title = h.left_menu = h.restore = false;
    tn.next = false;
    memset(h.prev, 0xff, sizeof h.prev);
    if (!home_enabled() || !playing()) return;
    h.mode = in_game(0) && in_game(1) ? MODE_VS : MODE_ARCADE;
}

bool home_paused(void) { return h.page == PAGE_PAUSE; }
bool home_menu_open(void) { return h.page != PAGE_CLOSED && h.page != PAGE_PAUSE; }

static void change_option(int dir)
{
    int d = dir ? dir : 1;
    switch (h.sel) {
    case OPT_DIFFICULTY: {
        int dl = adjust_difficulty();
        if (dl < 0) { osd_message("The disk's settings block is not readable"); return; }
        dl += d;
        adjust_set_difficulty(dl < 1 ? 5 : dl > 5 ? 1 : dl);
        h.switches_changed = true;      // the game reads it at power-on
        break;
    }
    case OPT_FIXED: cfg.home_fixed = !cfg.home_fixed; break;
    case OPT_BREAKER: cfg.easy_breaker = !cfg.easy_breaker; break;
    case OPT_RANDOM: cfg.home_random = !cfg.home_random; break;
    case OPT_TIMER: cfg.home_timer = !cfg.home_timer; break;
    case OPT_BLOOD: cfg.blood = ((cfg.blood + d) % 4 + 4) % 4; h.switches_changed = true; break;
    case OPT_FINISH: cfg.finishing_moves = !cfg.finishing_moves; h.switches_changed = true; break;
    case OPT_WP: case OPT_MP: case OPT_FP: case OPT_WK: case OPT_MK: case OPT_FK:
        if (dir) h.col = dir < 0 ? 0 : 1;
        return;
    case OPT_RESET:
        if (dir) { h.col = dir < 0 ? 0 : 1; return; }
        snprintf(cfg.home_pad[h.col], sizeof cfg.home_pad[h.col], "%s", pad_letters);
        break;
    case OPT_EXIT:
        if (dir) return;
        if (h.switches_changed) {
            // the board reads its switches when it powers on: restart it and
            // come back to this menu once the attract loop is running again
            h.switches_changed = false;
            h.title_saved = false;
            frontend_settings_changed();
            frontend_reset_game();
            h.page = PAGE_CLOSED;
            h.mode = MODE_NONE;
            h.starting = false;
            h.restore = false;
            h.press = 0;
            h.seq_wait = -1;            // reopen the menu after the reset
            return;
        }
        open_page(PAGE_MAIN, MAIN_OPTIONS);
        return;
    }
    frontend_settings_changed();
}

static void start_mode(int mode)
{
    h.mode = mode;
    h.cpu = mode == MODE_PRACTICE && cfg.dummy == DUMMY_CPU && game.opponents && game.cursor;
    h.cpu_step = 0;
    h.want_players = mode == MODE_ARCADE || h.cpu ? 1 : 2;
    training_forced = mode == MODE_PRACTICE;
    h.page = PAGE_CLOSED;
    h.starting = true;
    h.seq_wait = 0;
}

static void choose(int item)
{
    if (item == MAIN_OPTIONS) { open_page(PAGE_OPTIONS, 0); return; }
    if (item == MAIN_TOURNAMENT) { h.main_sel = item; open_page(PAGE_ROSTER, 0); return; }
    h.main_sel = item;
    start_mode(item == MAIN_ARCADE ? MODE_ARCADE : item == MAIN_VS ? MODE_VS : MODE_PRACTICE);
}

// The roster: Up/Down a player, Left/Right a letter, A or B enters it,
// Y deletes the name, X the player (name and record), Start fights,
// R (no Select button on the cabinet) goes back.
static void roster_frame(uint32_t pr, unsigned mb, int ud, int lr)
{
    char *nm = tn.name[tn.row];
    size_t len = strlen(nm);
    if (ud) tn.row = (tn.row + ud + TN_MAX) % TN_MAX;
    if (lr) tn.letter = (tn.letter + lr + 26) % 26;
    if (mb & MB_SELECT) { open_page(PAGE_MAIN, MAIN_TOURNAMENT); return; }
    if (mb & (MB_A | MB_B) && len < 3) { nm[len] = (char)('A' + tn.letter); nm[len + 1] = 0; }
    if (mb & MB_Y) nm[0] = 0;
    if (mb & MB_X) { nm[0] = 0; tn.w[tn.row] = tn.l[tn.row] = 0; }
    if (pr >> BTN_START & 1) {
        if (tn_named() < 2) { osd_message("Enter at least two players"); return; }
        tn_build_queue();
        start_mode(MODE_TOURNAMENT);
    }
}

// Presses from either player, with auto-repeat on a held up/down.
// The menu's own buttons (SNES places, never remapped) pressed this frame.
static unsigned menu_pressed(void)
{
    unsigned now = frontend_menu_buttons(), pr = now & ~h.mprev;
    h.mprev = now;
    return pr;
}

static void navigate(const uint32_t held[2], unsigned mb, int *ud, int *lr, bool *ok, bool *back)
{
    *ud = *lr = 0;
    *ok = *back = false;
    uint32_t now = held[0] | held[1], pressed = (held[0] & ~h.prev[0]) | (held[1] & ~h.prev[1]);
    h.prev[0] = held[0];
    h.prev[1] = held[1];
    if (pressed >> BTN_UP & 1) *ud = -1;
    if (pressed >> BTN_DOWN & 1) *ud = 1;
    if (pressed >> BTN_LEFT & 1) *lr = -1;
    if (pressed >> BTN_RIGHT & 1) *lr = 1;
    // A or Start chooses, B goes back, as on the SNES; the fighting
    // buttons play no part, so remapping them never changes the menu
    if (pressed >> BTN_START & 1 || mb & MB_A) *ok = true;
    if (mb & MB_B) *back = true;
    int dir = now >> BTN_UP & 1 ? -1 : now >> BTN_DOWN & 1 ? 1 : 0;
    if (dir && dir == h.repeat_dir) {
        if (++h.repeat > 18 && h.repeat % 5 == 0) *ud = dir;
    } else h.repeat = 0;
    h.repeat_dir = dir;
}

static int page_items(void)
{
    return h.page == PAGE_MAIN ? MAIN_COUNT : h.page == PAGE_OPTIONS ? OPT_COUNT : h.page == PAGE_PAUSE ? PAUSE_COUNT : 1;
}

// Practice against the CPU: its level in stars, 10 to 50 of the game's 50
static int practice_stars(void) { return cfg.practice_level < 1 ? 1 : cfg.practice_level > 5 ? 5 : cfg.practice_level; }

static bool pause_shown(int item)
{
    if (item == PAUSE_DUMMY) return h.mode == MODE_PRACTICE;
    if (item == PAUSE_LEVEL) return h.mode == MODE_PRACTICE && cfg.dummy == DUMMY_CPU;
    return true;
}

static void menu_frame(const uint32_t held[2])
{
    int ud, lr;
    bool ok, back;
    uint32_t pr = (held[0] & ~h.prev[0]) | (held[1] & ~h.prev[1]);
    unsigned mb = menu_pressed();
    navigate(held, mb, &ud, &lr, &ok, &back);
    h.anim++;
    if (h.page == PAGE_ROSTER) { roster_frame(pr, mb, ud, lr); return; }
    if (h.page == PAGE_NEXT) {
        // NEXT FIGHT: Start or A, or a few seconds, and the match begins
        if (ok || h.anim > 240) start_mode(MODE_TOURNAMENT);
        else if (back) open_page(PAGE_ROSTER, 0);
        return;
    }
    int n = page_items();
    if (ud) h.sel = (h.sel + ud + n) % n;
    if (ud && h.page == PAGE_PAUSE) while (!pause_shown(h.sel)) h.sel = (h.sel + ud + n) % n;
    if (h.page == PAGE_MAIN) {
        if (ok) choose(h.sel);
    } else if (h.page == PAGE_OPTIONS) {
        if (h.sel >= OPT_WP && h.sel <= OPT_FK && (pr & 0x3fu)) {
            // the button pressed is the one this move now uses; the move
            // that had it takes this one's old button
            int slot = __builtin_ctz(pr & 0x3fu), move = h.sel - OPT_WP;
            char *pad = cfg.home_pad[h.col], want = pad_letters[slot];
            for (int a = 0; a < 6; a++) if (pad[a] == want) pad[a] = pad[move];
            pad[move] = want;
            frontend_settings_changed();
            return;
        }
        if (lr) change_option(lr);
        else if (ok) change_option(0);
        else if (back) { h.sel = OPT_EXIT; change_option(0); }
    } else if (h.page == PAGE_PAUSE) {
        if (h.sel == PAUSE_LEVEL && (lr || ok)) {
            int l = practice_stars() + (lr ? lr : 1);
            cfg.practice_level = l < 1 ? 5 : l > 5 ? 1 : l;
            frontend_settings_changed();
        } else if (h.sel == PAUSE_DUMMY && (lr || ok)) {
            int d = lr ? lr : 1;
            cfg.dummy = ((cfg.dummy + d) % DUMMY_COUNT + DUMMY_COUNT) % DUMMY_COUNT;
            frontend_settings_changed();
        } else if (back || (ok && h.sel == PAUSE_CONTINUE)) {
            h.page = PAGE_CLOSED;
            // to or from the CPU: that is another kind of game, picked again
            if ((h.pause_dummy == DUMMY_CPU) != (cfg.dummy == DUMMY_CPU) && h.mode == MODE_PRACTICE) {
                h.again = MODE_PRACTICE;
                h.restore = true;
            }
        } else if (ok && h.sel == PAUSE_MENU) {
            h.page = PAGE_CLOSED;
            h.restore = true;
        }
    }
}

// Back to the title screen and the menu: the state saved there, or a reset.
// In a tournament between matches, to the NEXT FIGHT screen instead.
static void return_to_menu(void)
{
    char path[4200];
    title_state_path(path, sizeof path);
    int was = h.mode;
    bool next = tn.next;
    tn.next = false;
    home_reset();
    fprintf(stderr, "home: back to the menu (%s)\n", h.title_saved ? "title state" : "reset");
    if (h.title_saved && state_load_file(path)) {
        memset(h.prev, 0xff, sizeof h.prev);        // the button that chose this is still down
        if (next) start_mode(MODE_TOURNAMENT);
        else if (h.again) { int m = h.again; h.again = 0; start_mode(m); }
        else open_page(PAGE_MAIN, was == MODE_VS ? MAIN_VS : was == MODE_PRACTICE ? MAIN_PRACTICE : was == MODE_TOURNAMENT ? MAIN_TOURNAMENT : MAIN_ARCADE);
        return;
    }
    h.title_saved = false;
    frontend_reset_game();
    h.seq_wait = -1;
}

// The DCS sound board takes a few seconds to start: a Start before that
// reaches a silent title screen. Measured: frame 100 silent, 300 fine.
#define SOUND_READY 360
#define IDLE_ATTRACT (20 * 60)  // frames alone on the main menu before the attract loop rolls

// The game's title screen goes back to the attract loop when its counter
// runs out; while the menu is open it is kept up, and its music with it.
static void keep_title(void)
{
    if (!game.title_timer || in_attract() || playing()) return;
    unsigned t = ram2[game.title_timer] | ram2[game.title_timer + 1] << 8;
    if (t < 300) { poke(game.title_timer, 500 & 255); poke(game.title_timer + 1, 500 >> 8); }
}

// Options > CONTROLS: which SNES button does each move. The letters are
// the SNES pad's places, which the cabinet's buttons sit in by default
// (L Y X punches, R B A kicks), so a remap is a shuffle of the six.
static int letter_slot(char c)
{
    const char *p = strchr(pad_letters, c);
    return p && c ? (int)(p - pad_letters) : -1;
}

static uint32_t remap_one(uint32_t in, int col)
{
    uint32_t out = in & ~0x3fu;
    for (int a = 0; a < 6; a++) {
        int slot = letter_slot(cfg.home_pad[col][a]);
        if (slot < 0) slot = a;
        if (in >> slot & 1) out |= 1u << a;
    }
    return out;
}

static void remap(uint32_t held[2])
{
    for (int p = 0; p < 2; p++) held[p] = remap_one(held[p], p);
}

void home_remap_local(uint32_t *held) { *held = remap_one(*held, 0); }

static void net_frame(uint32_t held[2]);

void home_inputs(uint32_t held[2])
{
    if (!home_enabled()) return;
    if (net) { net_frame(held); return; }
    if (h.restore || tn.next) { h.restore = false; return_to_menu(); held[0] = held[1] = 0; return; }
    training_timer_off = !cfg.home_timer && h.mode != MODE_NONE;

    // The menu is open: it takes every button, and the game stays on its
    // title screen behind it (pressed again if the title times out).
    if (h.page != PAGE_CLOSED && h.page != PAGE_PAUSE) {
        // Left alone on the main menu, the game rolls on into its attract
        // loop (fighter profiles, high scores, its music), as the SNES does;
        // Start there brings the menu back.
        if (h.page == PAGE_MAIN && (held[0] | held[1])) h.idle = 0;
        if (h.page == PAGE_MAIN && ++h.idle > IDLE_ATTRACT && !in_attract() && game.title_timer) {
            h.idle = 0;
            h.page = PAGE_CLOSED;
            h.menu_after_title = false;
            h.left_menu = true;
            poke(game.title_timer, 30);
            poke(game.title_timer + 1, 0);
            held[0] = held[1] = 0;
            return;
        }
        keep_title();
        if (h.cool > 0) h.cool--;
        if (in_attract() && h.press == 0 && h.cool == 0) { h.press = 4; h.press_player = 0; h.cool = 40; }
        menu_frame(held);
        held[0] = held[1] = 0;
        if (h.press > 0) { held[0] |= 1u << BTN_START; h.press--; }
        return;
    }
    if (h.page == PAGE_PAUSE) {
        menu_frame(held);
        held[0] = held[1] = 0;
        return;
    }

    if (h.mode != MODE_NONE) remap(held);
    uint32_t pressed[2] = { held[0] & ~h.prev[0], held[1] & ~h.prev[1] };
    const uint32_t held_raw[2] = { held[0], held[1] };
    h.prev[0] = held[0];
    h.prev[1] = held[1];

    // After a reset from Options: open the menu once the attract loop runs.
    if (h.seq_wait < 0) {
        if (in_attract() && frontend_frames() >= SOUND_READY) { h.seq_wait = 0; h.press = 4; h.press_player = 0; h.cool = 40; open_page(PAGE_MAIN, MAIN_OPTIONS); }
        held[0] = held[1] = 0;
        return;
    }

    // Nobody playing. Start in the attract loop goes on to the arcade's own
    // title screen, as on the cabinet (held back until the sound board has
    // started, or the title's music is lost); Start on the title opens the
    // console menu instead of starting a game.
    if (!playing() && !h.starting) {
        if (h.mode != MODE_NONE) home_reset();          // a mode ended in game over
        if (h.menu_after_title && !in_attract() && h.press == 0) {
            // back from the attract loop the menu was left for: straight to it
            h.menu_after_title = false;
            open_page(PAGE_MAIN, h.main_sel);
            held[0] = held[1] = 0;
            return;
        }
        if (h.title_wait) {
            held[0] = held[1] = 0;
            if (frontend_frames() < SOUND_READY) return;
            h.title_wait = false;
            h.press = 4; h.press_player = 0; h.cool = 40;
        } else if ((pressed[0] | pressed[1]) >> BTN_START & 1) {
            held[0] = held[1] = 0;
            if (in_attract()) {
                h.menu_after_title = h.left_menu;
                h.left_menu = false;
                if (frontend_frames() < SOUND_READY) { h.title_wait = true; return; }
                h.press = 4; h.press_player = 0; h.cool = 40;
            } else {
                open_page(PAGE_MAIN, h.main_sel);
                h.prev[0] = h.prev[1] = 0xffffffffu;    // ignore the Start still held
                return;
            }
        }
    }

    // Starting the chosen mode: Start for player 1 until the game is on,
    // then for player 2 in the two-player modes.
    if (h.starting) {
        held[0] &= ~(1u << BTN_START);
        held[1] &= ~(1u << BTN_START);
        if (h.press == 0 && --h.seq_wait <= 0) {
            if (!in_game(0)) { h.press = 4; h.press_player = 0; h.seq_wait = 20; }
            else if (h.want_players > 1 && !in_game(1)) { h.press = 4; h.press_player = 1; h.seq_wait = 20; }
            else h.starting = false;
        }
    }
    // Practice and VS: when a match ends, back to the character screen for
    // both players (the console's rematch), before the arcade's continue,
    // winner-stays-on and high-score screens. Only a match that ends in this
    // mode counts: the flag can still be set from an earlier game until the
    // next fight starts.
    if (h.mode == MODE_VS && match_ended(&h.vs_end) && !h.starting) {
        h.again = h.mode;
        h.restore = true;
    }
    if (h.mode == MODE_PRACTICE && game.match_over) {
        bool over = ram2[game.match_over] != 0;
        if (!over && fighting()) h.live = true;
        if (over && h.live && !h.starting) {
            h.live = false;
            h.again = h.mode;
            h.restore = true;
        }
    }

    if (h.press > 0) { held[h.press_player] |= 1u << BTN_START; h.press--; }

    if (h.mode == MODE_PRACTICE && h.cpu && !h.starting) cpu_pick(held, pressed);
    if (h.mode == MODE_PRACTICE && h.cpu && game.cpu_level) poke(game.cpu_level, (uint8_t)(practice_stars() * 10));

    // Practice: player 1 picks both fighters, then the dummy stands still.
    if (h.mode == MODE_PRACTICE && in_game(1)) {
        uint32_t moves = (1u << BTN_START) - 1;
        if (picked(0) && !picked(1)) {
            // wait for the button that picked player 1's fighter to come up
            if (!(held[0] & moves)) h.route_armed = true;
            if (h.route_armed) held[1] = (held[1] & ~moves) | (held[0] & moves);
            held[0] &= ~moves;
        } else h.route_armed = false;
    }

    if (h.mode != MODE_PRACTICE) random_frame(held, pressed);
    if (h.mode == MODE_VS || h.mode == MODE_PRACTICE || h.mode == MODE_TOURNAMENT) stage_frame(held);
    if (h.mode == MODE_VS || h.mode == MODE_TOURNAMENT) match_log();
    if (h.mode == MODE_TOURNAMENT) tn_frame();
    if (h.mode == MODE_VS || h.mode == MODE_PRACTICE || h.mode == MODE_TOURNAMENT) music_frame();
    if (h.mode != MODE_NONE) speed_frame(held);
    if (h.mode == MODE_ARCADE && cfg.home_fixed) hold_level();

    // In one of the menu's modes, Start pauses at any time, as on the
    // console, except where the game itself wants it: the continue
    // countdown after a lost match, a second player joining, and Up + Start
    // (random fighter) on the character screen.
    if (h.mode != MODE_NONE && !h.starting) {
        for (int p = 0; p < 2; p++) {
            if (!(pressed[p] >> BTN_START & 1) || !in_game(p)) continue;
            if (h.mode == MODE_PRACTICE && p == 1) continue;
            if (game.match_over && ram2[game.match_over] && picked(p)) continue;   // continue countdown
            if (!picked(p) && held_raw[p] >> BTN_UP & 1) continue;
            held[0] &= ~(1u << BTN_START);
            held[1] &= ~(1u << BTN_START);
            open_page(PAGE_PAUSE, PAUSE_CONTINUE);
            h.pause_dummy = cfg.dummy;
            break;
        }
    }
}

/* ---------------------------------------------------------------- */
/* Online: the 2 Player game on both sides                            */
/* ---------------------------------------------------------------- */

// Each side runs this from the same inputs, frame by frame, so it may only
// depend on them, on the machine and on its own state (saved with the
// machine's, home_state). The title screen to come back to after each
// match is a snapshot in memory: putting it back is an ordinary change to
// the machine, which rollback can cross (a state file would cut its history).
static struct {
    int phase;                  // 0 to the title screen, 1 playing
    int wait;                   // frames on the title screen
} nf;
static uint8_t *net_title;      // the snapshot (the same on both sides)

void home_net(bool on)
{
    net = on;
    home_reset();
    // both sides start from the same state, whatever their menus were doing
    memset(&h, 0, sizeof h);
    memset(&stg, 0, sizeof stg);
    stg.want = stg.first_code = -1;
    memset(&rnd, 0, sizeof rnd);
    mus.before = -1; mus.done = false;
    spd.speed = 1; spd.chosen = false; spd.since = 0;
    memset(&nf, 0, sizeof nf);
    if (!on) { free(net_title); net_title = NULL; }
}

bool home_net_active(void) { return net; }

size_t home_state(uint8_t *buf, bool load)
{
    // plain data only (no pointers): the same bytes on both sides
#define HS(x) if (buf && load) memcpy(&(x), buf + n, sizeof (x)); else if (buf) memcpy(buf + n, &(x), sizeof (x)); n += sizeof (x);
    size_t n = 0;
    HS(h) HS(stg) HS(rnd) HS(mus) HS(spd) HS(nf)
#undef HS
    return n;
}

static void net_frame(uint32_t held[2])
{
    if (h.restore) {                    // a match ended: both pick again
        h.restore = false;
        if (net_title) state_snapshot_put(net_title);
        fprintf(stderr, "home: online: frame %llu, match over: both pick again\n", (unsigned long long)machine_frame());
        home_reset();
        start_mode(MODE_VS);
        memset(h.prev, 0xff, sizeof h.prev);
        held[0] = held[1] = 0;
        return;
    }
    if (nf.phase == 0) {
        // from the boot to the title screen, then the snapshot, then the game
        held[0] = held[1] = 0;
        if (h.cool > 0) h.cool--;
        if (in_attract()) { nf.wait = 0; if (h.press == 0 && h.cool == 0) { h.press = 4; h.press_player = 0; h.cool = 40; } }
        else if (!playing() && h.press == 0 && ++nf.wait > 30) {
            if (!net_title) net_title = malloc(state_snapshot_size());
            if (net_title) state_snapshot_take(net_title);
            nf.phase = 1;
            fprintf(stderr, "home: online: frame %llu, title screen kept, 2 Player game\n", (unsigned long long)machine_frame());
            start_mode(MODE_VS);
        }
        if (h.press > 0) { held[h.press_player] |= 1u << BTN_START; h.press--; }
        return;
    }
    uint32_t pressed[2] = { held[0] & ~h.prev[0], held[1] & ~h.prev[1] };
    h.prev[0] = held[0];
    h.prev[1] = held[1];
    if (h.starting) {
        held[0] &= ~(1u << BTN_START);
        held[1] &= ~(1u << BTN_START);
        if (h.press == 0 && --h.seq_wait <= 0) {
            if (!in_game(0)) { h.press = 4; h.press_player = 0; h.seq_wait = 20; }
            else if (!in_game(1)) { h.press = 4; h.press_player = 1; h.seq_wait = 20; }
            else { h.starting = false; fprintf(stderr, "home: online: frame %llu, both players in\n", (unsigned long long)machine_frame()); }
        }
    }
    // the match is over (or the game is, somehow): both pick again
    if (!h.starting && (match_ended(&h.vs_end) || !playing())) h.restore = true;
    if (h.press > 0) { held[h.press_player] |= 1u << BTN_START; h.press--; }
    random_frame(held, pressed);
    stage_frame(held);
    match_log();
    music_frame();
}

// The menu shows a few frames after the title was asked for: save it then.
static void maybe_save_title(void)
{
    if (h.title_saved || (h.page != PAGE_MAIN && h.page != PAGE_ROSTER) || h.anim != 30 || in_attract() || playing()) return;
    char path[4200];
    title_state_path(path, sizeof path);
    h.title_saved = state_save_file(path);
    fprintf(stderr, "home: title state %s\n", h.title_saved ? "saved" : "not saved");
}

/* ---------------------------------------------------------------- */
/* Drawing                                                           */
/* ---------------------------------------------------------------- */

#define C_ITEM   0xf2f2f2u      // white, as the SNES menu
#define C_SEL    0xffd23cu      // its yellow for the chosen line
#define C_DIM    0x707884u
#define C_VALUE  0x9fe8ffu      // the cyan of the arcade's credits text
#define C_HELP   0x58e058u      // the SNES screens' green labels
#define C_GOLD   0xf0c040u

static int fade_in(void) { return h.anim < 12 ? (int)h.anim * 255 / 12 : 255; }

// On the arcade's own title screen, under its logo and above its
// FREE PLAY / INSERT COIN line, as the SNES menu sits on its title.
static void draw_main(uint32_t *ov)
{
    for (int i = 0; i < MAIN_COUNT; i++)
        text_c(ov, F_BODY, 140 + i * 13, main_items[i], i == h.sel ? C_SEL : C_ITEM, fade_in());
}

// Five stars for the arcade's five CPU levels, as on the SNES.
static void stars(uint32_t *ov, int x, int y, int level, int alpha)
{
    static const char *const star[7] = { "   #   ", "  ###  ", "#######", " ##### ", "  ###  ", " ## ## ", "##   ##" };
    for (int i = 0; i < 5; i++) {
        uint32_t c = i < level ? C_SEL : 0x606870u;
        for (int r = 0; r < 7; r++)
            for (int k = 0; k < 7; k++)
                if (star[r][k] == '#') {
                    blend(&ov[(y + r + 1) * 320 + x + i * 9 + k + 1], 0x000000, (unsigned)alpha);
                    blend(&ov[(y + r) * 320 + x + i * 9 + k], row_tint(c, r, 7), (unsigned)alpha);
                }
    }
}

// A dim, even veil, so the title shows through as the SNES screens' K does.
static void veil(uint32_t *ov, unsigned a)
{
    for (int i = 0; i < 320 * 240; i++) blend(&ov[i], 0x100818u, a);
}

static void draw_options(uint32_t *ov)
{
    veil(ov, 225);
    int a = fade_in();
    text_c(ov, F_BIG, 8, "OPTIONS", C_ITEM, a);
    int dl = adjust_difficulty();
    char line[48];
    for (int i = OPT_DIFFICULTY; i <= OPT_BREAKER; i++) {
        int y = 30 + i * 11;
        uint32_t c = i == h.sel ? C_SEL : C_ITEM;
        switch (i) {
        case OPT_DIFFICULTY:
            text(ov, F_BODY, 20, y, "DIFFICULTY: EASY", c, a);
            stars(ov, 186, y + 1, dl < 1 || dl > 5 ? 0 : dl, a);
            text(ov, F_BODY, 236, y, "HARD", c, a);
            continue;
        case OPT_FIXED: snprintf(line, sizeof line, "FIXED DIFFICULTY: %s", cfg.home_fixed ? "YES" : "NO"); break;
        case OPT_RANDOM: snprintf(line, sizeof line, "RANDOM SELECT: %s", cfg.home_random ? "ON" : "OFF"); break;
        case OPT_TIMER: snprintf(line, sizeof line, "TIMER: %s", cfg.home_timer ? "ON" : "OFF"); break;
        case OPT_BLOOD: snprintf(line, sizeof line, "BLOOD: %s", blood_names[cfg.blood & 3]); break;
        case OPT_FINISH: snprintf(line, sizeof line, "FINISHING MOVES: %s", cfg.finishing_moves ? "ON" : "OFF"); break;
        default: snprintf(line, sizeof line, "EASY COMBO BREAKER: %s", cfg.easy_breaker ? "ON" : "OFF"); break;
        }
        text(ov, F_BODY, 20, y, line, c, a);
    }
    // the controls table: the move, then each player's SNES button
    static const char *const moves[6] = { "WEAK PUNCH:", "MEDIUM PUNCH:", "FIERCE PUNCH:", "WEAK KICK:", "MEDIUM KICK:", "FIERCE KICK:" };
    text(ov, F_BODY, 20, 120, "CONTROLS:", C_ITEM, a);
    text(ov, F_BODY, 222, 120, "P1", C_ITEM, a);
    text(ov, F_BODY, 272, 120, "P2", C_ITEM, a);
    bool blink = (h.anim / 12) % 2 == 0;
    for (int m = 0; m < 6; m++) {
        int y = 132 + m * 11, row = OPT_WP + m;
        text(ov, F_BODY, 206 - text_width(F_BODY, moves[m]), y, moves[m], row == h.sel ? C_SEL : C_HELP, a);
        for (int p = 0; p < 2; p++) {
            char l[2] = { cfg.home_pad[p][m] ? cfg.home_pad[p][m] : pad_letters[m], 0 };
            bool on = row == h.sel && p == h.col;
            if (on && !blink) continue;
            text(ov, F_BODY, 227 + p * 50, y, l, on ? C_ITEM : C_SEL, a);
        }
    }
    for (int p = 0; p < 2; p++)
        text(ov, F_SMALL, 207 + p * 50, 201, "RESET", h.sel == OPT_RESET && p == h.col ? C_SEL : C_ITEM, a);
    text(ov, F_BODY, 20, 200, "EXIT", h.sel == OPT_EXIT ? C_SEL : C_ITEM, a);
    const char *help = h.sel >= OPT_WP && h.sel <= OPT_FK ? "PRESS A BUTTON   LEFT/RIGHT: P1/P2"
                     : h.switches_changed ? "RESTARTS ON EXIT TO APPLY" : "LEFT/RIGHT: CHANGE";
    text_c(ov, F_SMALL, 222, help, C_DIM, a);
}

static void draw_roster(uint32_t *ov)
{
    veil(ov, 225);
    int a = fade_in();
    text_c(ov, F_BIG, 8, "TOURNAMENT", C_ITEM, a);
    text(ov, F_SMALL, 222, 30, "W  /  L", C_ITEM, a);
    for (int i = 0; i < TN_MAX; i++) {
        int y = 42 + i * 14;
        uint32_t c = i == tn.row ? C_SEL : C_ITEM;
        char b[24];
        snprintf(b, sizeof b, "PLAYER %d:", i + 1);
        text(ov, F_BODY, 24, y, b, c, a);
        text(ov, F_BODY, 130, y, tn.name[i], c, a);
        if (i == tn.row && strlen(tn.name[i]) < 3 && (h.anim / 16) % 2 == 0) {
            char l[2] = { (char)('A' + tn.letter), 0 };
            text(ov, F_BODY, 130 + text_width(F_BODY, tn.name[i]), y, l, C_VALUE, a);
        }
        snprintf(b, sizeof b, "%02d %02d", tn.w[i] % 100, tn.l[i] % 100);
        text(ov, F_BODY, 222, y, b, c, a);
    }
    text_c(ov, F_SMALL, 168, "START: FIGHT    X: DELETE PLAYER", C_HELP, a);
    text_c(ov, F_SMALL, 182, "A,B: ENTER    Y: DELETE NAME", C_HELP, a);
    text_c(ov, F_SMALL, 196, "LEFT/RIGHT: LETTER   SELECT: EXIT", C_HELP, a);
}

static void draw_next(uint32_t *ov)
{
    veil(ov, 225);
    int a = fade_in();
    text_c(ov, F_BIG, 14, "NEXT FIGHT", C_GOLD, a);
    if (tn.qn < 2) return;
    for (int side = 0; side < 2; side++) {
        int who = tn.queue[side], cx = side ? 240 : 80;
        char b[16];
        const char *nm = tn.name[who];
        text(ov, F_BIG, cx - text_width(F_BIG, nm) / 2, 60, nm, C_ITEM, a);
        snprintf(b, sizeof b, "%02d/%02d", tn.w[who] % 100, tn.l[who] % 100);
        text(ov, F_BODY, cx - text_width(F_BODY, b) / 2, 86, b, C_GOLD, a);
        const char *pl = side ? "PLAYER 2" : "PLAYER 1";
        text(ov, F_SMALL, cx - text_width(F_SMALL, pl) / 2, 104, pl, C_DIM, a);
    }
    text_c(ov, F_BIG, 64, "VS", C_GOLD, a);
    if ((h.anim / 24) % 2 == 0) text_c(ov, F_BODY, 132, "PRESS START", C_ITEM, a);
    // the line of players waiting, in two columns as on the SNES
    for (int k = 2; k < tn.qn; k++) {
        int i = tn.queue[k], col = (k - 2) / 3, row = (k - 2) % 3;
        char b[40];
        snprintf(b, sizeof b, "%d: %-3s %02d/%02d", i + 1, tn.name[i], tn.w[i] % 100, tn.l[i] % 100);
        text(ov, F_SMALL, col ? 176 : 24, 170 + row * 14, b, C_ITEM, a);
    }
}

static void draw_pause(uint32_t *ov)
{
    veil(ov, 150);
    char dummy[32];
    snprintf(dummy, sizeof dummy, "DUMMY: %s", dummy_names[cfg.dummy % DUMMY_COUNT]);
    const char *const items[PAUSE_COUNT] = { "CONTINUE", dummy, "CPU LEVEL: ", "MAIN MENU" };
    text_c(ov, F_BIG, 90, "PAUSE", C_ITEM, 255);
    int y = 120, a = fade_in();
    for (int i = 0; i < PAUSE_COUNT; i++) {
        if (!pause_shown(i)) continue;
        uint32_t c = i == h.sel ? C_SEL : C_ITEM;
        if (i == PAUSE_LEVEL) {         // the words, then five stars
            int w = text_width(F_BODY, items[i]) + 45, x = (320 - w) / 2;
            text(ov, F_BODY, x, y, items[i], c, a);
            stars(ov, x + w - 45, y + 1, practice_stars(), a);
        } else text_c(ov, F_BODY, y, items[i], c, a);
        y += 16;
    }
    if (h.mode == MODE_PRACTICE && (h.sel == PAUSE_DUMMY || h.sel == PAUSE_LEVEL))
        text_c(ov, F_SMALL, 222, h.sel == PAUSE_DUMMY && (cfg.dummy == DUMMY_CPU) != (h.pause_dummy == DUMMY_CPU) ? "FIGHTERS ARE PICKED AGAIN" : "LEFT/RIGHT: CHANGE", C_DIM, 255);
}

// Practice as on the SNES: no health bars or clock, just its name. The
// game draws them into its own picture, so they are covered over.
static void draw_practice_hud(uint32_t *ov)
{
    for (int y = 0; y < 26; y++) {
        unsigned a = y < 20 ? 255 : 255 - (unsigned)(y - 19) * 36;
        for (int x = 0; x < 320; x++) blend(&ov[y * 320 + x], 0x000000, a);
    }
    text(ov, F_BODY, 8, 6, "PRACTICE", C_ITEM, 255);
    if ((h.anim++ / 40) % 2 == 0) text(ov, F_SMALL, 104, 8, "- PRESS START", C_ITEM, 255);
}

// Practice against the CPU: which of the two picks this is.
static void draw_cpu_pick(uint32_t *ov)
{
    for (int y = 0; y < 22; y++)
        for (int x = 0; x < 320; x++) blend(&ov[y * 320 + x], 0x000000, y < 16 ? 200 : 200 - (unsigned)(y - 15) * 30);
    text_c(ov, F_BODY, 5, h.cpu_step == 0 ? "PRACTICE: PICK YOUR FIGHTER" : "PRACTICE: PICK THE CPU FIGHTER", C_SEL, 255);
}

// Tournament: the SNES NEXT FIGHT screen, laid over the arcade's own
// character screen (its fighters, names and countdown in the middle): the
// two players' names and records on top, the line of players waiting below.
static void draw_tournament_select(uint32_t *ov)
{
    if (tn.qn < 2) return;
    for (int y = 0; y < 240; y++) {
        if (y >= 40 && y < 194) continue;
        unsigned a = y < 34 || y >= 200 ? 250 : y < 40 ? 250 - (unsigned)(y - 33) * 35 : 250 - (unsigned)(199 - y) * 35;
        for (int x = 0; x < 320; x++) blend(&ov[y * 320 + x], 0x000000, a);
    }
    text_c(ov, F_BIG, 2, "NEXT FIGHT", C_GOLD, 255);
    for (int side = 0; side < 2; side++) {
        int who = tn.queue[side];
        char b[24];
        snprintf(b, sizeof b, "%s %02d/%02d", tn.name[who], tn.w[who] % 100, tn.l[who] % 100);
        int x = side ? 312 - text_width(F_BODY, b) : 8;
        text(ov, F_BODY, x, 22, b, C_ITEM, 255);
    }
    for (int k = 2; k < tn.qn; k++) {
        int i = tn.queue[k], col = (k - 2) / 3, row = (k - 2) % 3;
        char b[40];
        snprintf(b, sizeof b, "%d: %-3s %02d/%02d", i + 1, tn.name[i], tn.w[i] % 100, tn.l[i] % 100);
        text(ov, F_SMALL, col ? 170 : 16, 202 + row * 12, b, C_ITEM, 255);
    }
}

void home_draw(uint32_t *ov)
{
    if (!home_enabled()) return;
    if (tn.next || h.restore) {             // the game's after-match screen, gone next frame
        for (int i = 0; i < 320 * 240; i++) ov[i] = 0xff000000u;
        return;
    }
    if (h.page == PAGE_CLOSED && h.mode == MODE_TOURNAMENT && in_game(0) && in_game(1) && !all_picked() && !h.starting) {
        draw_tournament_select(ov);
        return;
    }
    if (h.page == PAGE_CLOSED && h.mode == MODE_PRACTICE && fighting()) { draw_practice_hud(ov); return; }
    if (h.page == PAGE_CLOSED && h.mode == MODE_PRACTICE && h.cpu && h.cpu_step < 2 && in_game(0) && !picked(0) && !h.starting) {
        draw_cpu_pick(ov);
        return;
    }
    if (h.page == PAGE_CLOSED) return;
    maybe_save_title();
    switch (h.page) {
    case PAGE_MAIN: draw_main(ov); break;
    case PAGE_OPTIONS: draw_options(ov); break;
    case PAGE_ROSTER: draw_roster(ov); break;
    case PAGE_NEXT: draw_next(ov); break;
    default: draw_pause(ov); break;
    }
}
