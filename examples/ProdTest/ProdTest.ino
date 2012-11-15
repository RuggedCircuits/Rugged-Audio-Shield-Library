/*
  Production test sketch for the Rugged Audio Shield. This sketch presents
  a keystroke menu that exercises the Rugged Audio Shield functionality. It
  also demonstrates just about all of the Rugged Audio Shield Library methods.

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
#include <math.h>
#include <SPI.h>

#include "RAS.h"

RAS RAS;

static void drawMenu(void)
{
  Serial.print(
      F("H/h : Headphone output on/off\n"
      "B/b : Headphone bass boost on/off\n"
      "G/g : Headphone gain boost on/off\n"
      "L/l : Line In gain up/down\n"
      "M/m : Mic In gain up/down\n"
      "F   : Initialize SD card\n"
      "P   : Play WAV file\n"
      "R   : Record WAV file\n"
      "S   : Presize WAV file\n"
      "s   : Streaming test\n"
      "A   : In->Out with audio effect\n"
      "Q   : Stop current function\n"
      "I   : Get program information\n"
      "!   : Install new application on shield\n"
      "E   : Report on any error condition\n"
      "@   : Erase filesystem on SD card\n"
      "?   : Redraw this menu\n")
      );
}

static unsigned long lastMillis;

void setup(void)
{
  RAS.begin();
  Serial.begin(38400);

  drawMenu();

  lastMillis = millis();
}

static void printInfo(void)
{
  RAS.ReadInfo();
  Serial.print(F("Rugged Audio Shield Application Version "));
  Serial.print(RAS.GetAppVersionMajor()); Serial.print('.');
  Serial.print(RAS.GetAppVersionMinor()); Serial.print('.');
  Serial.print(RAS.GetAppVersionBuild()); Serial.print('\n');
  Serial.print(F("Bootloader Version "));
  Serial.print(RAS.GetBootloaderVersionMajor()); Serial.print('.');
  Serial.print(RAS.GetBootloaderVersionMinor()); Serial.print('\n');
  Serial.print(F("SD card is ")); Serial.print(RAS.IsSDCardInserted() ? F("present\n") : F("not present\n"));
  Serial.print(F("Current state: "));
  switch (RAS.GetState()) {
    case STATE_IDLE: Serial.print(F("Idle\n")); break;
    case STATE_RECORDING_TO_SD: Serial.print(F("Recording to SD card\n")); break;
    case STATE_PLAYING_FROM_SD: Serial.print(F("Playing from SD card\n")); break;
    case STATE_PLAYING_FROM_SPI: Serial.print(F("Streaming from Arduino\n")); break;
    case STATE_RECORDING_TO_SPI: Serial.print(F("Streaming to Arduino\n")); break;
    case STATE_PASS_THROUGH: Serial.print(F("In/Out audio effects\n")); break;
    default: Serial.print(F("unknown")); break;
  }
}

static void printError(void)
{
  uint16_t err;

  err = RAS.GetLastError();
  Serial.print(F("\n**********\n"));
  Serial.print(RAS.InterpretError(err));
  Serial.print(F("\n**********\n"));
}

static void flushInput(void)
{
  unsigned long timeout;

  timeout = millis()+100;
  do {
    if (Serial.available()) {
      (void) Serial.read();
      timeout = millis() + 100;
    }
  } while ( (long)(timeout - millis()) > 0 );
}

static char staticbuf[RAS_STREAM_BUFFER_SIZE_BYTES+1];
static const char *getLine(void)
{
  uint8_t ix;

  flushInput();
  for (ix=0; ix < sizeof(staticbuf)-1; ix++) {
    while (! Serial.available()) /* NULL */ ;
    staticbuf[ix] = Serial.read();
    switch (staticbuf[ix]) {
      case '\n': case '\r':
        staticbuf[ix]=0;
        return staticbuf;
      case 0x08:
        if (ix >= 1) {
          ix -= 2;
          Serial.write(0x08);
        }
        break;
      default:
        break;
    }
  }
  staticbuf[ix] = 0;
  return staticbuf;
}

static void playWAV(void)
{
  const char * buf;

  Serial.print(F("Type WAV file name to play: "));
  buf = getLine();
  RAS.PlayWAV(buf);
  Serial.print('\n');
}

static void recordWAV(void)
{
  RAS_Stereo_t stereo;
  RAS_Source_t source;
  const char * buf;

  Serial.print(F("Mono (0) or stereo (1)? "));
  stereo = (RAS_Stereo_t)(atoi(getLine()) ? 1 : 0);

  Serial.print(F("\nWhich source (0:Line 1:Mic): "));
  source = (RAS_Source_t) atoi(getLine());

  Serial.print(F("\nType WAV file name to record to (8.3): "));
  buf = getLine();
  Serial.print(F("\n<")); Serial.print(buf); Serial.print(F(">\n"));
  Serial.print(F("Stereo: ")); Serial.print(stereo);
  Serial.print(F(" Source: ")); Serial.println(source);
  RAS.RecordWAV(30000, stereo, source, buf);
}

