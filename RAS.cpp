/*
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
#include <string.h>
#include "RAS.h"

// The Rugged Audio Shield needs time to recognize the command (first byte of
// each packet) and decide what to do with it. Given everything that (may be)
// going on, we should leave plenty of time for this first byte to be
// received, interpreted, and acted upon. 100 microseconds is generous.
// If you are trying to stream data to the shield, experiment with lowering
// this number to increase throughput (if you need higher throughput!)
#define SPI_CMD_TO_DATA_DELAY 100 // microseconds

#if 0
static void _dumpbuf(const uint8_t *buf, uint16_t size)
{
  while (size--) {
    Serial.print(*buf++, HEX); Serial.print(' ');
  }
}
#endif

static void _cmd_delay(void)
{
  delayMicroseconds(SPI_CMD_TO_DATA_DELAY);
}

uint8_t RAS::_send_cmd(uint8_t cmd)
{
  uint8_t retval;

  digitalWrite(sspin, LOW);

  retval = SPI.transfer(cmd);
  _cmd_delay();

  return retval;
}

void RAS::_end_spi(void)
{
  digitalWrite(sspin, HIGH);
}

void RAS::_spi_send_1byte(uint8_t cmd, uint8_t param)
{
  _send_cmd(cmd);
  SPI.transfer(param);
  _end_spi();
}

void RAS::_spi_get_buf(uint8_t *buf, uint8_t bytes)
{
  while (bytes--) {
    *buf++ = SPI.transfer(0);
  }
}

void RAS::_spi_send_buf(const uint8_t *buf, uint8_t bytes)
{
  while (bytes--) {
    (void) SPI.transfer(*buf++);
  }
}

void RAS::ReadInfo(void)
{
  _send_cmd('Z');
  _spi_get_buf((uint8_t *)&info, sizeof(info));
  _end_spi();
}

void RAS::AudioEffect(RAS_Effect_t effect, uint16_t Fs, RAS_Stereo_t stereo, RAS_Source_t source)
{
  uint8_t buf[4];
  *(uint16_t *)buf = Fs;
  buf[2] = stereo;
  buf[3] = source;

  _send_cmd(effect);
  _spi_send_buf(buf, sizeof(buf));
  _end_spi();
}

uint8_t RAS::_spi_get_1byte(uint8_t cmd)
{
  uint8_t val;

  _send_cmd(cmd);
  _spi_get_buf(&val, 1);
  _end_spi();
  return val;
}

void RAS::SetInputGains(RAS_Input_Gain_t line, RAS_Input_Gain_t mic)
{
  uint8_t buf[2];
  buf[0] = line;
  buf[1] = mic;

  _send_cmd('A');
  _spi_send_buf(buf, sizeof(buf));
  _end_spi();

  gainLine = line;
  gainMic = mic;
}

uint16_t RAS::GetLastError(void)
{
  uint8_t buf[2];

  _send_cmd('E');
  _spi_get_buf(buf, 2);
  _end_spi();
  return *(uint16_t *)buf;
}

void RAS::_send_filename(uint8_t cmd, const char *str)
{
  uint8_t buf[13];

  memset(buf, 0, sizeof(buf));
  strncpy((char *)buf, str, 12);
  _send_cmd(cmd);
  _spi_send_buf(buf, sizeof(buf));
  _end_spi();
}

void RAS::PlayStream(uint16_t Fs, RAS_Stereo_t stereo)
{
  uint8_t buf[3];
  *(uint16_t *)buf = Fs;
  buf[2] = stereo;
  _send_cmd('C');
  _spi_send_buf(buf, sizeof(buf));
  _end_spi();
}

uint8_t RAS::PlayStreamBuffer(int16_t buffer[])
{
  uint8_t freebuf;

  _send_cmd('D');
  freebuf = SPI.transfer(*(uint8_t *)buffer);
  _spi_send_buf(((uint8_t *)buffer)+1, RAS_STREAM_BUFFER_SIZE_BYTES-1);
  _end_spi();

  return freebuf;
}

void RAS::RecordWAV(uint16_t Fs, RAS_Stereo_t stereo, RAS_Source_t source, const char *str)
{
  uint8_t buf[17];

  memset(buf, 0, sizeof(buf));
  *(uint16_t *)buf = Fs;
  buf[2] = stereo;
  buf[3] = source;
  strncpy((char *)(buf+4), str, 12);
  _send_cmd('R');
  _spi_send_buf(buf, sizeof(buf));
  _end_spi();
}

const char *RAS::InterpretError(uint16_t err)
{
  uint8_t major, minor;
  static char buf[80];
  static const char *majorStr[] = {
    "Unknown",
    "WAV file open",
    "WAV file read",
    "WAV file create",
    "WAV file finalize",
    "WAV file record",
    "Filesystem",
    "MKFS",
    "Presize",
  };
#define NUM_MAJOR_STR (sizeof(majorStr)/sizeof(majorStr[0]))
  static const char *minorStr[] = {
    "No error",
    "No such file",
    "Bad header",
    "Not a WAV file",
    "No WAVE chunk",
    "No format information",
    "Bad format information",
    "Not a PCM-coded file",
    "No audio data",
    "Bad audio data",
    "Seek error",
    "Cannot write buffer",
    "Cannot initialize disk",
    "Cannot mount disk",
    "Successful mount",
    "Cannot make filesystem",
    "Bad key code",
    "Cannot truncate file",
  };
#define NUM_MINOR_STR (sizeof(minorStr)/sizeof(minorStr[0]))

  minor = (err >> 8);
  major = (err & 0xFFU);
  if ((major >= NUM_MAJOR_STR) || (minor >= NUM_MINOR_STR)) {
    sprintf(buf, "Bad error code %u.%u", major, minor);
    return buf;
  }

  sprintf(buf, "%s: %s", majorStr[major], minorStr[minor]);
  return buf;
}

void RAS::EraseFilesystem(void)
{
  uint8_t buf[4];

  *(uint16_t *)buf = 0x25E8;
  *(uint16_t *)(buf+2) = 0x9D3C;
  _send_cmd('@');
  _spi_send_buf(buf, sizeof(buf));
  _end_spi();
}

void RAS::PresizeFile(const char *fname, uint16_t megabytes)
{
  uint8_t buf[15];

  memset(buf, 0, sizeof(buf));
  *(uint16_t *)buf = megabytes;
  strncpy((char *)(buf+2), fname, 12);
  _send_cmd('S');
  _spi_send_buf(buf, sizeof(buf));
  _end_spi();
}

void RAS::WaitForIdle(void)
{
  RAS_State_t state;

  do {
    state = GetState();
  } while (state);
}
// vim: expandtab ts=2 sw=2 ai cindent
