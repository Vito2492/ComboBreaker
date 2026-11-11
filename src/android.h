// Android only: the Java side of the app (ComboBreakerActivity).
#pragma once
#include <stdbool.h>

// Ask the player for the folder holding their game files (the system's own
// folder chooser) and copy the files into the app's folder. `missing` names
// the files still needed. False: the player chose to quit.
bool android_pick_game_files(const char *missing);
