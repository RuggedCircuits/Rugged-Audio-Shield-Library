#!/usr/bin/env python
"""Generate a 256-point 16-bit signed full-wave of a sinusoid

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
"""

from math import sin, pi

Fs=22000.0
N=256

fid = open('sinewave_256.h', 'wt')
for i in range(N):
  print >> fid, int(32767*sin(i/float(N)*2*pi)), ','
fid.close()

# vim: expandtab ts=2 sw=2 ai cindent
