/**
 * @file status.cpp
 *
 * @brief Displays the main status screen. Part of the UI library.
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

#include <ui/common.h>
#include <ui/status.h>

UI::StatusScreen::StatusScreen()
{
    anim_playing = new Animation(bunny_playing, bunny_playing_num_frames);
    anim_playing->setDuration(1000);

    anim_stopped = new Animation(bunny_stopped, bunny_stopped_num_frames);
    uint8_t anim_stopped_sequence[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0 };
    anim_stopped->setSequence(anim_stopped_sequence, 13);
    anim_stopped->setDuration(150);

    /* Set up marquees */
    marquee_mediainfo = new Marquee();
    marquee_mediainfo->setSpeed(200);
    marquee_mediainfo->setSwitchInterval(1000 * 10);
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedFileName, transport), "File:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedURL, transport), "URL:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedArtist, transport), "Artist:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedAlbum, transport), "Album:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedTitle, transport), "Title:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedGenre, transport), "Genre:");

    marquee_datetime = new Marquee();
    marquee_datetime->setSwitchInterval(1000 * 10);
    marquee_datetime->addSource(std::bind(&SystemConfig::getCurrentDateTime, systemConfig, "%H:%M:%S %Z"));
    marquee_datetime->addSource(std::bind(&SystemConfig::getCurrentDateTime, systemConfig, "%a, %b %d, %Y"));

    marquee_connectStatus = new Marquee("Connecting");
    marquee_connectStatus->setSwitchInterval(150);
    marquee_connectStatus->addText("Connecting.");
    marquee_connectStatus->addText("Connecting..");
    marquee_connectStatus->addText("Connecting...");

    /* Set up the spectrum analyzer */
    spectrumAnalyzer = new SpectrumAnalyzer();
}

void
UI::StatusScreen::draw()
{
    display->clearDisplay();
    if (screensaver->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);
    display->setTextColor(WHITE, BLACK);

    // Draw the sprite for the TRANSPORT_PLAYING status
    if (transport->getStatus() == TRANSPORT_PLAYING) {
        anim_playing->draw(0, 0, 20, 20);
    } else {
        anim_stopped->draw(0, 0, 20, 20);
    }

    // Display the filename or info of the currently loaded media
    if (transport->getStatus() != TRANSPORT_IDLE && transport->getStatus() != TRANSPORT_CONNECTING) {
        marquee_mediainfo->draw(0, 24);
    } 
    else if (transport->getStatus() == TRANSPORT_CONNECTING) {
        marquee_connectStatus->draw(0, 24);
    }
    else {
        // Display the current system time and date
        marquee_datetime->draw(0, 24);
    }

    // Draw the volume level using bitmap_volume_muted for 0, bitmap_volume_low for 1-33, bitmap_volume_mid for 34-66, and bitmap_volume_full for 67-100
    if (transport->getVolume() < 3)
        display->drawBitmap(120, 0, bitmap_volume_muted, 7, 7, WHITE);
    else if (transport->getVolume() > 0 && transport->getVolume() <= 33)
        display->drawBitmap(120, 0, bitmap_volume_low, 7, 7, WHITE);
    else if (transport->getVolume() > 33 && transport->getVolume() <= 66)
        display->drawBitmap(120, 0, bitmap_volume_mid, 7, 7, WHITE);
    else
        display->drawBitmap(120, 0, bitmap_volume_full, 7, 7, WHITE);

    // If the playlist is enabled, draw the playlist icon, otherwise draw the note icon
    if (playlistEngine->isEnabled())
        display->drawBitmap(110, 0, bitmap_playlist, 7, 7, WHITE);
    else if (transport->getLoadedMedia().loaded)
        display->drawBitmap(110, 0, bitmap_note, 7, 7, WHITE);

    // Draw the network status
    if (WiFi.status() == WL_CONNECTED)
        display->drawBitmap(100, 0, bitmap_wifi_3, 7, 7, WHITE);
    else if (systemConfig->isWifiEnabled() && WiFi.status() == WL_DISCONNECTED)
        display->drawBitmap(100, 0, bitmap_wifi_error, 7, 7, WHITE);

    // Draw the bluetooth status
    if (bluetooth->getMode() == POWER_ON)
        display->drawBitmap(90, 0, bitmap_bluetooth, 7, 7, WHITE);

    // Draw the spectrum analyzer
    if (spectrumAnalyzer) {
        spectrumAnalyzer->draw(27, 2, 2, 9);
    }

    // Draw the play time
    uint32_t playTime = transport->getPlayTime();
    uint8_t hours = playTime / 3600;
    uint8_t minutes = (playTime % 3600) / 60;
    uint8_t seconds = playTime % 60;

    display->setCursor(23, 14);
    if (hours < 10)
        display->print("0");
    display->print(hours);
    display->print(":");
    if (minutes < 10)
        display->print("0");
    display->print(minutes);
    display->print(":");
    if (seconds < 10)
        display->print("0");
    display->print(seconds);

    display->display();
}