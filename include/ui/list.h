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

#include <file_explorer.h>
#include <ui/common.h>

class TableData;
class PlaylistEngine;
class File_Explorer;
class MediaData;

namespace UI {
enum menuType
{
    MENU_TYPE_CONST_CHAR,
    MENU_TYPE_STRING_VECTOR,
    MENU_TYPE_DATATABLE,
    MENU_TYPE_PLAYLIST,
    MENU_TYPE_FILE_EXPLORER,
    MENU_TYPE_CUSTOM
};

class ListSelection
{
  private:
    std::function<void(std::vector<std::string>*, uint32_t, uint32_t, uint8_t, uint8_t)> _get_list;

  public:
    ListSelection();
    ~ListSelection();

    /* Overloads for different data sources */
    uint16_t get(const char* const menuItems[], uint16_t numItems);
    uint16_t get(std::vector<std::string>& listItems);

    /* Returns the index of the selected item from a PlaylistEngine object.
    If playlist_showindex is true, the index of the currently selected track
    will be highlighted in the list. */
    uint16_t get(PlaylistEngine* playlist, bool playlist_showindex = false);
    uint16_t get(File_Explorer* file_explorer);

    template<typename T>
    uint16_t get(T* _object, bool show_index = false, bool show_icons = false)
    {
        menu_type = MENU_TYPE_CUSTOM;
        numItems = _object->size();
        selectedIndex = 0;
        page = 1;
        cursor = 0;
            _get_list = [_object](std::vector<std::string>* data, uint32_t index, uint32_t count, uint8_t sort_order, uint8_t sort_type) {
                _object->get_list(data, index, count, sort_order, sort_type);
            };
        return _get();
    }

    std::string getSelected() { return selected_item; }
    std::vector<MediaData> get_mediadata_list() { return _mediadata_list; }

  private:
    uint16_t _get();
    uint16_t selectedIndex = 0;
    uint16_t cursor = 0;
    uint16_t page = 1;
    const char* const* menuItems = nullptr;
    std::vector<std::string>* listItems_ptr = nullptr;
    std::vector<MediaData> _mediadata_list;
    TableData* table = nullptr;
    PlaylistEngine* _playlist_engine = nullptr;
    bool _playlist_showindex = false;
    uint16_t defaultIndex = 0;
    uint16_t numItems = 0;
    uint8_t menu_type;
    void draw();
    void cursorUp();
    void cursorDown();
    std::vector<std::string> getDisplayedItems();
    std::string selected_item = "";
    Marquee* marquee = nullptr;
    File_Explorer* _file_explorer = nullptr;
    uint16_t numPages();
};

}   // namespace UI

#endif