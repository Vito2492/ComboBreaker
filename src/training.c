// Training mode (see training.h). RAM locations come from the game profile
// (game.h); each value is sanity-checked before use, so other code versions
// are left alone, and a game without known locations gets no training mode.
#include <stdlib.h>
#include "training.h"
#include "config.h"
#include "mem.h"
#include "osd.h"
#include "game.h"
#include <string.h>

#define HEALTH_P1   game.health_p1
#define HEALTH_STEP game.health_step
#define HEALTH_MAX  game.health_max
#define TIMER_TICK  game.timer_tick     // frames until the clock ticks
#define TIMER_ONES  game.timer_ones
#define TIMER_TENS  game.timer_tens
#define REFILL_DELAY 45         // frames without damage before health refills

bool training_forced;
bool training_timer_off;

static void poke8(uint32_t off, uint8_t v)
{
    if (ram2[off] == v) return;
    ram2[off] = v;
    ram_touch(&ram2[off]);      // keep save states and rollback exact
}

static struct { uint8_t last; int still; } hp[2];

void training_apply(bool real)
{
    bool train = cfg.training || training_forced;
    if (!(train || training_timer_off) || !game.health_p1) return;
    // the clock: only touch it while it shows plausible digits
    if (ram2[TIMER_ONES] <= 9 && ram2[TIMER_TENS] <= 9 && ram2[TIMER_TICK] < 0x50) poke8(TIMER_TICK, 0x40);
    if (!train) return;
    for (int p = 0; p < 2; p++) {
        uint32_t off = HEALTH_P1 + (uint32_t)p * HEALTH_STEP;
        uint8_t h = ram2[off];
        if (h > HEALTH_MAX) continue;
        if (real) {
            if (h == hp[p].last) hp[p].still++;
            else { hp[p].last = h; hp[p].still = 0; }
        }
        // refill once a combo is over; a KO (0) is left to the game. In the
        // home menu's Practice the wait is shorter, so a bar is hardly ever
        // emptied, but never while a hit is landing: the game is then still
        // working on the health it took. The whole 16-bit health is written,
        // and the bar it shows with it, so there is nothing left to animate.
        int wait = training_forced ? 8 : REFILL_DELAY;
        if (h && h < HEALTH_MAX && hp[p].still >= wait) {
            poke8(off, HEALTH_MAX);
            if (game.health_frac) {
                uint32_t frac = game.health_frac + (uint32_t)p * HEALTH_STEP;
                poke8(frac, 0);
                if (game.health_shown) {
                    uint32_t shown = game.health_shown + (uint32_t)p * HEALTH_STEP;
                    poke8(shown, 0);
                    poke8(shown + 1, HEALTH_MAX);
                }
            }
            if (real) hp[p].last = HEALTH_MAX;
        }
    }
}

#define REC_MAX (60 * 10)
static uint32_t rec[REC_MAX];
static int rec_len, rec_pos;
static enum { DUMMY_OFF, DUMMY_RECORD, DUMMY_PLAY } mode;

void training_record_toggle(void)
{
    if (mode == DUMMY_RECORD) {
        mode = DUMMY_OFF;
        osd_message("Dummy recorded: %.1f s (F9 plays it)", rec_len / 60.0);
        return;
    }
    mode = DUMMY_RECORD;
    rec_len = 0;
    osd_message("Recording: you control player 2 (F8 stops)");
}

void training_play_toggle(void)
{
    if (mode == DUMMY_PLAY) { mode = DUMMY_OFF; osd_message("Dummy stopped"); return; }
    if (!rec_len) { osd_message("Nothing recorded yet (F8)"); return; }
    mode = DUMMY_PLAY;
    rec_pos = 0;
    osd_message("Dummy playing (F9 stops)");
}

const char *const dummy_names[DUMMY_COUNT] = { "STAND", "CROUCH", "JUMP", "BLOCK", "LOW BLOCK", "CPU" };

// The dummy's stance, held as player 2's directions while a fight is on (not
// on the character screen, where they would move its cursor). Blocking is
// holding away from player 1, and only while player 1 attacks, or the dummy would walk away instead.
static int since_attack = 1000;     // frames since player 1 pressed an attack

static uint32_t stance(void)
{
    if (!game.fight_scene || !ram2[game.fight_scene] || game_health(1) < 0) return 0;
    uint32_t back = 1u << BTN_LEFT;
    if (game.fighter_x) {
        int32_t x1, x2;
        memcpy(&x1, &ram2[game.fighter_x], 4);
        memcpy(&x2, &ram2[game.fighter_x + HEALTH_STEP], 4);
        if (x2 > x1) back = 1u << BTN_RIGHT;
    }
    switch (cfg.dummy) {
    case DUMMY_CROUCH: return 1u << BTN_DOWN;
    case DUMMY_JUMP: return 1u << BTN_UP;
    case DUMMY_BLOCK: return game.fighter_x && since_attack < 40 ? back : 0;
    case DUMMY_LOW_BLOCK: return 1u << BTN_DOWN | (game.fighter_x && since_attack < 40 ? back : 0);
    default: return 0;
    }
}

void training_inputs(uint32_t held[2])
{
    const uint32_t mask = (1u << BTN_START) - 1;     // directions and attacks only
    if (mode == DUMMY_OFF && (cfg.training || training_forced)) {
        static uint32_t prev;
        static int idle;
        uint32_t attacks = mask & ~(0xfu << BTN_UP);
        since_attack = held[0] & ~prev & attacks ? 0 : since_attack < 1000 ? since_attack + 1 : 1000;
        prev = held[0];
        uint32_t s = stance();
        // the game takes a direction held since before the round started
        // only once it is let go and pressed again: a dummy standing still
        // when it should not lets go for a frame
        bool still = game.fighter_state && ram2[game.fighter_state + HEALTH_STEP] == 0;
        idle = s & (1u << BTN_DOWN | 1u << BTN_UP) && still ? idle + 1 : 0;
        if (idle >= 10) idle = 0;
        else if (s) held[1] = (held[1] & ~(0xfu << BTN_UP)) | s;
    }
    if (mode == DUMMY_RECORD) {
        rec[rec_len++] = held[0] & mask;
        held[1] = (held[1] & ~mask) | (held[0] & mask);
        held[0] &= ~mask;
        if (rec_len == REC_MAX) training_record_toggle();
    } else if (mode == DUMMY_PLAY) {
        held[1] |= rec[rec_pos];
        rec_pos = (rec_pos + 1) % rec_len;
    }
}

const char *training_status(void)
{
    if (mode == DUMMY_RECORD) return "REC DUMMY";
    // the home menu's Practice is a mode of its own, with no label (as on the SNES)
    if (mode == DUMMY_PLAY) return cfg.training ? "TRAINING  DUMMY" : "DUMMY";
    return cfg.training ? "TRAINING" : NULL;
}

int game_health(int player)
{
    if (!game.health_p1) return -1;
    uint8_t h = ram2[HEALTH_P1 + (uint32_t)player * HEALTH_STEP];
    bool fight = ram2[TIMER_ONES] <= 9 && ram2[TIMER_TENS] <= 9;
    return fight && h <= HEALTH_MAX ? h : -1;
}
