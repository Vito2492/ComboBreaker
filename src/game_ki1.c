// UltraCombo: Killer Instinct (MAME kinst). RAM locations not found yet are
// 0, which turns the features using them off.
#include "game.h"

// Each fighter's moves for the console menu's MOVE LIST, by the fighter's
// place on the character screen. From a published move list of the Super
// Nintendo version; the arcade's are the same as far as tested (every
// combo breaker here was checked against the arcade game).
//
// One line per move, "NAME|INPUT"; a line starting with '#' is a heading.
// F toward the opponent, B away, D down, U up. QCF D,DF,F; QCB D,DB,B;
// HCF B,DB,D,DF,F; HCB F,DF,D,DB,B. CB,F: hold back a second, then
// forward. QP MP FP quick, medium, fierce punch; QK MK FK the kicks; P or
// K any strength. H: hold the button, R: let it go.

// What the finishing moves need, under each fighter's heading.
#define FINISH_NOTES \
    "!NO MERCY: AFTER THE 2ND KNOCKOUT,\n" \
    "!  AT THE DISTANCE SHOWN\n" \
    "!HUMILIATION: THE SAME, IF YOU\n" \
    "!  STILL HAVE YOUR FIRST BAR\n" \
    "!ULTIMATE, ULTRA: IN A COMBO,\n" \
    "!  2+ HITS, FOE'S BAR FLASHING RED\n"

