/*
 * lcd.c
 *
 *  Created on: 05/01/2012
 *      Author: Alan Green
 *
 * LCD Library graphics functions.
 *
 * This file defines the 1K byte graphics RAM. It is the only file in
 * this library which references that RAM block.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "lcd.h"

//
// Half the 328P's RAM
// Lay out is 64 Rows of 16 Bytes
uint8_t d_buffer[1024];

//
// Call this to paint the d_buffer ram onto the d_buffer
// Display will then be in graphics mode. Call lcd_reset() to go back to text mode
void display_refresh() {
    // Initialize graphics mode
    lcd_instruction(0b00110100); // 8bit data, extended instructions
    lcd_instruction(0b00110110); // +graphics

    uint8_t *p = d_buffer;
    for (int row = 0; row < 64; row++) {
        // To Start of Row
        lcd_instruction(0b10000000 | (row & 0x1f));
        lcd_instruction(row < 32 ? 0b10000000 : 0b10001000);
        // Iterate over bytes
        for (int j = 0; j < 16; j++) {
            lcd_data(*p);
            p++;
        }
    }
}

//
// Clear d_buffer to empty
void display_clear() {
    memset(d_buffer, 0, 1024);
}

//
// Set a bit on the d_buffer
// 0 <= x < 128, 0 <= y < 64
void display_set(uint8_t x, uint8_t y) {
    uint8_t *addr = d_buffer + (y * 16) + ((x & 0x78) >> 3);
    *addr = (*addr) | (0x80 >> (x & 7));
}

//
// A version of display_set that checks bounds
void display_set_check(int x, int y) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) {
        return;
    }
    display_set(x, y);
}

//
// Draw a line with Bresenhan's algorithm
// http://en.wikipedia.org/wiki/Bresenham's_line_algorithm
void display_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    bool steep = abs(y1 - y0) > abs(x1 - x0);
#define swap(a, b) {uint8_t c = a; a = b; b = c;}
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }
    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }
#undef swap
    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = deltax >> 1; // deltax/2
    int8_t ystep = (y0 < y1) ? 1 : -1;
    uint8_t y = y0;
    for (uint8_t x = x0; x != x1; x++) {
        if (steep) {
            display_set(y, x);
        } else {
            display_set(x, y);
        }
        error = error - deltay;
        if (error < 0) {
            y += ystep;
            error += deltax;
        }
    }
}

// An implementation of the midpoint circle algorithm
// http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
// 'cx' and 'cy' denote the offset of the circle centre from the origin.
static void _plot4points(int cx, int cy, int x, int y) {
    display_set_check(cx + x, cy + y);
    display_set_check(cx - x, cy + y);
    display_set_check(cx + x, cy - y);
    display_set_check(cx - x, cy - y);
}

static void _plot8points(int cx, int cy, int x, int y) {
    _plot4points(cx, cy, x, y);
    _plot4points(cx, cy, y, x);
}

void display_circle(uint8_t cx, uint8_t cy, uint8_t radius) {
    int error = -radius;
    int x = radius;
    int y = 0;

    while (x > y) {
        _plot8points(cx, cy, x, y);
        error += y;
        ++y;
        error += y;
        if (error >= 0) {
            error -= x;
            --x;
            error -= x;
        }
    }
    _plot4points(cx, cy, x, y);
}

