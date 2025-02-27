/**
 * @file filebrowser.h
 *
 * @brief File browser for selecting files from the filesystem. Part of the UI library.
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

#ifndef filebrowser_h
#define filebrowser_h

#include <ui/common.h>

class File_Explorer;
class MediaData;
class ListSelection
{
  public:
    struct position
    {
        uint16_t cursor = 0;
        uint32_t page = 1;
        uint32_t index = 0;
    };
};

namespace UI {

/****************************************************
 *
 * File Browser
 *
 ****************************************************/

class FileBrowser
{
  public:
    FileBrowser();

    void begin();

    void end();

    MediaData get();

    void refresh();

    void altMenu();

    bool setRoot(MediaData root);

    ~FileBrowser();

  private:
    std::vector<ListSelection::position> positionHistory;   // Keeps track of the cursor position and page for each
                                                            // directory we visit
    ListSelection::position currentPosition;                // Keeps track of the current position

    SystemMessage _status_message;

    File_Explorer* file_explorer = nullptr;

    std::function<void(uint32_t _count, uint32_t _total)> _status_callback;
    std::function<void()> _alt_menu;
};

}   // namespace UI

#endif