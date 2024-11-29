/**
 * @file spectrum_analyzer.h
 *
 * @brief Displays a spectrum analyzer. Retrieves data from the audio
 * transport. Part of the UI library.
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

#ifndef spectrum_analyzer_h
#define spectrum_analyzer_h

#include <ui/common.h>

namespace UI {

class SpectrumAnalyzer
{
  public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer()
    {
        if (_currentVal != nullptr) {
            delete[] _currentVal;
        }
        if (_peak != nullptr) {
            delete[] _peak;
        }
    }

    void draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

  private:
    Timer _updateTimer;
    uint8_t bands = 0;
    uint16_t* _currentVal = nullptr;
    uint16_t* _peak = nullptr;
    static const uint16_t refresh_interval = 20;
};

} // namespace UI

#endif