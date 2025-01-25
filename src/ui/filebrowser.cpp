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

}

void
UI::FileBrowser::begin()
{
    if (file_explorer == nullptr && Card_Manager::get_handle()->isReady()) {
        file_explorer = new File_Explorer();
        file_explorer->init(_status_callback);
        files.reserve(MAX_TEXT_LINES);
    }
    sort_order = File_Explorer::SORT_ASCENDING;
    sort_type = File_Explorer::SORT_NAME;
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
    lastPage = -1;
}

MediaData
UI::FileBrowser::get()
{
    SystemMessage notify;
    Timer selectedFileTimer; /* Timer for rotating the selected file if it's too long to fit on the screen */

    if (!Card_Manager::get_handle()->isReady()) {
        log_e("Filesystem is not initialized!");
        notify.show("No files found!", 2000, false);
        return MediaData();
    }

    if (file_explorer == nullptr) {
        begin();
        positionHistory.clear();
    }

    Transport::get_handle()->playUIsound(folder_open, folder_open_len);
    lastPage = -1;
    numFiles = file_explorer->size();
    if (currentPosition.selectedFile_index > numFiles - 1) {
        if (currentPosition.page > numPages()) {
            currentPosition.page = numPages();
            currentPosition.cursor = MAX_TEXT_LINES - 1;
            currentPosition.selectedFile_index = numFiles - 1;
        } else {
            currentPosition.cursor = currentPosition.cursor - 1;
            currentPosition.selectedFile_index = numFiles - 1;
        }
    }

    /* Main loop */
    while (true) {
        serviceLoop();

        if (file_explorer != nullptr && !Card_Manager::get_handle()->isReady()) {
            end();
            log_e("Filesystem is not ready!");
            notify.show("No files found!", 2000, false);
            return MediaData();
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, SHORTPRESS)) {
            cursorUp();
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, SHORTPRESS)) {
            cursorDown();
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            return MediaData();
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            /* Check whether the selected file is a directory or a file */
            if (files[currentPosition.cursor].type == FILETYPE_DIR) {
                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                if (file_explorer->open_dir(files[currentPosition.cursor], _status_callback) == File_Explorer::ERROR_NONE) {
                    /* Save the current position in the file list so we can return to it later */
                    positionHistory.push_back(currentPosition);

                    /* Reset the cursor position and page */
                    currentPosition.cursor = 0;
                    currentPosition.page = 1;
                    currentPosition.selectedFile_index = 0;
                    lastPage = -1;
                    numFiles = file_explorer->size();
                } else {
                    lastPage = -1;
                }
            } else {
                Transport::get_handle()->playUIsound(load_item, load_item_len);
                return files[currentPosition.cursor];
            }
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            if (positionHistory.size() == 0) {
                Transport::get_handle()->playUIsound(folder_close, folder_close_len);
                return MediaData();
            }

            /* If we're in a subdirectory, exit the directory and return to the parent directory */

            notify.show("Loading files...", 0, false);

            if (file_explorer->exit_dir() == File_Explorer::ERROR_NONE) {
                /* Restore the cursor position and page */
                currentPosition = positionHistory.back();
                positionHistory.pop_back();
                lastPage = -1; /* Force the file list to be reloaded */
                numFiles = file_explorer->size();
                Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            }
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_MENU, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_open, folder_open_len);
            altMenu();
        }

        /* Now check the longpress events */
        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            cursorUp();
            Buttons::get_handle()->repeat(BUTTON_UP);
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            cursorDown();
            Buttons::get_handle()->repeat(BUTTON_DOWN);
        }

        /* If the selected file is too long to fit on the screen, rotate it */
        if (selectedFileTimer.check(SELECTED_ITEM_ROTATE_SPEED) && selectedFile.size() > MAX_CHARACTERS_PER_LINE) {
            std::rotate(selectedFile.begin(), selectedFile.begin() + 1, selectedFile.end());
        }

        draw();
    }

    return MediaData();
}

uint32_t
UI::FileBrowser::numPages()
{
    uint32_t pages;
    uint8_t remainder = 0;

    if (numFiles <= MAX_TEXT_LINES) {
        pages = 1;
    }

    else {
        pages = (numFiles / MAX_TEXT_LINES);
        remainder = (numFiles % MAX_TEXT_LINES);
    }

    /* If the last page had less than four files, add another page for them */
    if (remainder > 0) {
        pages++;
    }

    return pages;
}

