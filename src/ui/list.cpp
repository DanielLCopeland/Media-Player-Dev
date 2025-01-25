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

uint16_t
UI::ListSelection::_get()
{
    /* Main loop */
    while (true) {
        serviceLoop();

        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, SHORTPRESS))
            cursorUp();

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, SHORTPRESS))
            cursorDown();

        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(load_item, load_item_len);
            return selectedIndex;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            Transport::get_handle()->playUIsound(folder_close, folder_close_len);
            return UI_EXIT;
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

uint16_t
UI::ListSelection::get(const char* const menuItems[], uint16_t numItems)
{
    menu_type = MENU_TYPE_CONST_CHAR;

    this->numItems = numItems;
    this->menuItems = menuItems;

    cursor = 0;
    page = 1;
    selectedIndex = 0;

    return _get();
}

uint16_t
UI::ListSelection::get(std::vector<std::string>& listItems)
{
    menu_type = MENU_TYPE_STRING_VECTOR;

    this->listItems_ptr = &listItems;
    numItems = listItems.size();

    cursor = 0;
    page = 1;
    selectedIndex = 0;

    return _get();
}

uint16_t
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
    selectedIndex = _playlist_engine->getCurrentTrackIndex();

    /* Calculate the page and cursor position based on the selected item index */
    page = (selectedIndex / MAX_TEXT_LINES) + 1;
    defaultIndex = selectedIndex % MAX_TEXT_LINES;
    cursor = defaultIndex;

    return _get();
}

uint16_t
UI::ListSelection::get(File_Explorer* file_explorer)
{
    menu_type = MENU_TYPE_FILE_EXPLORER;
    numItems = file_explorer->size();
    selectedIndex = 0;
    page = 1;
    cursor = 0;
    _file_explorer = file_explorer;

    return _get();
}

std::vector<std::string>
UI::ListSelection::getDisplayedItems()
{
    std::vector<std::string> displayedItems;
    MediaData item;

    for (uint16_t i = (page - 1) * MAX_TEXT_LINES; i < (page * MAX_TEXT_LINES); i++) {
        switch (menu_type) {
            case MENU_TYPE_CONST_CHAR:
                if (i < numItems)
                    displayedItems.push_back(menuItems[i]);
                break;
            case MENU_TYPE_STRING_VECTOR:
                if (i < numItems)
                    displayedItems.push_back((*listItems_ptr)[i]);
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
            default:
                break;
        }
    }

    if (menu_type == MENU_TYPE_CUSTOM) {
        uint8_t lines = MAX_TEXT_LINES;

        if ((page - 1) * MAX_TEXT_LINES + MAX_TEXT_LINES > numItems) {
            lines = numItems - (page - 1) * MAX_TEXT_LINES;
        }
        _get_list(&displayedItems, (page - 1) * MAX_TEXT_LINES, lines, SORT_ASC, SORT_NAME);
    }

    if (menu_type == MENU_TYPE_FILE_EXPLORER) {
        uint8_t lines = MAX_TEXT_LINES;

        if ((page - 1) * MAX_TEXT_LINES + MAX_TEXT_LINES > numItems) {
            lines = numItems - (page - 1) * MAX_TEXT_LINES;
        }
        _file_explorer->get_list(&_mediadata_list, (page - 1) * MAX_TEXT_LINES, lines, SORT_ASC, SORT_NAME);
        for (uint8_t i = 0; i < _mediadata_list.size(); i++) {
            if (_mediadata_list[i].source == LOCAL_FILE && _mediadata_list[i].loaded) {
                displayedItems.push_back(_mediadata_list[i].filename);
            } else if (_mediadata_list[i].source == REMOTE_FILE && _mediadata_list[i].loaded) {
                displayedItems.push_back(_mediadata_list[i].url);
            }
        }
    }

    return displayedItems;
}

void
UI::ListSelection::draw()
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
        if (menu_type == MENU_TYPE_PLAYLIST && _playlist_engine) {
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
UI::ListSelection::cursorUp()
{
    if (cursor > 0) {
        cursor--;
        selectedIndex--;
        Transport::get_handle()->playUIsound(click, click_len);
    }

    else if (cursor == 0 && page > 1) {
        page--;
        cursor = MAX_TEXT_LINES - 1;
        selectedIndex--;
        Transport::get_handle()->playUIsound(click, click_len);
    }
}

void
UI::ListSelection::cursorDown()
{
    if (cursor < MAX_TEXT_LINES - 1 && page <= numPages() && selectedIndex < numItems - 1) {
        cursor++;
        selectedIndex++;
        Transport::get_handle()->playUIsound(click, click_len);
    }

    else if (cursor == MAX_TEXT_LINES - 1 && page < numPages()) {
        page++;
        cursor = 0;
        selectedIndex++;
        Transport::get_handle()->playUIsound(click, click_len);
    }
}

uint16_t
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