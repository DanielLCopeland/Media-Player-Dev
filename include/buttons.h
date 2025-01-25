/**
 * @file buttons.h
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

#ifndef buttons_h
#define buttons_h

#include <Arduino.h>
#include <screensaver.h>

/* The actual GPIO pins that the buttons are connected to */
#define BUTTON_PIN_UP   16
#define BUTTON_PIN_DOWN 14
#define BUTTON_PIN_PLAY 47
#define BUTTON_PIN_STOP 21
#define BUTTON_PIN_MENU 4
#define BUTTON_PIN_EXIT 5

/* Timing constants */
#define SHORTPRESS_MS 40
#define LONGPRESS_MS  1000

/* Used to determine how often to repeat a longpress event. It will fire every
REPEAT_MS milliseconds after the first longpress event when called in a loop. */
#define REPEAT_MS 100

class Screensaver;
extern Screensaver* screensaver;

enum buttonNames
{
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_PLAY,
    BUTTON_STOP,
    BUTTON_MENU,
    BUTTON_EXIT,
    NUM_BUTTONS
};

enum buttonEvents
{
    SHORTPRESS,
    LONGPRESS
};

class Buttons
{
  private:
    Buttons();
    static Buttons* _handle;

  public:
    Buttons(Buttons const&) = delete;

    /* Returns true if button was pressed and released for SHORTPRESS_MS or LONGPRSS_MS.  Uses millis() so it's non-blocking. Keeps checking in a loop
    to see if it's still pressed after a delay for debouncing or filtering of spurious signals. Variable "button" corresponds to the pin number of the
    button. Variable "type" corresponds to the type of event we're looking for (SHORTPRESS or LONGPRESS). */
    bool getButtonEvent(uint8_t button, uint8_t type);

    /* Returns true if the button is held. Variable "button" corresponds to the pin number of the button. */
    bool isHeld(uint8_t button);

    /* Allows us to trigger a longpress again on each loop once the longpress triggers the first time, and doesn't stop until the button is released.
    Useful for fast scrolling. Adjust delay time with REPEAT_MS. */
    void repeat(uint8_t button);

    static Buttons* get_handle()
    {
        if (!_handle) {
            _handle = new Buttons();
        }
        return _handle;
    }

  private:
    struct button
    {
        bool lastState_shortPress;
        bool lastState_longPress;

        /* Used to prevent multiple short press events from being triggered by a single press */
        bool shortPressLock;

        /* Used to prevent multiple long press events from being triggered by a single press */
        bool longPressLock;
        uint8_t pinNumber;
        uint32_t lastDebounceTime_shortPress;
        uint32_t lastDebounceTime_longPress;

        /* Allows us to trigger a longpress again on each loop once the longpress triggers the first time, and doesn't stop until the button is
        released. */
        bool repeatLock;
        uint32_t lastRepeatTime;
    };

    button buttons[NUM_BUTTONS] = { { true, true, false, false, BUTTON_PIN_UP, 0, 0, 0 },   { true, true, false, false, BUTTON_PIN_DOWN, 0, 0, 0 },
                                    { true, true, false, false, BUTTON_PIN_PLAY, 0, 0, 0 }, { true, true, false, false, BUTTON_PIN_STOP, 0, 0, 0 },
                                    { true, true, false, false, BUTTON_PIN_MENU, 0, 0, 0 }, { true, true, false, false, BUTTON_PIN_EXIT, 0, 0, 0 } };
};

#endif