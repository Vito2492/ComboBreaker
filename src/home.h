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
// True when the home menu is available for this game and switched on.
bool home_enabled(void);
// Game speed chosen with the console codes (1 = the arcade's), for the frontend
// to run that many frames per frame of a fight.
double home_speed(void);
// The machine was reset or a state was loaded: forget any mode in progress.
void home_reset(void);

// Provided by the frontend.
void frontend_open_controls(void);      // the external menu's Controls page
void frontend_reset_game(void);         // power-cycle, applying DIP switch changes
void frontend_settings_changed(void);   // write the .ini when the menu closes