void
UI::FileBrowser::cursorUp()
{
    if (currentPosition.cursor > 0) {
        currentPosition.cursor--;
        currentPosition.selectedFile_index--;
        selectedFile = files[currentPosition.cursor].filename;
        if (selectedFile.length() > MAX_CHARACTERS_PER_LINE)
            selectedFile.append("   ");
        Transport::get_handle()->playUIsound(click, click_len);
    }

    else if (currentPosition.cursor == 0 && currentPosition.page > 1) {
        lastPage = -1; /* Force the file list to be reloaded */
        currentPosition.page--;
        currentPosition.cursor = MAX_TEXT_LINES - 1;
        currentPosition.selectedFile_index--;
        Transport::get_handle()->playUIsound(click, click_len);
    }
}

void
UI::FileBrowser::cursorDown()
{
    if (currentPosition.cursor < MAX_TEXT_LINES - 1 && currentPosition.page <= numPages() && currentPosition.selectedFile_index < numFiles - 1) {
        currentPosition.cursor++;
        currentPosition.selectedFile_index++;
        selectedFile = files[currentPosition.cursor].filename;
        if (selectedFile.length() > MAX_CHARACTERS_PER_LINE)
            selectedFile.append("   ");
        Transport::get_handle()->playUIsound(click, click_len);
    }

    else if (currentPosition.cursor == MAX_TEXT_LINES - 1 && currentPosition.page < numPages()) {
        lastPage = -1; /* Force the file list to be reloaded */
        currentPosition.page++;
        currentPosition.selectedFile_index++;
        currentPosition.cursor = 0;
        Transport::get_handle()->playUIsound(click, click_len);
    }
}

void
UI::FileBrowser::draw()
{
    display->clearDisplay();
    if (screensaver->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);

    if (lastPage != currentPosition.page) {
        uint8_t lines = MAX_TEXT_LINES;

        if ((currentPosition.page - 1) * MAX_TEXT_LINES + MAX_TEXT_LINES > numFiles) {
            lines = numFiles - (currentPosition.page - 1) * MAX_TEXT_LINES;
        }

        file_explorer->get_list(&files, (currentPosition.page - 1) * MAX_TEXT_LINES, lines, sort_order, sort_type);

        if (files.size() > 0) {
            selectedFile = files[currentPosition.cursor].filename;
            if (selectedFile.length() > MAX_CHARACTERS_PER_LINE) {
                selectedFile.append("   ");
            }
        }

        lastPage = currentPosition.page;
    }

    if (files.size() > 0) {
        /* Iterate through the files vector and draw the file names to the screen, highlighting the selected file based on the cursor position */
        for (uint8_t i = 0; i < files.size(); i++) {
            display->setCursor(10, i * 8);

            /* Draw the icon for the file type */
            if (files[i].type == FILETYPE_DIR) {
                display->drawBitmap(0, (i * 8), bitmap_folder, 7, 7, WHITE);
            }
            else if (files[i].type == FILETYPE_M3U) {
                display->drawBitmap(0, (i * 8), bitmap_playlist, 7, 7, WHITE);
            }
            else {
                display->drawBitmap(0, (i * 8), bitmap_note, 7, 7, WHITE);
            }

            if (i == currentPosition.cursor) {
                display->setTextColor(BLACK, WHITE);
                display->print(selectedFile.c_str());
                /* Add another 24 spaces to fill the rest of the line */
                for (uint8_t j = 0; j < 24; j++)
                    display->print(" ");
            }

            else {
                display->setTextColor(WHITE, BLACK);
                display->print(files[i].filename.c_str());
            }
        }

    }

    else {
        display->setCursor(0, 0);
        display->setTextColor(WHITE, BLACK);
        display->print("No files found!");
    }

    display->display();
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
    currentPosition.selectedFile_index = 0;

    switch (selectedItem) {
        case ASC:
            sort_order = File_Explorer::SORT_ASCENDING;
            break;

        case DESC:
            sort_order = File_Explorer::SORT_DESCENDING;
            break;

        case NAME:
            sort_type = File_Explorer::SORT_NAME;
            break;

        case TYPE:
            sort_type = File_Explorer::SORT_TYPE;
            break;

        default:
            return;
    }
    lastPage = -1; /* Force the file list to be reloaded */
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
    currentPosition.selectedFile_index = 0;
    lastPage = -1; /* Force the file list to be reloaded */
    return true;
}