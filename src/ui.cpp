/**
 * @file ui.cpp
 *
 * @brief draws all UI elements
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

#include <ui.h>

/****************************************************
 *
 * File Browser
 *
 ****************************************************/

FileBrowser::FileBrowser()
{
    if (sdfs->isReady()) {
        filesystem = new Filesystem();
        files.reserve(MAX_TEXT_LINES);
    }
}

void
FileBrowser::begin()
{
    if (filesystem == nullptr && sdfs->isReady()) {
        filesystem = new Filesystem();
        files.reserve(MAX_TEXT_LINES);
    }
}

void
FileBrowser::end()
{
    if (filesystem != nullptr) {
        delete filesystem;
        filesystem = nullptr;
    }
}

void
FileBrowser::refresh()
{
    filesystem->generateIndex(SORT_NONE, SORT_ORDER_NONE);
    lastPage = -1;
}

MediaData
FileBrowser::get()
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
FileBrowser::numPages()
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
FileBrowser::cursorUp()
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
FileBrowser::cursorDown()
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
FileBrowser::draw()
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
FileBrowser::altMenu()
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

FileBrowser::~FileBrowser()
{
    if (filesystem != nullptr) {
        delete filesystem;
        filesystem = nullptr;
    }
}

bool
FileBrowser::setRoot(MediaData root)
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

/****************************************************
 *
 * Menu
 *
 ****************************************************/

ListSelection::ListSelection() {
    if (marquee == nullptr) {
        marquee = new Marquee();
    }
    marquee->addSource(std::bind(&ListSelection::getSelected, this));
    marquee->setSpeed(100);
}

ListSelection::~ListSelection() {
    if (marquee != nullptr) {
        delete marquee;
        marquee = nullptr;
    }
}

uint16_t
ListSelection::get()
{
    /* Main loop */
    while (true) {
        serviceLoop();

        if (buttons->getButtonEvent(BUTTON_UP, SHORTPRESS))
            cursorUp();

        if (buttons->getButtonEvent(BUTTON_DOWN, SHORTPRESS))
            cursorDown();

        if (buttons->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            transport->playUIsound(load_item, load_item_len);
            return selectedIndex;
        }

        if (buttons->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            transport->playUIsound(folder_close, folder_close_len);
            return UI_EXIT;
        }

        /* Longpress events */

        if (buttons->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            cursorUp();
            buttons->repeat(BUTTON_UP);
        }

        if (buttons->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            cursorDown();
            buttons->repeat(BUTTON_DOWN);
        }

        draw();
    }

    return 0;
}

uint16_t
ListSelection::get(const char* const menuItems[], uint16_t numItems)
{
    menuType = MENU_TYPE_CONST_CHAR;

    this->numItems = numItems;
    this->menuItems = menuItems;

    cursor = 0;
    page = 1;
    selectedIndex = 0;

    return get();
}

uint16_t
ListSelection::get(std::vector<std::string>& listItems)
{
    menuType = MENU_TYPE_STRING_VECTOR;

    this->listItems_ptr = &listItems;
    numItems = listItems.size();

    cursor = 0;
    page = 1;
    selectedIndex = 0;

    return get();
}

uint16_t
ListSelection::get(TableData& table)
{
    menuType = MENU_TYPE_DATATABLE;

    this->table = &table;
    numItems = table.size();

    cursor = 0;
    page = 1;
    selectedIndex = 0;

    return get();
}

uint16_t
ListSelection::get(PlaylistEngine* playlist_engine, bool playlist_showindex)
{
    if (!playlist_engine->isLoaded()) {
        log_e("Playlist is empty!");
        return UI_EXIT;
    }

    this->_playlist_engine = playlist_engine;
    this->_playlist_showindex = playlist_showindex;

    menuType = MENU_TYPE_PLAYLIST;
    numItems = _playlist_engine->size();
    selectedIndex = _playlist_engine->getCurrentTrackIndex();

    /* Calculate the page and cursor position based on the selected item index */
    page = (selectedIndex / MAX_TEXT_LINES) + 1;
    defaultIndex = selectedIndex % MAX_TEXT_LINES;
    cursor = defaultIndex;

    return get();
}

std::vector<std::string>
ListSelection::getDisplayedItems()
{
    std::vector<std::string> displayedItems;
    MediaData item;

    for (uint16_t i = (page - 1) * MAX_TEXT_LINES; i < (page * MAX_TEXT_LINES); i++) {
        switch (menuType) {
            case MENU_TYPE_CONST_CHAR:
                if (i < numItems)
                    displayedItems.push_back(menuItems[i]);
                break;
            case MENU_TYPE_STRING_VECTOR:
                if (i < numItems)
                    displayedItems.push_back((*listItems_ptr)[i]);
                break;
            case MENU_TYPE_DATATABLE:
                if (i < numItems)
                    displayedItems.push_back(table->get(i, 0));
                break;
            case MENU_TYPE_PLAYLIST:
                if (i < numItems) {
                    item = _playlist_engine->getTrack(i);
                    if (item.source == LOCAL_FILE && item.loaded) {
                        displayedItems.push_back(item.filename);
                    } else if (item.source == REMOTE_FILE && item.loaded) {
                        displayedItems.push_back(item.url);
                    }
                }
                break;
            default:
                break;
        }
    }
    return displayedItems;
}

