// On-screen controls (see touch.h). The layout is the Super Nintendo pad's:
// a d-pad on the left, Y X A B in a diamond on the right with L and R above
// them, in the console's colours: L Y X punch, R B A kick, as in the
// gamepad's default mapping.
#include "touch.h"
#include "config.h"
#include "home.h"
#include <math.h>

enum { C_UP, C_DOWN, C_LEFT, C_RIGHT, C_L, C_Y, C_X, C_R, C_B, C_A, C_START, C_MENU, NCONTROLS };

typedef struct { float x, y, r; } circle_t;

// Where each control sits for a window of w x h pixels; the d-pad is one
// circle (its directions come from the angle).
static circle_t dpad;
static circle_t ctl[NCONTROLS];
static int lay_w, lay_h;

static void layout(int w, int h)
{
    if (w == lay_w && h == lay_h) return;
    lay_w = w; lay_h = h;
    float u = (w < h ? w : h) / 10.0f;          // one unit: a tenth of the short side
    float cy = h * 0.60f;
    dpad = (circle_t){ 2.3f * u, cy, 1.7f * u };
    float rx = w - 2.4f * u, s = 1.15f * u, br = 0.62f * u;
    ctl[C_Y] = (circle_t){ rx - s, cy, br };
    ctl[C_X] = (circle_t){ rx, cy - s, br };
    ctl[C_A] = (circle_t){ rx + s, cy, br };
    ctl[C_B] = (circle_t){ rx, cy + s, br };
    ctl[C_L] = (circle_t){ rx - 1.1f * u, cy - 2.55f * u, 0.55f * u };
    ctl[C_R] = (circle_t){ rx + 1.1f * u, cy - 2.55f * u, 0.55f * u };
    ctl[C_START] = (circle_t){ 2.3f * u, h - 0.75f * u, 0.5f * u };
    ctl[C_MENU] = (circle_t){ 0.75f * u, 0.75f * u, 0.45f * u };
}

// Fingers on the screen: which control each holds (-1 none, C_UP for the
// d-pad, whose direction is in dir).
#define MAX_FINGERS 10
static struct { SDL_FingerID id; int ctl; unsigned dir; bool used; } fingers[MAX_FINGERS];
static bool touched;                // a finger has touched the screen this run
static uint32_t prev_held;
static bool menu_tap;

static unsigned dpad_dir(float x, float y)
{
    float dx = x - dpad.x, dy = y - dpad.y;
    if (dx * dx + dy * dy < (dpad.r * 0.2f) * (dpad.r * 0.2f)) return 0;
    float a = atan2f(-dy, dx) * 180.0f / 3.14159265f;  // 0 = right, 90 = up
    if (a < 0) a += 360;
    unsigned d = 0;                                     // eight sectors of 45 degrees
    if (a < 67.5f || a >= 292.5f) d |= 1u << BTN_RIGHT;
    if (a >= 22.5f && a < 157.5f) d |= 1u << BTN_UP;
    if (a >= 112.5f && a < 247.5f) d |= 1u << BTN_LEFT;
    if (a >= 202.5f && a < 337.5f) d |= 1u << BTN_DOWN;
    return d;
}

static bool inside(const circle_t *c, float x, float y, float slack)
{
    float dx = x - c->x, dy = y - c->y, r = c->r * slack;
    return dx * dx + dy * dy <= r * r;
}

// The control under a point: the d-pad (C_UP), a button, or -1.
static int hit(float x, float y)
{
    if (inside(&dpad, x, y, 1.25f)) return C_UP;
    int best = -1;
    float bd = 1e30f;
    for (int i = C_L; i < NCONTROLS; i++) {
        float dx = x - ctl[i].x, dy = y - ctl[i].y, d = dx * dx + dy * dy;
        if (inside(&ctl[i], x, y, 1.35f) && d < bd) { bd = d; best = i; }
    }
    return best;
}

static int finger_slot(SDL_FingerID id, bool make)
{
    for (int i = 0; i < MAX_FINGERS; i++) if (fingers[i].used && fingers[i].id == id) return i;
    if (!make) return -1;
    for (int i = 0; i < MAX_FINGERS; i++)
        if (!fingers[i].used) { fingers[i].used = true; fingers[i].id = id; fingers[i].ctl = -1; fingers[i].dir = 0; return i; }
    return -1;
}

