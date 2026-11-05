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
#include <string.h>

/* ---------------------------------------------------------------- */
/* Lettering: anti-aliased Goudy, the arcade's own typeface family   */
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

static void text_layer(uint32_t *ov, int f, int x, int y, const char *s, uint32_t rgb, int alpha)
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
                blend(&ov[yy * 320 + xx], rgb, a);
            }
        }
        pen4 += g->adv4;
    }
}

// Text with a soft dark shadow under it, as the arcade sets its lettering
// against busy backgrounds.
static void text(uint32_t *ov, int f, int x, int y, const char *s, uint32_t rgb, int alpha)
{
    text_layer(ov, f, x + 1, y + 1, s, 0x000000, alpha * 3 / 4);
    text_layer(ov, f, x, y, s, rgb, alpha);
}

static void text_c(uint32_t *ov, int f, int y, const char *s, uint32_t rgb, int alpha)
{
    text(ov, f, (320 - text_width(f, s)) / 2, y, s, rgb, alpha);
}

// Darken a band of the screen, fading in at its top edge.
static void shade(uint32_t *ov, int y0, int y1, int fade, unsigned a)
{
    for (int y = y0 < 0 ? 0 : y0; y < y1 && y < 240; y++) {
        unsigned k = y - y0 < fade ? a * (unsigned)(y - y0) / (unsigned)fade : a;
        for (int x = 0; x < 320; x++) blend(&ov[y * 320 + x], 0x000000, k);
    }
}

/* ---------------------------------------------------------------- */
/* Game state, from the profile's RAM flags                          */
/* ---------------------------------------------------------------- */

static int flag(uint32_t off) { return off ? ram2[off] : -1; }
static bool in_attract(void) { return flag(game.attract) == 1; }
static bool in_game(int p) { return flag(game.in_game + (uint32_t)p * game.player_step) == 1; }
static bool picked(int p) { return flag(game.picked + (uint32_t)p * game.player_step) == 1; }
static bool playing(void) { return in_game(0) || in_game(1); }

bool home_enabled(void)
{
    return cfg.home_menu && game.attract && game.in_game && game.picked;
}

/* ---------------------------------------------------------------- */
/* Stage select: the home version's codes at the character screen    */
/* ---------------------------------------------------------------- */

// Hold a direction and pick a fighter with one of the buttons. The SNES pad
// maps to the cabinet as Y X L = quick, medium, fierce punch and B A R =
// quick, medium, fierce kick. The first player to pick chooses.
static const struct { int dir, button, stage; const char *name; } stage_codes[] = {
    { BTN_DOWN, BTN_HQ, 12, "BLOODY ARENA" },
    { BTN_DOWN, BTN_HF, 5,  "CANYON BRIDGE" },
    { BTN_UP,   BTN_LF, 6,  "CASTLE ROOF" },
    { BTN_UP,   BTN_LQ, 7,  "CITY ROOF" },
    { BTN_DOWN, BTN_LF, 2,  "CITY STREET" },
    { BTN_UP,   BTN_LM, 8,  "DESERT ROOF" },
    { BTN_DOWN, BTN_LM, 15, "FACTORY" },
    { BTN_DOWN, BTN_LQ, 1,  "FIREPLACE" },
    { BTN_UP,   BTN_HM, 11, "ICE SCULPTURE" },
    { BTN_UP,   BTN_HF, 3,  "ICE TEMPLE" },
    { BTN_DOWN, BTN_HM, 20, "LAVA PIT" },
    { BTN_UP,   BTN_HQ, 13, "SKULL ROOM" },
};
#define SKY_ROOFTOP 9           // Down + B on both players' picks

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
            bool sky = recent[p] >> BTN_DOWN & 1 && recent[p] >> BTN_LQ & 1;
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
enum { PAGE_CLOSED, PAGE_MAIN, PAGE_OPTIONS, PAGE_PAUSE };
enum { MAIN_ARCADE, MAIN_VS, MAIN_TOURNAMENT, MAIN_PRACTICE, MAIN_OPTIONS, MAIN_COUNT };
enum { OPT_DIFFICULTY, OPT_BLOOD, OPT_BLOODCOL, OPT_FINISH, OPT_CONTROLS, OPT_EXIT, OPT_COUNT };
enum { PAUSE_CONTINUE, PAUSE_MENU, PAUSE_COUNT };

static const char *const main_items[MAIN_COUNT] = { "ARCADE", "VS", "TOURNAMENT", "PRACTICE", "OPTIONS" };
static const char *const diff_names[] = { "?", "EXTRA EASY", "EASY", "MEDIUM", "HARD", "EXTRA HARD" };
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
    int repeat, repeat_dir;     // held up/down auto-repeat
    int cool;                   // frames before Start may be pressed again
    unsigned anim;              // frames the menu has shown, for its fade-in
} h;