void
ListSelection::draw()
{
    display->clearDisplay();
    if (screensaver->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);

    std::vector<std::string> displayedItems;
    displayedItems = getDisplayedItems();
    if (displayedItems.size() == 0) {
        display->setTextColor(WHITE, BLACK);
        display->print("No items found!");
        display->display();
        return;
    }
    selected_item = displayedItems[cursor];

    for (uint16_t i = 0; i < displayedItems.size(); i++) {
        display->setCursor(0, i * 8);

        /* Print out the index number of the item */
        if (menuType == MENU_TYPE_PLAYLIST && _playlist_engine) {
            if (_playlist_showindex && i == _playlist_engine->getCurrentTrackIndex() && _playlist_engine->isDriver())
                display->setTextColor(BLACK, WHITE);
            else
                display->setTextColor(WHITE, BLACK);
            display->print(i);
            display->print(":");
        }

        if (i == cursor) {
            display->setTextColor(BLACK, WHITE);
            marquee->draw(0, i * 8);
        }

        else {
            display->setTextColor(WHITE, BLACK);
            display->print(displayedItems[i].c_str());
        }
    }
    display->display();
}

void
ListSelection::cursorUp()
{
    if (cursor > 0) {
        cursor--;
        selectedIndex--;
        transport->playUIsound(click, click_len);
    }

    else if (cursor == 0 && page > 1) {
        page--;
        cursor = MAX_TEXT_LINES - 1;
        selectedIndex--;
        transport->playUIsound(click, click_len);
    }
}

void
ListSelection::cursorDown()
{
    if (cursor < MAX_TEXT_LINES - 1 && page <= numPages() && selectedIndex < numItems - 1) {
        cursor++;
        selectedIndex++;
        transport->playUIsound(click, click_len);
    }

    else if (cursor == MAX_TEXT_LINES - 1 && page < numPages()) {
        page++;
        cursor = 0;
        selectedIndex++;
        transport->playUIsound(click, click_len);
    }
}

uint16_t
ListSelection::numPages()
{
    uint32_t pages;
    uint8_t remainder = 0;

    if (numItems <= MAX_TEXT_LINES)
        pages = 1;

    else {
        pages = (numItems / MAX_TEXT_LINES);
        remainder = (numItems % MAX_TEXT_LINES);
    }

    /* If the last page had less than four files, add another page for them */
    if (remainder > 0)
        pages++;

    return pages;
}

/****************************************************
 *
 * Text Input
 *
 ****************************************************/

TextInput::TextInput() {}

