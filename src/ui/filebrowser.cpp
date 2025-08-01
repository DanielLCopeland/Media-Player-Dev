/**
 * @file filebrowser.cpp
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

#include <ui/common.h>

UI::FileBrowser::FileBrowser()
{
    _status_callback = [this](uint32_t count, uint32_t total) {
        if (total != 0) {
            _status_message.show("Building database:\n" + std::to_string(count) + " of " + std::to_string(total) + "\nfiles indexed", 0, false);
        } else {
            _status_message.show("Scanning files:\n" + std::to_string(count) + "\n files found", 0, false);
        }
    };
    _alt_menu = [this]() { altMenu(); };
}

void
UI::FileBrowser::begin()
{
    if (file_explorer == nullptr && Card_Manager::get_handle()->isReady()) {
        file_explorer = new File_Explorer();
        file_explorer->init(_status_callback);
    }
}

void
UI::FileBrowser::end()
{
    if (file_explorer != nullptr) {
        delete file_explorer;
        file_explorer = nullptr;
    }
}

void
UI::FileBrowser::refresh()
{
    MediaData mediadata;
    file_explorer->get_current_dir(mediadata);
    file_explorer->generate_index(mediadata, _status_callback);
}

MediaData
UI::FileBrowser::get()
{
    Transport::get_handle()->playUIsound(folder_open, folder_open_len);
    
    if (file_explorer == nullptr) {
        file_explorer = new File_Explorer();
        file_explorer->init(_status_callback);
        positionHistory.clear();
    }
    ListSelection listSelection;

    while (true) {
        int32_t selection = listSelection.get(file_explorer, _alt_menu);

        if (selection == UI_BACK) {
            if (positionHistory.empty()) {
                return MediaData();
            }

            if (file_explorer->exit_dir() == File_Explorer::ERROR_NONE && !positionHistory.empty()) {
                listSelection.set_position(positionHistory.back());
                positionHistory.pop_back();
            }
        } else if (selection == UI_EXIT) {
            return MediaData();
        } else {
            MediaData selectedFile = file_explorer->get_file(selection);
            if (selectedFile.type == FILETYPE_DIR) {
                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                if (file_explorer->open_dir(selectedFile, _status_callback) == File_Explorer::ERROR_NONE) {
                    positionHistory.push_back(listSelection.get_position());
                    listSelection.reset_position();
                }
            } else {
                return selectedFile;
            }
        }
    }
    return MediaData();
}

void
UI::FileBrowser::altMenu()
{
    using namespace MENUDATA;
    using namespace FILE_BROWSER_m;

    ListSelection sortMenu;
    SystemMessage notify;
    items selectedItem;

    selectedItem = (items) sortMenu.get(menu, SIZE);

    notify.show("Sorting...", 0, false);

    currentPosition.cursor = 0;
    currentPosition.page = 1;
    currentPosition.index = 0;

    if (file_explorer == nullptr) {
        return;
    }

    switch (selectedItem) {
        case ASC:
            file_explorer->set_sort_order(File_Explorer::SORT_ASCENDING);
            break;

        case DESC:
            file_explorer->set_sort_order(File_Explorer::SORT_DESCENDING);
            break;

        case NAME:
            file_explorer->set_sort_type(File_Explorer::SORT_NAME);
            break;

        case TYPE:
            file_explorer->set_sort_type(File_Explorer::SORT_TYPE);
            break;

        default:
            return;
    }
    return;
}

UI::FileBrowser::~FileBrowser()
{
    if (file_explorer != nullptr) {
        delete file_explorer;
        file_explorer = nullptr;
    }
}

bool
UI::FileBrowser::setRoot(MediaData root)
{
    file_explorer->init(root);
    currentPosition.cursor = 0;
    currentPosition.page = 1;
    currentPosition.index = 0;
    return true;
}