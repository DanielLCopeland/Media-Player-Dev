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
    struct fileBrowserPosition
    {
        uint8_t cursor = 0;                // The cursor position in the file list. 0 indexed
        uint32_t page = 1;                 // The page of the file list we are on. 1 indexed
        uint32_t selectedFile_index = 0;   // The index of the currently selected file. 0 indexed
    };

    std::vector<fileBrowserPosition> positionHistory;   // Keeps track of the cursor position and page for each
                                                        // directory we visit
    fileBrowserPosition currentPosition;                // Keeps track of the current position
                                                        // and page in the file list
    uint32_t lastPage = -1;                             // The last page we were on.  Used to indicate if we need to retrieve
                                                        // a fresh set of files from the filesystem to display
    std::string selectedFile = "";                      // The currently selected file

    File_Explorer* file_explorer = nullptr;

    uint8_t sort_order;   // The sort order for the file list
    uint8_t  sort_type;          // The sort type for the file list

    uint32_t numFiles = 0;   // The number of files in the current directory

    std::vector<MediaData> files;   // The files to be displayed in the file browser

    // Returns the number of pages to display by dividing the number of total
    // files in the directory into pages. The number of files per page is
    // determined by MAX_TEXT_LINES
    uint32_t numPages();

    void cursorUp();     // Moves selection cursor up and keeps it within bounds of
                         // the number of pages and files in a directory
    void cursorDown();   // Moves selection cursor down and keeps it within bounds
                         // of the number of pages and files in a directory

    void draw();   // Draws the file browser

    SystemMessage _status_message;
    std::function<void(uint32_t _count, uint32_t _total)> _status_callback;
};

} // namespace UI

#endif