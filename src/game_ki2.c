// ComboBreaker 2: Killer Instinct 2 (MAME kinst2), on its way to the
// console menus (branch ki2-gold). RAM locations are for the L1.4 code and
// were found by diffing deterministic runs.
#include "game.h"

// Each fighter's moves for the console menu's MOVE LIST, in the order of
// the fighter slots above. From a published move list of the Nintendo 64
// version (Killer Instinct Gold), which keeps the arcade's moves; not yet
// checked against the arcade game one by one. Gargos (slot 10) is the boss
// and has none.
//
// Format as in game_ki1.c. QCDF DB,D,DF; QCDB DF,D,DB. "1 BLOCK", "1/4 BAR",
// "1/2 BAR", "FULL BAR": how much of the super meter the move needs.

#define STUN_NOTE \
    "!STUN COUNTER: CHARGE B, THEN QP\n" \
    "!  AS A NORMAL MP FP MK FK HITS\n"

#define BREAKER_NOTE \
    "!PUNCHES BREAK KICKS, KICKS BREAK\n" \
    "!  PUNCHES; ANY STRENGTH WORKS\n"

#define FINISH_NOTES \
    "!MINI ULTRA: IN A COMBO, 2+ HITS,\n" \
    "!  FOE'S 1ST BAR FLASHING GREEN\n" \
    "!ULTRA, ULTIMATE: IN A COMBO, 2+\n" \
    "!  HITS, FOE'S 2ND BAR FLASHING RED\n" \
    "!NO MERCY: THE ULTIMATE MARKED SO,\n" \
    "!  OUTSIDE A COMBO, BAR FLASHING RED\n" \
    "!SUPREME VICTORY: WIN WITHOUT\n" \
    "!  LOSING YOUR FIRST BAR\n"