std::string
TextInput::get(std::string prompt, std::string defaultText, uint8_t maxLength, uint8_t inputType)
{
    this->maxLength = maxLength;
    this->prompt = prompt;
    this->input = defaultText;
    this->inputType = inputType;
    editMode = false;
    characterTableIndex = 0;

    /* In the code below, we need to pad out the default text with zeros if it's an IP address, time or date so everything lines up properly */

    switch (inputType) {
        case INPUT_FORMAT_IPADDRESS:
            maxLength = 15;
            this->characterTable = characterTable_numeric;
            this->characterTableLength = characterTable_numeric_length;
            /* Initialize the default text */
            if (input.length() == 0) {
                input = "000.000.000.000";
            }
            /* Otherwise we need to pad out each octet with zeros if it's less than 3 characters */
            else {
                std::vector<std::string> octets;
                while (input.find(".") != std::string::npos) {
                    octets.push_back(input.substr(0, input.find(".")));
                    input.erase(0, input.find(".") + 1);
                }
                /* Push back the last octet */
                octets.push_back(input);
                for (uint8_t i = 0; i < octets.size(); i++) {
                    while (octets[i].length() < 3) {
                        octets[i].insert(0, 1, '0');
                    }
                }
                input = octets[0] + "." + octets[1] + "." + octets[2] + "." + octets[3];
            }
            break;
        case INPUT_FORMAT_TIME:
            maxLength = 8;
            this->characterTable = characterTable_numeric;
            this->characterTableLength = characterTable_numeric_length;
            if (input.length() == 0) {
                input = "00:00:00";
            }
            /* Pad out the hours, minutes and seconds with zeros if they're less than 2 characters */
            else {
                std::vector<std::string> time;
                while (input.find(":") != std::string::npos) {
                    time.push_back(input.substr(0, input.find(":")));
                    input.erase(0, input.find(":") + 1);
                }
                /* Push back the last 2 digits */
                time.push_back(input);
                for (uint8_t i = 0; i < time.size(); i++) {
                    while (time[i].length() < 2) {
                        time[i].insert(0, 1, '0');
                    }
                }
                input = time[0] + ":" + time[1] + ":" + time[2];
            }
            break;
        case INPUT_FORMAT_DATE:
            maxLength = 10;
            this->characterTable = characterTable_numeric;
            this->characterTableLength = characterTable_numeric_length;
            if (input.length() == 0) {
                input = "2024-01-01";
            }
            /* Pad out the year, month and day with zeros if they're less than 2 characters */
            else {
                std::vector<std::string> date;
                while (input.find("-") != std::string::npos) {
                    date.push_back(input.substr(0, input.find("-")));
                    input.erase(0, input.find("-") + 1);
                }
                /* Push back the last 2 digits */
                date.push_back(input);
                for (uint8_t i = 0; i < date.size(); i++) {
                    if (i == 0) { /* Year is 4 characters */

                        while (date[i].length() < 4) {
                            date[i].insert(0, 1, '0');
                        }
                        continue; /* Break off and do the next iteration with the month and day */
                    }

                    while (date[i].length() < 2) {
                        date[i].insert(0, 1, '0');
                    }
                }
                input = date[0] + "-" + date[1] + "-" + date[2]; /* Put 'em all back together */
            }
            break;
        case INPUT_FORMAT_SERVADDR:
            this->characterTable = characterTable_serverAddress;
            this->characterTableLength = characterTable_serverAddress_length;
            break;
        case INPUT_FORMAT_NUMERIC:
            /* Pad out the default text with zeros if it's less than the maximum length */
            while (input.length() < maxLength) {
                input.insert(0, 1, '0');
            }
            this->characterTable = characterTable_numeric;
            this->characterTableLength = characterTable_numeric_length;
            break;
        case INPUT_FORMAT_PASSWORD:
            this->characterTable = characterTable_all;
            this->characterTableLength = characterTable_all_length;
            break;
        default:
            this->characterTable = characterTable_alphanumeric;
            this->characterTableLength = characterTable_alphanumeric_length;
            break;
    }

    if (input.length() > 0 && input.length() < MAX_DISPLAYED_CHARACTERS) {
        if (inputType != INPUT_FORMAT_IPADDRESS && inputType != INPUT_FORMAT_TIME && inputType != INPUT_FORMAT_DATE && inputType != INPUT_FORMAT_NUMERIC) {
            /* If it's not an IP address, time or date, set the cursor to the end of the string */
            stringIndex = input.length();
            cursor = input.length();
        } else {
            /* Set the cursor to the last character of the string */
            stringIndex = maxLength - 1;
            cursor = maxLength - 1;
        }
    } else if (input.length() > MAX_DISPLAYED_CHARACTERS) {
        stringIndex = input.length();
        cursor = MAX_DISPLAYED_CHARACTERS;
    } else {
        stringIndex = 0;
        cursor = 0;
    }

    if (inputType != INPUT_FORMAT_IPADDRESS && inputType != INPUT_FORMAT_TIME && inputType != INPUT_FORMAT_DATE && inputType != INPUT_FORMAT_NUMERIC)
        cursorCharacter = characterTable[characterTableIndex][0];
    else
        cursorCharacter = input[stringIndex];

    /* Main loop */
    while (true) {
        serviceLoop();

        if (buttons->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            if (editMode)
                addChar(characterTable[characterTableIndex]);
            else
                moveCursorRight();
        }

        if (buttons->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            if (editMode)
                removeChar();
            else
                moveCursorLeft();
        }

        if (buttons->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            if (editMode) {
                input[stringIndex] = cursorCharacter;
            }
            return input;
        }

        if (buttons->getButtonEvent(BUTTON_MENU, SHORTPRESS)) {
            if (editMode) {
                input[stringIndex] = cursorCharacter;
            }

            editMode = !editMode;

            /* Change the characterIndex to the character at the new cursor position from the input string */
            if (stringIndex < input.length() && inputType != INPUT_FORMAT_PASSWORD && !editMode) {
                characterTableIndex = 0;
                for (uint8_t i = 0; i < characterTableLength; i++) {
                    if (input[stringIndex] == (char) characterTable[i][0]) {
                        characterTableIndex = i;
                        cursorCharacter = characterTable[characterTableIndex][0];
                        break;
                    }
                }
            }

            else if (stringIndex < input.length() && inputType == INPUT_FORMAT_PASSWORD && !editMode) {
                cursorCharacter = '*';
            }

            else if (stringIndex == input.length() && !editMode) {
                characterTableIndex = 0;
                cursorCharacter = characterTable[characterTableIndex][0];
            }
        }

        if (buttons->getButtonEvent(BUTTON_UP, SHORTPRESS) && editMode)
            scrollUp();

        if (buttons->getButtonEvent(BUTTON_DOWN, SHORTPRESS) && editMode)
            scrollDown();

        /* Longpress events */
        if (buttons->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            scrollUp();
            buttons->repeat(BUTTON_UP);
        }

        if (buttons->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            scrollDown();
            buttons->repeat(BUTTON_DOWN);
        }

        if (buttons->getButtonEvent(BUTTON_PLAY, LONGPRESS)) {
            if (editMode)
                addChar(characterTable[characterTableIndex]);
            else
                moveCursorRight();
            buttons->repeat(BUTTON_PLAY);
        }

        if (buttons->getButtonEvent(BUTTON_STOP, LONGPRESS)) {
            if (editMode)
                removeChar();
            else
                moveCursorLeft();
            buttons->repeat(BUTTON_STOP);
        }

        draw();
    }
}

