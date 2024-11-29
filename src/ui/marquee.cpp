/**
 * @file marquee.cpp
 *
 * @brief Displays scrolling text. Part of the UI library.
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
#include <ui/marquee.h>

UI::Marquee::Marquee(std::string text)
{
    message newMessage;
    newMessage.text = text;
    newMessage.displayed_text = text;
    /* If the text is longer than the display, add a space to the end to create a gap */
    if (text.length() > MAX_DISPLAYED_CHARACTERS) {
        newMessage.displayed_text.append("   ");
    }
    _messages.push_back(newMessage);
}

void
UI::Marquee::addSource(std::function<std::string()> source, std::string label)
{

    if (!dynamic) {
        return;
    }

    message newMessage;
    newMessage.source = source;
    newMessage.label = label;
    _messages.push_back(newMessage);
}

void
UI::Marquee::addText(std::string text)
{
    if (dynamic) {
        return;
    }
    message newMessage;
    newMessage.text = text;
    newMessage.displayed_text = text;
    if (text.length() > MAX_DISPLAYED_CHARACTERS) {
        newMessage.displayed_text.append("   ");
    }
    _messages.push_back(newMessage);
}

void
UI::Marquee::setSpeed(uint16_t speed)
{
    _speed = speed;
}

void
UI::Marquee::setSwitchInterval(uint16_t interval)
{
    _switchInterval = interval;
}

void
UI::Marquee::draw(uint16_t x, uint16_t y, uint16_t width)
{
    if (_messages.size() == 0) {
        return; /* No messages to display */
    }

    refresh();

    size_t count = _messages.size();
    while (_messages[currentMessage].displayed_text == "" && count) {

        currentMessage++;
        if (currentMessage >= _messages.size()) {
            currentMessage = 0;
        }
        count--;
        refresh();
    }

    if (count == 0) {
        return; /* No messages to display */
    }

    std::string _displayed_text = _messages[currentMessage].displayed_text;

    /* Trim the string to the width given */
    if (_displayed_text.length() > width) {
        _displayed_text = _displayed_text.substr(0, width);
    }

    display->setCursor(x, y);
    display->print((_messages[currentMessage].label + _displayed_text).c_str());

    /* Add spaces to fill out the rest of the display, in case this class is being used as a cursor */
    if (_displayed_text.length() < MAX_DISPLAYED_CHARACTERS + 2) {
        for (uint8_t i = 0; i < MAX_DISPLAYED_CHARACTERS + 2 - _displayed_text.length(); i++) {
            display->print(" ");
        }
    }

    rotateText();

    if (_switchTimer.check(_switchInterval)) {
        currentMessage++;
        if (currentMessage >= _messages.size()) {
            currentMessage = 0;
        }
    }
}

void
UI::Marquee::refresh()
{
    if (!dynamic) {
        return;
    }
    if (_messages.size() == 0) {
        return;
    }

    /* Check the source for new data */
    if (_messages[currentMessage].source != nullptr) {
        std::string newText = _messages[currentMessage].source();
        if (newText != _messages[currentMessage].text) {
            _messages[currentMessage].text = newText;
            _messages[currentMessage].displayed_text = newText;
            if (newText.length() > MAX_DISPLAYED_CHARACTERS - _messages[currentMessage].label.length()) {
                _messages[currentMessage].displayed_text.append("   ");
            }
        }
    }
}

void
UI::Marquee::rotateText()
{
    if (_messages.size() == 0) {
        return;
    }

    if (_animationTimer.check(_speed)) {
        if (_messages[currentMessage].text.length() > MAX_DISPLAYED_CHARACTERS - _messages[currentMessage].label.length()) {
            std::rotate(_messages[currentMessage].displayed_text.begin(),
                        _messages[currentMessage].displayed_text.begin() + 1,
                        _messages[currentMessage].displayed_text.end());
        }
    }
}