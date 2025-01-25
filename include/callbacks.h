/**
 * @file callbacks.h
 *
 * @brief System callbacks
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

#ifndef callbacks_h
#define callbacks_h

#include <SPI.h>
#include <SdFat.h>
#include <USB.h>
#include <USBMSC.h>
#include <card_manager.h>
#include <esp_sntp.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

class Card_Manager;

/* WiFi events */
void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiConnected(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiLostIP(WiFiEvent_t event, WiFiEventInfo_t info);
void onWifiFailed(WiFiEvent_t event, WiFiEventInfo_t info);

/* USB MSC events */
int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize);
int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);
bool onStartStop(uint8_t power_condition, bool start, bool load_eject);
void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/* File Explorer callbacks */
int db_callback_get_files(void* data, int argc, char** argv, char** azColName);
int db_callback_get_checksum(void* data, int argc, char** argv, char** azColName);

#endif