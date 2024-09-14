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
    if (transport->getStatus() == TRANSPORT_PLAYING && transport->getLoadedMedia().source == REMOTE_FILE) {
        transport->stop();
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
    systemConfig->setIP(WiFi.localIP().toString().c_str());
    systemConfig->setNetmask(WiFi.subnetMask().toString().c_str());
    systemConfig->setGateway(WiFi.gatewayIP().toString().c_str());
    systemConfig->setDNS(WiFi.dnsIP().toString().c_str());
}

void
onWifiLostIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (transport->getStatus() == TRANSPORT_PLAYING && transport->getLoadedMedia().source == REMOTE_FILE)
        transport->stop();
    log_e("WiFi disconnected! Stopping network streams!");
}

void
onWifiFailed(WiFiEvent_t event, WiFiEventInfo_t info)
{
    log_e("WiFi connection failed!");
}