/* Scrolls to the next character in the character table index */
void
TextInput::scrollUp()
{
    if (characterTableIndex < characterTableLength - 1)
        characterTableIndex++;
    else
        characterTableIndex = 0;

    cursorCharacter = characterTable[characterTableIndex][0];
}

/* Scrolls to the previous character in the character table index */
void
TextInput::scrollDown()
{
    if (characterTableIndex > 0)
        characterTableIndex--;
    else
        characterTableIndex = characterTableLength - 1;

    cursorCharacter = characterTable[characterTableIndex][0];
}

void
TextInput::addChar(const char* c)
{
    if (input.length() < maxLength && inputType != INPUT_FORMAT_IPADDRESS && inputType != INPUT_FORMAT_TIME && inputType != INPUT_FORMAT_DATE) {
        input.insert(stringIndex, 1, *c);
        moveCursorRight();
    } else {
        input[stringIndex] = cursorCharacter;
        moveCursorRight();
    }
}

void
TextInput::removeChar()
{
    if (stringIndex > 0 && inputType != INPUT_FORMAT_IPADDRESS && inputType != INPUT_FORMAT_TIME && inputType != INPUT_FORMAT_DATE) {
        input.erase(stringIndex - 1, 1);
        moveCursorLeft();
    } else {
        input[stringIndex] = cursorCharacter;
        moveCursorLeft();
    }
}

void
TextInput::moveCursorLeft()
{
    /* Move the cursor left only if the cursor and stringIndex are greater than 10 */
    if (stringIndex > MAX_DISPLAYED_CHARACTERS / 2 && cursor > MAX_DISPLAYED_CHARACTERS / 2) {
        stringIndex--;
        cursor--;
    }

    else if (stringIndex > MAX_DISPLAYED_CHARACTERS / 2 && cursor == MAX_DISPLAYED_CHARACTERS / 2)
        stringIndex--;

    else if (stringIndex <= MAX_DISPLAYED_CHARACTERS / 2 && cursor > 0) {
        stringIndex--;
        cursor--;
    }

    /* Change the characterIndex to the character at the new cursor position from the input string */
    if (stringIndex < input.length() && inputType != INPUT_FORMAT_PASSWORD &&
        (!editMode || (editMode && inputType != INPUT_FORMAT_PASSWORD && inputType != INPUT_FORMAT_TEXT))) {
        characterTableIndex = 0;
        for (uint8_t i = 0; i < characterTableLength; i++) {
            if (input[stringIndex] == (char) characterTable[i][0]) {
                characterTableIndex = i;
                cursorCharacter = characterTable[characterTableIndex][0];
                break;
            }
        }
    }

    else if (stringIndex < input.length() && inputType == INPUT_FORMAT_PASSWORD && !editMode) {
        cursorCharacter = '*';
    }

    /* If this is not a text, internet domain name, or password input, and the current character is a ":", ".", or "-", move the cursor to the next character */
    if (inputType != INPUT_FORMAT_TEXT && inputType != INPUT_FORMAT_PASSWORD && inputType != INPUT_FORMAT_SERVADDR) {
        if (input[stringIndex] == ':' || input[stringIndex] == '.' || input[stringIndex] == '-') {
            moveCursorLeft();
        }
    }
}

void
TextInput::moveCursorRight()
{
    /* Move the cursor right only if the cursor and stringIndex are less than the maximum length */
    if (stringIndex < input.length() && cursor < MAX_DISPLAYED_CHARACTERS && inputType != INPUT_FORMAT_IPADDRESS && inputType != INPUT_FORMAT_TIME &&
        inputType != INPUT_FORMAT_DATE) {
        stringIndex++;
        cursor++;
    }

    else if (stringIndex < input.length() && cursor == MAX_DISPLAYED_CHARACTERS && inputType != INPUT_FORMAT_IPADDRESS && inputType != INPUT_FORMAT_TIME &&
             inputType != INPUT_FORMAT_DATE)
        stringIndex++;

    else if (stringIndex < maxLength - 1 && cursor < MAX_DISPLAYED_CHARACTERS) {
        stringIndex++;
        cursor++;
    }

    /* Change the characterIndex to the character at the new cursor position from the input string */
    if (stringIndex < input.length() && inputType != INPUT_FORMAT_PASSWORD &&
        (!editMode || (editMode && inputType != INPUT_FORMAT_PASSWORD && inputType != INPUT_FORMAT_TEXT))) {
        characterTableIndex = 0;
        for (uint8_t i = 0; i < characterTableLength; i++) {
            if (input[stringIndex] == (char) characterTable[i][0]) {
                characterTableIndex = i;
                cursorCharacter = characterTable[characterTableIndex][0];
                break;
            }
        }
    }

    else if (stringIndex < input.length() && inputType == INPUT_FORMAT_PASSWORD && !editMode) {
        cursorCharacter = '*';
    }

    else if (stringIndex == input.length() && !editMode) {
        characterTableIndex = 0;
        cursorCharacter = characterTable[characterTableIndex][0];
    }

    /* If this is not a text, internet domain name, or password input, and the current character is a ":", ".", or "-", move the cursor to the next character */
    if (inputType != INPUT_FORMAT_TEXT && inputType != INPUT_FORMAT_PASSWORD && inputType != INPUT_FORMAT_SERVADDR) {
        if (input[stringIndex] == ':' || input[stringIndex] == '.' || input[stringIndex] == '-') {
            moveCursorRight();
        }
    }
}

