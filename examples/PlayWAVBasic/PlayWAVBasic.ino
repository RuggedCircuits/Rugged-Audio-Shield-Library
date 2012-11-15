/*
  Rugged Audio Shield sketch that demonstrates WAV file playback. The
  sketch simply plays a WAV file called SOMEFILE.WAV in the root
  directory of the microSD card, delays 1s, then repeats. The SOMEFILE.WAV
  file should have a duration less than 1 second (otherwise, increase
  the delay from 1s to something longer than the WAV file duration).

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
   RAS.InitSD();
   delay(100);
   RAS.OutputEnable();
}

void loop(void) {
  delay(1000);

  RAS.PlayWAV("SOMEFILE.WAV");
}
// vim: expandtab ts=2 sw=2 ai cindent syntax=cpp