static void place(int s, float x, float y, bool down)
{
    int c = hit(x, y);
    // the d-pad keeps a finger that slides off it; buttons follow the finger
    if (fingers[s].ctl == C_UP && !down) c = C_UP;
    fingers[s].ctl = c;
    fingers[s].dir = c == C_UP ? dpad_dir(x, y) : 0;
    if (down && c == C_MENU) menu_tap = true;
}

bool touch_event(const SDL_Event *ev, int win_w, int win_h)
{
    if (ev->type != SDL_FINGERDOWN && ev->type != SDL_FINGERMOTION && ev->type != SDL_FINGERUP) return false;
    touched = true;
    layout(win_w, win_h);
    float x = ev->tfinger.x * win_w, y = ev->tfinger.y * win_h;
    int s = finger_slot(ev->tfinger.fingerId, ev->type == SDL_FINGERDOWN);
    if (s < 0) return true;
    if (ev->type == SDL_FINGERUP) fingers[s].used = false;
    else place(s, x, y, ev->type == SDL_FINGERDOWN);
    return true;
}

bool touch_shown(bool pad_connected)
{
    if (cfg.touch_controls == TOUCH_OFF) return false;
    if (cfg.touch_controls == TOUCH_ON) return true;
    // auto: while no pad is connected, or once the screen has been touched
    return SDL_GetNumTouchDevices() > 0 && (touched || !pad_connected);
}

static const int btn_of[NCONTROLS] = {
    [C_L] = BTN_HQ, [C_Y] = BTN_HM, [C_X] = BTN_HF, [C_R] = BTN_LQ, [C_B] = BTN_LM, [C_A] = BTN_LF,
    [C_START] = BTN_START, [C_UP] = -1, [C_DOWN] = -1, [C_LEFT] = -1, [C_RIGHT] = -1, [C_MENU] = -1,
};

uint32_t touch_held(void)
{
    uint32_t h = 0;
    for (int i = 0; i < MAX_FINGERS; i++) {
        if (!fingers[i].used || fingers[i].ctl < 0) continue;
        if (fingers[i].ctl == C_UP) h |= fingers[i].dir;
        else if (btn_of[fingers[i].ctl] >= 0) h |= 1u << btn_of[fingers[i].ctl];
    }
    return h;
}

unsigned touch_menu_buttons(void)
{
    unsigned m = 0;
    for (int i = 0; i < MAX_FINGERS; i++) {
        if (!fingers[i].used) continue;
        switch (fingers[i].ctl) {
        case C_A: m |= MB_A; break;
        case C_B: m |= MB_B; break;
        case C_X: m |= MB_X; break;
        case C_Y: m |= MB_Y; break;
        default: break;
        }
    }
    return m;
}

uint32_t touch_pressed(bool *menu_tapped)
{
    uint32_t h = touch_held(), p = h & ~prev_held;
    prev_held = h;
    if (menu_tapped) *menu_tapped = menu_tap;
    menu_tap = false;
    return p;
}

/* ---------------------------------------------------------------- */
/* Drawing                                                            */
/* ---------------------------------------------------------------- */

static void disc(SDL_Renderer *ren, float cx, float cy, float r, SDL_Color c)
{
    enum { N = 40 };
    SDL_Vertex v[N + 1];
    int idx[N * 3];
    v[0] = (SDL_Vertex){ { cx, cy }, c, { 0, 0 } };
    for (int i = 0; i < N; i++) {
        float a = i * 6.2831853f / N;
        v[i + 1] = (SDL_Vertex){ { cx + r * cosf(a), cy + r * sinf(a) }, c, { 0, 0 } };
        idx[i * 3] = 0; idx[i * 3 + 1] = i + 1; idx[i * 3 + 2] = (i + 1) % N + 1;
    }
    SDL_RenderGeometry(ren, NULL, v, N + 1, idx, N * 3);
}

