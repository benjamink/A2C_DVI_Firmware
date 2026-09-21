/*
MIT License

Copyright (c) 2026 A2C_DVI contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Draws a static logo, and optionally a live settings readout, into the
// black pillarbox bars beside the Apple IIc picture. No DVI timing or
// resolution change is involved: this only replaces some of the
// TMDS_SYMBOL_0_0 black words that render_a2c_full_line() already writes
// into the margins every scanline.
//
// See docs/sidebar-logo/PLAN.md in the A2C_DVI_SMD repo for the design
// rationale (bar geometry, TMDS indexing, burn-in mitigation).

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    SIDEBAR_OFF       = 0, // default: bars stay plain black, exactly like today
    SIDEBAR_LOGO      = 1, // logo only
    SIDEBAR_LOGO_INFO = 2, // logo + a settings readout underneath
} sidebar_mode_t;

extern sidebar_mode_t cfg_sidebar;

// Maximum number of settings-readout lines sidebar_set_info() accepts. Row
// budget: LOGO_START_ROW(4) + LOGO_ROWS(22) + TEXT_GAP_ROWS(4) + N*(8+2)
// must stay under SIDEBAR_ROWS(192) - 8 lines is 106 rows, comfortably so.
#define SIDEBAR_MAX_INFO_LINES 8

// One-time setup. Safe to call multiple times.
void sidebar_init(void);

// Update the (up to 4-character each) settings strings shown under the logo
// in SIDEBAR_LOGO_INFO mode, one line per array entry. `count` must be
// <= SIDEBAR_MAX_INFO_LINES. Cheap - just string compares and copies - so it
// is meant to be called once per frame from render_a2c(), not from a menu
// callback. Does not touch the scanline-critical rendering path itself.
void sidebar_set_info(const char* const* lines, uint8_t count);

// Rebuilds the precomputed per-line bar content if `sidebar_set_info()` or
// `cfg_sidebar` changed since the last rebuild. Call once per frame, e.g.
// from render_a2c(), never from inside the per-scanline render loop.
void sidebar_rebuild_if_dirty(void);

// Emits one scanline's worth of logo/settings content into the LEFT margin
// only - the right bar is left as the plain black
// render_a2c_full_line()'s fill loop already wrote there. Must be called
// AFTER that function's margin black-fill loop and BEFORE dvi_send_scanline().
//
//   line             - 0..191, same indexing as s_screen_buffer
//   color_mode       - COLOR_MODE_BW / GREEN / AMBER; the art inherits the
//                       user's chosen mono tint, same as the text renderer
//   red0/green0/blue0- pointers to the FIRST word of the left margin for
//                       this scanline (captured before the black-fill loop
//                       advances them)
//   left_margin      - the full margin width in words, as computed by
//                       render_a2c_full_line() (16 words/32px at 640x480,
//                       36 words/72px at 720x480). The bar's 16-word (32px)
//                       content is centered within it.
void sidebar_render_line(uint32_t line, uint8_t color_mode,
                          uint32_t* red0, uint32_t* green0, uint32_t* blue0,
                          uint32_t left_margin);
