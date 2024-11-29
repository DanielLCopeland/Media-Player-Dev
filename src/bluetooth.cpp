/**
 * @file bluetooth.cpp
 *
 * @brief Controls the KCX_BT_EMITTER Bluetooth module
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

#include "bluetooth.h"

/* Static variables */
SoftwareSerial* Bluetooth::_bluetooth = nullptr;
UI::SystemMessage* Bluetooth::_systemMessage = nullptr;
bool Bluetooth::data_received = false;
uint8_t Bluetooth::bytes_available = 0;
char Bluetooth::_buffer[256] = { 0 };
std::queue<const char*> Bluetooth::_cmdQueue;
BluetoothMode Bluetooth::_mode = POWER_OFF;
Timer Bluetooth::_timer;
uint8_t Bluetooth::scanCount = 0;

Bluetooth::Bluetooth()
{
    _bluetooth = nullptr;
}

void
Bluetooth::begin()
{
    if (_bluetooth) {
        delete _bluetooth;
    }
    _bluetooth = new SoftwareSerial(BLUETOOTH_RX, BLUETOOTH_TX);
    _bluetooth->onReceive(_onReceive);
    _bluetooth->begin(BLUETOOTH_BAUD);
    if (_systemMessage) {
        delete _systemMessage;
    }
    _systemMessage = new UI::SystemMessage();
    pinMode(BLUETOOTH_PWR, OUTPUT);
    digitalWrite(BLUETOOTH_PWR, false);
}

void
Bluetooth::end()
{
    powerOff();
    if (_bluetooth) {
        _bluetooth->end();
        delete _bluetooth;
        _bluetooth = nullptr;
    }
    if (_systemMessage) {
        delete _systemMessage;
        _systemMessage = nullptr;
    }
}

void
Bluetooth::_onReceive()
{
    data_received = true;
}

void
Bluetooth::cmds(const char* const cmdArray[], const uint8_t numCmds)
{
    if (!_bluetooth) {
        log_e("KCX_BT_RTX not initialized!");
        return;
    }

    /* Clear the command queue */
    while (!_cmdQueue.empty()) {
        _cmdQueue.pop();
    }

    for (uint8_t i = 0; i < numCmds; i++) {
        _cmdQueue.push(cmdArray[i]);
    }

    if (!data_received) {
        cmd((char*) _cmdQueue.front());
        _cmdQueue.pop();
    }
}

void
Bluetooth::cmd(char* cmd)
{
    if (!_bluetooth) {
        log_e("KCX_BT_RTX not initialized!");
        return;
    }
    log_i("KCX_BT_RTX sending: %s", cmd);
    _bluetooth->printf("%s%s", cmd, "\r\n");
}

void
Bluetooth::loop()
{
    if (data_received) {
        readData();
        data_received = false;
        std::string buffer(_buffer);
        std::smatch match;

        

        /* If we got a SCAN message, we are disconnected.  Advance the scan counter */
        if (std::regex_search(buffer, match, std::regex("SCAN"))) {
            scanCount++;
            /* If we have recieved 5 scan messages in a row, power off the module */
            if (scanCount >= 5) {
                scanCount = 0;
                powerOff();
                _systemMessage->show("Bluetooth disabled!", 2000, false);
            }
            if (scanCount == 1) {
                _systemMessage->show("Bluetooth disconnected!\nScanning...", 2000, false);
            }
        }

        else
            scanCount = 0;

        /* Use regex to search for a "CON" message */
        if (std::regex_search(buffer, match, std::regex("CON"))) {
            _systemMessage->show("Bluetooth connected!", 2000, false);
        }

        /* If the first two characters are OK, then we can assume the command was successful, if not, clear the command queue */
        if (strncmp(_buffer, "OK", 2) != 0 && !_cmdQueue.empty()) {
            log_e("KCX_BT_RTX error in response.");
            while (!_cmdQueue.empty()) {
                _cmdQueue.pop();
            }
            return;
        }

        /* Process the command queue */
        if (!_cmdQueue.empty()) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            cmd((char*) _cmdQueue.front());
            _cmdQueue.pop();
        }
    }
}

void
Bluetooth::readData()
{
    if (_bluetooth->available()) {
        /* Read the entire string from SoftwareSerial and store it in the buffer */
        bytes_available = _bluetooth->readBytes(_buffer, 256);
        _buffer[bytes_available] = '\0';
    }
    log_i("KCX_BT_RTX recieved: %s", _buffer);
}