/*
 * lcd.c
 *
 *  Created on: 05/01/2012
 *      Author: Alan Green
 *
 * LCD Library text functions.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "lcd.h"

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

void lcd_send_str_p(PGM_P p) {
    uint8_t b;
    while (b = pgm_read_byte(p), b) {
        lcd_data(b);
        p++;
    }
}