static const char *const ki2_moves[11] = {
    // Fulgore
    "#SPECIAL MOVES\n"
    "Energy charge|HCF+K\n"
    "Energy shield|QCB+QK (HOLD IT)\n"
    "Eye laser|QCDB+MP/FP\n"
    "Air eye laser|QCDB+FP IN AIR, 1 BLOCK\n"
    "Laser storm|QCF+P\n"
    "Fake laser storm|QCB+QP\n"
    "Lock on|QCDF+QK 1 BLOCK\n"
    "Triple laser storm|QCF+QP AFTER LOCK ON\n"
    "Teleport|B,QCB+K 1 BLOCK\n"
    "Invisibility|HCB+FK 1/2 BAR\n"
    "Overhead|B+FK\n"
    "Throw|F+FP\n"
    "#SUPER MOVES\n"
    "Twist uppercut|QCB,HCF+FP 1/2 BAR\n"
    "Multi-claw spin|HCF,B+FK 1/4 BAR\n"
    "Chest spark|HCB,F+QK 1/4 BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|HCF+MK\n"
    "Stun dizzy|F,QCF+QP\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|F,QCF+P IN THE AIR\n"
    "Air juggle|QCDB+FP\n"
    "Multi-hit juggle|B+FP\n"
    "Retaliation|F,QCF+P GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+P/K\n" BREAKER_NOTE
    "Ultra breaker|QCB,HCF+FP FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|F,QCF+QP\n"
    "Ultra combo|F,QCF+QK\n"
    "Ultimate|F,HCF+MK\n"
    "Ultimate, no mercy|HCF,B+MP\n",
    // T. J. Combo
    "#SPECIAL MOVES\n"
    "Backfist|CB,F+QP\n"
    "Double backfist|CF,B+QP\n"
    "Rolling punch|CB,F+MP\n"
    "Triple rolling punch|CB,HCF+MP\n"
    "Lunging punch|CB,F+FP\n"
    "Winding uppercut|H FP,R FP\n"
    "Tremor|CB,F+MK\n"
    "Lunging overhead|CB,F+FK\n"
    "Dash|B,F+QK CLOSE\n"
    "Fake dizzy|QCF+QK\n"
    "Overhead|B+FP\n"
    "Throw|F+MP\n"
    "#SUPER MOVES\n"
    "Combo of pain|HCB+FP FULL BAR\n"
    "Shocker|HCB+MK FULL BAR\n"
    "Multi rolling punch|HCB,F+MP 1/2 BAR\n"
    "Beat down|HCB,F+QP 1/2 BAR\n"
    "Shadow|HCB+MP 1/2 BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|CB,F+MP\n"
    "Stun dizzy|CB,F+FP\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|F,B+K IN THE AIR\n"
    "Air juggle|CB,F+FP\n"
    "Multi-hit juggle|F+FK\n"
    "Retaliation|B,F+K GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+P/K\n" BREAKER_NOTE
    "Ultra breaker|HCB+FP FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|CB,F+FK\n"
    "Ultra combo|CF,B+FP\n"
    "Ultimate|H QK,CF,B+R QK\n"
    "Ultimate, no mercy|CF,HCB,F+FK\n",
    // B. Orchid
    "#SPECIAL MOVES\n"
    "Flip kick|QCDB+K\n"
    "Sabre dash|QCDB+MP\n"
    "Sabre spin|QCDB+FP\n"
    "Airbuster|F,QCF+K\n"
    "Tonfa fire|QCF+P\n"
    "Fake tonfa fire|QCF+QK\n"
    "Slide|QCDF+K\n"
    "Overhead|B+FP\n"
    "Throw|F+FP\n"
    "#SUPER MOVES\n"
    "Helicopter kick|QCB,HCF+FK 1/2 BAR\n"
    "Tiger|HCB,F+MK 1/2 BAR\n"
    "Tonfa charge|HCF,B+FP 1/2 BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|QCDB+MP\n"
    "Stun dizzy|F,QCF+QK\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|QCDB+K IN THE AIR\n"
    "Air juggle|QCDF+FK\n"
    "Multi-hit juggle|F+FK\n"
    "Retaliation|F,QCF+K GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+P/K\n" BREAKER_NOTE
    "Ultra breaker|QCB,HCF+FK FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|QCDF+QK\n"
    "Ultra combo|QCDB+QK\n"
    "Ultimate|HCF,B+FP\n"
    "Ultimate, no mercy|CB,HCB+MK\n",
    // Jago
    "#SPECIAL MOVES\n"
    "Uppercut|F,QCF+P\n"
    "Wind kick|QCDB+K\n"
    "Laser blade|QCDB+MP/FP\n"
    "Slide|QCDF+K\n"
    "Endokuken|QCF+P\n"
    "Red endokuken|H FP,QCF+R FP\n"
    "Fake endokuken|QCF+QK\n"
    "Overhead|B+FP\n"
    "Throw|F+FP\n"
    "#SUPER MOVES\n"
    "Feel the juice|QCB,HCF+FP 1/2 BAR\n"
    "Fireball barrage|QCDB+QP 1/2 BAR\n"
    "Multi wind kick|HCB,F+MK 1/2 BAR\n"
    "Multi slide|HCF,B+FK 1/4 BAR\n"
    "Shadow|QCDB,F+MK FULL BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|QCDB+MK\n"
    "Stun dizzy|F,QCF+QP\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|QCDB+FK IN THE AIR\n"
    "Air juggle|QCDF+FK\n"
    "Multi-hit juggle|F+FK\n"
    "Retaliation|F,QCF+P GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+P/K\n" BREAKER_NOTE
    "Ultra breaker|QCB,HCF+FP FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|QCDF+QK\n"
    "Ultra combo|QCDB+QK\n"
    "Ultimate|CF,HCF+FK\n"
    "Ultimate, no mercy|HCB,F+MP\n",
    // Glacius
    "#SPECIAL MOVES\n"
    "Shoulder dash|QCF+MP\n"
    "Puddle uppercut|QCF+MK/FK\n"
    "Puddle teleport|QCF+QK\n"
    "Energy regain|QCF+H QK\n"
    "Icy grip|QCF+QP\n"
    "Arctic blast|QCB+P\n"
    "Blade uppercut|QCDB+FP\n"
    "Overhead|B+FP\n"
    "Throw|F+FP\n"
    "#SUPER MOVES\n"
    "Beat down|HCF,B+MP 1/4 BAR\n"
    "Fireball|HCB,F+FP 1/2 BAR\n"
    "Quad slam|HCF,B+QP 1/2 BAR\n"
    "Reverse uppercut|QCB+FK 1/2 BAR\n"
    "Uppercut rush|HCF,B+FK 1/4 BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|QCF+MK\n"
    "Stun dizzy|QCF+FP\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|F,QCF+K IN THE AIR\n"
    "Air juggle|QCF+FK\n"
    "Multi-hit juggle|F+FK\n"
    "Retaliation|QCF+FP GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|QCF+P/K\n" BREAKER_NOTE
    "Ultra breaker|HCF,B+QP FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|HCB+QK\n"
    "Ultra combo|HCF+QK\n"
    "Ultimate|QCB,F+MK\n"
    "Ultimate, no mercy|CB,HCB+QK\n",
    // Tusk
    "#SPECIAL MOVES\n"
    "Boot kick|QCDB+K\n"
    "Web of death|QCDB+FP (F+FP: 1 MORE)\n"
    "Skull splitter|QCDF+FK\n"
    "Fake skull splitter|QCDF+QK\n"
    "Conqueror|F,QCF+P\n"
    "Back stab|HCB+QP\n"
    "Overhead|B+FP\n"
    "Throw|F+FK\n"
    "#SUPER MOVES\n"
    "Destroyus|QCB,HCF+FP 1/2 BAR\n"
    "Rushing kick|HCB,F+FK 1/4 BAR\n"
    "Flaming sword|HCF,B+QK 1/2 BAR\n"
    "Shadow sword dive|HCF,B+FK 1/2 BAR\n"
    "Shadow|HCB,F+MK 1/2 BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|QCDB+FP\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|CF,QCDB+K IN THE AIR\n"
    "Air juggle|QCDF+FK\n"
    "Multi-hit juggle|F+FP\n"
    "Retaliation|F,QCF+P GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+P/K\n" BREAKER_NOTE
    "Ultra breaker|QCB,HCF+FP FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|HCF+MP\n"
    "Ultra combo|HCB+MP\n"
    "Ultimate|F,QCF+MK\n"
    "Ultimate|HCF,HCF+MP\n",
    // Spinal
    "#SPECIAL MOVES\n"
    "Teleport|QCB+K 1 SKULL\n"
    "Skele skewer|QCF+MP\n"
    "Skull throw|QCF+MK 1 SKULL\n"
    "Skull scrape|QCDF+FK\n"
    "Shield absorb|B+QP\n"
    "Soul drain|QCF+QP\n"
    "Skull restore|QCB+MP/FP 1/2 BAR\n"
    "!  MP GIVES 1 SKULL, FP ALL 6\n"
    "Dive kick|D+FK IN THE AIR\n"
    "Overhead|B+FP\n"
    "Throw|F+FK\n"
    "#SUPER MOVES\n"
    "Grim reaper|QCB,HCF+FP 1/2 BAR\n"
    "Skull throw|HCF+FK 1/4 BAR\n"
    "Stun skull|HCB,HCF+MK 1/2 BAR\n"
    "Multi sword stab|HCF,B+MP 1/2 BAR\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|D+K IN THE AIR\n"
    "Air juggle|QCF+MK 1 SKULL\n"
    "Multi-hit juggle|F+FP\n"
    "Retaliation|D+FP GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|QCB+P/K\n" BREAKER_NOTE
    "Ultra breaker|QCB,HCF+FK FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|QCF+FP\n"
    "Ultra combo|QCB+FP\n"
    "Ultimate|D,D+QK\n"
    "Ultimate, no mercy|HCF,HCF+QP\n",
    // Kim Wu
    "#SPECIAL MOVES\n"
    "Tornado kick|QCDB+K\n"
    "Firecracker|QCDB+MP/FP\n"
    "Air flame|QCDB+P IN THE AIR\n"
    "Split kick|QCDF+FK\n"
    "Kick uppercut|QCDF+MK\n"
    "Fire flower|QCF+QP\n"
    "Roll|F,F OR B,B\n"
    "Overhead|B+FK\n"
    "Throw|F+FP\n"
    "#SUPER MOVES\n"
    "Snapdragon|QCB,HCF+FP 1/2 BAR\n"
    "Shadow tornado kick|HCB,F+FK 1/2 BAR\n"
    "Spinning slashes|HCB,F+FP 1/4 BAR\n"
    "Air fireball|HCF,B+MP AIR, 1/2 BAR\n"
    "Shadow|HCB,F+MK 1/2 BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|QCDB+MK\n"
    "Stun dizzy|QCDB+MP\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|F,QCF+P IN THE AIR\n"
    "Air juggle|QCDB+FK\n"
    "Multi-hit juggle|F+FK\n"
    "Retaliation|F,QCF+P GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+P/K\n" BREAKER_NOTE
    "Ultra breaker|QCB,HCF+FP FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|QCDB+QK\n"
    "Ultra combo|QCF+QK\n"
    "Ultimate|HCF,B+QK\n"
    "Ultimate, no mercy|CF,HCF+MK\n",
    // Sabrewulf
    "#SPECIAL MOVES\n"
    "Claw spin|B,F+MK OR QCF+MK\n"
    "Claw pounce|B,F+FP OR QCF+FP\n"
    "Claw backflip|B,F+FK OR QCF+FK\n"
    "Claw cartwheel|B,F+MP OR QCF+MP\n"
    "Fake howl|B,F+QK OR F,B+QK\n"
    "Energy howl|F,B+QP\n"
    "Overhead|B+FP\n"
    "Throw|F+FK\n"
    "#SUPER MOVES\n"
    "Loopy|HCB,F+FK 1/2 BAR\n"
    "Ground spark|HCB+FP 1/2 BAR\n"
    "Triple fireball|HCB,F+FP 1/2 BAR\n"
    "Multi cartwheel|HCB,F+MP 1/4 BAR\n"
    "Multi spin|HCB,F+MK 1/4 BAR\n"
    "#AFTER A STUN COUNTER\n" STUN_NOTE
    "Stun reversal|B,F+MK OR QCF+MK\n"
    "Stun dizzy|B,F+FK OR QCF+FK\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|F,B+K IN THE AIR\n"
    "Air juggle|B,F+FP\n"
    "Multi-hit juggle|F+FK\n"
    "Retaliation|B,F+FK GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+P/K\n" BREAKER_NOTE
    "Ultra breaker|HCB,F+FK FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|CF,B+QK\n"
    "Ultra combo|CB,F+QK\n"
    "Ultimate|H FK,R FK\n"
    "Ultimate, no mercy|H QP,R QP\n",
    // Maya
    "#SPECIAL MOVES\n"
    "Flip kick|B,F+MK\n"
    "Savage blade|B,F+MP\n"
    "Mantis|B,F+FP\n"
    "Jungle leap|B,F+FK\n"
    "Cobra bite|B,F+QP\n"
    "Air mantis|F,B+FP IN THE AIR\n"
    "Overhead|B+FP\n"
    "Throw|F+FP\n"
    "#SUPER MOVES\n"
    "Tree cutter|HCB,F+FK 1/2 BAR\n"
    "Lawnmower|HCB,F+MP 1/4 BAR\n"
    "Flip kick|HCB,F+MK 1/4 BAR\n"
    "Multi mantis strike|HCB,F+FP 1/2 BAR\n"
    "#JUGGLES AND GETTING UP\n"
    "Air combo|F,B+K IN THE AIR\n"
    "Air juggle|B,F+FK\n"
    "Multi-hit juggle|F+FK\n"
    "Retaliation|B,F+QP GETTING UP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+P/K\n" BREAKER_NOTE
    "Ultra breaker|HCB,F+FK FULL BAR\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Mini ultra|CF,B+FP\n"
    "Ultra combo|CB,F+FK\n"
    "Ultimate|HCB,F+QK\n"
    "Ultimate, no mercy|HCF,B+QP\n",
    // Gargos: the boss, no list
    0,
};

