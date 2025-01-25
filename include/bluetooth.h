/**
 * @file bluetooth.h
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

#ifndef bluetooth_h
#define bluetooth_h

#include <SoftwareSerial.h>
#include <buttons.h>
#include <queue>
#include <string>
#include <timer.h>
#include <ui/common.h>
#include <vector>

#define BLUETOOTH_RX      1
#define BLUETOOTH_TX      2
#define BLUETOOTH_PWR     6
#define BLUETOOTH_BAUD    115200
#define cmd_array_size(a) (sizeof((a)) / sizeof(*(a)))

namespace UI {
class SystemMessage;
}

class Timer;
class Buttons;

enum BluetoothMode : uint8_t
{
    POWER_ON,
    POWER_OFF
};

const char* const cmd_AT PROGMEM = "AT+";
const char* const cmd_REST PROGMEM = "AT+RESET";
const char* const cmd_GMR PROGMEM = "AT+GMR?";
const char* const cmd_BAUD PROGMEM = "AT+BAUD?";
const char* const cmd_STATUS PROGMEM = "AT+STATUS?";
const char* const cmd_DISCON PROGMEM = "AT+DISCON";
const char* const cmd_SCAN PROGMEM = "AT+PAIR";
const char* const cmd_ADDLINKADD PROGMEM = "AT+ADDLINKADD=0x"; /* 12 character hex string */
const char* const cmd_ADDLINKNAME PROGMEM = "AT+ADDLINKNAME=";
const char* const cmd_VMLINK PROGMEM = "AT+VMLINK?";
const char* const cmd_DELVMLINK PROGMEM = "AT+DELVMLINK";
const char* const cmd_PWROFF PROGMEM = "AT+POWER_OFF";
static char cmd_bld_ADDLINKADD[32];
static char cmd_bld_ADDLINKNAME[64];

const char* const cmdsScan[] PROGMEM = { cmd_AT, cmd_REST, cmd_AT, cmd_SCAN };
const char* const cmdsDispRAM[] PROGMEM = { cmd_AT, cmd_VMLINK };
const char* const cmdsStatus[] PROGMEM = { cmd_AT, cmd_GMR, cmd_BAUD, cmd_STATUS };
const char* const cmdsAddRAM[] PROGMEM = { cmd_AT, cmd_DISCON, cmd_VMLINK, cmd_bld_ADDLINKADD, cmd_bld_ADDLINKNAME, cmd_REST, cmd_AT, cmd_VMLINK };
const char* const cmdsClearRAM[] PROGMEM = { cmd_AT, cmd_DISCON, cmd_DELVMLINK, cmd_REST, cmd_AT, cmd_VMLINK };
const char* const cmdsDiscon[] PROGMEM = { cmd_AT, cmd_DISCON };
const char* const cmdsPowerOff[] PROGMEM = { cmd_AT, cmd_PWROFF };

class Bluetooth
{
  private:
    Bluetooth() { _bluetooth = nullptr; }
    ~Bluetooth() { end(); }

    static SoftwareSerial* _bluetooth;
    static void _onReceive();
    static void readData();
    static bool data_received;
    static uint8_t bytes_available;
    static char _buffer[256];
    static std::queue<const char*> _cmdQueue;
    static BluetoothMode _mode;
    static Timer _timer;
    static uint8_t scanCount;
    static UI::SystemMessage* _systemMessage;
    static Bluetooth* _handle;

  public:
    Bluetooth(const Bluetooth& obj) = delete;
    static Bluetooth* get_handle()
    {
        if (Bluetooth::_handle == nullptr) {
            Bluetooth::_handle = new Bluetooth();
        }
        return Bluetooth::_handle;
    }
    void begin();
    void end();
    void loop(); /* Call this in the main loop to check for incoming data and run housekeeping tasks */
    void cmds(const char* const cmdArray[], const uint8_t numCmds);
    void cmd(char* cmd);
    void powerOff()
    {
        _mode = POWER_OFF;
        digitalWrite(BLUETOOTH_PWR, false);
    }
    void powerOn()
    {
        _mode = POWER_ON;
        digitalWrite(BLUETOOTH_PWR, true);
    }
    BluetoothMode getMode() { return _mode; }
    bool available() { return data_received; }
};

#endif