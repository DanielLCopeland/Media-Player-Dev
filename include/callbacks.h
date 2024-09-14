/**
 * @file eventhandlers.h
 *
 * @brief Global event handlers
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

#ifndef eventhandlers_h
#define eventhandlers_h

#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <USB.h>
#include <USBMSC.h>
#include <card_manager.h>
#include <esp_sntp.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <system.h>
#include <transport.h>

class SystemConfig;
class Transport;
class CardManager;
class PlaylistEngine;

extern CardManager* sdfs;
extern Transport* transport;
extern SystemConfig* systemConfig;
extern PlaylistEngine* playlistEngine;

/* WiFi events */
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiLostIP(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiFailed(WiFiEvent_t event, WiFiEventInfo_t info);

/* USB MSC */
static int32_t
onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    if (!sdfs->isReady()) {
        return -1;
    }
    bool rc;
#if SD_FAT_VERSION >= 20000
    rc = sdfs->card()->writeSectors(lba + offset, buffer, bufsize / 512);
#else
    rc = sdfs->card()->writeBlocks(lba + offset, buffer, bufsize / 512);
#endif
    return rc ? bufsize : -1;
};

static int32_t
onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    if (!sdfs->isReady()) {
        return -1;
    }
    bool rc;
#if SD_FAT_VERSION >= 20000
    rc = sdfs->card()->readSectors(lba + offset, (uint8_t*) buffer, bufsize / 512);
#else
    rc = sdfs->card()->readBlocks(lba + offset, (uint8_t*) buffer, bufsize / 512);
#endif
    return rc ? bufsize : -1;
};

static bool
onStartStop(uint8_t power_condition, bool start, bool load_eject)
{
    log_i("MSC START/STOP: power: %u, start: %u, eject: %u\n", power_condition, start, load_eject);
    return true;
};

static void
usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t* data = (arduino_usb_event_data_t*) event_data;
        switch (event_id) {
            case ARDUINO_USB_STARTED_EVENT:
                Serial.println("USB PLUGGED");
                break;
            case ARDUINO_USB_STOPPED_EVENT:
                Serial.println("USB UNPLUGGED");
                break;
            case ARDUINO_USB_SUSPEND_EVENT:
                Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en);
                break;
            case ARDUINO_USB_RESUME_EVENT:
                Serial.println("USB RESUMED");
                break;

            default:
                break;
        }
    }
}

#endif