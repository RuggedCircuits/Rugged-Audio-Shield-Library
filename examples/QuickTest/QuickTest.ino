/*
  Rugged Audio Shield sketch that does a quick automated test of the
  shield:
     - Plays a WAV file from the SD card
     - Perpetually streams LineIn+MicIn to the output, while allowing potentiometer
       to adjust the volume control

  These functions exercise all aspects of the Rugged Audio Shield's core functionality.

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

// Declare the Rugged Audio Shield object
RAS RAS;

// Used for keeping track of OutputVolumeSet updates
int lastVolume;

void setup(void) {
  delay(2000); // Let RAS do its own startup
  
  RAS.begin(); // Default SPI select on D8

  // Adjust these to 1X, 2X, 4X, or 8X depending on your source material
  RAS.SetInputGainLine(INPUT_GAIN_1X); RAS.WaitForIdle();
  RAS.SetInputGainMic(INPUT_GAIN_1X); RAS.WaitForIdle();

  // First, play a file from an SD card that is assumed to be inserted and contains
  // the file SOMEFILE.WAV
  RAS.InitSD(); RAS.WaitForIdle();

  RAS.OutputEnable(); RAS.WaitForIdle();
  RAS.OutputVolumeSet(20); RAS.WaitForIdle();
  RAS.PlayWAV("SOMEFILE.WAV"); RAS.WaitForIdle();
  
  // Start pass-through of mic+line (left channel) to output at 32 kHz
  RAS.AudioEffect(EFFECT_NONE, 32000, SOURCE_STEREO, SOURCE_MIC);
}

void loop(void) {
  int reading;

  // Update the volume every 300ms (not to overwhelm the SPI bus), and
  // don't bother sending a volume update unless the potentiometer reading
  // changed by more than 16 counts.
  delay(300);
  reading = analogRead(3); // Read shield potentiometer
  if (abs(reading - lastVolume) > 16) {
    lastVolume = reading;

    // Potentiometer reads 0V to 3.3V, which is 0 to about 700
    // when using the default 5V analog reference. So we map the range
    // 0<-->700 to the OutputVolumeSet() range of 0 to 31.
    RAS.OutputVolumeSet(map(reading, 0, 700, 0, 31));
  }
}
// vim: expandtab ts=2 sw=2 ai cindent syntax=cpp
