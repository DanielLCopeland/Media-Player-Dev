/**
 * @file spectrum_analyzer.cpp
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

#include <ui/common.h>
#include <ui/spectrum_analyzer.h>

UI::SpectrumAnalyzer::SpectrumAnalyzer()
{
    if (!Transport::get_handle()) {
        return;
    }
    bands = Transport::get_handle()->spectrumAnalyzer->getBands();
    _currentVal = new uint16_t[bands];
    _peak = new uint16_t[bands];
    memset(_currentVal, 0, sizeof(uint16_t) * bands);
    memset(_peak, 0, sizeof(uint16_t) * bands);
}

void
UI::SpectrumAnalyzer::draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if (_updateTimer.check(refresh_interval)) {
        Transport::get_handle()->spectrumAnalyzer->getVals(_currentVal, _peak);
    }
    
    /* Draw each line, with the width given, in pixels */
    uint8_t count = bands;
    for (uint8_t i = 0; i < bands; i++) {
        bool peak_visible = Transport::get_handle()->spectrumAnalyzer->isPeakVisible(i);
        count--;
        uint16_t val = map(_currentVal[i], 0, 2048, 0, height);
        if (val > height) {
            val = height;
        }
        uint16_t peak = map(_peak[i], 0, 2048, 0, height);
        if (peak > height) {
            peak = height;
        }
        for (uint8_t j = 0; j < width; j++) {
            uint8_t shift = count * width;
            display->drawFastVLine(x + shift + j + count, y + height - val, val, WHITE);
            if (peak_visible) {
                display->drawFastVLine(x + shift + j + count, y + height - peak, 1, WHITE);
            }
            shift = (i + width) * width;
            display->drawFastVLine(x + shift + j + i + (bands * width), y + height - val, val, WHITE);
            if (peak_visible) {
                display->drawFastVLine(x + shift + j + i + (bands * width), y + height - peak, 1, WHITE);
            }
        }
    }
}