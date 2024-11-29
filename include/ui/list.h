/**
 * @file list.h
 *
 * @brief Allows the user to select an item from a list. Part of the UI library.
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

#ifndef list_h
#define list_h

#include <ui/common.h>

class TableData;
class PlaylistEngine;

namespace UI {

enum menuType
{
    MENU_TYPE_CONST_CHAR,
    MENU_TYPE_STRING_VECTOR,
    MENU_TYPE_DATATABLE,
    MENU_TYPE_PLAYLIST
};

class ListSelection
{
  public:
    ListSelection();
    ~ListSelection();

    /* Overloads for different data sources */
    uint16_t get(const char* const menuItems[], uint16_t numItems);
    uint16_t get(std::vector<std::string>& listItems);

    /* Returns the index of the selected item from a TableData object.  The
    TableData object must be passed by reference. Originaly built to parse
    the contents of a timezone array in timezones.h */
    uint16_t get(TableData& table);

    /* Returns the index of the selected item from a PlaylistEngine object.
    If playlist_showindex is true, the index of the currently selected track
    will be highlighted in the list. */
    uint16_t get(PlaylistEngine* playlist, bool playlist_showindex = false);

    std::string getSelected() { return selected_item; }

  private:
    uint16_t get();
    uint16_t selectedIndex = 0;
    uint16_t cursor = 0;
    uint16_t page = 1;
    const char* const* menuItems = nullptr;
    std::vector<std::string>* listItems_ptr = nullptr;
    TableData* table = nullptr;
    PlaylistEngine* _playlist_engine = nullptr;
    bool _playlist_showindex = false;
    uint16_t defaultIndex = 0;
    uint16_t numItems = 0;
    uint8_t menuType;
    void draw();
    void cursorUp();
    void cursorDown();
    std::vector<std::string> getDisplayedItems();
    std::string selected_item = "";
    Marquee* marquee = nullptr;
    uint16_t numPages();
};

} // namespace UI

#endif