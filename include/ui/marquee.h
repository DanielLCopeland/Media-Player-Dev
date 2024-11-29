/**
 * @file marquee.h
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

#ifndef marquee_h
#define marquee_h

#include <ui/common.h>

namespace UI {

class Marquee

{
  public:
    Marquee()
      : dynamic(true)
    {
    }
    Marquee(std::string text);

    void draw(uint16_t x, uint16_t y, uint16_t width = Constants::MAX_DISPLAYED_CHARACTERS);
    void addSource(std::function<std::string()> source, std::string label = "");
    void addText(std::string text);
    void setSpeed(uint16_t speed);
    void setSwitchInterval(uint16_t interval);

  private:
    void refresh();
    void rotateText();
    bool dynamic = false;
    struct message
    {
        std::string label;
        std::string text;           /* The text to display, we save this so we can tell if the text has changed on the next refresh */
        std::string displayed_text; /* We rotate the displayed text to create the marquee effect */
        std::function<std::string()> source;
    };
    std::vector<message> _messages;
    uint8_t currentMessage = 0;
    uint16_t _speed = 100;
    uint16_t _switchInterval = 5000;
    Timer _animationTimer;
    Timer _switchTimer;
};

} // namespace UI

#endif