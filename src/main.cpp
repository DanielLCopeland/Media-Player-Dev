/**
 * @file main.cpp
 *
 * @brief Main entry point for the core program
 *
 * @author Dan Copeland
 *
 * Dedicated to the memory of my father, Byron D. Copeland, who passed away
 * on September 14, 2024, and who started me on this journey of discovery and
 * learning so many decades ago.  I miss you, Dad.
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

#define SDFAT_FILE_TYPE 3
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SdFat.h>
#include <Wire.h>
#include <bluetooth.h>
#include <playlist_engine2.h>
#include <buttons.h>
#include <card_manager.h>
#include <functional>
#include <menu.h>
#include <playlist_engine.h>
#include <screensaver.h>
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>
#include <system.h>
#include <transport.h>
#include <ui/common.h>
#include <ui_sounds.h>
#include <vfs.h>
#include <callbacks.h>

SET_LOOP_TASK_STACK_SIZE(16 * 1024); /* 16KB */

/* Display config */
#define DISPLAY_DATA_PIN  40
#define DISPLAY_CLOCK_PIN 41
#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     32
#define OLED_RESET        -1
#define SCREEN_ADDRESS    0x3C

/**
 * The following global singleton objects represent various physical components
 * of the system or the UI. All other code will interact with the hardware
 * through a pointer to one of these objects.  Warning: the WiFi password is
 * stored in plaintext in the preferences storage.  This is a security risk
 * should the device be lost or stolen.
 */
Adafruit_SSD1306* display = nullptr;      /* Display */
UI::FileBrowser* filebrowser = nullptr;   /* File browser */
UI::SystemMessage* notify = nullptr;      /* System messages */
PlaylistEngine* playlistEngine = nullptr; /* Playlist engine */

void
checkButtons()
{
    if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
        if (Buttons::get_handle()->isHeld(BUTTON_MENU) && playlistEngine->isEnabled()) {
            uint16_t selection = playlistEngine->view(true);
            if (selection != UI::UI_EXIT) {
                playlistEngine->setCurrentTrack(selection);
                bool wasPlaying = false;
                if (Transport::get_handle()->getStatus() == TRANSPORT_PLAYING) {
                    Transport::get_handle()->stop();
                    wasPlaying = true;
                }
                Transport::get_handle()->load(playlistEngine->getCurrentTrack());
                if (wasPlaying) {
                    Transport::get_handle()->play();
                }
            }
        }

        else {
            if (Transport::get_handle()->getStatus() != TRANSPORT_CONNECTING) {

                MediaData mediadata = filebrowser->get();

                if (mediadata.loaded) {
                    if (mediadata.type == FILETYPE_M3U) {
                        playlistEngine->load(mediadata);
                    } else {
                        if (playlistEngine->isEnabled()) {
                            playlistEngine->eject();
                        }
                        Transport::get_handle()->load(mediadata);
                    }
                }
            }
        }
    }

    if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
        if (Transport::get_handle()->getStatus() == TRANSPORT_PLAYING) {
            Transport::get_handle()->pause();
            if (playlistEngine->isEnabled()) {
                playlistEngine->stop();
            }
        } else if (Transport::get_handle()->getStatus() == TRANSPORT_STOPPED || Transport::get_handle()->getStatus() == TRANSPORT_PAUSED) {
            if (Transport::get_handle()->getLoadedMedia().source == REMOTE_FILE && WiFi.status() == WL_CONNECTED) {
                Transport::get_handle()->play();
                if (playlistEngine->isEnabled()) {
                    playlistEngine->play();
                }
            }

            else if (Transport::get_handle()->getLoadedMedia().source == REMOTE_FILE && WiFi.status() != WL_CONNECTED)
                notify->show("WiFi not connected!", 2000, false);

            else if (Transport::get_handle()->getLoadedMedia().source == LOCAL_FILE)
                Transport::get_handle()->play();
        }
    }
    if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
        if (Transport::get_handle()->getStatus() == TRANSPORT_PLAYING || Transport::get_handle()->getStatus() == TRANSPORT_PAUSED) {
            Transport::get_handle()->stop();
            if (playlistEngine->isEnabled()) {
                playlistEngine->stop();
            }
        } else if (Transport::get_handle()->getStatus() == TRANSPORT_STOPPED) {
            Transport::get_handle()->eject();
            if (playlistEngine->isEnabled()) {
                playlistEngine->eject();
            }
        }
    }

    if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, SHORTPRESS)) {
        if (Buttons::get_handle()->isHeld(BUTTON_MENU) && playlistEngine->isEnabled()) {
            if (playlistEngine->available()) {
                notify->show("Loading next...", 200, false);
                playlistEngine->next();
                if (Transport::get_handle()->getStatus() == TRANSPORT_PLAYING) {
                    Transport::get_handle()->stop();
                    Transport::get_handle()->load(playlistEngine->getCurrentTrack());
                    Transport::get_handle()->play();
                }
            } else
                notify->show("End of playlist!", 1000, false);
        }

        /* If this isn't an alt button press, then it's a volume up command */
        else {
            /* Create a ValueSelector object to adjust the volume using callbacks to the transport */
            UI::ValueSelector* volumeSelector = new UI::ValueSelector("Volume",
                                                                      std::bind(&Transport::getVolume, Transport::get_handle()),
                                                                      std::bind(&Transport::volumeUp, Transport::get_handle()),
                                                                      std::bind(&Transport::volumeDown, Transport::get_handle()),
                                                                      Transport::get_handle()->getMinVolume(),
                                                                      Transport::get_handle()->getMaxVolume());
            volumeSelector->get();
            delete volumeSelector;
        }
    }

    if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, SHORTPRESS)) {
        if (Buttons::get_handle()->isHeld(BUTTON_MENU) && playlistEngine->isEnabled()) {
            if (playlistEngine->getCurrentTrackIndex() > 0) {
                notify->show("Loading previous...", 200, false);
                playlistEngine->previous();
                if (Transport::get_handle()->getStatus() == TRANSPORT_PLAYING) {
                    Transport::get_handle()->stop();
                    Transport::get_handle()->load(playlistEngine->getCurrentTrack());
                    Transport::get_handle()->play();
                }
            } else
                notify->show("Start of playlist!", 1000, false);
        }

        /* If this isn't an alt button press, then it's a volume down command */
        else {
            /* Create a ValueSelector object to adjust the volume using callbacks to the transport */
            UI::ValueSelector* volumeSelector = new UI::ValueSelector("Volume",
                                                                      std::bind(&Transport::getVolume, Transport::get_handle()),
                                                                      std::bind(&Transport::volumeUp, Transport::get_handle()),
                                                                      std::bind(&Transport::volumeDown, Transport::get_handle()),
                                                                      Transport::get_handle()->getMinVolume(),
                                                                      Transport::get_handle()->getMaxVolume());
            volumeSelector->get();
            delete volumeSelector;
        }
    }

    if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, LONGPRESS)) {
        Transport::get_handle()->volumeUp();
        Buttons::get_handle()->repeat(BUTTON_UP);
    }

    if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
        Transport::get_handle()->volumeDown();
        Buttons::get_handle()->repeat(BUTTON_DOWN);
    }

    if (Buttons::get_handle()->getButtonEvent(BUTTON_MENU, LONGPRESS)) {
        Transport::get_handle()->playUIsound(folder_open, folder_open_len);
        mainMenu();
    }

    /* Since we aren't using the shortpress event for the menu button, we can use it to disable the screensaver */
    if (Buttons::get_handle()->isHeld(BUTTON_MENU)) {
        Screensaver::get_handle()->reset();
    }
}

