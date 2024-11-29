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
#include <ui/filebrowser.h>

UI::FileBrowser::FileBrowser()
{
    if (sdfs->isReady()) {
        filesystem = new Filesystem();
        files.reserve(MAX_TEXT_LINES);
    }
}

void
UI::FileBrowser::begin()
{
    if (filesystem == nullptr && sdfs->isReady()) {
        filesystem = new Filesystem();
        files.reserve(MAX_TEXT_LINES);
    }
}

void
UI::FileBrowser::end()
{
    if (filesystem != nullptr) {
        delete filesystem;
        filesystem = nullptr;
    }
}

void
UI::FileBrowser::refresh()
{
    filesystem->generateIndex(SORT_NONE, SORT_ORDER_NONE);
    lastPage = -1;
}

MediaData
UI::FileBrowser::get()
{
    SystemMessage notify;

    if (filesystem == nullptr && !sdfs->isReady()) {
        log_e("Filesystem is not initialized!");
        notify.show("No files found!", 2000, false);
        return MediaData();
    }

    if (filesystem == nullptr && sdfs->isReady()) {
        begin();
    }

    Timer selectedFileTimer; /* Timer for rotating the selected file if it's too long to fit on the screen */
    
    notify.show("Loading files...", 0, false);
    transport->playUIsound(folder_open, folder_open_len);

    lastPage = -1;
    numFiles = filesystem->numFiles();
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

        if (filesystem != nullptr && !sdfs->isReady()) {
            end();
            log_e("Filesystem is not ready!");
            notify.show("No files found!", 2000, false);
            return MediaData();
        }

        if (buttons->getButtonEvent(BUTTON_UP, SHORTPRESS))
            cursorUp();

        if (buttons->getButtonEvent(BUTTON_DOWN, SHORTPRESS))
            cursorDown();

        if (buttons->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            transport->playUIsound(folder_close, folder_close_len);
            return MediaData();
        }

        if (buttons->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            /* Check whether the selected file is a directory or a file */
            if (files[currentPosition.cursor].type == FILETYPE_DIR) {
                transport->playUIsound(folder_open, folder_open_len);
                notify.show("Loading files...", 0, false);

                if (filesystem->openDir(files[currentPosition.cursor])) {
                    /* Save the current position in the file list so we can return to it later */
                    positionHistory.push_back(currentPosition);

                    /* Reset the cursor position and page */
                    currentPosition.cursor = 0;
                    currentPosition.page = 1;
                    currentPosition.selectedFile_index = 0;
                    lastPage = -1;
                    numFiles = filesystem->numFiles();
                }

            }

            else {
                transport->playUIsound(load_item, load_item_len);
                return files[currentPosition.cursor];
            }
        }

        if (buttons->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            if (positionHistory.size() == 0) {
                transport->playUIsound(folder_close, folder_close_len);
                return MediaData();
            }

            /* If we're in a subdirectory, exit the directory and return to the parent directory */

            notify.show("Loading files...", 0, false);

            if (filesystem->exitDir()) {
                /* Restore the cursor position and page */
                currentPosition = positionHistory.back();
                positionHistory.pop_back();
                lastPage = -1; /* Force the file list to be reloaded */
                numFiles = filesystem->numFiles();
                transport->playUIsound(folder_close, folder_close_len);
            }
        }

        if (buttons->getButtonEvent(BUTTON_MENU, SHORTPRESS)) {
            transport->playUIsound(folder_open, folder_open_len);
            altMenu();
        }

        /* Now check the longpress events */
        if (buttons->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            cursorUp();
            buttons->repeat(BUTTON_UP);
        }

        if (buttons->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            cursorDown();
            buttons->repeat(BUTTON_DOWN);
        }

        /* If the selected file is too long to fit on the screen, rotate it */
        if (selectedFileTimer.check(SELECTED_ITEM_ROTATE_SPEED) && selectedFile.size() > MAX_CHARACTERS_PER_LINE)
            std::rotate(selectedFile.begin(), selectedFile.begin() + 1, selectedFile.end());

        draw();
    }

    return MediaData();
}

uint32_t
UI::FileBrowser::numPages()
{
    uint32_t pages;
    uint8_t remainder = 0;

    if (numFiles <= MAX_TEXT_LINES)
        pages = 1;

    else {
        pages = (numFiles / MAX_TEXT_LINES);
        remainder = (numFiles % MAX_TEXT_LINES);
    }

    /* If the last page had less than four files, add another page for them */
    if (remainder > 0)
        pages++;

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
        transport->playUIsound(click, click_len);
    }

    else if (currentPosition.cursor == 0 && currentPosition.page > 1) {
        lastPage = -1; /* Force the file list to be reloaded */
        currentPosition.page--;
        currentPosition.cursor = MAX_TEXT_LINES - 1;
        currentPosition.selectedFile_index--;
        transport->playUIsound(click, click_len);
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
        transport->playUIsound(click, click_len);
    }

    else if (currentPosition.cursor == MAX_TEXT_LINES - 1 && currentPosition.page < numPages()) {
        lastPage = -1; /* Force the file list to be reloaded */
        currentPosition.page++;
        currentPosition.selectedFile_index++;
        currentPosition.cursor = 0;
        transport->playUIsound(click, click_len);
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

        if ((currentPosition.page - 1) * MAX_TEXT_LINES + MAX_TEXT_LINES > numFiles)
            lines = numFiles - (currentPosition.page - 1) * MAX_TEXT_LINES;

        files = filesystem->getFiles((currentPosition.page - 1) * MAX_TEXT_LINES, lines);

        if (files.size() > 0) {
            selectedFile = files[currentPosition.cursor].filename;
            if (selectedFile.length() > MAX_CHARACTERS_PER_LINE)
                selectedFile.append("   ");
        }

        lastPage = currentPosition.page;
    }

    if (files.size() > 0) {
        /* Iterate through the files vector and draw the file names to the screen, highlighting the selected file based on the cursor position */
        for (uint8_t i = 0; i < files.size(); i++) {
            display->setCursor(10, i * 8);

            /* Draw the icon for the file type */
            if (files[i].type == FILETYPE_DIR)
                display->drawBitmap(0, (i * 8), bitmap_folder, 7, 7, WHITE);
            else if (files[i].type == FILETYPE_M3U)
                display->drawBitmap(0, (i * 8), bitmap_playlist, 7, 7, WHITE);
            else
                display->drawBitmap(0, (i * 8), bitmap_note, 7, 7, WHITE);

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

    switch (selectedItem) {
        case ASC:
            filesystem->generateIndex(SORT_FILENAME, SORT_ORDER_ASCENDING, true);
            break;

        case DESC:
            filesystem->generateIndex(SORT_FILENAME, SORT_ORDER_DESCENDING, true);
            break;

        case DIR:
            filesystem->generateIndex(SORT_DIR, SORT_ORDER_ASCENDING, true);
            break;

        case UI_EXIT:
            return;
    }

    currentPosition.cursor = 0;
    currentPosition.page = 1;
    currentPosition.selectedFile_index = 0;
    lastPage = -1; /* Force the file list to be reloaded */
    return;
}

UI::FileBrowser::~FileBrowser()
{
    if (filesystem != nullptr) {
        delete filesystem;
        filesystem = nullptr;
    }
}

bool
UI::FileBrowser::setRoot(MediaData root)
{
    if (filesystem->setRoot(root)) {
        currentPosition.cursor = 0;
        currentPosition.page = 1;
        currentPosition.selectedFile_index = 0;
        lastPage = -1; /* Force the file list to be reloaded */

        return true;
    }

    return false;
}