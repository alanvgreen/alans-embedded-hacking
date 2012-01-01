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

// Clears the screen
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

static uint8_t display[1024];

void lcd_refresh() {
    // Initialize graphics mode
    lcd_instruction(0b00110100); // 8bit data, extended instructions
    lcd_instruction(0b00110110); // +graphics

//    lcd_instruction(0b10000000); // to start of ram
//    lcd_instruction(0b10000000); // to start of ram

    uint8_t *p = display;
    for (int row = 0; row < 64; row++) {
        lcd_instruction(0b10000000 | (row & 0x1f)); // to start of row
        lcd_instruction(row < 32 ? 0b10000000 : 0b10001000);
        // Iterate over row
        for (int j = 0 ; j < 16; j++) {
            lcd_data(*p);
            p++;
        }
    }

//    lcd_instruction(0b00110100); // -graphics
//    lcd_instruction(0b00100100); // 8bit data, basic instructions
}

void display_clear() {
    memset(display, 0, 1024);
}

int main(int argc, char **argv) {

    spi_init();
    _delay_ms(10);
    lcd_reset();

    while (1) {
        display_clear();
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 16; j++) {
                display[i * 16 + j] = (i & 4) ? 0xf0 : 0x0f;
            }
        }
        lcd_refresh();

        _delay_ms(100);
//        lcd_instruction(0x80);
//        for (uint8_t c = 0; c < 64; c++) {
//            lcd_data('1' + (c >> 4));
//        }
//
//        _delay_ms(1000);
//        lcd_clear();
//
//        lcd_instruction(0x80);
//        for (uint8_t c = 0; c < 64; c++) {
//            lcd_data('A' + (c >> 4));
//        }
//        _delay_ms(1000);
//        lcd_clear();
    }
}