std::string
TextInput::getDisplayedInput()
{
    std::string displayedInput;

    if (cursor == MAX_DISPLAYED_CHARACTERS) {
        displayedInput = input.substr(stringIndex - MAX_DISPLAYED_CHARACTERS, MAX_DISPLAYED_CHARACTERS);
    }

    else if (cursor == 0) {
        displayedInput = input.substr(stringIndex, MAX_DISPLAYED_CHARACTERS);
    }

    else {
        displayedInput = input.substr(stringIndex - cursor, cursor) + input.substr(stringIndex, MAX_DISPLAYED_CHARACTERS - cursor);
    }

    return displayedInput;
}

void
TextInput::draw()
{
    display->clearDisplay();
    if (screensaver->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);

    /* Draw the prompt text at the top of the screen */
    display->setCursor(0, 0);
    display->setTextColor(WHITE, BLACK);
    display->print(prompt.c_str());

    /* Draw the input string on both sides of the cursor, with the cursor character in the middle with inverted colors */
    if (inputType != INPUT_FORMAT_PASSWORD) {
        display->setCursor(0, 12);
        display->setTextColor(WHITE, BLACK);
        display->print(getDisplayedInput().c_str());
    }

    else {
        display->setCursor(0, 12);
        display->setTextColor(WHITE, BLACK);
        for (uint8_t i = 0; i < getDisplayedInput().size(); i++)
            display->print("*");
    }

    /* Set to current cursor position */
    display->setCursor(cursor * 6, 12);

    /* Check the cursor blink timer to see if we should invert the cursor character */
    if (cursorBlinkTimer.check(CURSOR_BLINK_INTERVAL)) {
        cursorBlink = !cursorBlink;
    }

    /* If the cursor is blinking, print the cursor character with inverted colors */
    if (cursorBlink)
        display->setTextColor(BLACK, WHITE);
    else
        display->setTextColor(WHITE, BLACK);

    /* Print the cursor character */
    display->print(cursorCharacter);

    /* If we are in edit mode, display the edit mode text at the bottom of the screen */
    if (editMode) {
        display->setCursor(0, 24);
        display->setTextColor(WHITE, BLACK);
        display->print("<EDIT MODE>");
    } else {
        display->setCursor(0, 24);
        display->setTextColor(WHITE, BLACK);
        display->print("<VIEW MODE>");
    }

    display->display();
}

/****************************************************
 *
 * Status Screen
 *
 ****************************************************/

StatusScreen::StatusScreen()
{
    anim_playing = new Animation(bunny_playing, bunny_playing_num_frames);
    anim_playing->setDuration(1000);

    anim_stopped = new Animation(bunny_stopped, bunny_stopped_num_frames);
    uint8_t anim_stopped_sequence[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0 };
    anim_stopped->setSequence(anim_stopped_sequence, 13);
    anim_stopped->setDuration(150);

    /* Set up marquees */
    marquee_mediainfo = new Marquee();
    marquee_mediainfo->setSpeed(200);
    marquee_mediainfo->setSwitchInterval(1000 * 10);
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedFileName, transport), "File:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedURL, transport), "URL:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedArtist, transport), "Artist:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedAlbum, transport), "Album:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedTitle, transport), "Title:");
    marquee_mediainfo->addSource(std::bind(&Transport::getLoadedGenre, transport), "Genre:");

    marquee_datetime = new Marquee();
    marquee_datetime->setSwitchInterval(1000 * 10);
    marquee_datetime->addSource(std::bind(&SystemConfig::getCurrentDateTime, systemConfig, "%H:%M:%S %Z"));
    marquee_datetime->addSource(std::bind(&SystemConfig::getCurrentDateTime, systemConfig, "%a, %b %d, %Y"));

    marquee_connectStatus = new Marquee("Connecting");
    marquee_connectStatus->setSwitchInterval(150);
    marquee_connectStatus->addText("Connecting.");
    marquee_connectStatus->addText("Connecting..");
    marquee_connectStatus->addText("Connecting...");

    /* Set up the spectrum analyzer */
    spectrumAnalyzer = new SpectrumAnalyzer();
}

