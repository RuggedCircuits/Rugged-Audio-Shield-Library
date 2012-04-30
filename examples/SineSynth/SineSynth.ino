/*
  Demonstrate the Rugged Audio Shield PlayStream() function by synthesizing a
  variable-frequency sinusoid on the fly.

  The Rugged Audio Shield potentiometer adjusts the sinusoid's frequency.

  The approach is to use a pre-computed 256-point sinewave table and read
  samples from the table at a rate that depends upon the chosen frequency.

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

// Set the target sampling frequency for synthesis. Don't go much
// above 30 kHz else the SPI bus from the Arduino to the shield
// will be overwhelmed.
static const uint16_t Fs = 30000;

// Set the bounds of frequencies you want to synthesize. The range
// of the potentiometer will select between these frequencies
static const uint16_t Fmin = 110;  // Hertz
static const uint16_t Fmax = 880;  // Hertz

////////// End of configuration data /////////////////////////////




// Declare the Rugged Audio Shield object
RAS RAS;

/* This is the predefined 16-bit sinewave table, with 256 entries.
   See the gensine.py Python program that was used to create this
   table.
*/
const uint16_t sintable[256] = {
#include "sinewave_256.h"
};

/* This is a "8.8 pointer" into the sinewave table. The sinewave
   table has 256 entries, hence requires an 8-bit pointer. To maintain
   precision in generating higher frequencies, we actually maintain
   a 16-bit pointer in which the upper 8 bits are an index into the
   sinewave table, and the lower 8 bits are used as decimal places.

   For example, if the step size through the table is 0x0180, this
   represents an "8.8" number of 1.5: the upper 8 bits 0x01 represents
   an "integer" step size of 1, and the lower 8 bits 0x80 are half-way
   between 0 and 255, hence represent a "decimal" step size of 0.5.
   
   Here is how our 8.8 pointer will step through the table when we add 
   0x0180 for every sample:

      Sample  |   8.8 Pointer    |          Sinewave Element
      Number  |(phaseAccumulator)| (index is upper 8 bits of 8.8 pointer)
   -----------+------------------+---------------------------------------
         0    |      0x0000      |           sintable[0]
         1    |      0x0180      |           sintable[1]
         2    |      0x0300      |           sintable[3]
         3    |      0x0480      |           sintable[4]
         4    |      0x0600      |           sintable[6]
         5    |      0x0780      |           sintable[7]
   etc.
*/
uint16_t phaseAccumulator;

// This is the 8.8 step size through the table, updated on-the-fly in
// response to the potentiometer.
uint16_t phaseIncrement;

// This is storage space for the audio samples to send to the shield
int16_t samples[RAS_STREAM_BUFFER_SIZE_BYTES/2];

/* We pre-compute phase increments here since they take a "long" time and
   can slow down sample generation. Every potentiometer reading (in the
   range 0 to about 700) is divided by 3 to get it to fit into an 8-bit
   number. Thus, this table is 700/3=233 elements in size.
*/
uint16_t incrementTable[700/3];
static const uint8_t incrementTableSize = sizeof(incrementTable)/sizeof(incrementTable[0]);

// This function computes the phase increment given a desired target
// frequency in Hertz.
uint16_t computeIncrement(uint16_t freq)
{
  /* We compute phase increment as follows:

            cycles    1  seconds       samples
       freq ------ * --- ------- * 256 -------
            second   Fs  sample         cycle

     We want this to be multiplied by 256 so that we get an "8.8"
     number with 8 integer digits and 8 decimal digits. Therefore:

        phaseIncrement = freq/Fs*256*256
                       = (freq*65536)/Fs

     To round to the nearest 16-bit integer:

        phaseIncrement = (freq*65536 + Fs/2) / Fs

     Hopefully the compiler will be smart enough to optimize *65536UL to
     a left-shift by 16 bits.
  */
  return (uint16_t) ((freq * 65536UL + Fs/2) / Fs);
}

void setup(void) {
  RAS.begin(); // Default SPI select on D8
  Serial.begin(38400);

  /* Pre-compute increment table. The first element of the increment
     table will represent a frequency of Fmin, and the last a frequency
     of Fmax. */
  for (uint8_t i=0; i < incrementTableSize; i++) {
    incrementTable[i] = computeIncrement(map(i, 0, incrementTableSize-1, Fmin, Fmax));
  }

  // Enable the headphone output. Add a 10ms delay (probably excessive) to allow the
  // shield time to respond.
  RAS.OutputEnable(); delay(10);

  // Set nearly maximum output volume (without gain boost)
  RAS.OutputVolumeSet(30); delay(10);

  // Begin audio streaming
  RAS.PlayStream(Fs, SOURCE_MONO); delay(10);

  phaseAccumulator = 0;
}

void loop(void) {
  /* Read the Rugged Audio Shield potentiometer at A3 and map to the
     desired frequency. Remember that the potentiometer returns voltages
     in the range 0V to 3.3V, so referenced to a 5V analog reference this
     will return numbers in the range 0 to about 675. We play it safe
     and expect numbers from 0 to 700. Then, we divide the reading by 3
     to map it into the increment table that was pre-computed. */
  phaseIncrement = incrementTable[analogRead(3)/3];

  /* Compute all the audio samples by indexing through the sinewave table
     using our phase accumulator and phase increment */
  for (uint8_t i=0; i < sizeof(samples)/sizeof(samples[0]); i++) {
    samples[i] = sintable[(uint8_t)(phaseAccumulator >> 8)];
    phaseAccumulator += phaseIncrement;
  }

  /* Stream out the audio samples. The return value of PlayStreamBuffer()
     represents how many shield buffers (out of 16) are available. If this gets
     too low, we're writing too fast so insert a delay. Each buffer represents
     RAS_STREAM_BUFFER_SIZE_BYTES/2 samples at Fs samples per second, so takes
     RAS_STREAM_BUFFER_SIZE_BYTES/(2*Fs) seconds to play.  We want to delay for
     about 6 buffers to let the shield become somewhat free, so the total
     desired delay is 3*RAS_STREAM_BUFFER_SIZE_BYTES/Fs.  Multiply by 1000 to
     get the delay in milliseconds.

     If the delay is much longer than ~6 buffers we can actually underflow the
     shield because of the time it takes to do an analog reading, fill in the
     samples of the next buffer and transmit them over SPI. */
  if (RAS.PlayStreamBuffer(samples) < 2) {
    delay(3000UL*RAS_STREAM_BUFFER_SIZE_BYTES/Fs);
  }
}
// vim: expandtab ts=2 sw=2 ai cindent syntax=cpp