const game_t game_ki2 = {
    .id = "ki2",
    .title = "Killer Instinct 2",
    .product = "ComboBreaker 2",
    .icon_text = "CB2",

    .versions = { { "1.4", "ki2-l14.u98", 0x27d0285e } },
    .sound_roms = { "ki2_l1.u10", "ki2_l1.u11", "ki2_l1.u12", "ki2_l1.u13",
                    "ki2_l1.u33", "ki2_l1.u34", "ki2_l1.u35", "ki2_l1.u36" },
    .sound_crc = { 0xfdf6ed51, 0xf9e70024, 0x2994c199, 0x3fe6327b,
                   0x6f4dcdcf, 0x5db48206, 0x7245ce69, 0x8920acbb },
    // the boot loader page, the sound OS it reboots into (bank 1), and the
    // two programs the OS copies into external RAM
    .sound_code = { { 0x0000, 0x0000, 0, true }, { 0x1000, 0x0000, 0, true },
                    { 0x0800, 0x0800, 0x300 }, { 0x2000, 0x0800, 0x400 } },
    .set = "kinst2",
    .disk = "kinst2.raw",
    .chd = "kinst2.chd",
    .chd_sha1 = "e7c9291b4648eae0012ea0cc230731ed4987d1d5",
    .cyls = 1463, .heads = 13, .secs = 47,
    .serial_offset = 11,
    .io = { .p1 = 0x10000098, .p2 = 0x10000090, .volume = 0x10000080,
            .unused = 0x100000a0, .dsw = 0x10000088, .coin = 0x100000b8, .nop = 0x100000b0 },

    .snapshot_frames = 900,
    .input_lag = 1,

    .credits = 0x7ff124, .credits_needed = 2,
    .health_p1 = 0x7fc105, .health_step = 0x100, .health_max = 120,
    .timer_tick = 0x7f7a10, .timer_ones = 0x7f7a11, .timer_tens = 0x7f7a12,
    .vars = 0x7f0000, .vars_len = 0x10000,

    // checksum of the 88 bytes after it, computed by the game at 0x8802d400;
    // +4 is set once the adjustments were edited
    // The console menu (home.c), found the same way as UltraCombo's: the
    // attract loop shows where 7ff29c is 0 (the title screen stays up once
    // Start is pressed there; it has no countdown). Per player, 0x130 apart:
    // in a game, picked a fighter.
    .attract = 0x7ff29c, .attract_zero = true,
    .in_game = 0x7ff148, .picked = 0x7ff149, .player_step = 0x130,
    // the top byte of a pointer in player 1's fighter, there from the
    // round's start through the winner's pose (not on the VS screen nor the
    // victory screen); the continue screen after a match
    .fight_scene = 0x7fc0d3, .after_match = 0x7ff136, .match_over = 0x7ff136,
    // a fighter's place on screen (16.16) and what it is doing (0 standing,
    // 1 walking, 11 crouching), per fighter 0x100 apart, as in the first game
    .fighter_x = 0x7fc118, .fighter_state = 0x7fc0cc,
    // the fighter each player has, also the character screen's cursor; who
    // is still to be fought (poking it picks the next opponent)
    .fighter_slot = 0x7ff15b, .opponents = 0x7ff428,
    .moves = ki2_moves,
    .fighter_names = { "FULGORE", "T. J. COMBO", "B. ORCHID", "JAGO", "GLACIUS", "TUSK",
                       "SPINAL", "KIM WU", "SABREWULF", "MAYA", "GARGOS" },
    .adj_sum = 0xa00, .adj_data = 0xa04, .adj_len = 88, .adj_difficulty = 0xa1c,
};