static bool supported(int item)
{
    return item != MAIN_TOURNAMENT;     // the tournament ladder is not built yet
}

static void title_state_path(char *buf, size_t n)
{
    snprintf(buf, n, "%scache/home-%s.state", data_dir(), game.id);
}

static void open_page(int page, int sel)
{
    h.page = page;
    h.sel = sel;
    h.anim = 0;
}

void home_reset(void)
{
    stage_clear();
    training_forced = false;
    h.mode = MODE_NONE;
    h.page = PAGE_CLOSED;
    h.starting = false;
    h.press = 0;
}

bool home_paused(void) { return h.page == PAGE_PAUSE; }

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
    case OPT_BLOOD: cfg.blood = ((cfg.blood + d) % 4 + 4) % 4; h.switches_changed = true; break;
    case OPT_BLOODCOL: cfg.blood_colour = !cfg.blood_colour; h.switches_changed = true; break;
    case OPT_FINISH: cfg.finishing_moves = !cfg.finishing_moves; h.switches_changed = true; break;
    case OPT_CONTROLS: if (!dir) frontend_open_controls(); return;
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

static void choose(int item)
{
    if (!supported(item)) { osd_message("Tournament: coming soon"); return; }
    if (item == MAIN_OPTIONS) { open_page(PAGE_OPTIONS, 0); return; }
    h.main_sel = item;
    h.mode = item == MAIN_ARCADE ? MODE_ARCADE : item == MAIN_VS ? MODE_VS : MODE_PRACTICE;
    h.want_players = h.mode == MODE_ARCADE ? 1 : 2;
    training_forced = h.mode == MODE_PRACTICE;
    h.page = PAGE_CLOSED;
    h.starting = true;
    h.seq_wait = 0;
}

// Presses from either player, with auto-repeat on a held up/down.
static void navigate(const uint32_t held[2], int *ud, int *lr, bool *ok, bool *back)
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
    if (pressed & (1u << BTN_START | 1u << BTN_HQ | 1u << BTN_LQ)) *ok = true;
    if (pressed & (1u << BTN_HM | 1u << BTN_LM)) *back = true;
    int dir = now >> BTN_UP & 1 ? -1 : now >> BTN_DOWN & 1 ? 1 : 0;
    if (dir && dir == h.repeat_dir) {
        if (++h.repeat > 18 && h.repeat % 5 == 0) *ud = dir;
    } else h.repeat = 0;
    h.repeat_dir = dir;
}

static int page_items(void)
{
    return h.page == PAGE_MAIN ? MAIN_COUNT : h.page == PAGE_OPTIONS ? OPT_COUNT : PAUSE_COUNT;
}

