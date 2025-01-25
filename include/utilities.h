/**
 * @file utilities.h
 *
 * @brief Various utility functions I couldn't find a better place for.
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

#ifndef utilities_h
#define utilities_h

#include <string>

static std::string
escape_single_quotes(const std::string& input)
{
    std::string escaped_string;
    escaped_string.reserve(input.size());

    for (char c : input) {
        if (c == '\'') {
            escaped_string += "''";
        } else {
            escaped_string += c;
        }
    }
    return escaped_string;
}

#endif