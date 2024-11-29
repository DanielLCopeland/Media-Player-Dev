/**
 * @file constants.h
 *
 * @brief Constants used throughout the UI library.
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

#include <stdint.h>
#include <limits>

namespace UI {

enum Constants : int32_t
{
    SCREEN_WIDTH = 128,
    SCREEN_HEIGHT = 32,
    OLED_RESET = -1,
    SCREEN_ADDRESS = 0x3C,

    MAX_DISPLAYED_CHARACTERS = 21,
    CURSOR_BLINK_INTERVAL = 250,
    SELECTED_ITEM_ROTATE_SPEED = 150,
    MAX_TEXT_LINES = 4,
    MAX_CHARACTERS_PER_LINE = 19,
    UI_EXIT = std::numeric_limits<uint16_t>::max(),
    UI_EXIT_TIMEOUT = 4000,
    NOTIFICATION_ANIMATION_FRAME_DURATION = 250
};

} /* End namespace UI */