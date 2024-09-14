/**
 * @file ui.h
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

#ifndef ui_h
#define ui_h

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <WiFi.h>
#include <bluetooth.h>
#include <buttons.h>
#include <card_manager.h>
#include <data.h>
#include <filesystem.h>
#include <gfx.h>
#include <playlist_engine.h>
#include <screensaver.h>
#include <string>
#include <system.h>
#include <timer.h>
#include <transport.h>
#include <ui_sounds.h>
#include <vector>

// Display config
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  32
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

#define SELECTED_ITEM_ROTATE_SPEED                                                                                                                             \
    150                                 // The speed at which the selected item moves across the screen if it's
                                        // too long to fit
#define MAX_TEXT_LINES          4       // The maximum number of lines of text that can fit on the screen at once
#define MAX_CHARACTERS_PER_LINE 19      // The maximum number of characters that can fit on a line
#define UI_EXIT                 65534   // The value returned when the user exits a UI element
#define UI_EXIT_TIMEOUT         4000    // The time in milliseconds before a UI element exits automatically

class Adafruit_SSD1306;
class CardManager;
class Bluetooth;
class Transport;
class Filesystem;
class Buttons;
class MediaData;
class TableData;
class PlaylistEngine;
class Screensaver;

extern Buttons* buttons;
extern CardManager* sdfs;
extern Bluetooth* bluetooth;
extern Transport* transport;
extern Adafruit_SSD1306* display;
extern PlaylistEngine* playlistEngine;
extern Screensaver* screensaver;

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

    Filesystem* filesystem = nullptr;

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
};

/****************************************************
 *
 * List selection generator
 *
 ****************************************************/

enum menuType
{
    MENU_TYPE_CONST_CHAR,
    MENU_TYPE_STRING_VECTOR,
    MENU_TYPE_DATATABLE,
    MENU_TYPE_PLAYLIST
};
class Marquee;
class ListSelection
{
  public:
    ListSelection();
    ~ListSelection();

    /* Overloads for different data sources */
    uint16_t get(const char* const menuItems[], uint16_t numItems);
    uint16_t get(std::vector<std::string>& listItems);

    /* Returns the index of the selected item from a TableData object.  The
    TableData object must be passed by reference. Originaly built to parse
    the contents of a timezone array in timezones.h */
    uint16_t get(TableData& table);

    /* Returns the index of the selected item from a PlaylistEngine object.
    If playlist_showindex is true, the index of the currently selected track
    will be highlighted in the list. */
    uint16_t get(PlaylistEngine* playlist, bool playlist_showindex = false);

    std::string getSelected() { return selected_item; }

  private:
    uint16_t get();
    uint16_t selectedIndex = 0;
    uint16_t cursor = 0;
    uint16_t page = 1;
    const char* const* menuItems = nullptr;
    std::vector<std::string>* listItems_ptr = nullptr;
    TableData* table = nullptr;
    PlaylistEngine* _playlist_engine = nullptr;
    bool _playlist_showindex = false;
    uint16_t defaultIndex = 0;
    uint16_t numItems = 0;
    uint8_t menuType;
    void draw();
    void cursorUp();
    void cursorDown();
    std::vector<std::string> getDisplayedItems();
    std::string selected_item = "";
    Marquee* marquee = nullptr;
    uint16_t numPages();
};

/****************************************************
 *
 * User Input
 *
 ****************************************************/

#define MAX_DISPLAYED_CHARACTERS 21
#define CURSOR_BLINK_INTERVAL    250

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

/****************************************************
 *
 * Status Screen
 *
 ****************************************************/

class Animation;
class Marquee;
class SpectrumAnalyzer;
class StatusScreen
{
  public:
    StatusScreen();
    void draw();

  private:
    uint16_t* spectrumAnalyzerCurrentVal;
    uint16_t* spectrumAnalyzerPeak;
    size_t playTime = 0;
    Animation* anim_playing = nullptr;
    Animation* anim_stopped = nullptr;
    Marquee* marquee_mediainfo = nullptr;
    Marquee* marquee_datetime = nullptr;
    Marquee* marquee_connectStatus = nullptr;
    SpectrumAnalyzer* spectrumAnalyzer = nullptr;
};

/****************************************************
 *
 * Notification
 *
 ****************************************************/

#define NOTIFICATION_ANIMATION_FRAME_DURATION 250

class SystemMessage
{
  public:
    SystemMessage();

    void show(std::string message, uint16_t duration, bool animated);
    void resetAnimation();   // Resets the animation frame to 0

  private:
    Timer notificationTimer;
    Timer animationTimer;
    std::string message = "";
    uint16_t duration = 0;
    uint8_t animationFrame = 0;   // The current frame of the animation, 0-20

