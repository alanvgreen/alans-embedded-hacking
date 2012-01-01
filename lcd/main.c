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

void spi_init(void) {
    // SPI enable, master mode, clock idle high, sample data on trailing edge
    // Clock Frequency = Fosc / 8 = 2MHz
    SPCR = 0b01011101;
    SPSR = 0b00000001;

    // Set SS, MISO and PB0 as outputs
    DDRB = 0xff;
    PORTB = 0x1; // set PB0 - nonreset
}

void spi_send(uint8_t b) {
    SPDR = b;
    while (!(SPSR & 0x80)) {
        // busy wait
    }
}

void lcd_instruction(uint8_t ins) {
    spi_send(0b11111000); // 5 1 bits, RS = 0, RW = 0
    spi_send(ins & 0xf0);
    spi_send(ins << 4);
    _delay_us(72);
}

void lcd_data(uint8_t data) {
    spi_send(0b11111010); // 5 1 bits, RS = 1, RW = 0
    spi_send(data & 0xf0);
    spi_send(data << 4);
    _delay_us(72);
}

void lcd_set_cursor(uint8_t line, uint8_t col) {
    // Contrary to the data sheet, the starting addresses for lines 0, 1, 2 and 3 are 80, 90, 88 and 98
    uint8_t d = 0x80 + col;
    if (line & 1) {
        d |= 0x10;
    }
    if (line & 2) {
        d |= 0x8;
    }
    lcd_instruction(d);
}

// Clears the text screen
void lcd_clear() {
    lcd_instruction(0b00000001); // clear
    _delay_ms(2); // Needs 1.62ms delay
}

// Reset, as per page 34 of 7920 data sheet
void lcd_reset() {
    // Bring (PB0) low, then high
    PORTB &= ~0x01;
    _delay_ms(1);
    PORTB |= 0x01;
    _delay_ms(10);

    lcd_instruction(0b00110000); // 8 bit data, basic instructions
    lcd_instruction(0b00110000); // 8 bit data, basic instructions

    lcd_instruction(0b00001100); // display on
    lcd_clear();
    lcd_instruction(0b00000110); // increment, no shift
}

static char string_1[] PROGMEM = "Hello, World!";

void lcd_send_str_p(PGM_P p) {
    uint8_t b;
    while (b = pgm_read_byte(p), b) {
        lcd_data(b);
        p++;
    }
}

//
// Half the 328P's RAM
// Lay out is 64 Rows of 16 Bytes
static uint8_t display[1024];

//
// Call this to paint the display ram onto the display
// Display will then be in graphics mode. Call lcd_reset() to go back to text mode
void display_refresh() {
    // Initialize graphics mode
    lcd_instruction(0b00110100); // 8bit data, extended instructions
    lcd_instruction(0b00110110); // +graphics

    uint8_t *p = display;
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
// Clear display to empty
void display_clear() {
    memset(display, 0, 1024);
}

//
// Set a bit on the display
// 0 <= x < 64, 0 <= y < 128
void display_set(uint8_t x, uint8_t y) {
    uint8_t *addr = display + (y * 16) + ((x & 0x78) >> 3);
    *addr = (*addr) | (0x80 >> (x & 7));
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

// Checker board made by direct manipulation of display ram
void demo_checker_board() {
    // Checker board
    display_clear();
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 16; j++) {
            display[i * 16 + j] = (i & 4) ? 0xf0 : 0x0f;
        }
    }

    display_refresh();
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
}

// Draw lines with line drawing algorithm
void demo_lines() {
    int const num = 8;
    int x0s[num], y0s[num], x1s[num], y1s[num];
    int dx0 = -2, dx1 = 3, dy0 = 3, dy1 = 2;

    // init
    for (int t = 0; t < num ; t++) {
        x0s[t] = 33;
        x1s[t] = 58;
        y0s[t] = 0;
        y1s[t] = 1;
    }

    for (int t = 0; t < 500; t++) {
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

int main(int argc, char **argv) {
    spi_init();
    _delay_ms(10);
    lcd_reset();

    while (1) {
        demo_lines();

        demo_pixel_set();
        _delay_ms(1000);

        demo_checker_board();
        _delay_ms(1000);
    }
}

