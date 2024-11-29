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

#ifndef notification_h
#define notification_h

#include <ui/common.h>

namespace UI {

class SystemMessage
{
  public:
    SystemMessage();

    void show(std::string message, uint16_t duration, bool animated);
    void resetAnimation();   // Resets the animation frame to 0

  private:
    Timer notificationTimer;
    Timer animationTimer;
    std::string message = "";
    uint16_t duration = 0;
    uint8_t animationFrame = 0;   // The current frame of the animation, 0-20

    void draw();
};

} // namespace UI

#endif
