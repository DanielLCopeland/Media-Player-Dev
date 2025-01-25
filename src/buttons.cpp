/**
 * @file buttons.cpp
 *
 * @brief Manages button events
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

#include <buttons.h>

Buttons* Buttons::_handle = nullptr;

Buttons::Buttons()
{
    pinMode(BUTTON_PIN_UP, INPUT_PULLUP);
    pinMode(BUTTON_PIN_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_PIN_PLAY, INPUT_PULLUP);
    pinMode(BUTTON_PIN_STOP, INPUT_PULLUP);
    pinMode(BUTTON_PIN_MENU, INPUT_PULLUP);
    pinMode(BUTTON_PIN_EXIT, INPUT_PULLUP);
}

bool
Buttons::getButtonEvent(uint8_t button, uint8_t type)
{
    switch (type) {
        
        case SHORTPRESS:

            /* Read the button, determine if we need to start a debounce timer, and use the shortPressLock to prevent multiple events from firing. */
            if (!digitalRead(buttons[button].pinNumber) && buttons[button].lastState_shortPress && !buttons[button].shortPressLock) {
                buttons[button].lastDebounceTime_shortPress = millis();
                buttons[button].shortPressLock = true;
            }

            /* If the button is still pressed after SHORTPRESS_MS, then we can assume it's a short press event. */
            if (!digitalRead(buttons[button].pinNumber) && buttons[button].lastState_shortPress &&
                (millis() - buttons[button].lastDebounceTime_shortPress > SHORTPRESS_MS)) {
                buttons[button].lastState_shortPress = false;
                buttons[button].shortPressLock = false;
                if (!screensaver->is_blanked()) {
                    screensaver->reset();
                    return true;
                }
                else {
                    screensaver->reset();
                    return false;
                }
            }

            /* If the button is released, then we can assume it's not a short press event. */
            if (digitalRead(buttons[button].pinNumber)) {
                buttons[button].lastState_shortPress = true;
                buttons[button].shortPressLock = false;
            }

            return false;

        case LONGPRESS:

            /* Read the button, determine if we need to start a debounce timer, and use the longPressLock to prevent multiple events from firing. */
            if (!digitalRead(buttons[button].pinNumber) && buttons[button].lastState_longPress && !buttons[button].longPressLock) {
                buttons[button].lastDebounceTime_longPress = millis();
                buttons[button].longPressLock = true;
            }

            /* If the button is still pressed after LONGPRESS_MS, then we can assume it's a long press event. */
            if (!digitalRead(buttons[button].pinNumber) && buttons[button].lastState_longPress &&
                (millis() - buttons[button].lastDebounceTime_longPress > LONGPRESS_MS)) {
                buttons[button].lastState_longPress = false;
                buttons[button].longPressLock = false;
                if (!screensaver->is_blanked()) {
                    screensaver->reset();
                    return true;
                }
                else {
                    screensaver->reset();
                    return false;
                }
            }

            /* If the button is released, then we can assume it's not a long press event. */
            if (digitalRead(buttons[button].pinNumber)) {
                buttons[button].lastState_longPress = true;
                buttons[button].longPressLock = false;
                buttons[button].repeatLock = false;
            }

            if (buttons[button].repeatLock && !digitalRead(buttons[button].pinNumber)) {
                if (millis() - buttons[button].lastRepeatTime > REPEAT_MS) {
                    return true;
                } else
                    return false;
            }

            return false;

        default:

            return false;
            break;
    }
}

void
Buttons::repeat(uint8_t button)
{
    /* Use repeatLock to allow us to trigger a longpress again on each loop once
    the longpress triggers the first time, and doesn't stop until the button is
    released. */
    buttons[button].lastRepeatTime = millis();
    buttons[button].repeatLock = true;
}

bool
Buttons::isHeld(uint8_t button)
{
    return !digitalRead(buttons[button].pinNumber);
}