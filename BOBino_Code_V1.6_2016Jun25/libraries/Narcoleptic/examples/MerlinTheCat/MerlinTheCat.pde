/**
 * Narcoleptic - A sleep library for Arduino
 * Copyright (C) 2010 Peter Knight (Cathedrow)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
** Welcome to Narcoleptic. This library allows you to dramatically reduce
** the power consumption of your Arduino project.
**
** Whilst in a Narcoleptic delay, the CPU current consumption drops from
** around 15mA down to about 7µA.
**
** Note that Narcoleptic only shuts down the CPU. It does not shut down
** anything else consuming current - LEDs, or the USB to serial chip.
** For low current applications, consider using boards without those,
** such as the Sparkfun Arduino Pro with the power LED removed.
**
** Narcoleptic is available from: http://narcoleptic.googlecode.com
*/

/* To use Narcoleptic, you will need the following line. Arduino will
** auto-insert it if you select Sketch > Import Library > Narcoleptic. */

#include <Narcoleptic.h>

void setup() {
  pinMode(2,INPUT);
  digitalWrite(2,HIGH);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  
  // Narcoleptic.disableMillis(); Do not disable millis - we need it for our delay() function.
  Narcoleptic.disableTimer1();
  Narcoleptic.disableTimer2();
  Narcoleptic.disableSerial();
  Narcoleptic.disableADC();
  Narcoleptic.disableWire();
  Narcoleptic.disableSPI();
}

void loop() {
  int a;

  // Merlin the cat is snoozing... Connect digital pin 2 to ground to wake him up.
  Narcoleptic.delay(500); // During this time power consumption is minimised

  while (digitalRead(2) == LOW) {
    // Wake up CPU. Unfortunately, Merlin does not like waking up.

    // Swipe claws left
    digitalWrite(13,HIGH);
    delay(50);
    
    // Swipe claws right
    digitalWrite(13,LOW);
    delay(50);
  }

  // Merlin the cat goes to sleep...
}