/*
 * main.c
 *
 *  Created on: 05/01/2012
 *      Author: Alan
 */

#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>

#include <Arduino.h>

#include "lcd.h"

void setup(void) {
    Serial.begin(57600); //  setup serial
}

void loop(void) {
    static int missed = 0;
    int v = analogRead(4);
    if (v > 10) {
        Serial.print(missed);
        Serial.print(": ");
        Serial.print(v);
        Serial.println();
        missed = 0;
    } else {
        delay(50);
        missed++;
    }
}

int main() {
    init();

    setup();

    for (;;) {
        loop();
        if (serialEventRun)
            serialEventRun();
    }

    return 0;
}
