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

bool home_enabled(void)
{
    // the RAM locations were found in the default version (L1.5D for
    // UltraCombo); a ROM given by path is taken to be that one
    return cfg.home_menu && game.attract && game.in_game && game.picked && machine_version() <= 0;
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
    static uint32_t hist[2][8];
    static unsigned at;
    uint32_t recent[2] = { 0, 0 };
    at = (at + 1) % 8;
    for (int p = 0; p < 2; p++) {
        hist[p][at] = held[p];
        for (int i = 0; i < 8; i++) recent[p] |= hist[p][i];
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
enum { OPT_DIFFICULTY, OPT_RANDOM, OPT_TIMER, OPT_BLOOD, OPT_FINISH,
       OPT_WP, OPT_MP, OPT_FP, OPT_WK, OPT_MK, OPT_FK, OPT_RESET, OPT_EXIT, OPT_COUNT };
enum { PAUSE_CONTINUE, PAUSE_MENU, PAUSE_COUNT };

// as on the SNES title screen
static const char *const main_items[MAIN_COUNT] = { "1 PLAYER GAME", "2 PLAYER GAME", "TOURNAMENT GAME", "PRACTICE GAME", "OPTIONS" };
static const char *const blood_names[] = { "OFF", "LOW", "MEDIUM", "HIGH" };

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
    uint32_t prev[2];           // last frame's buttons, for presses
    unsigned mprev;             // last frame's menu buttons (frontend_menu_buttons)
    int repeat, repeat_dir;     // held up/down auto-repeat
    int cool;                   // frames before Start may be pressed again
    int rejoin_t, rejoin_next;  // VS: Start presses that bring both players back
    bool practice_again;        // Practice ended in a KO: back to its character screen
    bool practice_live;         // a practice fight has started (the KO check is armed)
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
static uint32_t rnd_ticks(void) { static uint32_t n; return ++n * 2654435761u ^ (uint32_t)clock(); }

static void random_frame(uint32_t held[2], const uint32_t pressed[2])
{
    rnd.seed = rnd.seed * 1103515245u + 12345u + rnd_ticks();
    for (int p = 0; p < 2; p++) {
        if (!in_game(p) || picked(p)) { rnd.steps[p] = rnd.t[p] = rnd.wait[p] = 0; continue; }
        // Options > RANDOM SELECT: every human player's fighter is drawn
        bool always = cfg.home_random && ++rnd.wait[p] == 45;
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
    bool armed;                 // win counters noted for the match being fought
    int wins_before[2];
    int over_t;                 // frames since the match ended
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

static int wins_of(int p) { return ram2[game.wins + (uint32_t)p * game.player_step]; }

// Called each frame of a tournament match.
static void tn_frame(void)
{
    if (!game.wins || !game.match_over) return;
    if (all_picked() && !ram2[game.match_over] && !tn.armed) {
        tn.wins_before[0] = wins_of(0);
        tn.wins_before[1] = wins_of(1);
        tn.armed = true;
        tn.over_t = 0;
    }
    if (!tn.armed || !ram2[game.match_over]) return;
    int won = wins_of(0) != tn.wins_before[0] ? 0 : wins_of(1) != tn.wins_before[1] ? 1 : -1;
    if (won < 0 && ++tn.over_t < 900) return;           // the counters change a little after
    tn.armed = false;
    if (won >= 0 && tn.qn >= 2) {
        int winner = tn.queue[won], loser = tn.queue[won ^ 1];
        tn.w[winner]++;
        tn.l[loser]++;
        fprintf(stderr, "home: tournament: %s beat %s\n", tn.name[winner], tn.name[loser]);
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
    h.practice_live = false;
    stage_clear();
    tn.armed = false;
    training_timer_off = false;
    spd.speed = 1;
    spd.chosen = false;
    training_forced = false;
    h.mode = MODE_NONE;
    h.page = PAGE_CLOSED;
    h.starting = false;
    h.press = 0;
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
    h.want_players = mode == MODE_ARCADE ? 1 : 2;
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
        if (back || (ok && h.sel == PAUSE_CONTINUE)) h.page = PAGE_CLOSED;
        else if (ok && h.sel == PAUSE_MENU) {
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
        else if (h.practice_again) { h.practice_again = false; start_mode(MODE_PRACTICE); }
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

static void remap(uint32_t held[2])
{
    for (int p = 0; p < 2; p++) {
        uint32_t in = held[p], out = in & ~0x3fu;
        for (int a = 0; a < 6; a++) {
            int slot = letter_slot(cfg.home_pad[p][a]);
            if (slot < 0) slot = a;
            if (in >> slot & 1) out |= 1u << a;
        }
        held[p] = out;
    }
}

void home_inputs(uint32_t held[2])
{
    if (!home_enabled()) return;
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
    // Practice: if a match ends after all, start the practice over rather
    // than let the arcade go on with the CPU.
    // Only a match that ends during this practice counts: the flag can
    // still be set from an earlier game until the next fight starts.
    if (h.mode == MODE_PRACTICE && game.match_over) {
        bool over = ram2[game.match_over] != 0;
        if (!over && fighting()) h.practice_live = true;
        if (over && h.practice_live && !h.starting) {
            h.practice_live = false;
            h.practice_again = true;
            h.restore = true;
        }
    }

    // VS: when a match is over both players go back to the character
    // screen, as on the console, instead of the arcade's winner-stays-on.
    // Start is pressed for each in turn: the loser continues, and a player
    // the game has handed to the CPU joins again.
    if (h.mode == MODE_VS && !h.starting && h.press == 0 && ++h.rejoin_t >= 30) {
        h.rejoin_t = 0;
        bool over = game.match_over && ram2[game.match_over];
        for (int k = 0; k < 2; k++) {
            int p = (h.rejoin_next + k) & 1;
            if ((over && in_game(p) && picked(p)) || (!in_game(p) && in_game(p ^ 1))) {
                h.press = 4;
                h.press_player = p;
                h.rejoin_next = p ^ 1;
                break;
            }
        }
    }
    if (h.press > 0) { held[h.press_player] |= 1u << BTN_START; h.press--; }

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
    if (h.mode == MODE_TOURNAMENT) tn_frame();
    if (h.mode == MODE_VS || h.mode == MODE_PRACTICE || h.mode == MODE_TOURNAMENT) music_frame();
    if (h.mode != MODE_NONE) speed_frame(held);

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
            break;
        }
    }
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
    for (int i = OPT_DIFFICULTY; i <= OPT_FINISH; i++) {
        int y = 34 + i * 14;
        uint32_t c = i == h.sel ? C_SEL : C_ITEM;
        switch (i) {
        case OPT_DIFFICULTY:
            text(ov, F_BODY, 20, y, "DIFFICULTY: EASY", c, a);
            stars(ov, 186, y + 1, dl < 1 || dl > 5 ? 0 : dl, a);
            text(ov, F_BODY, 236, y, "HARD", c, a);
            continue;
        case OPT_RANDOM: snprintf(line, sizeof line, "RANDOM SELECT: %s", cfg.home_random ? "ON" : "OFF"); break;
        case OPT_TIMER: snprintf(line, sizeof line, "TIMER: %s", cfg.home_timer ? "ON" : "OFF"); break;
        case OPT_BLOOD: snprintf(line, sizeof line, "BLOOD: %s", blood_names[cfg.blood & 3]); break;
        default: snprintf(line, sizeof line, "FINISHING MOVES: %s", cfg.finishing_moves ? "ON" : "OFF"); break;
        }
        text(ov, F_BODY, 20, y, line, c, a);
    }
    // the controls table: the move, then each player's SNES button
    static const char *const moves[6] = { "WEAK PUNCH:", "MEDIUM PUNCH:", "FIERCE PUNCH:", "WEAK KICK:", "MEDIUM KICK:", "FIERCE KICK:" };
    text(ov, F_BODY, 20, 108, "CONTROLS:", C_ITEM, a);
    text(ov, F_BODY, 222, 108, "P1", C_ITEM, a);
    text(ov, F_BODY, 272, 108, "P2", C_ITEM, a);
    bool blink = (h.anim / 12) % 2 == 0;
    for (int m = 0; m < 6; m++) {
        int y = 122 + m * 12, row = OPT_WP + m;
        text(ov, F_BODY, 206 - text_width(F_BODY, moves[m]), y, moves[m], row == h.sel ? C_SEL : C_HELP, a);
        for (int p = 0; p < 2; p++) {
            char l[2] = { cfg.home_pad[p][m] ? cfg.home_pad[p][m] : pad_letters[m], 0 };
            bool on = row == h.sel && p == h.col;
            if (on && !blink) continue;
            text(ov, F_BODY, 227 + p * 50, y, l, on ? C_ITEM : C_SEL, a);
        }
    }
    for (int p = 0; p < 2; p++)
        text(ov, F_SMALL, 207 + p * 50, 196, "RESET", h.sel == OPT_RESET && p == h.col ? C_SEL : C_ITEM, a);
    text(ov, F_BODY, 20, 196, "EXIT", h.sel == OPT_EXIT ? C_SEL : C_ITEM, a);
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
    static const char *const items[PAUSE_COUNT] = { "CONTINUE", "MAIN MENU" };
    text_c(ov, F_BIG, 90, "PAUSE", C_ITEM, 255);
    for (int i = 0; i < PAUSE_COUNT; i++)
        text_c(ov, F_BODY, 120 + i * 16, items[i], i == h.sel ? C_SEL : C_ITEM, fade_in());
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
    if (h.page == PAGE_CLOSED && h.mode == MODE_TOURNAMENT && in_game(0) && in_game(1) && !all_picked() && !h.starting) {
        draw_tournament_select(ov);
        return;
    }
    if (h.page == PAGE_CLOSED && h.mode == MODE_PRACTICE && fighting()) { draw_practice_hud(ov); return; }
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