static void sd_init(void)
{
  Serial.print(F("Type SPI prescale (0: 8MB/s, 1: 2MB/s, 6: 1MB/s)"));
  RAS.InitSD((RAS_SPI_Rate_t)atoi(getLine()));
  RAS.WaitForIdle();
}

static void presizeWAV(void)
{
  char fname[80];

  Serial.print(F("Type WAV file name to presize: "));
  strncpy(fname, getLine(), sizeof(fname));
  Serial.print(F("\nEnter the size of the file in megabytes: "));
  RAS.PresizeFile(fname, atoi(getLine()));
}

static void doEffect(void)
{
  const char * buf;
  RAS_Effect_t effect;
  RAS_Source_t source;

  Serial.print(F("Which effect (0-2): "));
  effect = (RAS_Effect_t)(getLine()[0]);
  Serial.print(F("\nWhich source (0:Line 1:Mic): "));
  source = (RAS_Source_t) atoi(getLine());

  RAS.AudioEffect(effect, 30000, SOURCE_STEREO, source);
  Serial.print('\n');
}

static void lineGainAdj(int8_t adj)
{
  int8_t gain = RAS.GetInputGainLine();

  gain += adj;
  if (gain < 0) gain = 3;
  else if (gain > 3) gain = 0;
  RAS.SetInputGainLine((RAS_Input_Gain_t) gain);
  Serial.print(F("Line Input Gain: ")); Serial.print(1<<gain); Serial.print(F("X\n"));
}

static void micGainAdj(int8_t adj)
{
  int8_t gain = RAS.GetInputGainMic();

  gain += adj;
  if (gain < 0) gain = 3;
  else if (gain > 3) gain = 0;
  RAS.SetInputGainMic((RAS_Input_Gain_t) gain);
  Serial.print(F("Mic Input Gain: ")); Serial.print(1<<gain); Serial.print(F("X\n"));
}

static void streamingTest(void) 
{
  double phase;
  const double freq = 500.0;
  const double Fs = 16000.0;

  RAS.PlayStream((unsigned)Fs, SOURCE_MONO);
  flushInput();

  phase = 0.0;
  
  while (! Serial.available()) {
    int8_t freebuffers;
    uint8_t sample;
    int16_t *samplebuf = (int16_t *)staticbuf;
    int16_t val;

    for (sample=0; sample < (RAS_STREAM_BUFFER_SIZE_BYTES/2); sample++) {
      *samplebuf++ = 16000*sin(phase);
      phase += 2*M_PI/Fs*freq;
      if (phase > 2*M_PI) phase -= 2*M_PI;
    }

    freebuffers = RAS.PlayStreamBuffer((int16_t *)staticbuf);

    // At 24kHz sampling rate it takes 2.6ms to play 64 samples (128 bytes) of audio. If we delay
    // at least this long we'll be sure to have some more free buffers
    if (freebuffers < 2) {
      delay(4);
    }
  }
  RAS.Stop();
}


void loop(void)
{
  // Last analog reading from the volume control pot, shifted right by 5 to occupy 5 LSB's.
  // When this changes, we update the volume on the Rugged Audio Shield.
  static uint16_t gLastVolume;
  uint16_t reading;

  if ((long)(millis() - lastMillis) >= 100) {
    lastMillis = millis();

    reading = analogRead(3);
    if (abs((int16_t)reading - (int16_t)gLastVolume) >= 16) {
      gLastVolume = reading;
        // Pot is referenced to 3.3V, or an analog reading of 675 out of 1023. We map to [0,700]
        // for a little extra headroom.
      RAS.OutputVolumeSet(map(reading, 0, 700, 0, 0x1FU));
    }
  }

  if (! Serial.available()) return;

  switch (Serial.read()) {
    case 'I': printInfo(); break;
    case 'F': sd_init(); break;
    case 'E': printError(); break;
    case 'b': RAS.OutputBassBoostDisable(); break;
    case 'B': RAS.OutputBassBoostEnable(); break;
    case 'g': RAS.OutputGainBoostDisable(); break;
    case 'G': RAS.OutputGainBoostEnable(); break;
    case 'h': RAS.OutputDisable(); Serial.print(F("\nHeadphones disabled\n")); break;
    case 'H': RAS.OutputEnable(); Serial.print(F("\nHeadphones enabled\n")); break;
    case 'P': playWAV(); break;
    case 'R': recordWAV(); break;
    case 'S': presizeWAV(); break;
    case 'A': doEffect(); break;
    case 'Q': RAS.Stop(); break;
    case 'l': lineGainAdj(-1); break;
    case 'L': lineGainAdj(+1); break;
    case 'm': micGainAdj(-1); break;
    case 'M': micGainAdj(+1); break;
    case 's': streamingTest(); break;
    case '?': drawMenu(); break;
    case '@': RAS.EraseFilesystem(); Serial.print(F("\nFilesystem erased\n")); break;

    default: break;
  }
}
// vim: expandtab ts=2 sw=2 ai cindent syntax=cpp