void
StatusScreen::draw()
{
    display->clearDisplay();
    if (screensaver->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);
    display->setTextColor(WHITE, BLACK);

    // Draw the sprite for the TRANSPORT_PLAYING status
    if (transport->getStatus() == TRANSPORT_PLAYING) {
        anim_playing->draw(0, 0, 20, 20);
    } else {
        anim_stopped->draw(0, 0, 20, 20);
    }

    // Display the filename or info of the currently loaded media
    if (transport->getStatus() != TRANSPORT_IDLE && transport->getStatus() != TRANSPORT_CONNECTING) {
        marquee_mediainfo->draw(0, 24);
    } 
    else if (transport->getStatus() == TRANSPORT_CONNECTING) {
        marquee_connectStatus->draw(0, 24);
    }
    else {
        // Display the current system time and date
        marquee_datetime->draw(0, 24);
    }

    // Draw the volume level using bitmap_volume_muted for 0, bitmap_volume_low for 1-33, bitmap_volume_mid for 34-66, and bitmap_volume_full for 67-100
    if (transport->getVolume() < 3)
        display->drawBitmap(120, 0, bitmap_volume_muted, 7, 7, WHITE);
    else if (transport->getVolume() > 0 && transport->getVolume() <= 33)
        display->drawBitmap(120, 0, bitmap_volume_low, 7, 7, WHITE);
    else if (transport->getVolume() > 33 && transport->getVolume() <= 66)
        display->drawBitmap(120, 0, bitmap_volume_mid, 7, 7, WHITE);
    else
        display->drawBitmap(120, 0, bitmap_volume_full, 7, 7, WHITE);

    // If the playlist is enabled, draw the playlist icon, otherwise draw the note icon
    if (playlistEngine->isEnabled())
        display->drawBitmap(110, 0, bitmap_playlist, 7, 7, WHITE);
    else if (transport->getLoadedMedia().loaded)
        display->drawBitmap(110, 0, bitmap_note, 7, 7, WHITE);

    // Draw the network status
    if (WiFi.status() == WL_CONNECTED)
        display->drawBitmap(100, 0, bitmap_wifi_3, 7, 7, WHITE);
    else if (systemConfig->isWifiEnabled() && WiFi.status() == WL_DISCONNECTED)
        display->drawBitmap(100, 0, bitmap_wifi_error, 7, 7, WHITE);

    // Draw the bluetooth status
    if (bluetooth->getMode() == POWER_ON)
        display->drawBitmap(90, 0, bitmap_bluetooth, 7, 7, WHITE);

    // Draw the spectrum analyzer
    if (spectrumAnalyzer) {
        spectrumAnalyzer->draw(27, 2, 2, 9);
    }

    // Draw the play time
    uint32_t playTime = transport->getPlayTime();
    uint8_t hours = playTime / 3600;
    uint8_t minutes = (playTime % 3600) / 60;
    uint8_t seconds = playTime % 60;

    display->setCursor(23, 14);
    if (hours < 10)
        display->print("0");
    display->print(hours);
    display->print(":");
    if (minutes < 10)
        display->print("0");
    display->print(minutes);
    display->print(":");
    if (seconds < 10)
        display->print("0");
    display->print(seconds);

    display->display();
}

/****************************************************
 *
 * Notification
 *
 ****************************************************/

SystemMessage::SystemMessage() {}

void
SystemMessage::show(std::string message, uint16_t duration, bool animated)
{
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextWrap(false);
    display->setTextColor(WHITE, BLACK);
    display->setCursor(0, 0);
    display->print(message.c_str());

    serviceLoop();

    if (animated) {
        if (animationTimer.check(NOTIFICATION_ANIMATION_FRAME_DURATION)) {
            animationFrame++;
            if (animationFrame > 3)
                animationFrame = 0;

            switch (animationFrame) {
                case 0:
                    display->display();
                    break;
                case 1:
                    display->print(".");
                    display->display();
                    break;
                case 2:
                    display->print("..");
                    display->display();
                    break;
                case 3:
                    display->print("...");
                    display->display();
                    break;
            }
        }

    }

    else {
        display->display();
        notificationTimer.reset();
    }

    if (duration > 0 && !animated)
        while (!notificationTimer.check(duration)) {
            serviceLoop();

            // If any of the buttons are pushed, immediately exit the notification
            if (buttons->getButtonEvent(BUTTON_PLAY, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_STOP, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_UP, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_DOWN, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_MENU, SHORTPRESS && !buttons->isHeld(BUTTON_MENU)) ||
                buttons->getButtonEvent(BUTTON_EXIT, SHORTPRESS && !buttons->isHeld(BUTTON_MENU))) {
                notificationTimer.reset();
                return;
            }
        }

    notificationTimer.reset();
}

void
SystemMessage::resetAnimation()
{
    animationFrame = 0;
}

/****************************************************
 *
 * Value Selector
 *
 ****************************************************/

ValueSelector::ValueSelector(std::string prompt, uint16_t min, uint16_t max, uint16_t step, uint16_t defaultValue)
  : _prompt(prompt)
  , _minVal(min)
  , _maxVal(max)
  , _step(step)
  , _value(defaultValue)
  , useCallbacks(false)

{
}

ValueSelector::ValueSelector(std::string prompt,
                             std::function<uint8_t()> valueCallback,
                             std::function<void()> incCallback,
                             std::function<void()> decCallback,
                             uint8_t minVal,
                             uint8_t maxVal)
  : _prompt(prompt)
  , _valueCallback(valueCallback)
  , _incCallback(incCallback)
  , _decCallback(decCallback)
  , _minVal(minVal)
  , _maxVal(maxVal)
  , useCallbacks(true)

{
}

