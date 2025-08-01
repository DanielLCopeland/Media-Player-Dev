/**
 * @file input.cpp
 *
 * @brief Allows the user to input text. Part of the UI library.
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
#include <ui/input.h>

UI::TextInput::TextInput() {}

std::string
UI::TextInput::get(std::string prompt, std::string defaultText, uint8_t maxLength, uint8_t inputType)
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

        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            if (editMode)
                addChar(characterTable[characterTableIndex]);
            else
                moveCursorRight();
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            if (editMode)
                removeChar();
            else
                moveCursorLeft();
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            if (editMode) {
                input[stringIndex] = cursorCharacter;
            }
            return input;
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_MENU, SHORTPRESS)) {
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

        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, SHORTPRESS) && editMode)
            scrollUp();

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, SHORTPRESS) && editMode)
            scrollDown();

        /* Longpress events */
        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            scrollUp();
            Buttons::get_handle()->repeat(BUTTON_UP);
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            scrollDown();
            Buttons::get_handle()->repeat(BUTTON_DOWN);
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, LONGPRESS)) {
            if (editMode)
                addChar(characterTable[characterTableIndex]);
            else
                moveCursorRight();
            Buttons::get_handle()->repeat(BUTTON_PLAY);
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, LONGPRESS)) {
            if (editMode)
                removeChar();
            else
                moveCursorLeft();
            Buttons::get_handle()->repeat(BUTTON_STOP);
        }

        draw();
    }
}

/* Scrolls to the next character in the character table index */
void
UI::TextInput::scrollUp()
{
    if (characterTableIndex < characterTableLength - 1)
        characterTableIndex++;
    else
        characterTableIndex = 0;

    cursorCharacter = characterTable[characterTableIndex][0];
}

/* Scrolls to the previous character in the character table index */
void
UI::TextInput::scrollDown()
{
    if (characterTableIndex > 0)
        characterTableIndex--;
    else
        characterTableIndex = characterTableLength - 1;

    cursorCharacter = characterTable[characterTableIndex][0];
}

void
UI::TextInput::addChar(const char* c)
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
UI::TextInput::removeChar()
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
UI::TextInput::moveCursorLeft()
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
UI::TextInput::moveCursorRight()
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
UI::TextInput::getDisplayedInput()
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
UI::TextInput::draw()
{
    display->clearDisplay();
    if (Screensaver::get_handle()->is_blanked()) {
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