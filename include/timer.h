/**
 * @file timer.h
 *
 * @brief Non-blocking timer functions
 *
 * @author Dan Copeland
 *
 * Licensed under GPL v3.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef timer_h
#define timer_h

#include <Arduino.h>

class Timer
{
  public:
    Timer();
    bool check(uint32_t ms);   // Checks to see if the given time has elapsed,
                                  // and if so, return "true"
    void reset();                 // Resets the timer

  private:
    bool timerEnabled;
    uint32_t lastMillis;
};

#endif