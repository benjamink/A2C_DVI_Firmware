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

#include <pico/stdlib.h>
#include <string.h>

#include "sidebar.h"
#include "sidebar_logo.h"
#include "config/config.h"
#include "applebus/buffers.h"   // character_rom[]
#include "dvi/tmds.h"           // tmds_mono_pixel_pair[]

sidebar_mode_t cfg_sidebar = SIDEBAR_OFF;

// One precomputed content row per Apple II scanline slot (0..191), shared by
// both the left and right bar - the art is identical on both sides, not
// mirrored. Bit 31 = leftmost of the 32 addressable bar pixels, matching the
// MSB-first convention render_a2c_full_line() uses for s_screen_buffer.
#define SIDEBAR_ROWS 192
static uint32_t s_sidebar_bits[SIDEBAR_ROWS];

static volatile bool s_dirty = true;
static sidebar_mode_t s_last_mode = (sidebar_mode_t)0xff; // sentinel: forces a rebuild on the first call
static char    s_info_lines[SIDEBAR_MAX_INFO_LINES][5];
static uint8_t s_info_count = 0;

// Layout, in Apple II scanline slots (192 available):
//   rows [0, LOGO_START_ROW)                         - blank
//   rows [LOGO_START_ROW, LOGO_START_ROW+22)          - TV logo
//   rows [.., + TEXT_GAP_ROWS)                        - blank
//   rows [.., + 8), +2 gap, +8, +2 gap, ...           - one block per settings line
// With SIDEBAR_MAX_INFO_LINES(8) lines: 4 + 22 + 4 + (8*8 + 7*2) = 108 of 192
// rows - see the SIDEBAR_MAX_INFO_LINES comment in sidebar.h for the budget.
#define LOGO_START_ROW  4
#define LOGO_ROWS       SIDEBAR_LOGO_BITS_ROWS
#define TEXT_GAP_ROWS   4
#define GLYPH_ROWS      8
#define LINE_GAP_ROWS   2
#define TEXT_START_ROW  (LOGO_START_ROW + LOGO_ROWS + TEXT_GAP_ROWS)

void sidebar_init(void)
{
    s_dirty = true;
}

void sidebar_set_info(const char* const* lines, uint8_t count)
{
    if (count > SIDEBAR_MAX_INFO_LINES)
        count = SIDEBAR_MAX_INFO_LINES;

    // Cheap early-out: only mark dirty (and trigger a rebuild) if something
    // actually changed, so an idle screen doesn't rebuild every frame.
    bool changed = (count != s_info_count);
    for (uint8_t i = 0; (!changed) && (i < count); i++)
    {
        if (strncmp(s_info_lines[i], lines[i], 4) != 0)
            changed = true;
    }

    if (changed)
    {
        for (uint8_t i = 0; i < count; i++)
        {
            strncpy(s_info_lines[i], lines[i], 4);
            s_info_lines[i][4] = 0;
        }
        s_info_count = count;
        s_dirty = true;
    }
}

// Reads one row of a glyph out of the Apple II character ROM. The ROM
// stores 256 glyphs * 8 rows, 7 significant bits per row, with bit 0 = the
// LEFTMOST source pixel (see render_text.c: char_text_bits() is consumed
// with `bits >>= 1` starting at bit 0). That is the opposite convention from
// s_sidebar_bits, so the bits get reversed while packing below.
//
// Index with (ASCII | 0x80) to select the "normal" (non-inverse,
// non-flashing) glyph bank - the same selection char_text_bits() makes when
// bit 7 of the character byte is already set.
static inline uint8_t sidebar_glyph_row(char ch, uint row)
{
    uint8_t index = ((uint8_t)ch) | 0x80;
    return character_rom[((uint16_t)index << 3) | row] & 0x7f;
}

// Packs up to 4 characters, left-aligned at 7px each (28 of the 32 bar
// pixels; 4px of right-hand padding), into out[8] - one MSB-first word per
// glyph row.
static void sidebar_pack_text4(uint32_t out[GLYPH_ROWS], const char* text)
{
    memset(out, 0, GLYPH_ROWS * sizeof(uint32_t));
    for (uint ci = 0; (ci < 4) && (text[ci] != 0); ci++)
    {
        for (uint row = 0; row < GLYPH_ROWS; row++)
        {
            uint8_t bits = sidebar_glyph_row(text[ci], row);
            for (uint col = 0; col < 7; col++)
            {
                if (bits & (1u << col))
                {
                    uint global_col = ci * 7 + col;          // 0..27
                    out[row] |= (1u << (31 - global_col));   // MSB-first
                }
            }
        }
    }
}

