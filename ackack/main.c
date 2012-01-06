/*
 * main.c
 *
 *  Created on: 05/01/2012
 *      Author: Alan
 */

#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>

#include "lcd.h"

const char string_1[] PROGMEM = "ADC";

// Reads the given analog port
int16_t read_an(uint8_t port) {
    // Set 5V, MUX to given port
    ADMUX = 0x40 | port;

    // Start a conversion, wait until it is done
    ADCSRA = 0xc7;
    while (ADCSRA & 0x40) {
        // wait
    }

    // Get result
    // ADCL must be read first
    uint8_t low_bits = ADCL;
    return ADCH << 8 | low_bits;
}

static void lcd_print_line(uint8_t line, PGM_P fmt, ...) {
    char buf[17];
    va_list ap;

    lcd_set_cursor(line, 0);
    va_start(ap, fmt);
    int i = vsnprintf_P(buf, 17, fmt, ap);
    va_end(ap);
    for (; i < 16; i++) {
        buf[i] = ' ';
    }
    for (int i = 0; i < 16; i++) {
        lcd_data(buf[i]);
    }
}

void add_dir(uint8_t ch) {
    static uint8_t idx = 16;
    static uint8_t last = 0;
    static uint8_t dirs[16];

    // ignore dupes
    if (ch == last) {
        return;
    }
    last = ch;

    // no action on button up
    if (ch == 0) {
        return;
    }

    // if reached end of line, clear before adding a new char
    if (idx >= 16) {
        memset(dirs, ' ', 16);
        idx = 0;
    }

    // Add new char
    dirs[idx++] = ch;

    // output the collected buffer
    lcd_set_cursor(1, 0);
    for (int i = 0; i < 16; i++) {
        lcd_data(dirs[i]);
    }
}

int main() {
    // Intialize lcd
    spi_init();
    lcd_reset();
    _delay_ms(10);
    // Set PORTC registers to inputs, then disable inputs
    DDRC = 0x00;
    DIDR0 = 0x3f;

    // Enable ADC
    PRR &= ~(1 << PRADC);

    while (true) {
        // Convert AN4 and AN5
        int16_t an4 = read_an(4);
        int16_t an5 = read_an(5);

        // For AN5, briefly set it to output 0 after reading
        PORTC = 0;
        DDRC = 0x20;
        _delay_ms(1);
        DDRC = 0x00;

        // Print their values
        lcd_print_line(0, PSTR("Button: %4d"), an4);

        if (an4 < 100) {
            add_dir(0x1b); // left
        } else if (an4 > 490 && an4 < 520) {
            add_dir(0x19); // down
        } else if (an4 > 670 && an4 < 690) {
            add_dir(0x1a); // right
        } else if (an4 > 755 && an4 < 775) {
            add_dir(0x18); // up
        } else {
            add_dir(0);
        }

        lcd_print_line(2, PSTR("Knock:  %4d"), an5);

        _delay_ms(100);
    }
}
