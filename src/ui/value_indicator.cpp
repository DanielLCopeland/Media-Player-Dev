/**
 * @file value_indicator.cpp
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

#include <ui/common.h>
#include <ui/value_indicator.h>

void
UI::ValueIndicator::draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t value)
{
    display->drawRect(x, y, width, height, WHITE);
    uint16_t fillWidth = map(value, _minVal, _maxVal, 0, width);
    display->fillRect(x, y, fillWidth, height, WHITE);
}