static const char *const ki1_moves[11] = {
    // T. J. Combo
    "#SPECIAL MOVES\n"
    "Single backfist|CB,F+QP\n"
    "Double backfist|CF,B+QP\n"
    "Triple backfist|CF,B+QP,QP\n"
    "Double roll punch|CB,F+MP\n"
    "Triple roll punch|CB (2 S),F+MP\n"
    "Lunging punch|CB,F+FP\n"
    "Winding uppercut|H FP 2 S,R FP,FP\n"
    "Flying knee|CB,F+QK/FK\n"
    "Rising knee|CB,F+MK\n"
    "Punching bag|QP,QP,QP... CLOSE\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Shadow lunging punch|H FP,CB,F+R FP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+K\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Neck breaker|B,F,F+MP CLOSE\n"
    "Screen punch|HCF+FK CLOSE\n"
    "Humiliation|D,D,D+QP\n"
    "Ultimate|HCB+MK IN A COMBO\n"
    "Ultra combo|CF,B+FP IN A COMBO\n",
    // Jago
    "#SPECIAL MOVES\n"
    "Green fireball|QCF+P\n"
    "Yellow fireball|H QP,QCF+R QP\n"
    "Flying uppercut|F,QCF+P\n"
    "Sliding uppercut|CF,QCF+P\n"
    "Green sword|DF,D,DB+FP\n"
    "Flying heel kick|DB,D,DF+QK\n"
    "Flying heel kick|DF,D,DB+K\n"
    "Fast flying kick|H MK,DF,D,DB+R MK\n"
    "Fast flying kick|H FK,DF,D,DB+R FK\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Fast red fireball|H FP,QCF+R FP\n"
    "Shadow fast heel kick|H FK,DF,D,DB+R FK\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+P\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Sword stab|B,F,F+QP CLOSE\n"
    "Falling car|B,B,F,F+MP 3 AWAY\n"
    "Humiliation|HCB+MK\n"
    "Ultimate|HCF+FP IN A COMBO\n"
    "Ultra combo|DF,D,DB+QK IN A COMBO\n",
    // Thunder
    "#SPECIAL MOVES\n"
    "Flying mohawk|QCB+QP/MP OR HCB+FP\n"
    "Spinning chop|CB,F+P OR CF,B+MP\n"
    "Tomahawk dive|QCB+FP IN THE AIR\n"
    "Phoenix throw|QCF+K\n"
    "Fast phoenix|H MK,QCF+R MK\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Shadow tomahawk dive|H FP,HCB+R FP IN THE AIR\n"
    "Giant red phoenix|QCF+MK\n"
    "Small red phoenix|H MK,QCF+R MK\n"
    "#COMBO BREAKER\n"
    "Combo breaker|QCB+P\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Rain dance|HCF+FP 2 AWAY\n"
    "Axe uppercut|HCB+FK CLOSE\n"
    "Humiliation|D,F,F+QK\n"
    "Ultimate|DF,D,DB+MP IN A COMBO\n"
    "Ultra combo|B,F+QP IN A COMBO\n",
    // Glacius
    "#SPECIAL MOVES\n"
    "Blade uppercut|DF,D,DB+QP\n"
    "Shoulder charge|CB,F+P\n"
    "Double charge|CB,F+QP\n"
    "Bouncing iceball|QCF+P\n"
    "Puddle teleport|QCF+QK\n"
    "Puddle uppercut|QCF+MK/FK\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Fast bouncing iceball|H QP,QCF+R QP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+P\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Blob engulf|HCB+MK 1 AWAY\n"
    "Needle freeze|HCF+MP 1 AWAY\n"
    "Acid puddle|B,B,B+FK 1 AWAY\n"
    "Humiliation|F,F,B+QK\n"
    "Ultimate|HCB+QP IN A COMBO\n"
    "Ultra combo|B,F+FP IN A COMBO\n",
    // B. Orchid
    "#SPECIAL MOVES\n"
    "Sabre charge|DF,D,DB+QP/MP\n"
    "Swinging sabres|DF,D,DB+FP\n"
    "Tiger charge|CB,F+P\n"
    "Laser boomerang|QCF+P\n"
    "Helicopter kick|CB,F+K\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Shadow sabre charge|DF,D,DB+MP\n"
    "Shadow tiger charge|H FP,CB,F+R FP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+K\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Frog squash|D,F,B+QK... FK CLOSE\n"
    "Heart attack|B,F,F+QP 1 AWAY\n"
    "Humiliation|HCB+FP\n"
    "Ultimate|HCB+MK IN A COMBO\n"
    "Ultra combo|B,F+MP IN A COMBO\n",
    // Cinder
    "#SPECIAL MOVES\n"
    "Flaming palm|B,B+QP\n"
    "Red outline|HCB+MP\n"
    "Invisibility|HCB+FP\n"
    "Flame torpedo|F,F+P OR CB,F+P\n"
    "Flame stream|F,F+K OR CB,F+K\n"
    "Flip kick|F,QCF+K\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Shadow flame torpedo|H MP,F,F+R MP\n"
    "Invisible teleport front|HCB+QP\n"
    "Invisible teleport back|H QP,HCB+R QP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+K\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Lava pool|B,B,B+MP 2 AWAY\n"
    "Meltdown|HCF+QK 2 AWAY\n"
    "Humiliation|B,B,B+FK\n"
    "Ultimate|HCB+MK IN A COMBO\n"
    "Ultra combo|F,F+FP IN A COMBO\n",
    // Sabrewulf
    "#SPECIAL MOVES\n"
    "Flaming bat|QCB+P\n"
    "Spinning claw|CB,F+P OR CF,B+MP\n"
    "Rolling claw|CB,F+QK\n"
    "Charging claw|CB,F+MK\n"
    "Leaping claw|CB,F+FK\n"
    "Energy howl|DF,D,DB+FK STARTS SHADOW\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Double spinning claw|CB,F+P\n"
    "Double rolling claw|CB,F+QK\n"
    "Long leaping claw|CB,F+FK\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+K\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Claw stab|B,B+MK CLOSE\n"
    "Screen punch|B,B,F+MP 2 AWAY\n"
    "Humiliation|F,F+QP\n"
    "Ultimate|HCF+FP IN A COMBO\n"
    "Ultra combo|F,B+QK IN A COMBO\n",
    // Riptor
    "#SPECIAL MOVES\n"
    "Claw uppercut|DF,D,DB+QP\n"
    "Charge attack|CB,F+P\n"
    "Fireball|QCB+P\n"
    "Air fireball|QCB+FP IN THE AIR\n"
    "Flame jet|DF,D,DB+FP\n"
    "Leaping kick|CB,F+K OR HCF+QK\n"
    "Tail flip|DF,D,DB+K\n"
    "Air tail flip|DF,D,DB+FK IN THE AIR\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Fast leaping kick|H FK,CB,F+R FK\n"
    "Fast purple fireball|H MP,QCB+R MP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|B,F+K\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Eat opponent|HCF+MP 2 AWAY\n"
    "Acid spit|B,B,B+MK 2 AWAY\n"
    "Tail stab|F,F,B+FK\n"
    "Humiliation|D,D,F,F+FP\n"
    "Ultimate|HCF+QP IN A COMBO\n"
    "Ultra combo|B,F+QK IN A COMBO\n",
    // Spinal
    "#SPECIAL MOVES\n"
    "Absorb shield|B+H QP\n"
    "Shield charge|F,F+P\n"
    "Teleport front|D,D+P\n"
    "Teleport behind|D,D+K\n"
    "Flaming sword|CB,F+MP OR CF,B+MP\n"
    "Slide|DB,D,DF+FK\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Flaming skull|QCF+P\n"
    "Double flaming sword|CB,F+MP\n"
    "Fast red fireball|H P,QCF+R P\n"
    "Two flaming skulls|H QP,QCF+MP+R QP\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,F+P OR D,D+K\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Skeleton grab|B,B,B+MK 1 AWAY\n"
    "Shield stab|B,B,F+QK 1 AWAY\n"
    "Humiliation|HCF+FK\n"
    "Ultimate|DF,D,DB+QP IN A COMBO\n"
    "Ultra combo|HCF+FP IN A COMBO\n",
    // Fulgore
    "#SPECIAL MOVES\n"
    "Flying uppercut|F,QCF+P\n"
    "Energy bolt|QCF+P\n"
    "Two energy bolts|B,B,QCF+QP\n"
    "Three energy bolts|F,B,B,QCF+QP\n"
    "Reflect shield|QCB+P\n"
    "Teleport front|B,QCB+P\n"
    "Teleport behind|B,QCB+K\n"
    "Claw charge|CB,F+K\n"
    "Eye laser|DF,D,DB+FK\n"
    "#SHADOW MOVES (AFTER A BREAKER)\n"
    "Three energy bolts|QCF+QP\n"
    "Shadow flying uppercut|H FP,F,QCF+R FP\n"
    "Shadow energy charge|H FK,CB,F+R FK\n"
    "#COMBO BREAKER\n"
    "Combo breaker|F,QCF+P\n"
    "#FINISHING MOVES\n" FINISH_NOTES
    "Eye laser|HCB+FP 2 AWAY\n"
    "Machine gun|HCF+FK 2 AWAY\n"
    "Humiliation|HCF+MK\n"
    "Ultimate|DF,D,DB+MP IN A COMBO\n"
    "Ultra combo|F,QCF+QP IN A COMBO\n",
    0,                                  // Eyedol
};

