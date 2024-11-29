/**
 * @file card_manager.cpp
 *
 * @brief Keeps track of the insertion status of the SD card and
 * initializes the SD card when it is inserted.
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

#include <card_manager.h>

void
CardManager::begin()
{
    pinMode(CARD_DETECT_PIN, INPUT_PULLUP);
    log_i("Card detect pin set to %d", CARD_DETECT_PIN);
    /* Initialize the SD card here */
    if (!digitalRead(CARD_DETECT_PIN)) {
        if (!SdFs::begin(SD_CONFIG)) {
            log_e("SD Card initialization failed!");
            isReadyFlag = false;
        } else {
            log_i("Card inserted.");
            isReadyFlag = true;   /* Update isReady based on successful initialization */
        }
    }
}

void
CardManager::update()
{
}

bool
CardManager::isReady()
{
    return isReadyFlag;
}

bool
CardManager::check_card_detect()
{
    static unsigned long lastDebounceTime = 0;
    bool currentState = digitalRead(CARD_DETECT_PIN);

    /* Check if the pin state has changed */
    if (currentState != lastState) {
        lastDebounceTime = millis();   /* Reset debounce timer */
    }

    /* Check if the current state has been stable for longer than the debounce period */
    if ((millis() - lastDebounceTime) >= (currentState == false ? INSERTION_DEBOUNCE_MS : REMOVAL_DEBOUNCE_MS)) {
        // If the card was inserted and is now ready
        if (currentState == false && !isReadyFlag) {
            // Initialize the SD card here
            if (!SdFs::begin(SD_CONFIG)) {
                log_e("SD Card initialization failed!");
                isReadyFlag = false;
            } else {
                log_i("Card inserted.");
                isReadyFlag = true;   /* Update isReady based on successful initialization */
            }
        }
        /* If the card was remove */
        else if (currentState == true && isReadyFlag) {
            /* End the use of the SD card here */
            if (transport->getLoadedMedia().source == LOCAL_FILE){
                transport->stop();
                transport->eject();
            }
            playlistEngine->eject();
            SdFs::end();
            log_i("Card removed.");
            isReadyFlag = false;   /* Update isReady based on the card being removed */
        }
    }

    lastState = currentState;   /* Update lastState for the next call */

    return isReady();
}