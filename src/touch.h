// On-screen controls for touch screens (phones and tablets): a d-pad and
// Start on the left, the Super Nintendo's six buttons on the right, a menu
// button in the corner. Drawn by the SDL renderer over the picture.
#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

enum { TOUCH_AUTO, TOUCH_ON, TOUCH_OFF };

// A finger event; true if the controls took it.
bool touch_event(const SDL_Event *ev, int win_w, int win_h);
// Whether the controls show: on, or auto with a touch screen and no pad.
bool touch_shown(bool pad_connected);
// Player 1's buttons held on the screen, one bit per BTN_*.
uint32_t touch_held(void);
// The menu buttons held (MB_* in home.h), by position as on a Nintendo pad.
unsigned touch_menu_buttons(void);
// Buttons newly pressed since the last call (BTN_* bits), and whether the
// menu button was tapped, for the program's own menu.
uint32_t touch_pressed(bool *menu_tapped);
// Draw the controls over the frame.
void touch_draw(SDL_Renderer *ren, int win_w, int win_h);
