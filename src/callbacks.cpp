/**
 * @file eventhandlers.cpp
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

#include <callbacks.h>

/****************************************************
 *
 * WiFi events
 *
 ****************************************************/

void
onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (Transport::get_handle()->getStatus() == TRANSPORT_PLAYING && Transport::get_handle()->getLoadedMedia().source == REMOTE_FILE) {
        Transport::get_handle()->stop();
    }
    if (playlistEngine->isEnabled() && playlistEngine->isPlaying()) {
        playlistEngine->stop();
    }
    log_e("WiFi disconnected! Stopping network streams!");
}

void
onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    log_i("Connected to WiFi!");
}

void
onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Config_Manager::get_handle()->setIP(WiFi.localIP().toString().c_str());
    Config_Manager::get_handle()->setNetmask(WiFi.subnetMask().toString().c_str());
    Config_Manager::get_handle()->setGateway(WiFi.gatewayIP().toString().c_str());
    Config_Manager::get_handle()->setDNS(WiFi.dnsIP().toString().c_str());
}

void
onWifiLostIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (Transport::get_handle()->getStatus() == TRANSPORT_PLAYING && Transport::get_handle()->getLoadedMedia().source == REMOTE_FILE)
        Transport::get_handle()->stop();
    log_e("WiFi disconnected! Stopping network streams!");
}

void
onWifiFailed(WiFiEvent_t event, WiFiEventInfo_t info)
{
    log_e("WiFi connection failed!");
}

/****************************************************
 *
 * USB MSC events
 *
 ****************************************************/

int32_t
onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }
    bool rc;
#if SD_FAT_VERSION >= 20000
    rc = Card_Manager::get_handle()->card()->writeSectors(lba + offset, buffer, bufsize / 512);
#else
    rc = Card_Manager::get_handle()->card()->writeBlocks(lba + offset, buffer, bufsize / 512);
#endif
    return rc ? bufsize : -1;
};

int32_t
onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }
    bool rc;
#if SD_FAT_VERSION >= 20000
    rc = Card_Manager::get_handle()->card()->readSectors(lba + offset, (uint8_t*) buffer, bufsize / 512);
#else
    rc = Card_Manager::get_handle()->card()->readBlocks(lba + offset, (uint8_t*) buffer, bufsize / 512);
#endif
    return rc ? bufsize : -1;
};

bool
onStartStop(uint8_t power_condition, bool start, bool load_eject)
{
    log_i("MSC START/STOP: power: %u, start: %u, eject: %u\n", power_condition, start, load_eject);
    return true;
};

void
usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t* data = (arduino_usb_event_data_t*) event_data;
        switch (event_id) {
            case ARDUINO_USB_STARTED_EVENT:
                log_i("USB PLUGGED");
                break;
            case ARDUINO_USB_STOPPED_EVENT:
                log_i("USB UNPLUGGED");
                break;
            case ARDUINO_USB_SUSPEND_EVENT:
                log_i("USB SUSPENDED");
                break;
            case ARDUINO_USB_RESUME_EVENT:
                log_i("USB RESUMED");
                break;

            default:
                break;
        }
    }
}

/****************************************************
 *
 * File Explorer callbacks
 *
 ****************************************************/

int
db_callback_get_files(void* data, int argc, char** argv, char** azColName)
{
    MediaData mediadata;
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "filename") == 0) {
            mediadata.filename = argv[i] ? argv[i] : "";
        } else if (strcmp(azColName[i], "path") == 0) {
            mediadata.path = argv[i] ? argv[i] : "";
        } else if (strcmp(azColName[i], "type") == 0) {
            mediadata.type = atoi(argv[i]);
        }
    }
    mediadata.source = LOCAL_FILE;
    mediadata.loaded = true;
    mediadata.text = mediadata.filename;
    static_cast<std::vector<MediaData>*>(data)->push_back(mediadata);
    return 0;
}

int
db_callback_get_checksum(void* data, int argc, char** argv, char** azColName)
{
    char checksum[MD5_DIGEST_STRING_LEN];
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "checksum") == 0) {
            for (int j = 0; j < MD5_DIGEST_LENGTH; j++) {
                checksum[j] = argv[i][j];
            }
        }
    }
    checksum[MD5_DIGEST_STRING_LEN - 1] = '\0';
    memcpy(data, checksum, MD5_DIGEST_STRING_LEN);
    return 0;
}