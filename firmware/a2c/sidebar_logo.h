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

// Sidebar logo bitmap (a simple CRT television), 32 pixels wide.
//
// Source art: firmware/a2c/art/sidebar_logo_tv.txt
// Regenerate with: tools/sidebar_logo2header.py firmware/a2c/art/sidebar_logo_tv.txt
//
// One array entry = one Apple II scanline slot (0..191, same indexing as
// s_screen_buffer). DVI_VERTICAL_REPEAT doubles each to two display lines,
// so a visually square shape needs roughly a 2:1 width:height ratio in the
// source art. Bit 31 is the leftmost pixel, matching the MSB-first
// convention used by s_screen_buffer / render_a2c_full_line().

#pragma once

#include <stdint.h>

#define SIDEBAR_LOGO_BITS_ROWS 22
#define SIDEBAR_LOGO_BITS_WIDTH 32
static const uint32_t sidebar_logo_bits[SIDEBAR_LOGO_BITS_ROWS] = {
    0x00200400, // ..........#..........#..........
    0x00100800, // ...........#........#...........
    0x00081000, // ............#......#............
    0x00042000, // .............#....#.............
    0x00024000, // ..............#..#..............
    0x00018000, // ...............##...............
    0x0ffffff0, // ....########################....
    0x3ffffffc, // ..############################..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3000000c, // ..##........................##..
    0x3ffffffc, // ..############################..
    0x0ffffff0, // ....########################....
    0x030000c0, // ......##................##......
    0x078001e0, // .....####..............####.....
};
