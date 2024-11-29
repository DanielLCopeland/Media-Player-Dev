/**
 * @file input.h
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

#ifndef input_h
#define input_h

#include <ui/common.h>

namespace UI {

enum inputType
{
    INPUT_FORMAT_TEXT,
    INPUT_FORMAT_NUMERIC,
    INPUT_FORMAT_IPADDRESS,
    INPUT_FORMAT_PASSWORD,
    INPUT_FORMAT_DATE,
    INPUT_FORMAT_TIME,
    INPUT_FORMAT_SERVADDR
};

class TextInput
{
  public:
    TextInput();

    std::string get(std::string prompt, std::string defaultText, uint8_t maxLength, uint8_t inputType);

  private:
    std::string prompt; /* The prompt to display on the screen */
    std::string input = "";
    uint8_t cursor = 0;     /* The cursor position on the screen */
    uint8_t maxLength = 20; /* The maximum length of the string that will be displayed */
    const char* const* characterTable;
    char cursorCharacter;             /* The character at the cursor position */
    uint16_t characterTableLength;    /* The length of the character table */
    uint16_t characterTableIndex = 0; /* The index of the character table that is currently being displayed */
    bool editMode = false;            /* Determine if we move the cursor or insert/delete characters */
    uint8_t stringIndex = 0;          /* The index where the cursor is in the string */
    uint8_t inputType = 0;            /* Determines if the input is hidden, used for passwords */
    Timer cursorBlinkTimer;           /* Timer to control the cursor blink */
    bool cursorBlink = true;          /* Determines if the cursor is currently being displayed */

    void draw();                     /* Draws the text input screen */
    void scrollUp();                 /* Changes the character at the cursor position by scrolling through the character table */
    void scrollDown();               /* Changes the character at the cursor position by scrolling through the character table */
    void addChar(const char* c);     /* Adds a character at the cursor position and shifts the rest of the string to the right */
    void removeChar();               /* Removes the character at the cursor position and shifts the rest of the string to the left */
    void moveCursorLeft();           /* Moves the cursor left without deleting a character. If the cursor is at the beginning of the string, it
                                        does nothing. */
    void moveCursorRight();          /* Moves the cursor right without adding a character.  If the cursor is at the end of the string, it does nothing */
    std::string getDisplayedInput(); /* Returns the string currently selected by the cursor */
};

}   // namespace UI

#endif