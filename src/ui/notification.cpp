/**
 * @file notification.h
 *
 * @brief Displays a notification message. Part of the UI library.
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
#include <ui/notification.h>

UI::SystemMessage::SystemMessage() {}

void
UI::SystemMessage::show(std::string message, uint16_t duration, bool animated)
{
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextWrap(false);
    display->setTextColor(WHITE, BLACK);
    display->setCursor(0, 0);
    display->print(message.c_str());

    serviceLoop();

    if (animated) {
        if (animationTimer.check(NOTIFICATION_ANIMATION_FRAME_DURATION)) {
            animationFrame++;
            if (animationFrame > 3)
                animationFrame = 0;

            switch (animationFrame) {
                case 0:
                    display->display();
                    break;
                case 1:
                    display->print(".");
                    display->display();
                    break;
                case 2:
                    display->print("..");
                    display->display();
                    break;
                case 3:
                    display->print("...");
                    display->display();
                    break;
            }
        }

    }

    else {
        display->display();
        notificationTimer.reset();
    }

    if (duration > 0 && !animated)
        while (!notificationTimer.check(duration)) {
            serviceLoop();

            // If any of the buttons are pushed, immediately exit the notification
            if (buttons->getButtonEvent(BUTTON_PLAY, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_STOP, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_UP, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_DOWN, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_MENU, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_EXIT, SHORTPRESS && !buttons->isHeld(BUTTON_MENU))) {
                notificationTimer.reset();
                return;
            }
        }

    notificationTimer.reset();
}

void
UI::SystemMessage::resetAnimation()
{
    animationFrame = 0;
}