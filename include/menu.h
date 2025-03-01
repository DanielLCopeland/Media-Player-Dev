/**
 * @file menu.h
 *
 * @brief Menu system
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

#ifndef menu_h
#define menu_h

#include <USBMSC.h>
#include <callbacks.h>
#include <buttons.h>
#include <esp_sntp.h>
#include <card_manager.h>
#include <bluetooth.h>
#include <system.h>
#include <time.h>
#include <timer.h>
#include <timezones.h>
#include <transport.h>
#include <ui/common.h>
#include <snake.h>
#include <tetris.h>

#define SYSTEM_INFO_DISPLAY_TIME_MS 10000
#define WIFI_CONNECTION_TIMEOUT_MS  10000
#define WIFI_MAX_DISPLAYED_NETWORKS 20
#define MENU_TIMEOUT 20

class Transport;
class Buttons;
class Card_Manager;
class Bluetooth;
class PlaylistEngine;
class Config_Manager;
class MediaData;
class TableData;
class Timer;

extern Adafruit_SSD1306* display;

void mainMenu();
void systemMenu();
void wifiMenu();
void bluetoothMenu();
void dhcpToggleMenu();
void dateTimeMenu();
void alarmMenu();
void reboot();
void networkMenu();
void ntpConfigMenu();
void playlistEditor_mainMenu();
void playlistEditor_trackMenu(PlaylistEngine* _playlistEngine = nullptr);
void audioMenu();
void usbMenu();
void screensaverMenu();
void infoScreen();

void ssidScanner();

#endif