static void menu_frame(const uint32_t held[2])
{
    int ud, lr;
    bool ok, back;
    navigate(held, &ud, &lr, &ok, &back);
    h.anim++;
    int n = page_items();
    if (ud) h.sel = (h.sel + ud + n) % n;
    if (h.page == PAGE_MAIN) {
        if (ok) choose(h.sel);
    } else if (h.page == PAGE_OPTIONS) {
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
static void return_to_menu(void)
{
    char path[4200];
    title_state_path(path, sizeof path);
    int was = h.mode;
    home_reset();
    fprintf(stderr, "home: back to the menu (%s)\n", h.title_saved ? "title state" : "reset");
    if (h.title_saved && state_load_file(path)) {
        memset(h.prev, 0xff, sizeof h.prev);        // the button that chose this is still down
        open_page(PAGE_MAIN, was == MODE_VS ? MAIN_VS : was == MODE_PRACTICE ? MAIN_PRACTICE : MAIN_ARCADE);
        return;
    }
    h.title_saved = false;
    frontend_reset_game();
    h.seq_wait = -1;
}

void home_inputs(uint32_t held[2])
{
    if (!home_enabled()) return;
    if (h.restore) { h.restore = false; return_to_menu(); held[0] = held[1] = 0; return; }

    // The menu is open: it takes every button, and the game stays on its
    // title screen behind it (pressed again if the title times out).
    if (h.page != PAGE_CLOSED && h.page != PAGE_PAUSE) {
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

    uint32_t pressed[2] = { held[0] & ~h.prev[0], held[1] & ~h.prev[1] };
    h.prev[0] = held[0];
    h.prev[1] = held[1];

    // After a reset from Options: open the menu once the attract loop runs.
    if (h.seq_wait < 0) {
        if (in_attract()) { h.seq_wait = 0; h.press = 4; h.press_player = 0; h.cool = 40; open_page(PAGE_MAIN, MAIN_OPTIONS); }
        held[0] = held[1] = 0;
        return;
    }

    // Nobody playing: a Start press opens the menu instead of starting.
    if (!playing() && !h.starting) {
        if (h.mode != MODE_NONE) home_reset();          // a mode ended in game over
        if ((pressed[0] | pressed[1]) >> BTN_START & 1) {
            held[0] = held[1] = 0;
            if (in_attract()) { h.press = 4; h.press_player = 0; h.cool = 40; }   // on to the title screen
            open_page(PAGE_MAIN, h.main_sel);
            h.prev[0] = h.prev[1] = 0xffffffffu;        // ignore the Start still held
            return;
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

    if (h.mode == MODE_VS || h.mode == MODE_PRACTICE) stage_frame(held);

    // In a fight of one of the menu's modes, Start pauses.
    if (h.mode != MODE_NONE && !h.starting && pressed[0] >> BTN_START & 1 && game_health(0) >= 0) {
        held[0] &= ~(1u << BTN_START);
        open_page(PAGE_PAUSE, PAUSE_CONTINUE);
    }
}

// The menu shows a few frames after the title was asked for: save it then.
static void maybe_save_title(void)
{
    if (h.title_saved || h.page != PAGE_MAIN || h.anim != 30 || in_attract() || playing()) return;
    char path[4200];
    title_state_path(path, sizeof path);
    h.title_saved = state_save_file(path);
    fprintf(stderr, "home: title state %s\n", h.title_saved ? "saved" : "not saved");
}

/* ---------------------------------------------------------------- */
/* Drawing                                                           */
/* ---------------------------------------------------------------- */

#define C_ITEM   0xd8e4f0u      // pale steel, like the arcade's captions
#define C_SEL    0xffd23cu      // gold
#define C_DIM    0x707884u
#define C_VALUE  0x9fe8ffu      // the cyan of the arcade's credits text

static void draw_list(uint32_t *ov, const char *const *items, const char *const *values, int n, int sel, int y, int step)
{
    int fade = h.anim < 12 ? (int)h.anim * 255 / 12 : 255;
    for (int i = 0; i < n; i++) {
        bool on = i == sel;
        uint32_t c = on ? C_SEL : C_ITEM;
        if (h.page == PAGE_MAIN && !supported(i)) c = on ? 0xa08a40u : C_DIM;
        if (!values || !values[i]) {
            text_c(ov, F_BODY, y + i * step, items[i], c, fade);
            if (on) {
                int w = text_width(F_BODY, items[i]);
                text(ov, F_BODY, (320 - w) / 2 - 16, y + i * step, "-", c, fade);
                text(ov, F_BODY, (320 + w) / 2 + 8, y + i * step, "-", c, fade);
            }
        } else {
            text(ov, F_BODY, 52, y + i * step, items[i], c, fade);
            char v[48];
            snprintf(v, sizeof v, on ? "< %s >" : "%s", values[i]);
            int w = text_width(F_BODY, v);
            text(ov, F_BODY, 268 - w, y + i * step, v, on ? C_SEL : C_VALUE, fade);
        }
    }
}

void home_draw(uint32_t *ov)
{
    if (!home_enabled() || h.page == PAGE_CLOSED) return;
    maybe_save_title();
    if (h.page == PAGE_MAIN) {
        shade(ov, 112, 240, 46, 252);         // covers the title's own "FREE PLAY"
        draw_list(ov, main_items, NULL, MAIN_COUNT, h.sel, 138, 17);
    } else if (h.page == PAGE_OPTIONS) {
        shade(ov, 0, 240, 1, 225);
        text_c(ov, F_BIG, 26, "OPTIONS", C_ITEM, 255);
        static const char *const items[OPT_COUNT] = { "DIFFICULTY", "BLOOD", "BLOOD COLOUR", "FINISHING MOVES", "CONTROLS", "EXIT" };
        const char *values[OPT_COUNT] = { 0 };
        int dl = adjust_difficulty();
        values[OPT_DIFFICULTY] = dl < 1 || dl > 5 ? "?" : diff_names[dl];
        values[OPT_BLOOD] = blood_names[cfg.blood & 3];
        values[OPT_BLOODCOL] = cfg.blood_colour ? "WHITE" : "RED";
        values[OPT_FINISH] = cfg.finishing_moves ? "ON" : "OFF";
        int n = OPT_COUNT;
        const char *it[OPT_COUNT];
        const char *va[OPT_COUNT];
        for (int i = 0; i < n; i++) { it[i] = items[i]; va[i] = values[i]; }
        if (!game.has_blood_colour) { it[OPT_BLOODCOL] = "BLOOD COLOUR"; va[OPT_BLOODCOL] = "-"; }
        draw_list(ov, it, va, n, h.sel, 70, 22);
        if (h.switches_changed)
            text_c(ov, F_SMALL, 212, "The game restarts on Exit to apply these", C_DIM, 255);
    } else {
        shade(ov, 80, 160, 1, 170);
        static const char *const items[PAUSE_COUNT] = { "CONTINUE", "MAIN MENU" };
        text_c(ov, F_BIG, 86, "PAUSE", C_ITEM, 255);
        draw_list(ov, items, NULL, PAUSE_COUNT, h.sel, 116, 18);
    }
}
