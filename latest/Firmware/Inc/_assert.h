/*
  Copyright (c) 2016-2020 Peter Antypas

  This file is part of the MAIANA™ transponder firmware.

  The firmware is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#ifndef ASSERT_H_
#define ASSERT_H_



#ifdef DEBUG
#define ASSERT(b) if (!(b)) {asm("bkpt 0");}
#else
#define ASSERT(b)
#endif

#ifdef DEBUG
#define ASSERT_VALID_PTR(p) ASSERT((uint32_t)p >= 0x20000000 && (uint32_t)p <= 0x20009FFF)
#else
#define ASSERT_VALID_PTR(p)
#endif


#endif /* ASSERT_H_ */
// Local Variables:
// mode: c++
// End:
