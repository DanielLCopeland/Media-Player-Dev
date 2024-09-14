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

#include "system.h"

SystemConfig::SystemConfig()
{   
    preferences = new Preferences();
}

void
SystemConfig::begin()
{
    preferences->begin("config", false);

    if (!preferences->getBool("initialized")) {
        preferences->putBool("initialized", true);
        preferences->putBool("wifi_enabled", false);
        preferences->putString("ssid", "");
        preferences->putString("password", "");
        preferences->putBool("dhcp", true);
        preferences->putString("ip", "");
        preferences->putString("netmask", "");
        preferences->putString("gateway", "");
        preferences->putString("dns", "");
        preferences->putString("ntp_server", "pool.ntp.org");
        preferences->putInt("ntp_interval", 60);
        preferences->putString("timezone", "UTC0");
        preferences->putString("hostname", "mediaplayer");
        preferences->putBool("alarm_enabled", false);
        preferences->putInt("alarm_hour", 0);
        preferences->putInt("alarm_minute", 0);
        preferences->putInt("alarm_second", 0);
        preferences->putInt("alarm_day", 0);
        preferences->putInt("alarm_month", 0);
        preferences->putInt("alarm_year", 0);
        preferences->putString("alarm_media_f", "");
        preferences->putString("alarm_media_p", "");
        preferences->putString("alarm_media_u", "");
        preferences->putInt("alarm_media_t", FILETYPE_UNKNOWN);
        preferences->putInt("alarm_media_s", NO_SOURCE_LOADED);
        preferences->putInt("volume", 50);
        preferences->putInt("system_volume", 50);
        preferences->putInt("eq_bass", 50);
        preferences->putInt("eq_mid", 50);
        preferences->putInt("eq_treble", 50);
        preferences->putInt("zipcode", 0);
        preferences->putBool("scrnsvr_enabled", false);
        preferences->putInt("scrnsvr_timeout", 30);
    }

    this->wifi_enabled = preferences->getBool("wifi_enabled");
    this->ssid = preferences->getString("ssid").c_str();
    this->password = preferences->getString("password").c_str();
    this->dhcp = preferences->getBool("dhcp");
    this->ip = preferences->getString("ip").c_str();
    this->netmask = preferences->getString("netmask").c_str();
    this->gateway = preferences->getString("gateway").c_str();
    this->dns = preferences->getString("dns").c_str();
    this->ntp_server = preferences->getString("ntp_server").c_str();
    this->ntp_interval = preferences->getInt("ntp_interval");
    this->timezone = preferences->getString("timezone").c_str();
    this->hostname = preferences->getString("hostname").c_str();
    this->alarm_enabled = preferences->getBool("alarm_enabled");

    this->alarm_dateTime.tm_hour = preferences->getInt("alarm_hour");
    this->alarm_dateTime.tm_min = preferences->getInt("alarm_minute");
    this->alarm_dateTime.tm_sec = preferences->getInt("alarm_second");
    this->alarm_dateTime.tm_mday = preferences->getInt("alarm_day");
    this->alarm_dateTime.tm_mon = preferences->getInt("alarm_month");
    this->alarm_dateTime.tm_year = preferences->getInt("alarm_year");
    this->alarm_media.filename = preferences->getString("alarm_media_f").c_str();
    this->alarm_media.path = preferences->getString("alarm_media_p").c_str();
    this->alarm_media.url = preferences->getString("alarm_media_u").c_str();
    this->alarm_media.type = preferences->getInt("alarm_media_t");
    this->alarm_media.source = preferences->getInt("alarm_media_s");

    if (this->alarm_media.source != NO_SOURCE_LOADED)
        this->alarm_media.loaded = true;
    else
        this->alarm_media.loaded = false;

    this->volume = preferences->getInt("volume");
    this->eq_bass = preferences->getInt("eq_bass");
    this->eq_mid = preferences->getInt("eq_mid");
    this->eq_treble = preferences->getInt("eq_treble");
    this->system_volume = preferences->getInt("system_volume");
    this->zipcode = preferences->getInt("zipcode");
    this->screensaver_enabled = preferences->getBool("scrnsvr_enabled");
    this->screensaver_timeout = preferences->getInt("scrnsvr_timeout");

    /* Set the timezone */
    setenv("TZ", preferences->getString("timezone").c_str(), true);
    tzset();

    /* Set the hostname */
    WiFi.setHostname(this->hostname.c_str());

    /* Start NTP */
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, this->ntp_server.c_str());
    sntp_set_sync_interval(this->ntp_interval * 60000);
    sntp_sync_time(nullptr);
    sntp_init();

    /* Start the WiFi connection */
    if (this->wifi_enabled) {
        enableWifi();
    }

    /* Set the volume stored in preferences */
    extern Transport* transport;
    transport->setVolume(this->volume);
    transport->setSystemVolume(this->system_volume);
    transport->eq->setBass(this->eq_bass);
    transport->eq->setMid(this->eq_mid);
    transport->eq->setTreble(this->eq_treble);

    /* Start the screensaver */
    if (this->screensaver_enabled) {
        screensaver->enable();
    } else {
        screensaver->disable();
    }
}

void
SystemConfig::enableWifi()
{
    WiFi.disconnect();

    if (this->ssid == "") {
        return;
    }

    preferences->putBool("wifi_enabled", true);
    this->wifi_enabled = true;

    /* Set DHCP or static IP */
    if (this->dhcp) {
        WiFi.config(0u, 0u, 0u);
    }

    else {
        IPAddress ip;
        ip.fromString(this->ip.c_str());
        IPAddress gateway;
        gateway.fromString(this->gateway.c_str());
        IPAddress netmask;
        netmask.fromString(this->netmask.c_str());
        IPAddress dns;
        dns.fromString(this->dns.c_str());
        WiFi.config(ip, gateway, netmask, dns);
    }

    WiFi.begin(this->ssid.c_str(), this->password.c_str());
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
}

void
SystemConfig::disableWifi()
{
    preferences->putBool("wifi_enabled", false);
    this->wifi_enabled = false;
    WiFi.disconnect();
    WiFi.setAutoReconnect(false);
    WiFi.setSleep(true);
}

void
SystemConfig::setWifiSSID(std::string ssid)
{
    preferences->putString("ssid", ssid.c_str());
    this->ssid = ssid;
}

void
SystemConfig::setWifiPassword(std::string password)
{
    preferences->putString("password", password.c_str());
    this->password = password;
}

void
SystemConfig::enableDHCP()
{
    preferences->putBool("dhcp", true);
    this->dhcp = true;
}

void
SystemConfig::disableDHCP()
{
    preferences->putBool("dhcp", false);
    this->dhcp = false;
}

bool
SystemConfig::setIP(std::string ip)
{
    if (!validateIPString(ip)) {
        return false;
    }

    preferences->putString("ip", ip.c_str());
    this->ip = ip;
    return true;
}

bool
SystemConfig::setNetmask(std::string netmask)
{
    if (!validateIPString(ip)) {
        return false;
    }

    preferences->putString("netmask", netmask.c_str());
    this->netmask = netmask;
    return true;
}

bool
SystemConfig::setGateway(std::string gateway)
{
    if (!validateIPString(ip)) {
        return false;
    }

    preferences->putString("gateway", gateway.c_str());
    this->gateway = gateway.c_str();
    return true;
}

bool
SystemConfig::setDNS(std::string dns)
{
    if (!validateIPString(ip)) {
        return false;
    }

    preferences->putString("dns", dns.c_str());
    this->dns = dns.c_str();
    return true;
}

bool
SystemConfig::setNTPServer(std::string ntp_server)
{
    /* Check the ntp_server string to see if it looks like a server address and makes sense */
    if (ntp_server.length() < 4) {
        return false;
    }

    std::string disallowed_chars = " ;:/\\,\"\''`~!@#$%^&*()-+=[]{}|<>?";
    for (char c : disallowed_chars) {
        if (ntp_server.find(c) != std::string::npos) {
            return false;
        }
    }

    if (ntp_server.find(".") == std::string::npos) {
        return false;
    }

    preferences->putString("ntp_server", ntp_server.c_str());
    this->ntp_server = ntp_server;
    sntp_setservername(0, this->ntp_server.c_str());
    return true;
}

bool
SystemConfig::setNTPInterval(uint32_t ntp_interval)
{
    /* NTP interval must be between 1 and 1440 minutes */
    if (ntp_interval < 1 || ntp_interval > 1440) {
        return false;
    }

    preferences->putInt("ntp_interval", ntp_interval);
    this->ntp_interval = ntp_interval;
    sntp_set_sync_interval(this->ntp_interval * 60000);
    sntp_sync_time(nullptr);
    return true;
}

void
SystemConfig::updateNTP()
{
    sntp_sync_time(nullptr);
}

void
SystemConfig::setTimezone(std::string timezone)
{
    preferences->putString("timezone", timezone.c_str());
    this->timezone = timezone;
    setenv("TZ", timezone.c_str(), true);
    tzset();
}

bool
SystemConfig::setTime(std::string time)
{
    /* Check the time string */
    if (time.length() != 8) {
        return false;
    }

    /* Check the time string for invalid format */
    if (time.find(":") == std::string::npos) {
        return false;
    }

    /* Check if the hour is valid */
    if (atoi(time.substr(0, 2).c_str()) > 23) {
        return false;
    }

    /* Check if the minute is valid */
    if (atoi(time.substr(3, 2).c_str()) > 59) {
        return false;
    }

    /* Check if the second is valid */
    if (atoi(time.substr(6, 2).c_str()) > 59) {
        return false;
    }

    /* Check the time string for invalid characters */
    std::string disallowed_chars = " ;/\\,\"\''`~!@#$%^&*()-+=[]{}|<>?";
    for (char c : disallowed_chars) {
        if (time.find(c) != std::string::npos) {
            return false;
        }
    }

    if (time.find(":") == std::string::npos) {
        return false;
    }

    struct tm timeinfo;
    timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);

    timeinfo.tm_hour = atoi(time.substr(0, 2).c_str());
    timeinfo.tm_min = atoi(time.substr(3, 2).c_str());
    timeinfo.tm_sec = atoi(time.substr(6, 2).c_str());
    tv.tv_sec = mktime(&timeinfo);
    settimeofday(&tv, nullptr);

    return true;
}

bool
SystemConfig::setDate(std::string date)
{
    /* Check the date string */
    if (date.length() != 10) {
        return false;
    }

    /* Check if the date string is in the format YYYY-MM-DD */
    if (date.substr(4, 1) != "-" || date.substr(7, 1) != "-") {
        return false;
    }

    /* Check if the year does not exceed 2038 (the year 2038 problem for 32-bit date/time systems) */
    if (atoi(date.substr(0, 4).c_str()) > 2038) {
        return false;
    }

    /* Check if the month is between 1 and 12 */
    if (atoi(date.substr(5, 2).c_str()) < 1 || atoi(date.substr(5, 2).c_str()) > 12) {
        return false;
    }

    /* Check if the day is between 1 and 31 for months with 31 days, 1 and 30 for months with 30 days, and 1 and 28 or 29 for February */
    if ((atoi(date.substr(5, 2).c_str()) == 1 || atoi(date.substr(5, 2).c_str()) == 3 || atoi(date.substr(5, 2).c_str()) == 5 ||
         atoi(date.substr(5, 2).c_str()) == 7 || atoi(date.substr(5, 2).c_str()) == 8 || atoi(date.substr(5, 2).c_str()) == 10 ||
         atoi(date.substr(5, 2).c_str()) == 12) &&
        (atoi(date.substr(8, 2).c_str()) < 1 || atoi(date.substr(8, 2).c_str()) > 31)) {
        return false;
    } else if ((atoi(date.substr(5, 2).c_str()) == 4 || atoi(date.substr(5, 2).c_str()) == 6 || atoi(date.substr(5, 2).c_str()) == 9 ||
                atoi(date.substr(5, 2).c_str()) == 11) &&
               (atoi(date.substr(8, 2).c_str()) < 1 || atoi(date.substr(8, 2).c_str()) > 30)) {
        return false;
    } else if (atoi(date.substr(5, 2).c_str()) == 2 &&
               ((atoi(date.substr(0, 4).c_str()) % 4 == 0 && atoi(date.substr(0, 4).c_str()) % 100 != 0) || (atoi(date.substr(0, 4).c_str()) % 400 == 0)) &&
               (atoi(date.substr(8, 2).c_str()) < 1 || atoi(date.substr(8, 2).c_str()) > 29)) {
        return false;
    } else if (atoi(date.substr(5, 2).c_str()) == 2 &&
               ((atoi(date.substr(0, 4).c_str()) % 4 != 0) || (atoi(date.substr(0, 4).c_str()) % 100 == 0 && atoi(date.substr(0, 4).c_str()) % 400 != 0)) &&
               (atoi(date.substr(8, 2).c_str()) < 1 || atoi(date.substr(8, 2).c_str()) > 28)) {
        return false;
    }

    /* Check the date string for invalid characters */
    std::string disallowed_chars = " :;/\\,\"\''`~!@#$%^&*()+=[]{}|<>?";
    for (char c : disallowed_chars) {
        if (date.find(c) != std::string::npos) {
            return false;
        }
    }

    if (date.find("-") == std::string::npos) {
        return false;
    }

    struct tm timeinfo;
    timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);

    timeinfo.tm_year = atoi(date.substr(0, 4).c_str()) - 1900;
    timeinfo.tm_mon = atoi(date.substr(5, 2).c_str()) - 1;
    timeinfo.tm_mday = atoi(date.substr(8, 2).c_str());
    tv.tv_sec = mktime(&timeinfo);
    settimeofday(&tv, nullptr);

    return true;
}

void
SystemConfig::setHostname(std::string hostname)
{
    preferences->putString("hostname", hostname.c_str());
    this->hostname = hostname;
    WiFi.setHostname(hostname.c_str());
}

bool
SystemConfig::isWifiEnabled()
{
    return this->wifi_enabled;
}

std::string
SystemConfig::getWifiSSID()
{
    return this->ssid;
}

std::string
SystemConfig::getWifiPassword()
{
    return this->password;
}

bool
SystemConfig::isDHCPEnabled()
{
    return this->dhcp;
}

std::string
SystemConfig::getIP()
{
    return this->ip;
}

std::string
SystemConfig::getNetmask()
{
    return this->netmask;
}

std::string
SystemConfig::getGateway()
{
    return this->gateway;
}

std::string
SystemConfig::getDNS()
{
    return this->dns;
}

std::string
SystemConfig::getNTPServer()
{
    return this->ntp_server;
}

uint32_t
SystemConfig::getNTPInterval()
{
    return this->ntp_interval;
}

std::string
SystemConfig::getTimezone()
{
    return this->timezone;
}

std::string
SystemConfig::getHostname()
{
    return this->hostname;
}

std::string
SystemConfig::getCurrentDateTime(const char* format)
{
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    time(&now);
    localtime_r(&now, &timeinfo);
    uint8_t size = strftime(strftime_buf, sizeof(strftime_buf), format, &timeinfo);

    /* Trim the buffer to the size of the string */
    strftime_buf[size + 1] = '\0';

    return strftime_buf;
}

bool
SystemConfig::isAlarmEnabled()
{
    return this->alarm_enabled;
}

void
SystemConfig::enableAlarm()
{
    preferences->putBool("alarm_enabled", true);
    this->alarm_enabled = true;
}

void
SystemConfig::disableAlarm()
{
    preferences->putBool("alarm_enabled", false);
    this->alarm_enabled = false;
}

bool
SystemConfig::setAlarmTime(std::string time)
{
    struct tm timeinfo;
    timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);

    /* Check the time string */
    if (time.length() != 8) {
        return false;
    }

    /* Check the time string for invalid characters */
    std::string disallowed_chars = " ;/\\,\"\''`~!@#$%^&*()-+=[]{}|<>?";
    for (char c : disallowed_chars) {
        if (time.find(c) != std::string::npos) {
            return false;
        }
    }

    if (time.find(":") == std::string::npos) {
        return false;
    }

    timeinfo.tm_hour = atoi(time.substr(0, 2).c_str());
    timeinfo.tm_min = atoi(time.substr(3, 2).c_str());
    timeinfo.tm_sec = atoi(time.substr(6, 2).c_str());

    /* If the alarm time is in the past for today, add a day */
    if (timeinfo.tm_hour < timeinfo.tm_hour || (timeinfo.tm_hour == timeinfo.tm_hour && timeinfo.tm_min < timeinfo.tm_min) ||
        (timeinfo.tm_hour == timeinfo.tm_hour && timeinfo.tm_min == timeinfo.tm_min && timeinfo.tm_sec <= timeinfo.tm_sec)) {
        timeinfo.tm_mday = timeinfo.tm_mday + 1;
        timeinfo.tm_mon = timeinfo.tm_mon;
        timeinfo.tm_year = timeinfo.tm_year;
    } else {
        timeinfo.tm_mday = timeinfo.tm_mday;
        timeinfo.tm_mon = timeinfo.tm_mon;
        timeinfo.tm_year = timeinfo.tm_year;
    }

    this->alarm_dateTime = timeinfo;
    preferences->putInt("alarm_hour", timeinfo.tm_hour);
    preferences->putInt("alarm_minute", timeinfo.tm_min);
    preferences->putInt("alarm_second", timeinfo.tm_sec);
    preferences->putInt("alarm_day", timeinfo.tm_mday);
    preferences->putInt("alarm_month", timeinfo.tm_mon);
    preferences->putInt("alarm_year", timeinfo.tm_year);
    return true;
}

std::string
SystemConfig::getAlarmTime()
{
    char time[9];
    sprintf(time, "%02d:%02d:%02d", this->alarm_dateTime.tm_hour, this->alarm_dateTime.tm_min, this->alarm_dateTime.tm_sec);
    return time;
}

