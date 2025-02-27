/**
 * @file data.h
 *
 * @brief Menus, symbols, and other data used throughout the program.
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

#ifndef data_h
#define data_h

enum error_code_t : uint8_t
{
    ERR_SUCCESS,
    ERR_FAILURE,
    ERR_NOT_FOUND,
    ERR_ALREADY_EXISTS,
    ERR_INVALID,
};

enum sort_order_t : uint8_t
{
    SORT_ASC,
    SORT_DESC,
    SORT_NAME,
    SORT_TYPE,
};

namespace MENUDATA {

/****************************************************
 *
 * Menus
 *
 ****************************************************/

/* Main menu */
namespace MAIN_m {
enum items : int32_t
{
    WIFI,
    BLUETOOTH,
    SYSTEM,
    PLAYLIST_EDITOR,
    INFO,
    SIZE
};
const char* const menu[] PROGMEM = { "Wifi", "Bluetooth", "System", "Playlist Editor", "Info" };
}

/* System menu */
namespace SYSTEM_m {
enum items : int32_t
{
    AUDIO,
    DATETIME,
    SCREENSAVER,
    USB_TRANSFER,
    REBOOT,
    RESET,
    SIZE
};
const char* const menu[] PROGMEM = { "Audio", "Date/Time", "Screen Saver", "USB File Transfer", "Reboot", "Factory Reset"};
}

/* Date/Time menu */
namespace DATETIME_m {
enum items : int32_t
{
    TIME,
    DATE,
    TIMEZONE,
    ALARM,
    SIZE
};
const char* const menu[] PROGMEM = { "Set Time", "Set Date", "Set Timezone", "Alarm" };
}

namespace ALARM_m {
/* Alarm menu */
enum items : int32_t
{
    SET,
    MEDIA,
    ENABLE,
    DISABLE,
    SIZE
};
const char* const menu[] PROGMEM = { "Set Alarm", "Alarm Media", "Enable Alarm", "Disable Alarm" };
}

/* Network settings menu */
namespace NETWORK_m {
enum items : int32_t
{
    TOGGLE,
    DHCP,
    SEARCH,
    SSID,
    PASSWORD,
    IP_ADDRESS,
    NETMASK,
    GATEWAY,
    DNS,
    NTP_CONFIG,
    SIZE
};
const char* const menu[] PROGMEM = { "Connect/Disconnect", "DHCP",    "SSID Scan", "Enter SSID", "Password",
                                     "IP Address",         "Netmask", "Gateway",   "DNS",        "NTP Config" };
}

/* Connect/Disconnect menu */
namespace WIFI_m {
enum items : int32_t
{
    ENABLE,
    DISABLE,
    SIZE
};
const char* const menu[] PROGMEM = { "Connect", "Disconnect" };
}

/* Bluetooth menu */
namespace BLUETOOTH_m {
enum items : int32_t
{
    ENABLE,
    DISABLE,
    SIZE
};
const char* const menu[] PROGMEM = { "Enable", "Disable" };
}

/* DHCP menu */
namespace DHCP_TOGGLE_m {
enum items : int32_t
{
    ENABLE,
    DISABLE,
    SIZE
};
const char* const menu[] PROGMEM = { "Enable", "Disable" };
}

/* NTP Config menu */
namespace NTP_m {
enum items : int32_t
{
    SERVER,
    INTERVAL,
    TIMEZONE,
    UPDATE,
    SIZE
};
const char* const menu[] PROGMEM = { "NTP Server", "Update Interval", "Timezone", "Update Now" };
}

/* Playlist editor menu */
namespace PLAYLIST_EDITOR_m {
enum items : int32_t
{
    LOAD,
    EDIT,
    ADD,
    REMOVE,
    SIZE
};
const char* const menu[] PROGMEM = { "Load Playlist", "Edit Playlist", "Create Playlist", "Delete Playlist" };
}

/* Playlist editor add/remove menu */
namespace PLAYLIST_EDITOR_EDIT_m {
enum items : int32_t
{
    ADDTRACK,
    REMOVETRACK,
    SIZE
};
const char* const menu[] PROGMEM = { "Add Track", "Delete Track" };
}

namespace AUDIO_m {
enum items : int32_t
{
    BASS,
    MID,
    TREBLE,
    SYSVOL,
    SIZE
};
const char* const menu[] PROGMEM = { "Bass", "Mid", "Treble", "UI Volume" };
}

/* Screen saver menu */
namespace SCREENSAVER_m {
enum items : int32_t
{
    ENABLE,
    DISABLE,
    TIMEOUT,
    SIZE
};
const char* const menu[] PROGMEM = { "Enable", "Disable", "Timeout" };
}

/* File browser alt menu */
namespace FILE_BROWSER_m {
enum items : int32_t
{
    ASC,
    DESC,
    NAME,
    TYPE,
    SIZE
};
const char* const menu[] PROGMEM = { "Sort ascending", "Sort descending", "Sort by name", "Sort by type" };
}

}

/****************************************************
 *
 * Symbols
 *
 ****************************************************/

const char* const characterTable_alphanumeric[] PROGMEM = { " ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
                                                            "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "a", "b", "c", "d", "e",
                                                            "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u",
                                                            "v", "w", "x", "y", "z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "." };
const uint8_t characterTable_alphanumeric_length = 64;

const char* const characterTable_all[] PROGMEM = { " ", "!", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", ":", ";", "<", "=",
                                                   ">", "?", "@", "[", "]", "^", "_", "`", "{", "|", "}", "~", "A", "B", "C", "D", "E", "F", "G",
                                                   "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
                                                   "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
                                                   "t", "u", "v", "w", "x", "y", "z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
const uint8_t characterTable_all_length = 93;

const char* const characterTable_serverAddress[] PROGMEM = { " ", ".", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f", "g", "h",
                                                             "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "-" };
const uint8_t characterTable_serverAddress_length = 39;

const char* const characterTable_numeric[] PROGMEM = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
const uint8_t characterTable_numeric_length = 10;

#endif