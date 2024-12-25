/**
 * @file system.h
 *
 * @brief System utilities
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

#ifndef system_h
#define system_h

#ifndef PLAYLIST_DIR
#define PLAYLIST_DIR "/playlists"
#endif

#include <Preferences.h>
#include <WiFi.h>
#include <bluetooth.h>
#include <card_manager.h>
#include <esp_sntp.h>
#include <playlist_engine.h>
#include <screensaver.h>
#include <time.h>
#include <transport.h>

enum file_type
{
    FILETYPE_MP3,
    FILETYPE_WAV,
    FILETYPE_FLAC,
    FILETYPE_OGG,
    FILETYPE_M3U,
    FILETYPE_DIR,
    FILETYPE_UNKNOWN
};

enum file_Source
{
    NO_SOURCE_LOADED,
    LOCAL_FILE,
    REMOTE_FILE
};

class CardManager;
class Transport;
class SystemConfig;
class PlaylistEngine;
class Bluetooth;
class Screensaver;

extern CardManager* sdfs;
extern Transport* transport;
extern SystemConfig* systemConfig;
extern PlaylistEngine* playlistEngine;
extern Bluetooth* bluetooth;
extern Screensaver* screensaver;

/* Used to pass file/stream info around various parts of the system */
class MediaData
{
  public:
    std::string filename;
    std::string path;
    std::string url;
    uint8_t type; /* MP3, WAV, FLAC, OGG, M3U, or DIR */
    uint16_t port;
    uint8_t source;         // LOCAL_FILE or REMOTE_FILE
    bool loaded;            // whether or not the struct is loaded with an actual media
                            // source or is empty
    uint32_t nextElement;   // This is a pointer to the next element in the index
                            // file. Functions like a linked list.

    // Used to store temporary strings so we don't invalidate any pointers to them
    std::string buffer;

    MediaData operator&(const MediaData& other) const { return MediaData(other); }

    MediaData(std::string filename, std::string path, std::string url, uint8_t type, uint16_t port, uint8_t source, bool loaded)
      : filename(filename)
      , path(path)
      , url(url)
      , type(type)
      , port(port)
      , source(source)
      , loaded(loaded)
      , nextElement(0)
      , buffer("")
    {
    }

    MediaData()
      : filename("")
      , path("")
      , url("")
      , type(FILETYPE_UNKNOWN)
      , port(0)
      , source(NO_SOURCE_LOADED)
      , loaded(false)
      , nextElement(0)
      , buffer("")
    {
    }

    MediaData(const MediaData& other)
      : filename(other.filename)
      , path(other.path)
      , url(other.url)
      , type(other.type)
      , port(other.port)
      , source(other.source)
      , loaded(other.loaded)
      , nextElement(other.nextElement)
      , buffer(other.buffer)
    {
    }

    /* Takes a standard C string of a filesystem path and populates the struct with the data. */
    MediaData(const char* path)
    {
        /* Path is everything up to the last slash */
        this->path = std::string(path).substr(0, std::string(path).find_last_of("/"));
        if (this->path == "") {
            this->path = "/";
        }

        /* Filename is everything after the last slash */
        this->filename = std::string(path).substr(std::string(path).find_last_of("/") + 1);
        if (this->filename == "") {
            this->filename = "/";
        }

        /* Get the file extension */
        if (this->filename.find(".mp3") != std::string::npos)
            this->type = FILETYPE_MP3;
        else if (this->filename.find(".wav") != std::string::npos)
            this->type = FILETYPE_WAV;
        else if (this->filename.find(".flac") != std::string::npos)
            this->type = FILETYPE_FLAC;
        else if (this->filename.find(".ogg") != std::string::npos)
            this->type = FILETYPE_OGG;
        else if (this->filename.find(".m3u") != std::string::npos)
            this->type = FILETYPE_M3U;
        else
            this->type = FILETYPE_DIR;

        this->url = "";
        this->port = 0;
        this->source = LOCAL_FILE;
        this->loaded = true;
        this->nextElement = 0;
        this->buffer = "";
    }

    /* Comparison operators */
    bool operator==(const MediaData& other) const
    {
        if (this->filename == other.filename && this->path == other.path && this->url == other.url && this->type == other.type && this->port == other.port &&
            this->source == other.source && this->loaded == other.loaded)
            return true;

        else
            return false;
    }

    bool operator!=(const MediaData& other) const
    {
        if (this->filename != other.filename || this->path != other.path || this->url != other.url || this->type != other.type || this->port != other.port ||
            this->source != other.source || this->loaded != other.loaded)
            return true;

        else
            return false;
    }

    /* For passing directly into functions such as sd.open() from SdFat */
    const char* getPath()
    {
        if (this->source == LOCAL_FILE) {
            if (this->path == "/" && this->filename == "/")
                this->buffer = "/";
            else if (this->path != "/")
                this->buffer = this->path + "/" + this->filename;
            else
                this->buffer = this->path + this->filename;
            return this->buffer.c_str();
        } else
            return this->url.c_str();
    }