void
SystemConfig::getAlarmTime(struct tm* time)
{
    *time = this->alarm_dateTime;
}

bool
SystemConfig::setAlarmTime(struct tm time)
{
    timeval tv;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &time);

    /* If the alarm time is in the past for today, add a day */
    if (time.tm_hour < time.tm_hour || (time.tm_hour == time.tm_hour && time.tm_min < time.tm_min) ||
        (time.tm_hour == time.tm_hour && time.tm_min == time.tm_min && time.tm_sec <= time.tm_sec)) {
        time.tm_mday = time.tm_mday + 1;
        time.tm_mon = time.tm_mon;
        time.tm_year = time.tm_year;
    } else {
        time.tm_mday = time.tm_mday;
        time.tm_mon = time.tm_mon;
        time.tm_year = time.tm_year;
    }

    this->alarm_dateTime = time;
    preferences->putInt("alarm_hour", time.tm_hour);
    preferences->putInt("alarm_minute", time.tm_min);
    preferences->putInt("alarm_second", time.tm_sec);
    preferences->putInt("alarm_day", time.tm_mday);
    preferences->putInt("alarm_month", time.tm_mon);
    preferences->putInt("alarm_year", time.tm_year);
    return true;
}

MediaData
SystemConfig::getAlarmMedia()
{
    return this->alarm_media;
}

void
SystemConfig::saveAlarmMedia(MediaData mediadata)
{
    preferences->putString("alarm_media_f", mediadata.filename.c_str());
    preferences->putString("alarm_media_p", mediadata.path.c_str());
    preferences->putString("alarm_media_u", mediadata.url.c_str());
    preferences->putInt("alarm_media_t", mediadata.type);
    preferences->putInt("alarm_media_s", mediadata.source);
    this->alarm_media = mediadata;
}

bool
SystemConfig::validateIPString(std::string ip)
{
    IPAddress address;

    if (address.fromString(ip.c_str())) {
        /* Check the first octet */
        if (address[0] == 0 || address[0] > 255) {
            return false;
        }

        /* Check the next three octets */
        for (uint8_t i = 1; i < 4; i++) {
            if (address[i] > 255) {
                return false;
            }
        }

        return true;
    }

    return false;
}

void
SystemConfig::resetPreferences()
{
    preferences->clear();
    ESP.restart();
}

void
serviceLoop()
{
    if (systemConfig) {

        /* Check for Bluetooth data */
        if (bluetooth) {
            bluetooth->loop();
        }

        /* Check for sd card insertion or removal */
        if (sdfs) {
            sdfs->check_card_detect();
        }

        /* Maintain audio transport */
        if (playlistEngine) {
            playlistEngine->loop();
        }
        if (transport) {
            transport->loop();
        }

        /* Screen saver */
        if (screensaver) {
            screensaver->loop();
        }

        /* Check alarms */
        if (systemConfig->isAlarmEnabled()) {
            tm currentTime;
            tm alarmTime;
            systemConfig->getAlarmTime(&alarmTime);
            timeval tv;
            gettimeofday(&tv, NULL);
            localtime_r(&tv.tv_sec, &currentTime);

            if (currentTime.tm_hour == alarmTime.tm_hour && currentTime.tm_min == alarmTime.tm_min && currentTime.tm_sec == alarmTime.tm_sec &&
                currentTime.tm_mday == alarmTime.tm_mday) {
                if (transport->load(systemConfig->getAlarmMedia())) {
                    transport->play();
                }
            }

            /* If the alarm time is in the past, reset it and add a day if necessary */
            if (mktime(&alarmTime) <= mktime(&currentTime)) {
                if (alarmTime.tm_hour < currentTime.tm_hour || (alarmTime.tm_hour == currentTime.tm_hour && alarmTime.tm_min < currentTime.tm_min) ||
                    (alarmTime.tm_hour == currentTime.tm_hour && alarmTime.tm_min == currentTime.tm_min && alarmTime.tm_sec <= currentTime.tm_sec)) {
                    alarmTime.tm_mday = currentTime.tm_mday + 1;
                    alarmTime.tm_mon = currentTime.tm_mon;
                    alarmTime.tm_year = currentTime.tm_year;
                } else {
                    alarmTime.tm_mday = currentTime.tm_mday;
                    alarmTime.tm_mon = currentTime.tm_mon;
                    alarmTime.tm_year = currentTime.tm_year;
                }
                systemConfig->setAlarmTime(alarmTime);
            }
        }
    }
}