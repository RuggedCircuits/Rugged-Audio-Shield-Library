/*
  Rugged Audio Shield sketch that demonstrates recording audio to the
  microSD card. The sketch waits for a keypress from the serial port
  (at 38400 bps), then records the Line In input in stereo for 5 seconds
  to a file called RECFILE.WAV. The sketch then plays the recorded
  file back endlessly.

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
#include <RAS.h>

RAS RAS;

void setup(void) {
   RAS.begin();
   Serial.begin(38400);

   Serial.println("Press ENTER to start recording");
   while (! Serial.available()) /* NULL */ ;
   Serial.println("Recording...");

   RAS.InitSD(/*SPI_RATE_4MHz*/);
   RAS.WaitForIdle();
   RAS.RecordWAV(32000, SOURCE_MONO, SOURCE_MIC, "RECFILE.WAV");
   delay(5000); // Record for 5s
   RAS.Stop(); RAS.WaitForIdle();
   RAS.OutputEnable(); RAS.WaitForIdle();
   RAS.OutputVolumeSet(27);
}

void loop(void) {
  delay(1000);
  Serial.println("Playing RECFILE.WAV...");
  RAS.PlayWAV("RECFILE.WAV");
  RAS.WaitForIdle();
}
// vim: expandtab ts=2 sw=2 ai cindent syntax=cpp
