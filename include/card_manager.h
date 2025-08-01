/**
 * @file card_manager.h
 *
 * @brief Keeps track of the insertion status of the SD card and
 * initializes the SD card when it is inserted.
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

#ifndef card_manager_h
#define card_manager_h

#define CARD_DETECT_PIN       39
#define INSERTION_DEBOUNCE_MS 500
#define REMOVAL_DEBOUNCE_MS   5
#define SD_CS_PIN             38
#define SD_CONFIG             SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(20), &SPI)

#include <SdFat.h>
#include <transport.h>

class Transport;
class PlaylistEngine;
extern PlaylistEngine* playlistEngine;

class Card_Manager : public SdFs
{
  public:
    Card_Manager(const Card_Manager&) = delete;
    void begin();
    void update();
    bool isReady();
    void end() { return SdFs::end(); }
    bool check_card_detect();
    static Card_Manager* get_handle()
    {
        if (!_handle) {
            _handle = new Card_Manager();
        }
        return _handle;
    }

  private:
    Card_Manager()
      : SdFs()
    {
    }
    ~Card_Manager()
    {
        SdFs::end();
        delete _handle;
    }
    bool isReadyFlag = false;
    bool lastState = false;
    bool lock = false;
    uint32_t lastInsertionCheck = 0;
    uint32_t lastRemovalCheck = 0;
    static Card_Manager* _handle;
    SemaphoreHandle_t _mutex;
};

#endif