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
#include <functional>

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
    std::function<void(std::vector<MediaData>*, uint32_t, uint32_t)> _get_list;

  public:
    ListSelection();
    ~ListSelection();

    struct position
    {
        position() { cursor = 0; page = 1; index = 0; };
        uint16_t cursor = 0;
        uint32_t page = 1;
        uint32_t index = 0;
    };

    void refresh() { 
        _refresh = true;
    }

    /* Overloads for different data sources */
    int32_t get(const char* const menuItems[], uint16_t numItems);
    int32_t get(std::vector<std::string>& listItems);

    /* Returns the index of the selected item from a PlaylistEngine object.
    If playlist_showindex is true, the index of the currently selected track
    will be highlighted in the list. */
    int32_t get(PlaylistEngine* playlist, bool playlist_showindex = false);
    // uint16_t get(File_Explorer* file_explorer);

    template<typename T>
    int32_t get(T* _object, std::function<void()> callback = nullptr)
    {
        menu_type = MENU_TYPE_CUSTOM;
        numItems = _object->size();
        if (callback) {
            _callback = callback;
        } else {
            _callback = nullptr;
        }
        _get_list = [_object](std::vector<MediaData>* data, uint32_t index, uint32_t count) { _object->get_list(data, index, count); };
        return _get();
    }

    std::string getSelected() { return selected_item; }

    uint16_t cursor_position() { return current_position.cursor; }
    uint16_t selected_index() { return current_position.index; }
    uint16_t current_page() { return current_position.page; }
    position get_position() { return current_position; }
    void set_position(position pos) { current_position = pos; }
    void reset_position()
    {
        current_position.cursor = 0;
        current_position.page = 1;
        current_position.index = 0;
        displayedItems.clear();
        _refresh = true;
    }

  private:
    int32_t _get();
    std::function<void()> _callback = nullptr;
    bool _refresh = true;
    position current_position;
    const char* const* menuItems = nullptr;
    std::vector<std::string>* listItems_ptr = nullptr;
    std::vector<MediaData> displayedItems;
    TableData* table = nullptr;
    PlaylistEngine* _playlist_engine = nullptr;
    bool _playlist_showindex = false;
    uint32_t defaultIndex = 0;
    uint32_t numItems = 0;
    uint8_t menu_type;
    void draw();
    void cursorUp();
    void cursorDown();
    std::vector<MediaData> getDisplayedItems();
    std::string selected_item;
    Marquee* marquee = nullptr;
    File_Explorer* _file_explorer = nullptr;
    uint32_t numPages();
};

}   // namespace UI

#endif