uint16_t
ValueSelector::get()
{

    if (useCallbacks) {
        _value = _valueCallback();
    }

    draw();

    while (true) {

        serviceLoop();

        if (buttons->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            transport->playUIsound(folder_close, folder_close_len);
            return UI_EXIT;
        }

        if (buttons->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            transport->playUIsound(folder_close, folder_close_len);
            return UI_EXIT;
        }

        if (buttons->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            transport->playUIsound(folder_close, folder_close_len);
            return _value;
        }

        if (buttons->getButtonEvent(BUTTON_UP, SHORTPRESS)) {
            inc();
        }

        if (buttons->getButtonEvent(BUTTON_DOWN, SHORTPRESS)) {
            dec();
        }

        /* Longpress events */
        if (buttons->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            inc();
            buttons->repeat(BUTTON_UP);
        }

        if (buttons->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            dec();
            buttons->repeat(BUTTON_DOWN);
        }

        draw();

        if (exitTimer.check(UI_EXIT_TIMEOUT)) {
            return UI_EXIT;
        }
    }
}

void
ValueSelector::inc()
{
    if (useCallbacks) {
        _incCallback();
        _value = _valueCallback();
    } else {
        if (_value < _maxVal)
            _value += _step;
    }
    transport->playUIsound(click, click_len);
    exitTimer.reset();
}

void
ValueSelector::dec()
{
    if (useCallbacks) {
        _decCallback();
        _value = _valueCallback();
    } else {
        if (_value > _minVal)
            _value -= _step;
    }
    transport->playUIsound(click, click_len);
    exitTimer.reset();
}

void
ValueSelector::draw()
{
    display->clearDisplay();
    if (screensaver->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);
    display->setTextColor(WHITE, BLACK);
    display->setCursor(0, 0);
    display->print(_prompt.c_str());
    display->print(": ");
    display->print(_value);

    /* Draw four lines to form the border of the rectangle */
    display->drawFastHLine(0, 12, 127, WHITE);
    display->drawFastHLine(0, 24, 127, WHITE);
    display->drawFastVLine(0, 12, 12, WHITE);
    display->drawFastVLine(127, 12, 12, WHITE);

    /* Fill the rectangle with the current value in reference to the min and max values */
    uint8_t fillWidth = map(_value, _minVal, _maxVal, 0, 127);
    display->fillRect(0, 12, fillWidth, 12, WHITE);

    display->display();
}

/****************************************************
 *
 * Binary Selector
 *
 ****************************************************/

BinarySelector::BinarySelector(std::string falseText, std::string trueText)
  : _options{ falseText = "No", trueText = "Yes" }
{
}

bool
BinarySelector::get()
{
    ListSelection list;
    uint16_t selection = list.get(_options);
    bool result;
    if (selection == UI_EXIT) {
        return false;
    } else if (selection == 0) {
        return false;
    } else {
        return true;
    }
}

/****************************************************
 *
 * Animation
 *
 ****************************************************/

Animation::Animation(const unsigned char* const frames[], uint8_t numFrames)
{

    _frames = new unsigned char*[numFrames];
    /* Set up a basic sequence of frames */
    for (uint8_t i = 0; i < numFrames; i++) {
        _sequence.push_back(i);
    }
    for (uint8_t i = 0; i < numFrames; i++) {
        _frames[i] = (unsigned char*) frames[i];
    }
    _numFrames = numFrames;
}

void
Animation::setSequence(uint8_t* sequence, uint8_t len)
{
    _sequence.clear();
    for (uint8_t i = 0; i < len; i++) {
        if (sequence[i] < _numFrames) {
            _sequence.push_back(sequence[i]);
        } else {
            _sequence.push_back(0);
        }
    }
}

void
Animation::setDuration(uint16_t duration)
{
    _duration = duration;
}

void
Animation::setFrames(const unsigned char* const frames[], uint8_t numFrames)
{
    if (_frames != nullptr) {
        delete[] _frames;
    }
    _frames = new unsigned char*[numFrames];
    for (uint8_t i = 0; i < numFrames; i++) {
        _frames[i] = (unsigned char*) frames[i];
    }
    _numFrames = numFrames;

    /* Set up a new default sequence */
    _sequence.clear();
    for (uint8_t i = 0; i < numFrames; i++) {
        _sequence.push_back(i);
    }
}

void
Animation::draw(size_t x, size_t y, size_t width, size_t height)
{
    if (_frames == nullptr) {
        return;
    }

    if (_animationTimer.check(_duration)) {
        _currentFrame++;
        if (_currentFrame >= _sequence.size()) {
            _currentFrame = 0;
        }
    }

    /* Draw the current frame */
    display->drawBitmap(x, y, _frames[_sequence[_currentFrame]], width, height, WHITE);
}

/****************************************************
 *
 * Marquee
 *
 ****************************************************/

Marquee::Marquee(std::string text)
{
    message newMessage;
    newMessage.text = text;
    newMessage.displayed_text = text;
    /* If the text is longer than the display, add a space to the end to create a gap */
    if (text.length() > MAX_DISPLAYED_CHARACTERS) {
        newMessage.displayed_text.append("   ");
    }
    _messages.push_back(newMessage);
}

void
Marquee::addSource(std::function<std::string()> source, std::string label)
{

    if (!dynamic) {
        return;
    }

    message newMessage;
    newMessage.source = source;
    newMessage.label = label;
    _messages.push_back(newMessage);
}

