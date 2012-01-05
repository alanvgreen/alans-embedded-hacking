/*
 * lcd.h
 *
 *  Created on: 05/01/2012
 *      Author: Alan Green
 *
 * Library for drivin a serial ST9720 controlled LCD.
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


#ifndef LCD_H_
#define LCD_H_

#include <avr/pgmspace.h>

// Utility function to initialze SPI in a mode suitable for the LCD d_buffer
void spi_init(void);

// Utility function to send a byte via SPI, without receiving
void spi_send(uint8_t b);

// Reset, as per page 34 of 7920 data sheet
void lcd_reset();

// Clears the text screen
void lcd_clear();

// Sends the given instruction in ST7920 format, via SPI
void lcd_instruction(uint8_t ins);

// Sends the given data in ST7920 format, via SPI
void lcd_data(uint8_t data);

// Sets the text cursor to the given line and column
void lcd_set_cursor(uint8_t line, uint8_t col);

// Utility function to send a string constant
void lcd_send_str_p(PGM_P p);

// Graphic buffer display RAM
// Layout is 64 Rows of 16 Bytes
extern uint8_t d_buffer[1024];

//
// Call this to paint the d_buffer RAM onto the display
// Display will then be in graphics mode. Call lcd_reset() to go back to text mode
void display_refresh();

// Clear d_buffer RAM to empty
void display_clear();

// Set a bit on the d_buffer
// 0 <= x < 128, 0 <= y < 64
void display_set(uint8_t x, uint8_t y);

// A version of display_set that checks bounds
void display_set_check(int x, int y);

// Draw a line with Bresenhan's algorithm
// http://en.wikipedia.org/wiki/Bresenham's_line_algorithm
void display_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

// An implementation of the midpoint circle algorithm
// http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
// 'cx' and 'cy' denote the offset of the circle centre from the origin.
void display_circle(uint8_t cx, uint8_t cy, uint8_t radius);

#endif /* LCD_H_ */
