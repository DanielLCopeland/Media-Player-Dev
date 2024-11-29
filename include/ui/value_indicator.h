/**
 * @file value_indicator.h
 *
 * @brief Provides a visual representation of a value. Part of the UI library.
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

#ifndef value_indicator_h
#define value_indicator_h

#include <ui/common.h>

namespace UI {

class ValueIndicator
{

    enum valueIndicatorType : uint8_t
    {
        PROGRESS_BAR,
        VOLUME_BAR,
        BATTERY_BAR,
        SCROLL_BAR,
        WIFI_SIGNAL
    };

  public:
    ValueIndicator(uint16_t minVal = 0, uint16_t maxVal = 100, valueIndicatorType type = PROGRESS_BAR)
      : _minVal(minVal)
      , _maxVal(maxVal)
      , _type(type)
    {
    }

  private:
    uint16_t _minVal = 0;
    uint16_t _maxVal = 0;
    valueIndicatorType _type = PROGRESS_BAR;

    void draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t _value = 0);
};

} // namespace UI

#endif