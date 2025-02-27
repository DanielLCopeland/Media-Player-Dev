/**
 * @file status.h
 *
 * @brief Displays the main status screen. Part of the UI library.
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

#ifndef status_h
#define status_h

#include <ui/common.h>

class Marquee;
class Animation;
class SpectrumAnalyzer;
class Bluetooth;

namespace UI {

class StatusScreen
{
  public:
    StatusScreen(const StatusScreen&) = delete;
    static StatusScreen* get_handle()
    {
        if (!_handle) {
            _handle = new StatusScreen();
        }
        return _handle;
    }
    void draw();

  private:
    StatusScreen();
    ~StatusScreen() { delete _handle; }
    static StatusScreen* _handle;
    uint16_t* spectrumAnalyzerCurrentVal;
    uint16_t* spectrumAnalyzerPeak;
    size_t playTime = 0;
    Animation* anim_playing = nullptr;
    Animation* anim_stopped = nullptr;
    Marquee* marquee_mediainfo = nullptr;
    Marquee* marquee_datetime = nullptr;
    Marquee* marquee_connectStatus = nullptr;
    SpectrumAnalyzer* spectrumAnalyzer = nullptr;
};

}   // namespace UI

#endif