    /* For conversion to std::string */
    operator std::string() const
    {
        if (this->source == LOCAL_FILE) {
            std::string fullPath;
            if (this->path == "/" && this->filename == "/")
                fullPath = "/";
            else if (this->path != "/")
                fullPath = this->path + "/" + this->filename;
            else
                fullPath = this->path + this->filename;
            return fullPath;
        } else
            return url;
    }
};

/* Takes a single dimensional const char array and allows one to select a row
and column as if it were a 2D array. All iterator values are 0-indexed. */
class TableData
{
  public:
    TableData(const char* const table[], size_t const length, size_t columns)
    { /* Specify the column count in "columns" */
        this->table = table;
        this->columns = columns;
        this->length = length / columns;
    }
    const char* get(size_t row, size_t column) { return table[(row * columns) + column]; }
    uint16_t size() { return length; }

  private:
    const char* const* table;
    size_t columns;
    size_t length;
};

class SystemConfig
{
  public:
    SystemConfig();
    void begin();
    void enableWifi();
    void disableWifi();
    void setWifiSSID(std::string ssid);
    void setWifiPassword(std::string password);
    void enableDHCP();
    void disableDHCP();
    bool setIP(std::string ip);
    bool setNetmask(std::string netmask);
    bool setGateway(std::string gateway);
    bool setDNS(std::string dns);
    bool setNTPServer(std::string ntp_server);
    bool setNTPInterval(uint32_t ntp_interval);
    void updateNTP();
    void setTimezone(std::string timezone);
    void setHostname(std::string hostname);

    std::string getWifiSSID();
    std::string getWifiPassword();
    std::string getIP();
    std::string getNetmask();
    std::string getGateway();
    std::string getDNS();
    std::string getNTPServer();
    uint32_t getNTPInterval();
    std::string getTimezone();
    std::string getHostname();

    std::string getCurrentDateTime(const char* format);
    bool setTime(std::string time);
    bool setDate(std::string date);
    struct tm getDateTime();

    /* Audio settings */
    void setVolume(uint8_t volume)
    {
        this->volume = volume;
        preferences->putInt("volume", volume);
    }
    uint8_t getVolume() { return volume; }

    void setSystemVolume(uint8_t volume)
    {
        system_volume = volume;
        preferences->putInt("system_volume", volume);
    }
    uint8_t getSystemVolume() { return system_volume; }

    void setBass(uint8_t bass)
    {
        eq_bass = bass;
        preferences->putInt("eq_bass", bass);
    }
    uint8_t getBass() { return eq_bass; }

    void setMid(uint8_t mid)
    {
        eq_mid = mid;
        preferences->putInt("eq_mid", mid);
    }
    uint8_t getMid() { return eq_mid; }

    void setTreble(uint8_t treble)
    {
        eq_treble = treble;
        preferences->putInt("eq_treble", treble);
    }
    uint8_t getTreble() { return eq_treble; }

    bool isWifiEnabled();
    bool isDHCPEnabled();

    void enableAlarm();
    void disableAlarm();
    MediaData getAlarmMedia();
    void saveAlarmMedia(MediaData mediadata);
    bool setAlarmTime(std::string time);
    bool setAlarmTime(struct tm time);
    std::string getAlarmTime();
    void getAlarmTime(struct tm* time);
    void resetAlarm();
    bool isAlarmEnabled();
    bool isAlarmTriggered();
    bool isScreenSaverEnabled() { return screensaver_enabled; }
    void setScreenSaverTimeout(uint16_t timeout)
    {
        screensaver_timeout = timeout;
        preferences->putInt("scrnsvr_timeout", timeout);
        screensaver->set_timeout(timeout);
    }
    uint16_t getScreenSaverTimeout() { return screensaver_timeout; }
    void enableScreenSaver()
    {
        screensaver_enabled = true;
        preferences->putBool("scrnsvr_enabled", true);
        screensaver->enable();
    }
    void disableScreenSaver()
    {
        screensaver_enabled = false;
        preferences->putBool("scrnsvr_enabled", false);
        screensaver->disable();
    }

    void resetPreferences(); /* Factory defaults */

  private:
    bool wifi_enabled;
    std::string ssid;
    std::string password;
    bool dhcp;
    std::string ip;
    std::string netmask;
    std::string gateway;
    std::string dns;
    std::string ntp_server;
    uint32_t ntp_interval;
    std::string timezone;
    std::string hostname;
    uint8_t volume;
    uint8_t system_volume;
    uint8_t eq_bass;
    uint8_t eq_mid;
    uint8_t eq_treble;
    uint8_t zipcode;
    bool screensaver_enabled;
    uint8_t screensaver_timeout;

    bool alarm_enabled = false;
    bool alarm_triggered = false;
    struct tm alarm_dateTime;
    MediaData alarm_media; /* Local file/Playlist */

    bool validateIPString(std::string ip);

    Preferences* preferences;
};

void serviceLoop(); /* This function is called in the main loop and in most other loops
                       to handle system services such as Bluetooth, audio playback, etc. */

#endif