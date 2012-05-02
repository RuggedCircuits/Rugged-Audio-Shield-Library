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
#ifndef _RAS_H_
#define _RAS_H_

static const uint8_t RAS_VERSION_MAJOR = 1;
static const uint8_t RAS_VERSION_MINOR = 0;

#if ARDUINO >= 100
#  include "Arduino.h"
#else
#  include "WProgram.h"
#endif

#include <inttypes.h>

typedef enum {
  SOURCE_MONO,
  SOURCE_STEREO
} RAS_Stereo_t;

typedef enum {
  EFFECT_NONE       ='0',
  EFFECT_ECHO       ='1',
  EFFECT_FLANGE     ='2',
} RAS_Effect_t;

typedef enum {
  SOURCE_LINE,
  SOURCE_MIC
} RAS_Source_t;

typedef enum {
  STATE_IDLE             =0,

  STATE_RECORDING_TO_SD  =1,
  STATE_PLAYING_FROM_SD  =2,
  STATE_PLAYING_FROM_SPI =3,
  STATE_RECORDING_TO_SPI =4,
  STATE_PASS_THROUGH     =5,
  STATE_BUSY             =6,

  STATE_NUM_STATES
} RAS_State_t;

typedef enum {
  INPUT_GAIN_1X,
  INPUT_GAIN_2X,
  INPUT_GAIN_4X,
  INPUT_GAIN_8X
} RAS_Input_Gain_t;

typedef enum {
  SPI_RATE_8MHz   = 0,
  SPI_RATE_2MHz   = 1,
  SPI_RATE_500kHz = 2,
  SPI_RATE_250kHz = 3,
  SPI_RATE_16MHz  = 4,
  SPI_RATE_4MHz   = 5,
  SPI_RATE_1MHz   = 6
} RAS_SPI_Rate_t;

// Number of BYTES to send in a PlayStreamBuffer() call
// This MUST match definition of SPI_STREAM_SIZE_BYTES in the shield application
#define RAS_STREAM_BUFFER_SIZE_BYTES 128

class RAS {
public:
  RAS(void) { }

  void begin(uint8_t pin = 8) {
    SPI.begin();

    // Can't really go faster than 1 MHz
    SPI.setClockDivider(SPI_CLOCK_DIV16); 

    // Default gains are 1X on the shield...reflect this in the initial values.
    gainLine = gainMic = INPUT_GAIN_1X;

    // Store the slave-select pin and drive it high to deselect the shield
    sspin = pin;
    digitalWrite(sspin, HIGH);
    pinMode(sspin, OUTPUT);
  }

  //
  //
  // Input source control functions
  //
  //
  void SetInputGains(RAS_Input_Gain_t line, RAS_Input_Gain_t mic);
  void SetInputGainLine(RAS_Input_Gain_t line) { SetInputGains(line, gainMic); }
  void SetInputGainMic(RAS_Input_Gain_t mic) { SetInputGains(gainLine, mic); }
  RAS_Input_Gain_t GetInputGainLine(void) { return gainLine; }
  RAS_Input_Gain_t GetInputGainMic(void) { return gainMic; }

  //
  //
  // Play functions
  //
  //
  void PlayWAV(const char *fname) { _send_filename('P', fname); } // 8.3 format, UPPERCASE ONLY
  void PlayStream(uint16_t Fs, RAS_Stereo_t stereo);
  uint8_t PlayStreamBuffer(int16_t buffer[]); // Returns how many buffers are free

  //
  //
  // Record functions
  //
  //
  void RecordWAV(uint16_t Fs, RAS_Stereo_t stereo, RAS_Source_t source, const char *fname); // fname in 8.3 format, UPPERCASE ONLY

  //
  //
  // Headphone output control functions
  //
  //
  void OutputControl(uint8_t enable) { _spi_send_1byte('H', enable); }
  void OutputDisable(void) { OutputControl(0); }
  void OutputEnable(void) { OutputControl(1); }

  void OutputBassBoostControl(uint8_t enable) { _spi_send_1byte('B', enable); }
  void OutputBassBoostEnable(void) { OutputBassBoostControl(1); }
  void OutputBassBoostDisable(void) { OutputBassBoostControl(0); }

  void OutputGainBoostControl(uint8_t enable) { _spi_send_1byte('G', enable); }
  void OutputGainBoostEnable(void) { OutputGainBoostControl(1); }
  void OutputGainBoostDisable(void) { OutputGainBoostControl(0); }

  void OutputVolumeSet(uint8_t volume) { _spi_send_1byte('V', volume&0x1FU); }

  //
  //
  // Audio Effect Functions
  //
  //
  void AudioEffect(RAS_Effect_t effect, uint16_t Fs, RAS_Stereo_t stereo, RAS_Source_t source);
  void Stop(void) { _send_cmd('Q'); _end_spi(); }

  //
  //
  // Filesystem functions
  //
  //
  void EraseFilesystem(void);
  void InitSD(RAS_SPI_Rate_t rate=SPI_RATE_8MHz) { _spi_send_1byte('F', rate); }
  void PresizeFile(const char *fname, uint16_t megabytes); // fname in 8.3 format, UPPERCASE ONLY!

  //
  //
  // Administrative functions
  //
  //
  void ReadInfo(void);
  uint8_t GetAppVersionMajor(void) { return info.major; }
  uint8_t GetAppVersionMinor(void) { return info.minor; }
  uint8_t GetAppVersionBuild(void) { return info.build; }
  uint8_t GetBootloaderVersionMajor(void) { return info.bootloaderVersion>>8; }
  uint8_t GetBootloaderVersionMinor(void) { return info.bootloaderVersion&0xFFU; }
  uint8_t IsSDCardInserted(void) { return info.sockins; }
  RAS_State_t GetState(void) { return (RAS_State_t) _send_cmd('?'); }
  void TxControl(uint8_t enable) { _spi_send_1byte('T', enable); }
  void TxEnable(void) { TxControl(1); }
  void TxDisable(void) { TxControl(0); }
  uint16_t GetLastError(void);
  const char *InterpretError(uint16_t err);
  void ReplaceApp(const char *fname) { _send_filename('!', fname); } // 8.3 format, UPPERCASE ONLY!
  void WaitForIdle(void);

private:
  uint8_t _spi_get_1byte(uint8_t cmd);
  void _spi_send_1byte(uint8_t cmd, uint8_t param);
  void _spi_get_buf(uint8_t *buf, uint8_t bytes);
  void _spi_send_buf(const uint8_t *buf, uint8_t bytes);
  void _send_filename(uint8_t cmd, const char *fname);
  uint8_t _send_cmd(uint8_t cmd);
  void _end_spi(void);

  struct {
    uint8_t major;
    uint8_t minor;
    uint16_t build;
    uint16_t bootloaderVersion;
    uint8_t sockins; // True if SD card is plugged in
  } info;

  RAS_Input_Gain_t gainLine, gainMic;
  uint8_t sspin;
};

#endif // _RAS_H_
// vim: expandtab ts=2 sw=2 ai cindent
