/*
 * Drive an ST7920 LCD module via the serial interface.
 *
 * Data sheets:
 * - 12864ZB module http://read.pudn.com/downloads135/doc/573419/7920.pdf (in
 *   Chinese, but Google Translate is helpful)
 * - st7920 LCD controller http://www.crystalfontz.com/controllers/ST7920.pdf
 * - 328p MCU http://www.atmel.com/dyn/resources/prod_documents/doc8271.pdf
 *
 * Configuration
 * LCD | 328/Arduino
 * ------------------------
 * GND | GND
 * VCC | 5V
 * E   | SCLK == PB5 == D13
 * RW  | MOSI == PB3 == D11
 * RST | PB0 == D8
 * RS  | 5V
 * A   | 390R to 5V
 * K   | GND
 *
 * Also bridged JP1 and JP2 to use onboard pot for contrast control.
 *
 * The trickiest part of getting this right was the control of the reset pin.
 * I found it quite finicky, and needing to be cycled just-so.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "lcd.h"

static char string_1[] PROGMEM = "Hello, World!";


// Checker board made by direct manipulation of display ram
void demo_checker_board() {
    // Checker board
    display_clear();
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 16; j++) {
            d_buffer[i * 16 + j] = (i & 4) ? 0xf0 : 0x0f;
        }
    }

    display_refresh();
    _delay_ms(1000);
}

// diagonal line down and up
void demo_pixel_set() {
    // Lines with line algorithm
    // diagonal line down and up
    display_clear();
    for (int x = 0; x < 128; x++) {
        if (x < 64) {
            display_set(x, x);
        } else {
            display_set(x, 127 - x);
        }
    }

    display_refresh();
    _delay_ms(1000);
}

// Demo lines to make a moving Qix thing
void demo_lines() {
    int const num = 8;
    int x0s[num], y0s[num], x1s[num], y1s[num];
    int dx0 = -2, dx1 = 3, dy0 = 3, dy1 = 2;

    // init
    for (int t = 0; t < num; t++) {
        x0s[t] = 33;
        x1s[t] = 58;
        y0s[t] = 0;
        y1s[t] = 1;
    }

    // Move the lines aound
    for (int t = 0; t < 250; t++) {
        display_clear();
        // draw each line
        for (uint8_t j = 0; j < num; j++) {
            display_line(x0s[j], y0s[j], x1s[j], y1s[j]);
        }

        // move them down
        for (uint8_t j = num - 1; j >= 1; j--) {
            x0s[j] = x0s[j - 1];
            x1s[j] = x1s[j - 1];
            y0s[j] = y0s[j - 1];
            y1s[j] = y1s[j - 1];
        }

        // calculate next
        x0s[0] += dx0;
        x1s[0] += dx1;
        y0s[0] += dy0;
        y1s[0] += dy1;
#define limit(v, dv, max_v) {\
        if (v < 0) { v = 0; dv = (rand() & 3) + 2; } \
        if (v >= max_v) { v = max_v - 1; dv = -(rand() & 3) - 2; } \
}
        limit(x0s[0], dx0, 128);
        limit(x1s[0], dx1, 128);
        limit(y0s[0], dy0, 64);
        limit(y1s[0], dy1, 64);
#undef limit
        display_refresh();
    }
}

void demo_circles() {
    for (int i = 0; i < 100; i += 2) {
        display_clear();
        for (int j = 0; j < 6; j++) {
            display_circle(64 + (i - 50) / 6 - j * (i - 50) / 10, 32 + j,
                    j * 12 + 5);
        }
        display_refresh();
    }
}

int main(int argc, char **argv) {
    spi_init();
    _delay_ms(20);
    lcd_reset();

    while (1) {
        lcd_reset();
        _delay_ms(10);
        lcd_set_cursor(1, 1);
        lcd_send_str_p(string_1);
        _delay_ms(3000);
        lcd_clear();

        demo_circles();
        demo_lines();
        demo_pixel_set();
        demo_checker_board();
    }
}

