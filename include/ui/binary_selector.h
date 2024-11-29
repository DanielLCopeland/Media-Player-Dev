/**
 * @file binary_selector.h
 *
 * @brief Allows the user to select between two options. Part of the UI library.
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

#ifndef binary_selector_h
#define binary_selector_h

#include <ui/common.h>

namespace UI {

class BinarySelector
{
  public:
    BinarySelector(std::string falseText = "No", std::string trueText = "Yes");
    bool get();

  private:
    std::vector<std::string> _options;
    void draw();
    void toggle();
};

} // namespace UI

#endif