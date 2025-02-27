/**
 * @file screensaver.cpp
 *
 * @brief Blanks the screen after a period of inactivity
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

#include <screensaver.h>

Screensaver* Screensaver::_handle = nullptr;

Screensaver::Screensaver() : _timeout(DEFAULT_SCREENSAVER_TIMEOUT), _enabled(false), _blanked(false)
{
}

void
Screensaver::loop()
{
    if (_enabled) {
        if (_timer.check(_timeout * 1000)) {
            _blanked = true;
        }
    }
}

void
Screensaver::set_timeout(uint8_t timeout)
{
    _timeout = timeout;
    _timer.reset();
    _blanked = false;
}