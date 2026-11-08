// UltraCombo: Killer Instinct (MAME kinst). RAM locations not found yet are
// 0, which turns the features using them off.
#include "game.h"

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
    .music_set = 0x86240, .music = 0x86241,     // read at 880007a0, table at 8800e4d4

    // found by changing the difficulty in the game's operator menu (both versions)
    .adj_data = 0x1a00, .adj_len = 512, .adj_difficulty = 0x1a18,
    .adj_headers = { 0x1800, 0x2400 },
};