// A 5x7 font for the button letters.
static const uint8_t glyphs[][7] = {
    ['A' - 'A'] = { 0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11 },
    ['B' - 'A'] = { 0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e },
    ['L' - 'A'] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f },
    ['R' - 'A'] = { 0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11 },
    ['S' - 'A'] = { 0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e },
    ['X' - 'A'] = { 0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11 },
    ['Y' - 'A'] = { 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04 },
};

static void letter(SDL_Renderer *ren, char ch, float cx, float cy, float size, SDL_Color c)
{
    const uint8_t *g = glyphs[ch - 'A'];
    float px = size / 7;
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y] >> (4 - x) & 1) {
                SDL_FRect r = { cx - 2.5f * px + x * px, cy - 3.5f * px + y * px, px + 0.5f, px + 0.5f };
                SDL_RenderFillRectF(ren, &r);
            }
}

static bool held_ctl(int c)
{
    for (int i = 0; i < MAX_FINGERS; i++) if (fingers[i].used && fingers[i].ctl == c) return true;
    return false;
}

void touch_draw(SDL_Renderer *ren, int w, int h)
{
    layout(w, h);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    uint32_t held = touch_held();
    // d-pad: a cross on a faint disc, the held directions lit
    disc(ren, dpad.x, dpad.y, dpad.r, (SDL_Color){ 255, 255, 255, 28 });
    float arm = dpad.r * 0.34f, len = dpad.r * 0.92f;
    const struct { unsigned bit; float dx, dy; } arms[] = {
        { 1u << BTN_UP, 0, -1 }, { 1u << BTN_DOWN, 0, 1 }, { 1u << BTN_LEFT, -1, 0 }, { 1u << BTN_RIGHT, 1, 0 },
    };
    for (int i = 0; i < 4; i++) {
        bool on = held & arms[i].bit;
        SDL_SetRenderDrawColor(ren, 210, 210, 210, on ? 170 : 80);
        SDL_FRect r = arms[i].dx ? (SDL_FRect){ dpad.x + (arms[i].dx < 0 ? -len : arm * 0.5f), dpad.y - arm * 0.5f, len - arm * 0.5f, arm }
                                 : (SDL_FRect){ dpad.x - arm * 0.5f, dpad.y + (arms[i].dy < 0 ? -len : arm * 0.5f), arm, len - arm * 0.5f };
        SDL_RenderFillRectF(ren, &r);
    }
    SDL_SetRenderDrawColor(ren, 210, 210, 210, 80);
    SDL_FRect mid = { dpad.x - arm * 0.5f, dpad.y - arm * 0.5f, arm, arm };
    SDL_RenderFillRectF(ren, &mid);

    // the buttons in the Super Nintendo's colours
    static const struct { int c; char ch; SDL_Color col; } btns[] = {
        { C_X, 'X', { 60, 90, 220, 0 } }, { C_A, 'A', { 220, 50, 50, 0 } },
        { C_B, 'B', { 230, 190, 40, 0 } }, { C_Y, 'Y', { 40, 165, 80, 0 } },
        { C_L, 'L', { 150, 150, 160, 0 } }, { C_R, 'R', { 150, 150, 160, 0 } },
        { C_START, 'S', { 120, 120, 130, 0 } },
    };
    for (size_t i = 0; i < sizeof btns / sizeof btns[0]; i++) {
        const circle_t *c = &ctl[btns[i].c];
        SDL_Color col = btns[i].col;
        col.a = held_ctl(btns[i].c) ? 200 : 90;
        disc(ren, c->x, c->y, c->r, col);
        letter(ren, btns[i].ch, c->x, c->y, c->r * 0.8f, (SDL_Color){ 255, 255, 255, 190 });
    }
    // the menu button: three bars
    const circle_t *m = &ctl[C_MENU];
    disc(ren, m->x, m->y, m->r, (SDL_Color){ 120, 120, 130, held_ctl(C_MENU) ? 200 : 90 });
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 190);
    for (int i = -1; i <= 1; i++) {
        SDL_FRect r = { m->x - m->r * 0.5f, m->y + i * m->r * 0.35f - m->r * 0.08f, m->r, m->r * 0.16f };
        SDL_RenderFillRectF(ren, &r);
    }
}
