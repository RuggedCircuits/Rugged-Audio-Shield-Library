/*
  Rugged Audio Shield sketch that demonstrates WAV file playback. The
  sketch simply plays a WAV file called SOMEFILE.WAV in the root
  directory of the microSD card, delays 1s, then repeats.

  Moving beyond the PlayWAVBasic sketch, this sketch introduces more
  advanced usage such as checking for whether or not a microSD card
  is present, waiting for the WAV file to finish playing, and displaying
  an error code (if there was an error in playback).

  Copyright (c) 2012 Rugged Circuits LLC.  All rights reserved.
  http://ruggedcircuits.com

  This file is part of the Rugged Circuits Rugged Audio Shield library 
  for Arduino.

        <http://ruggedcircuits.com/html/rugged_audio_shield.html>

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 3 of the License, or (at your option) any later
  version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  A copy of the GNU General Public License version 3 can be viewed at
  <http://www.gnu.org/licenses/gpl-3.0.html>
*/
#include <SPI.h>
#include <RAS.h> // Rugged Audio Shield library

RAS RAS;

void setup(void) {
   RAS.begin();
   Serial.begin(38400);
   RAS.InitSD();
   delay(100);
   RAS.OutputEnable();
}

void loop(void) {
  delay(1000);

  // Don't bother trying to play a WAV file if we don't even have an SD card!
  RAS.ReadInfo();
  if (RAS.IsSDCardInserted()) {

    // We have an SD card inserted. Try to play a WAV file.
    RAS.PlayWAV("SOMEFILE.WAV");

    // Wait for the file to stop playing by checking the state and waiting for it to be Idle
    do {
      delay(100);
      if (RAS.GetState() == STATE_IDLE) break;
    } while (1);

    // Was playing the file successful? or did we have an error?
    {
      uint16_t error = RAS.GetLastError();

      if (error) {
        Serial.println(RAS.InterpretError(error));
      } else {
        Serial.println("Played SOMEFILE.WAV");
      }
    }
  }
}
// vim: syntax=cpp expandtab ts=2 sw=2 ai cindent