void
Marquee::addText(std::string text)
{
    if (dynamic) {
        return;
    }
    message newMessage;
    newMessage.text = text;
    newMessage.displayed_text = text;
    if (text.length() > MAX_DISPLAYED_CHARACTERS) {
        newMessage.displayed_text.append("   ");
    }
    _messages.push_back(newMessage);
}

void
Marquee::setSpeed(uint16_t speed)
{
    _speed = speed;
}

void
Marquee::setSwitchInterval(uint16_t interval)
{
    _switchInterval = interval;
}

void
Marquee::draw(uint16_t x, uint16_t y, uint16_t width)
{
    if (_messages.size() == 0) {
        return; /* No messages to display */
    }

    refresh();

    size_t count = _messages.size();
    while (_messages[currentMessage].displayed_text == "" && count) {

        currentMessage++;
        if (currentMessage >= _messages.size()) {
            currentMessage = 0;
        }
        count--;
        refresh();
    }

    if (count == 0) {
        return; /* No messages to display */
    }

    std::string _displayed_text = _messages[currentMessage].displayed_text;

    /* Trim the string to the width given */
    if (_displayed_text.length() > width) {
        _displayed_text = _displayed_text.substr(0, width);
    }

    display->setCursor(x, y);
    display->print((_messages[currentMessage].label + _displayed_text).c_str());

    /* Add spaces to fill out the rest of the display, in case this class is being used as a cursor */
    if (_displayed_text.length() < MAX_DISPLAYED_CHARACTERS + 2) {
        for (uint8_t i = 0; i < MAX_DISPLAYED_CHARACTERS + 2 - _displayed_text.length(); i++) {
            display->print(" ");
        }
    }

    rotateText();

    if (_switchTimer.check(_switchInterval)) {
        currentMessage++;
        if (currentMessage >= _messages.size()) {
            currentMessage = 0;
        }
    }
}

void
Marquee::refresh()
{
    if (!dynamic) {
        return;
    }
    if (_messages.size() == 0) {
        return;
    }

    /* Check the source for new data */
    if (_messages[currentMessage].source != nullptr) {
        std::string newText = _messages[currentMessage].source();
        if (newText != _messages[currentMessage].text) {
            _messages[currentMessage].text = newText;
            _messages[currentMessage].displayed_text = newText;
            if (newText.length() > MAX_DISPLAYED_CHARACTERS - _messages[currentMessage].label.length()) {
                _messages[currentMessage].displayed_text.append("   ");
            }
        }
    }
}

void
Marquee::rotateText()
{
    if (_messages.size() == 0) {
        return;
    }

    if (_animationTimer.check(_speed)) {
        if (_messages[currentMessage].text.length() > MAX_DISPLAYED_CHARACTERS - _messages[currentMessage].label.length()) {
            std::rotate(_messages[currentMessage].displayed_text.begin(),
                        _messages[currentMessage].displayed_text.begin() + 1,
                        _messages[currentMessage].displayed_text.end());
        }
    }
}

/****************************************************
 *
 * Progress Bar
 *
 ****************************************************/

void
ValueIndicator::draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t value)
{
    display->drawRect(x, y, width, height, WHITE);
    uint16_t fillWidth = map(value, _minVal, _maxVal, 0, width);
    display->fillRect(x, y, fillWidth, height, WHITE);
}

/****************************************************
 *
 * Spectrum Analyzer
 *
 ****************************************************/

SpectrumAnalyzer::SpectrumAnalyzer()
{
    if (!transport) {
        return;
    }
    bands = transport->spectrumAnalyzer->getBands();
    _currentVal = new uint16_t[bands];
    _peak = new uint16_t[bands];
    memset(_currentVal, 0, sizeof(uint16_t) * bands);
    memset(_peak, 0, sizeof(uint16_t) * bands);
}

void
SpectrumAnalyzer::draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if (_updateTimer.check(refresh_interval)) {
        transport->spectrumAnalyzer->getVals(_currentVal, _peak);
    }
    
    /* Draw each line, with the width given, in pixels */
    uint8_t count = bands;
    for (uint8_t i = 0; i < bands; i++) {
        bool peak_visible = transport->spectrumAnalyzer->isPeakVisible(i);
        count--;
        uint16_t val = map(_currentVal[i], 0, 2048, 0, height);
        if (val > height) {
            val = height;
        }
        uint16_t peak = map(_peak[i], 0, 2048, 0, height);
        if (peak > height) {
            peak = height;
        }
        for (uint8_t j = 0; j < width; j++) {
            uint8_t shift = count * width;
            display->drawFastVLine(x + shift + j + count, y + height - val, val, WHITE);
            if (peak_visible) {
                display->drawFastVLine(x + shift + j + count, y + height - peak, 1, WHITE);
            }
            shift = (i + width) * width;
            display->drawFastVLine(x + shift + j + i + (bands * width), y + height - val, val, WHITE);
            if (peak_visible) {
                display->drawFastVLine(x + shift + j + i + (bands * width), y + height - peak, 1, WHITE);
            }
        }
    }
}