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

#include <ui/common.h>
#include <ui/list.h>

UI::ListSelection::ListSelection() {
    if (marquee == nullptr) {
        marquee = new UI::Marquee();
    }
    marquee->addSource(std::bind(&ListSelection::getSelected, this));
    marquee->setSpeed(100);
}

UI::ListSelection::~ListSelection() {
    if (marquee != nullptr) {
        delete marquee;
        marquee = nullptr;
    }
}

uint16_t
UI::ListSelection::get()
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
UI::ListSelection::get(const char* const menuItems[], uint16_t numItems)
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
UI::ListSelection::get(std::vector<std::string>& listItems)
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
UI::ListSelection::get(TableData& table)
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
UI::ListSelection::get(PlaylistEngine* playlist_engine, bool playlist_showindex)
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
UI::ListSelection::getDisplayedItems()
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
UI::ListSelection::cursorUp()
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
UI::ListSelection::cursorDown()
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