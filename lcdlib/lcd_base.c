/*
 * lcd.c
 *
 *  Created on: 05/01/2012
 *      Author: Alan Green
 *
 * Base LCD Library functions
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "lcd.h"

void spi_init(void) {
    // SPI enable, master mode, clock idle high, sample data on trailing edge
    // Clock Frequency = Fosc / 2 = 8MHz
    SPCR = 0b01011100;
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
    _delay_us(40);
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

    lcd_instruction(0b00001100); // d_buffer on
    lcd_clear();
    lcd_instruction(0b00000110); // increment, no shift
}
