/**
 * @file value_selector.cpp
 *
 * @brief Allows the user to select a value from a range. Part of the UI library.
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
#include <ui/value_selector.h>

UI::ValueSelector::ValueSelector(std::string prompt, uint16_t min, uint16_t max, uint16_t step, uint16_t defaultValue)
  : _prompt(prompt)
  , _minVal(min)
  , _maxVal(max)
  , _step(step)
  , _value(defaultValue)
  , useCallbacks(false)

{
}

UI::ValueSelector::ValueSelector(std::string prompt,
                             std::function<uint8_t()> valueCallback,
                             std::function<void()> incCallback,
                             std::function<void()> decCallback,
                             uint8_t minVal,
                             uint8_t maxVal)
  : _prompt(prompt)
  , _valueCallback(valueCallback)
  , _incCallback(incCallback)
  , _decCallback(decCallback)
  , _minVal(minVal)
  , _maxVal(maxVal)
  , useCallbacks(true)

{
}

uint16_t
UI::ValueSelector::get()
{

    if (useCallbacks) {
        _value = _valueCallback();
    }

    draw();

    while (true) {

        if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            return UI_EXIT;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            return UI_EXIT;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            return _value;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, SHORTPRESS)) {
            inc();
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, SHORTPRESS)) {
            dec();
        }

        /* Longpress events */
        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            inc();
            Buttons::get_handle()->repeat(BUTTON_UP);
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            dec();
            Buttons::get_handle()->repeat(BUTTON_DOWN);
        }

        draw();

        if (exitTimer.check(UI_EXIT_TIMEOUT)) {
            return UI_EXIT;
        }
    }
}

void
UI::ValueSelector::inc()
{
    if (useCallbacks) {
        _incCallback();
        _value = _valueCallback();
    } else {
        if (_value < _maxVal)
            _value += _step;
    }
    Transport::get_handle()->playUIsound(click, click_len);
    exitTimer.reset();
}

void
UI::ValueSelector::dec()
{
    if (useCallbacks) {
        _decCallback();
        _value = _valueCallback();
    } else {
        if (_value > _minVal)
            _value -= _step;
    }
    Transport::get_handle()->playUIsound(click, click_len);
    exitTimer.reset();
}

void
UI::ValueSelector::draw()
{
    display->clearDisplay();
    if (Screensaver::get_handle()->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);
    display->setTextColor(WHITE, BLACK);
    display->setCursor(0, 0);
    display->print(_prompt.c_str());
    display->print(": ");
    display->print(_value);

    /* Draw four lines to form the border of the rectangle */
    display->drawFastHLine(0, 12, 127, WHITE);
    display->drawFastHLine(0, 24, 127, WHITE);
    display->drawFastVLine(0, 12, 12, WHITE);
    display->drawFastVLine(127, 12, 12, WHITE);

    /* Fill the rectangle with the current value in reference to the min and max values */
    uint8_t fillWidth = map(_value, _minVal, _maxVal, 0, 127);
    display->fillRect(0, 12, fillWidth, 12, WHITE);

    display->display();
}