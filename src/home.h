// Console-style home menu: the modes of the home versions (Arcade, VS,
// Tournament, Practice, Options), on top of the arcade game. It drives the
// game only through its own inputs (Start presses, which player's controls
// go where), so emulation stays exactly the arcade's.
#pragma once
#include <stdbool.h>
#include <stdint.h>

// Rewrite this frame's inputs (bits per BTN_*): menu navigation, the Start
// presses that take the game to the chosen mode, input routing in a mode.
void home_inputs(uint32_t held[2]);
// Draw the menu (or the pause panel) into the 320x240 ARGB overlay.
void home_draw(uint32_t *ov);
// True while the game should stand still (the pause panel is showing).
bool home_paused(void);
// True while one of the home menu's pages is up (not the pause panel).
bool home_menu_open(void);
// True when the home menu is available for this game and switched on.
bool home_enabled(void);
// Game speed chosen with the console codes (1 = the arcade's), for the frontend
// to run that many frames per frame of a fight.
double home_speed(void);
// The machine was reset or a state was loaded: forget any mode in progress.
void home_reset(void);
// A save state was loaded: close the menu, go on in the mode it shows.
void home_state_loaded(void);

// Provided by the frontend.
void frontend_open_controls(void);      // the external menu's Controls page
void frontend_reset_game(void);         // power-cycle, applying DIP switch changes
void frontend_settings_changed(void);   // write the .ini when the menu closes
uint64_t frontend_frames(void);         // frames emulated since the machine was powered on
// The pads' face buttons by where they sit, as the SNES names them, and the
// keyboard's menu keys: the home menu's controls never follow game remaps.
enum { MB_A = 1, MB_B = 2, MB_X = 4, MB_Y = 8, MB_SELECT = 16 };
unsigned frontend_menu_buttons(void);
// The name of what player 1 or 2 presses for one of the six game buttons
// (the pad's button as labelled on it, or the key without a pad).
const char *frontend_button_name(int player, int button);
