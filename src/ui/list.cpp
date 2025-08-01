/**
 * @file list.cpp
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

#include <ui/list.h>

UI::ListSelection::ListSelection()
{
    if (marquee == nullptr) {
        marquee = new UI::Marquee();
    }
    marquee->addSource(std::bind(&ListSelection::getSelected, this));
    marquee->setSpeed(100);
}

UI::ListSelection::~ListSelection()
{
    if (marquee != nullptr) {
        delete marquee;
        marquee = nullptr;
    }
}

int32_t
UI::ListSelection::_get()
{
    /* Main loop */
    while (true) {

        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, SHORTPRESS))
            cursorUp();

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, SHORTPRESS))
            cursorDown();

        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(load_item, load_item_len);
            _refresh = true;
            return current_position.index;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            _refresh = true;
            return UI_EXIT;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            _refresh = true;
            return UI_BACK;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_MENU, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_open, folder_close_len);
            _refresh = true;
            if (_callback) {
                this->_callback();
            }
        }

        /* Longpress events */

        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            cursorUp();
            Buttons::get_handle()->repeat(BUTTON_UP);
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            cursorDown();
            Buttons::get_handle()->repeat(BUTTON_DOWN);
        }

        draw();
    }

    return 0;
}

int32_t
UI::ListSelection::get(const char* const menuItems[], uint16_t numItems)
{
    menu_type = MENU_TYPE_CONST_CHAR;

    this->numItems = numItems;
    this->menuItems = menuItems;

    current_position.cursor = 0;
    current_position.page = 1;
    current_position.index = 0;

    return _get();
}

int32_t
UI::ListSelection::get(std::vector<std::string>& listItems)
{
    menu_type = MENU_TYPE_STRING_VECTOR;

    this->listItems_ptr = &listItems;
    numItems = listItems.size();

    current_position.cursor = 0;
    current_position.page = 1;
    current_position.index = 0;

    return _get();
}

int32_t
UI::ListSelection::get(PlaylistEngine* playlist_engine, bool playlist_showindex)
{
    if (!playlist_engine->isLoaded()) {
        log_e("Playlist is empty!");
        return UI_EXIT;
    }

    this->_playlist_engine = playlist_engine;
    this->_playlist_showindex = playlist_showindex;

    menu_type = MENU_TYPE_PLAYLIST;
    numItems = _playlist_engine->size();
    current_position.index = _playlist_engine->getCurrentTrackIndex();

    /* Calculate the page and cursor position based on the selected item index */
    current_position.page = (current_position.index / MAX_TEXT_LINES) + 1;
    current_position.index = current_position.index % MAX_TEXT_LINES;
    current_position.cursor = defaultIndex;

    return _get();
}

std::vector<MediaData>
UI::ListSelection::getDisplayedItems()
{
    std::vector<MediaData> displayedItems;
    MediaData menudata;
    uint8_t lines = MAX_TEXT_LINES;
    uint16_t start_index = (current_position.page - 1) * lines;
    uint16_t end_index = std::min((int) current_position.page * (int) lines, (int) numItems);

    for (uint16_t i = start_index; i < end_index; i++) {
        switch (menu_type) {
            case MENU_TYPE_CONST_CHAR:
                if (i < numItems)
                    menudata.text = menuItems[i];
                menudata.type = FILETYPE_TEXT;
                displayedItems.push_back(menudata);
                break;
            case MENU_TYPE_STRING_VECTOR:
                if (i < numItems)
                    menudata.text = (*listItems_ptr)[i];
                menudata.type = FILETYPE_TEXT;
                displayedItems.push_back(menudata);
                break;
            case MENU_TYPE_PLAYLIST:
                if (i < numItems) {
                    menudata = _playlist_engine->getTrack(i);
                    if (menudata.source == LOCAL_FILE && menudata.loaded) {
                        menudata.text = menudata.filename;
                    } else if (menudata.source == REMOTE_FILE && menudata.loaded) {
                        menudata.text = menudata.url;
                    }
                    menudata.type = FILETYPE_TEXT;
                    displayedItems.push_back(menudata);
                }
            default:
                break;
        }
    }

    if (menu_type == MENU_TYPE_CUSTOM) {
        if ((current_position.page - 1) * MAX_TEXT_LINES + MAX_TEXT_LINES > numItems) {
            lines = numItems - (current_position.page - 1) * MAX_TEXT_LINES;
        }
        _get_list(&displayedItems, (current_position.page - 1) * MAX_TEXT_LINES, lines);
    }

    return displayedItems;
}

void
UI::ListSelection::draw()
{
    display->clearDisplay();
    if (Screensaver::get_handle()->is_blanked()) {
        display->display();
        return;
    }
    display->setTextSize(1);
    display->setTextWrap(false);

    uint8_t offset = 0; /* How many pixels to shift the text to the right to make room for icons */

    if (_refresh) {
        displayedItems = getDisplayedItems();
        _refresh = false;
    }

    if (numItems == 0) {
        display->setTextColor(WHITE, BLACK);
        display->print("No items found!");
        display->display();
        return;
    }
    selected_item = displayedItems[current_position.cursor].text;

    for (uint16_t i = 0; i < displayedItems.size(); i++) {
        display->setCursor(0, i * 8);

        /* Print out the index number of the item */
        if (menu_type == MENU_TYPE_PLAYLIST && _playlist_engine) {
            if (_playlist_showindex && i == _playlist_engine->getCurrentTrackIndex() && _playlist_engine->isDriver())
                display->setTextColor(BLACK, WHITE);
            else
                display->setTextColor(WHITE, BLACK);
            display->print(i);
            display->print(":");
        }

        if (displayedItems[i].type != FILETYPE_TEXT) {
            switch (displayedItems[i].type) {
                case FILETYPE_DIR:
                    display->drawBitmap(0, i * 8, bitmap_folder, 7, 7, WHITE);
                    break;
                case FILETYPE_M3U:
                    display->drawBitmap(0, i * 8, bitmap_playlist, 7, 7, WHITE);
                    break;
                default:
                    display->drawBitmap(0, i * 8, bitmap_note, 7, 7, WHITE);
                    break;
            }
            offset = 8;
            display->setCursor(offset, i * 8);
        }

        if (i == current_position.cursor) {
            display->setTextColor(BLACK, WHITE);
            marquee->draw(offset, i * 8);
        }

        else {
            display->setTextColor(WHITE, BLACK);
            display->print(displayedItems[i].text.c_str());
        }
    }
    display->display();
}

void
UI::ListSelection::cursorUp()
{
    if (current_position.cursor > 0) {
        current_position.cursor--;
        current_position.index--;
        Transport::get_handle()->playUIsound(click, click_len);
    }

    else if (current_position.cursor == 0 && current_position.page > 1) {
        current_position.page--;
        current_position.cursor = MAX_TEXT_LINES - 1;
        current_position.index--;
        _refresh = true;
        Transport::get_handle()->playUIsound(click, click_len);
    }
}

void
UI::ListSelection::cursorDown()
{
    if (current_position.cursor < MAX_TEXT_LINES - 1 && current_position.page <= numPages() && current_position.index < numItems - 1) {
        current_position.cursor++;
        current_position.index++;
        Transport::get_handle()->playUIsound(click, click_len);
    }

    else if (current_position.cursor == MAX_TEXT_LINES - 1 && current_position.page < numPages()) {
        current_position.page++;
        current_position.cursor = 0;
        current_position.index++;
        _refresh = true;
        Transport::get_handle()->playUIsound(click, click_len);
    }
}

uint32_t
UI::ListSelection::numPages()
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