void sidebar_rebuild_if_dirty(void)
{
    // cfg_sidebar itself is changed directly by the menu (sidebar_command()
    // in a2c.c), not through this file, so detect that change here too -
    // this function is already polled once per frame, so an extra enum
    // compare is free.
    if (cfg_sidebar != s_last_mode)
    {
        s_last_mode = cfg_sidebar;
        s_dirty = true;
    }

    if (!s_dirty)
        return;

    memset(s_sidebar_bits, 0, sizeof(s_sidebar_bits));

    if (cfg_sidebar != SIDEBAR_OFF)
    {
        for (uint i = 0; i < LOGO_ROWS; i++)
            s_sidebar_bits[LOGO_START_ROW + i] = sidebar_logo_bits[i];
    }

    if (cfg_sidebar == SIDEBAR_LOGO_INFO)
    {
        uint32_t glyph_rows[GLYPH_ROWS];
        uint     row = TEXT_START_ROW;

        for (uint8_t i = 0; i < s_info_count; i++)
        {
            if (row + GLYPH_ROWS > SIDEBAR_ROWS)
                break; // defensive: SIDEBAR_MAX_INFO_LINES already keeps this from happening

            sidebar_pack_text4(glyph_rows, s_info_lines[i]);
            for (uint g = 0; g < GLYPH_ROWS; g++)
                s_sidebar_bits[row + g] = glyph_rows[g];
            row += GLYPH_ROWS + LINE_GAP_ROWS;
        }
    }

    s_dirty = false;
}

// Writes 16 words (32 pixels) per channel from a packed bit row, using the
// same bit-balanced TMDS pair table and color_mode*12 tint indexing the
// 80-column text renderer uses - so the art inherits the user's chosen
// white/green/amber tint for free.
static inline void sidebar_emit16(uint32_t* r, uint32_t* g, uint32_t* b,
                                   uint32_t bits, uint8_t color_offset)
{
    for (uint i = 0; i < 16; i++)
    {
        // Same "reverse and pair" extraction render_a2c_full_line() uses
        // for s_screen_buffer: take the top two bits of the word as one
        // pixel pair, reversed, then shift.
        uint32_t dot = ((bits >> 29) & 0x02) | ((bits >> 31) & 0x01);
        uint32_t idx = color_offset + dot;

        *(r++) = tmds_mono_pixel_pair[idx + 0];
        *(g++) = tmds_mono_pixel_pair[idx + 4];
        *(b++) = tmds_mono_pixel_pair[idx + 8];

        bits <<= 2;
    }
}

// Width, in words, of the precomputed bar content (16 words = 32px, matching
// SIDEBAR_LOGO_BITS_WIDTH). Kept private to this file so callers only ever
// need to hand in the margin width they already compute; they don't need to
// know how wide the art itself is.
#define SIDEBAR_CONTENT_WORDS 16

void sidebar_render_line(uint32_t line, uint8_t color_mode,
                          uint32_t* red0, uint32_t* green0, uint32_t* blue0,
                          uint32_t right_margin, uint32_t left_margin)
{
    if ((cfg_sidebar == SIDEBAR_OFF) || (line >= SIDEBAR_ROWS))
        return;

    uint32_t bits = s_sidebar_bits[line];
    if (bits == 0)
        return; // row is blank content - leave the existing black fill alone

    uint8_t  color_offset = color_mode * 12; // matches render_a2c_full_line()'s RM_BW indexing

    // Center the 16-word (32px) content within whichever margin width the
    // current video mode has: 0 words at 640x480 (16-word margin), 10 words
    // at 720x480 (36-word margin).
    uint32_t ofs = (left_margin > SIDEBAR_CONTENT_WORDS)
                       ? (left_margin - SIDEBAR_CONTENT_WORDS) / 2
                       : 0;

    sidebar_emit16(red0 + ofs, green0 + ofs, blue0 + ofs, bits, color_offset);
    sidebar_emit16(red0 + right_margin + ofs, green0 + right_margin + ofs,
                    blue0 + right_margin + ofs, bits, color_offset);
}