void
setup()
{
    /* Register custom VFS from vfs.h using SdFat as the backend. This does the magic for
    SQLite3 to be able to read and write to the SD card using Bill Greiman's
    awesome SdFat library */
    esp_vfs_register("", &sdfat_vfs, NULL);

    Serial.begin(115200);
    log_i("This software is licensed under the GNU Public License v3.0");
    log_i("Free heap: %d", ESP.getFreeHeap());
    log_i("Free PSRAM: %d", ESP.getFreePsram());
    log_i("Free stack: %d", uxTaskGetStackHighWaterMark(NULL));
    log_i("Starting system...");
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); /* Display object */
    Wire.setPins(DISPLAY_DATA_PIN, DISPLAY_CLOCK_PIN);
    Wire.begin();

    /* Scan the I2C bus for devices and print their addresses to the serial console */
    uint8_t count = 0;

    for (uint8_t i = 8; i < 120; i++) {
        Wire.beginTransmission(i);

        if (Wire.endTransmission() == 0) {
            log_i("Found device at address: %d", i);
            log_i("Reading data from device...");

            Wire.requestFrom(i, sizeof(uint8_t));

            while (Wire.available()) {
                log_i("Data: %d", Wire.read());
            }

            count++;
            delay(1);
        }
    }

    log_i("Found %d devices on the I2C bus", count);

    /* Initialize global objects */
    if (!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        log_e("SSD1306 init failed");
        for (;;)
            ;
    }

    playlistEngine = new PlaylistEngine(std::function<bool(MediaData)>(std::bind(&Transport::load, Transport::get_handle(), std::placeholders::_1)),
                                        std::function<bool()>(std::bind(&Transport::play, Transport::get_handle())),
                                        std::function<void()>(std::bind(&Transport::stop, Transport::get_handle())),
                                        std::function<uint8_t()>(std::bind(&Transport::getStatus, Transport::get_handle())));
    notify = new UI::SystemMessage();
    Transport::get_handle()->begin();
    Config_Manager::get_handle()->begin();
    Card_Manager::get_handle()->begin();
    filebrowser = new UI::FileBrowser();
    notify->show("Starting system...", 0, false);
    Bluetooth::get_handle()->begin();
    Bluetooth::get_handle()->powerOff();

    /* WiFi event callbacks */
    WiFiEventId_t wifiDisconnected = WiFi.onEvent(onWifiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFiEventId_t wifiConnected = WiFi.onEvent(onWifiConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFiEventId_t wifiGotIP = WiFi.onEvent(onWifiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFiEventId_t wifiLostIP = WiFi.onEvent(onWifiLostIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);

    /* Event handler for when we cannot connect to the network */
    WiFiEventId_t wifiConnectFailed = WiFi.onEvent(onWifiFailed, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_START);
    MediaData mediadata;
    mediadata.source = LOCAL_FILE;
    mediadata.path = "/";
    mediadata.filename = "test.m3u";
    mediadata.loaded = true;
    Playlist_Engine2::get_handle()->begin();
    Playlist_Engine2::get_handle()->add_playlist(mediadata, "WTAQAM");
}

void
loop()
{
    /* Draw the main screen showing the currently playing file/url, transport status, and other info */
    UI::StatusScreen::get_handle()->draw();
    /* Check for button presses and handle them */
    checkButtons();
    /* Handle streams and other housekeeping */
    serviceLoop();

    vTaskDelay(10 / portTICK_PERIOD_MS);
}
