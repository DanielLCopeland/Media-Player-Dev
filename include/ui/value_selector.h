/**
 * @file value_selector.h
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

#ifndef value_selector_h
#define value_selector_h

#include <ui/common.h>

namespace UI {

class ValueSelector
{
  public:
    ValueSelector(std::string prompt, uint16_t minVal = 0, uint16_t maxVal = 100, uint16_t step = 1, uint16_t value = 0);

    ValueSelector(std::string prompt,
                  std::function<uint8_t()> valueCallback = NULL,
                  std::function<void()> incCallback = NULL,
                  std::function<void()> decCallback = NULL,
                  uint8_t minVal = 0,
                  uint8_t maxVal = 100);

    uint16_t get();

  private:
    std::string _prompt;
    uint16_t _value = 0;
    uint16_t cursor = 0;
    uint16_t _minVal = 0;
    uint16_t _maxVal = 0;
    uint16_t _step = 0;
    std::function<uint8_t()> _valueCallback;
    std::function<void()> _incCallback;
    std::function<void()> _decCallback;
    bool useCallbacks = false;
    Timer exitTimer;

    void draw();
    void inc();
    void dec();
};

} // namespace UI

#endif