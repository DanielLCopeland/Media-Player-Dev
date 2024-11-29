/**
 * @file binary_selector.cpp
 *
 * @brief Allows the user to select between two options. Part of the UI library.
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
#include <ui/binary_selector.h>

UI::BinarySelector::BinarySelector(std::string falseText, std::string trueText)
{
    _options.push_back(falseText);
    _options.push_back(trueText);
}

bool
UI::BinarySelector::get()
{
    ListSelection list;
    uint16_t selection = list.get(_options);
    bool result;
    if (selection == UI_EXIT) {
        return false;
    } else if (selection == 0) {
        return false;
    } else {
        return true;
    }
}