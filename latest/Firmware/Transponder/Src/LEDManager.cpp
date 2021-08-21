/*
  Copyright (c) 2016-2021 Peter Antypas

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

#include "LEDManager.hpp"
#include "bsp.hpp"
#include "Configuration.hpp"

LEDManager::LEDManager()
{
  // Do nothing
}

LEDManager &LEDManager::instance()
{
  static LEDManager __instance;
  return __instance;
}

void tickCB()
{
  LEDManager::instance().onTick();
}

static bool mForceTXLedOff = false;

void LEDManager::init()
{
  if ( !Configuration::instance().isStationDataProvisioned() )
    {
      mForceTXLedOff = true;

      // This call actually has the opposite effect as it will cause the TX led to be pulled to GND
      bsp_tx_led_on();
    }
  bsp_set_tick_callback(tickCB);
}

void LEDManager::onTick()
{
  static int count1 = 1;
  static int count2 = 1;
  if ( count1++ % 20 == 0 )
    {
      count1 = 1;
      bsp_rx_led_off();
    }


  if ( !mForceTXLedOff && count2++ == 250 )
    {
      count2 = 1;
      bsp_tx_led_off();
    }
}