// The CODES page, from a published move list (not tested here).
static const char ki1_codes[] =
    "#CHARACTER SCREEN\n"
    "Random fighter|U+START\n"
    "Other colours|HOLD U OR D, PICK\n"
    "#STAGE: HOLD THE DIRECTION, PICK\n"
    "Riptor's stage|D+MP\n"
    "Thunder's stage|D+QP\n"
    "Eyedol's stage|D+FP\n"
    "Fulgore's stage|D+FK\n"
    "Chicago alleyway|D+QK\n"
    "Sabrewulf's stage|D+MK\n"
    "Ice temple|U+QP\n"
    "Cave|U+MP\n"
    "Orchid's stage|U+MK\n"
    "Ice shrine|U+FP\n"
    "Cinder's stage|U+FK\n"
    "Castle roof|U+QK\n"
    "Sky platform|D+MK, BOTH PLAYERS\n"
    "#VS SCREEN\n"
    "Easy combo breakers|HOLD D, START\n"
    "!  BREAKERS WITH ANY STRENGTH; THE\n"
    "!  ANNOUNCER SAYS COMBO BREAKER\n"
    "Hyper mode (faster)|HOLD F+QP+MP+FP\n"
    "!  UNTIL A SWOOSH AT THE END\n"
    "Play as Eyedol|PICK CINDER, THEN\n"
    "|HOLD F, QP QK FP MK MP FK\n"
    "!2 PLAYERS: BOTH ENTER VS CODES\n";

const game_t game_ki1 = {
    .id = "ki1",
    .title = "Killer Instinct",
    .product = "ComboBreaker",
    .icon_text = "CB",

    .versions = { { "1.5d", "ki-l15d.u98", 0x7b65ca3d }, { "1.4", "ki-l14.u98", 0xafedb75f } },
    .sound_roms = { "u10-l1", "u11-l1", "u12-l1", "u13-l1",
                    "u33-l1", "u34-l1", "u35-l1", "u36-l1" },
    .sound_crc = { 0xb6cc155f, 0x0b5e05df, 0xd05ce6ad, 0x7d0954ea,
                   0x8bbe4f0c, 0xb2e73603, 0x0aaef4fc, 0x0577bb60 },
    // the boot loader page, the sound OS it reboots into (bank 1), and the
    // two programs the OS copies into external RAM
    .sound_code = { { 0x0000, 0x0000, 0, true }, { 0x1000, 0x0000, 0, true },
                    { 0x0800, 0x0800, 0x300 }, { 0x2000, 0x0800, 0x400 } },
    .set = "kinst",
    .disk = "kinst.raw",
    .chd = "kinst.chd",
    .chd_sha1 = "81d833236e994528d1482979261401b198d1ca53",
    .cyls = 419, .heads = 13, .secs = 47,
    .serial_offset = 27,
    .io = { .p1 = 0x10000080, .p2 = 0x10000088, .volume = 0x10000090,
            .unused = 0x10000098, .dsw = 0x100000a0, .coin = 0x100000b0 },

    .snapshot_frames = 900,
    .input_lag = 0,             // measured: the next frame already shows a press
    .has_blood_colour = true,

    .health_p1 = 0x8bc55, .health_step = 0x100, .health_max = 120,   // each of the two bars
    .health_frac = 0x8bc54,     // health is 16 bits (0x7800 full); the bar drawer at 8800b33c reads both
    .health_shown = 0x8bc56,    // and eases this one toward it
    .timer_tick = 0x884f8, .timer_ones = 0x884f9, .timer_tens = 0x884fa,
    .vars = 0x880000, .vars_len = 0x10000,
    // found by comparing RAM in attract, title, character select and fights
    .attract = 0x8f2da, .in_game = 0x8f326, .picked = 0x8f334, .player_step = 0x68,
    // the stage routine at 880004a8 (traced by hand)
    .stage_pool = 0xe4a4, .stage_last = 0x8dc28, .stage = 0x86238,
    .match_over = 0x86230, .title_timer = 0x8f2c0,
    // the winner, stored by the match-end routine at 880070a8; the scene's
    // own byte (zero outside fights) clears with the fight's objects as the
    // continue or high-score screen comes up, on a KO or a time-out alike
    // (match_over stays 0 on a time-out). Found by diffing RAM.
    .match_result = 0x86204, .fight_scene = 0x884fd,
    // set by the routine at 8802cf60 as the continue and high-score screens
    // start (after the victory screen, when there is one), cleared at 8802d01c
    .after_match = 0x40e50,
    // the level routine at 880005b0: level = table[difficulty][fight] plus
    // an adjustment, stored once per fight at 88000690
    .cpu_level = 0x8dc2c, .level_table = 0x55c, .difficulty = 0x3a168,
    // found by setting them in RAM and watching who comes out
    .opponents = 0x8f428, .cursor = 0x8f314,
    .fighter_x = 0x8bc68, .fighter_state = 0x8bc1c,     // by holding directions in a fight
    // found by breaking combos in scripted fights: a quick breaks a medium,
    // a medium a fierce, a fierce a quick
    .fighter_combo = 0x8bc8d, .fighter_strength = 0x8bc62, .fighter_slot = 0x8f33b,
    .fighter_shadow = 0x8bcbb,  // 3 after a breaker, down by one with each of T. J. Combo's shadow lunging punches
    .breakers = {
        "B DB D DF F K",    // Combo
        "F D DF P",         // Jago
        "F DF D DB B P",    // Thunder
        "B DB D DF F P",    // Glacius
        "B DB D DF F K",    // Orchid
        "F D DF K",         // Cinder
        "B DB D DF F K",    // Sabrewulf
        "B DB D DF F K",    // Riptor
        "F N F P",          // Spinal
        "F D DF P",         // Fulgore
        0,                  // Eyedol
    },
    .moves = ki1_moves,
    .codes = ki1_codes,
    .fighter_names = { "T. J. COMBO", "JAGO", "CHIEF THUNDER", "GLACIUS", "B. ORCHID", "CINDER",
                       "SABREWULF", "RIPTOR", "SPINAL", "FULGORE", "EYEDOL" },
    .combos = {             // found the same way, each tried on T. J. Combo
        "59 C LQ LM", "51 Q HF HM", "57 C HQ HM", "56 C HQ HM", "53 C LM HM",
        "58 C HM LM", "64 C HM HM", "98 C HM HQ", "76 Q HQ HM", "63 C LQ HM", 0,
    },
    .music_set = 0x86240, .music = 0x86241,     // read at 880007a0, table at 8800e4d4

    // found by changing the difficulty in the game's operator menu (both versions)
    .adj_data = 0x1a00, .adj_len = 512, .adj_difficulty = 0x1a18,
    .adj_headers = { 0x1800, 0x2400 },
};