    void draw();
};

/****************************************************
 *
 * Value Selector
 *
 ****************************************************/
class ValueSelector
{
  public:
    ValueSelector(std::string prompt, uint16_t minVal = 0, uint16_t maxVal = 100, uint16_t step = 1, uint16_t value = 0);

    ValueSelector(std::string prompt,
                  std::function<uint8_t()> valueCallback = NULL,
                  std::function<void()> incCallback = NULL,
                  std::function<void()> decCallback = NULL,
                  uint8_t minVal = 0,
                  uint8_t maxVal = 100);

    uint16_t get();

  private:
    std::string _prompt;
    uint16_t _value = 0;
    uint16_t cursor = 0;
    uint16_t _minVal = 0;
    uint16_t _maxVal = 0;
    uint16_t _step = 0;
    std::function<uint8_t()> _valueCallback;
    std::function<void()> _incCallback;
    std::function<void()> _decCallback;
    bool useCallbacks = false;
    Timer exitTimer;

    void draw();
    void inc();
    void dec();
};

/****************************************************
 *
 * Binary Selector
 *
 ****************************************************/
class BinarySelector
{
  public:
    BinarySelector(std::string falseText = "No", std::string trueText = "Yes");

    bool get();

  private:
    std::vector<std::string> _options;

    void draw();
    void toggle();
};

/****************************************************
 *
 * Animation
 *
 ****************************************************/
class Animation
{
  public:
    Animation();
    Animation(const unsigned char* const frames[], uint8_t numFrames);
    ~Animation()
    {
        if (_frames != nullptr) {
            delete[] _frames;
        }
    }
    void draw(size_t x, size_t y, size_t width, size_t height);
    void setFrames(const unsigned char* const frames[], uint8_t numFrames); /* Set the frames for the animation from a bitmap array */
    void setSequence(uint8_t* sequence, uint8_t len);                       /* Set the order in which the frames are displayed */
    void setDuration(uint16_t duration);                                    /* Set the duration of each frame in milliseconds */

  private:
    unsigned char** _frames = nullptr; /* The pointer to the array of bitmap frames */
    uint8_t _numFrames = 0;
    uint8_t _currentFrame = 0;
    Timer _animationTimer;
    std::vector<uint8_t> _sequence;
    uint16_t _duration = 100; /* The duration of each frame in milliseconds */
};

/****************************************************
 *
 * Marquee
 *
 ****************************************************/
class Marquee

{
  public:
    Marquee()
      : dynamic(true)
    {
    }
    Marquee(std::string text);

    void draw(uint16_t x, uint16_t y, uint16_t width = MAX_DISPLAYED_CHARACTERS);
    void addSource(std::function<std::string()> source, std::string label = "");
    void addText(std::string text);
    void setSpeed(uint16_t speed);
    void setSwitchInterval(uint16_t interval);

  private:
    void refresh();
    void rotateText();
    bool dynamic = false;
    struct message
    {
        std::string label;
        std::string text;           /* The text to display, we save this so we can tell if the text has changed on the next refresh */
        std::string displayed_text; /* We rotate the displayed text to create the marquee effect */
        std::function<std::string()> source;
    };
    std::vector<message> _messages;
    uint8_t currentMessage = 0;
    uint16_t _speed = 100;
    uint16_t _switchInterval = 5000;
    Timer _animationTimer;
    Timer _switchTimer;
};

/****************************************************
 *
 * Spectrum Analyzer
 *
 ****************************************************/

class SpectrumAnalyzer
{
  public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer()
    {
        if (_currentVal != nullptr) {
            delete[] _currentVal;
        }
        if (_peak != nullptr) {
            delete[] _peak;
        }
    }

    void draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

  private:
    Timer _updateTimer;
    uint8_t bands = 0;
    uint16_t* _currentVal = nullptr;
    uint16_t* _peak = nullptr;
    static const uint16_t refresh_interval = 20;
};

/****************************************************
 *
 * Value Indicator (Progress Bar, etc.)
 *
 ****************************************************/

class ValueIndicator
{

    enum valueIndicatorType : uint8_t
    {
        PROGRESS_BAR,
        VOLUME_BAR,
        BATTERY_BAR,
        SCROLL_BAR,
        WIFI_SIGNAL
    };

  public:
    ValueIndicator(uint16_t minVal = 0, uint16_t maxVal = 100, valueIndicatorType type = PROGRESS_BAR)
      : _minVal(minVal)
      , _maxVal(maxVal)
      , _type(type)
    {
    }

  private:
    uint16_t _minVal = 0;
    uint16_t _maxVal = 0;
    valueIndicatorType _type = PROGRESS_BAR;

    void draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t _value = 0);
};

#endif