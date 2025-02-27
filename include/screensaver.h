/**
 * @file screensaver.h
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

#ifndef screensaver_h
#define screensaver_h

#include <Arduino.h>
#include <timer.h>

#define DEFAULT_SCREENSAVER_TIMEOUT 30

class Screensaver
{
    private:
        Timer _timer;
        bool _enabled = false;
        bool _blanked = false;
        uint16_t _timeout; /* seconds */
        static Screensaver* _handle;
        Screensaver();
        ~Screensaver() { delete _handle; }
        Screensaver(Screensaver const&) = delete;

    public:
        static Screensaver* get_handle()
        {
            if (!_handle) {
                _handle = new Screensaver();
            }
            return _handle;
        }
        void loop();
        void enable() { _enabled = true; _blanked = false; }
        void disable() { _enabled = false; _blanked = false; }
        void set_timeout(uint8_t timeout);
        bool is_enabled() { return _enabled; }
        bool is_blanked() { return _blanked; }
        void reset() { _timer.reset(); _blanked = false; }
};

#endif