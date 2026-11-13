// Training mode: health refill, timer freeze, and a recordable player 2 dummy.
#pragma once
#include <stdbool.h>
#include <stdint.h>

// Rewrite this frame's inputs (bits per BTN_*) for dummy recording/playback.
void training_inputs(uint32_t held[2]);
// Apply health/timer changes to game RAM; call before every emulated frame.
// `real` is false for run-ahead frames, which must not advance any counters.
void training_apply(bool real);

// Training mode for this session only (the home menu's Practice), not saved.
extern bool training_forced;
// The clock stands still (the home menu's TIMER: OFF), without the rest of training.
extern bool training_timer_off;

// What player 2 does in training when no recording plays: stands still,
// crouches, jumps, blocks standing or low (always away from player 1), or,
// in the console menu's Practice only, is the game's own CPU fighter.
enum { DUMMY_STAND, DUMMY_CROUCH, DUMMY_JUMP, DUMMY_BLOCK, DUMMY_LOW_BLOCK, DUMMY_CPU, DUMMY_COUNT };
extern const char *const dummy_names[];     // for the menus, upper case

void training_record_toggle(void);   // start/stop recording the dummy
void training_play_toggle(void);
const char *training_status(void);   // short label for the screen, or NULL
int  game_health(int player);        // current health bar, 0-120, or -1 outside a fight
