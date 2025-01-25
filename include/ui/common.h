/**
 * @file common.h
 *
 * @brief Common headers for the UI library.
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

#ifndef common_h
#define common_h

#include <system.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <buttons.h>
#include <data.h>
#include <gfx.h>
#include <playlist_engine.h>
#include <screensaver.h>
#include <string>
#include <timer.h>
#include <transport.h>
#include <ui_sounds.h>
#include <card_manager.h>
#include <vector>
#include <functional>
#include <type_traits>
#include <typeinfo>

#include <ui/constants.h>
#include <ui/animation.h>
#include <ui/binary_selector.h>
#include <ui/input.h>
#include <ui/marquee.h>
#include <ui/list.h>
#include <ui/notification.h>
#include <ui/filebrowser.h>
#include <ui/spectrum_analyzer.h>
#include <ui/status.h>
#include <ui/value_indicator.h>
#include <ui/value_selector.h>

class Adafruit_SSD1306;
class Card_Manager;
class Transport;
class Buttons;
class Screensaver;

extern Adafruit_SSD1306* display;
extern Screensaver* screensaver;

#endif

