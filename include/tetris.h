/**
 * @file tetris.h
 *
 * @author Dan Copeland
 *
 * @brief Tetris game header file
 */
 
/*
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

#ifndef tetris_h
#define tetris_h

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <array>
#include <buttons.h>
#include <random>
#include <system.h>
#include <timer.h>
#include <ui/common.h>
#include <vector>
#include <queue>

extern Buttons* buttons;
extern Adafruit_SSD1306* display;

/**
 * @brief Tetris game data
 */
namespace Tetris_Data {

/**
 * @brief A 4x4 grid representing a Tetris piece, implemented as a
 * 16 byte array of type uint8_t. These are divided into four 4-bit 
 * rows (ignoring the most significant 4 bits), with each bit
 * representing a block in the piece. The first byte is the top row of the
 * piece, and the last byte is the bottom row. 
 */
typedef const uint8_t Shape;

/* Shape I */
Shape I[16] = {
  0b0000,
  0b0000, 
  0b1111,
  0b0000,
  
  0b0010,
  0b0010,
  0b0010,
  0b0010,

  0b0000,
  0b0000,
  0b1111,
  0b0000,

  0b0010,
  0b0010,
  0b0010,
  0b0010
};

/* Shape J */
Shape J[16] = {
  0b0000,
  0b1110,
  0b0010, 
  0b0000,
  
  0b0100,
  0b0100,
  0b1100,
  0b0000,

  0b1000,
  0b1110,
  0b0000,
  0b0000,

  0b0110,
  0b0100,
  0b0100,
  0b0000  
};

/* Shape L */
Shape L[16] = {  
  0b0000,
  0b1110, 
  0b1000, 
  0b0000,
  
  0b1100,
  0b0100,
  0b0100,
  0b0000,

  0b0010,
  0b1110,
  0b0000,
  0b0000,

  0b0100,
  0b0100,
  0b0110,
  0b0000
};

/* Shape O */
Shape O[16] = { 
  0b0000, 
  0b0110,
  0b0110, 
  0b0000,

  0b0000, 
  0b0110,
  0b0110, 
  0b0000,

  0b0000, 
  0b0110,
  0b0110, 
  0b0000,

  0b0000, 
  0b0110,
  0b0110, 
  0b0000,
};

Shape S[16] = {
  0b0000, 
  0b0110, 
  0b1100, 
  0b0000,

  0b0100,
  0b0110,
  0b0010,
  0b0000,

  0b0000,
  0b0110,
  0b1100,
  0b0000,

  0b0100,
  0b0110,
  0b0010,
  0b0000
};

/* Shape T */
Shape T[16] = {
  0b0000, 
  0b1110, 
  0b0100, 
  0b0000,

  0b0100,
  0b1100,
  0b0100,
  0b0000,

  0b0100,
  0b1110,
  0b0000,
  0b0000,

  0b0100,
  0b0110,
  0b0100,
  0b0000
};

/* Shape Z */
Shape Z[16] = {
  0b0000, 
  0b1100, 
  0b0110, 
  0b0000,

  0b0010,
  0b0110,
  0b0100,
  0b0000,

  0b0000,
  0b1100,
  0b0110,
  0b0000,

  0b0010,
  0b0110,
  0b0100,
  0b0000
};

/**
 * @brief Configuration settings for the Tetris game, such as display resolution,
 * playfield size, and game speed.
 *  
 */
enum Constants : uint16_t
{
    /** 
     * @brief Display resolution width 
     */
    RESOLUTION_WIDTH = 128,
    /** 
     * @brief Display resolution height 
     */
    RESOLUTION_HEIGHT = 32,

    /** 
     * @brief Playfield width in blocks 
     */
    PLAYFIELD_WIDTH = 10,
    /** 
     * @brief Playfield height in blocks 
     */
    PLAYFIELD_HEIGHT = 26,

    /** 
     * @brief Used to adapt to different screen orientations. Default is 270 degrees, which is portrait mode 
     */
    GAME_ROTATION = 90,

    /** @note The following constants are based on the display resolution. It is
     * recommended to use values that are multiples of the resolution width and height to ensure the display
     * looks sharp with no aliasing artifacts. */

    /** 
     * @brief Viewport x coordinate for the upper left corner after rotation 
     */
    VIEWPORT_X1 = 0,

    /** 
     * @brief Viewport y coordinate for the upper left corner after rotation 
     */
    VIEWPORT_Y1 = 0,

    /** 
     * @brief Viewport x coordinate for the lower right corner after rotation 
     */
    VIEWPORT_X2 = 100,

    /**
     * @brief Viewport y coordinate for the lower right corner after rotation 
     */
    VIEWPORT_Y2 = 32,

    /** 
     * @brief Game speed, one second per tick. Decrease to speed up the game 
     */
    GAME_SPEED = 1000,

    /** 
     * @brief The number of lines to clear before increasing the game speed 
     */
    LINES_PER_LEVEL = 10,

    /** 
     * @brief The number of pieces to queue up 
     */
    PIECE_QUEUE_SIZE = 7
};

/**
 * @brief Collision types
 */
enum CollisionType : uint8_t
{
    FLOOR,
    WALL,
    PIECE,
    NONE
};

/**
 * @brief Directions for movement, rotation, and collision detection
 */
enum Direction : uint8_t
{
    LEFT,
    RIGHT,
    DOWN,
    ROTATION_LEFT,
    ROTATION_RIGHT
};

} /* End namespace Tetris_Data */

/**
 * @class Tetris
 * @brief Main Tetris game class
 */
class Tetris
{
    /**
     * @class Piece 
     * @brief Represents a Tetris piece
     */
    class Piece
    {
      public:
        Piece();

        /** 
         * @brief Piece ID corresponding to the shape of the piece 
         * @return Pointer to the ID of the piece 
         */
        const uint8_t* const getID() { return &id; }

        /** 
         * @brief Rotates the piece 90 degrees to the left 
         */
        void rotateLeft();

        /** 
         * @brief Rotates the piece 90 degrees to the right
         */
        void rotateRight();

        /**
         * @brief Sets the rotation of the piece
         * @param rotation The rotation of the piece in degrees (0, 90, 180, 270)
         */
        void setRotation(uint16_t rotation);

        /**
         * @brief Returns a pointer to the rotation of the piece
         * @return The rotation of the piece in degrees (0, 90, 180, 270)
         */
        const uint16_t* const getRotation() { return &rotation; }

        /**
         * @brief Loads a random shape
         * @param seed The seed for the random number generator
         * @return True if the shape was loaded successfully
         */
        bool loadShape(uint32_t seed = 0);

        /**
         * @brief Returns the shape of the piece as a 4x4 grid
         * @return An array of 4 bytes representing the shape of the piece
         */
        std::array<uint8_t, 4> getShape() { return _rotatedShape; }

        /** 
         * @brief Sets the x-coordinate of the piece 
         */
        void setX(int16_t x) { this->x = x; }

        /** 
         * @brief Sets the y-coordinate of the piece 
         */
        void setY(int16_t y) { this->y = y; }

        /**
         * @brief Returns the x-coordinate of the piece
         * @return A pointer to the x-coordinate, nullptr if the piece is not loaded
         */
        const int16_t* const getX() { return &x; }

        /**
         * @brief Returns the y-coordinate of the piece
         * @return A pointer to the y-coordinate, nullptr if the piece is not loaded
         */
        const int16_t* const getY() { return &y; }

      private:
        Tetris_Data::Shape* getRandomShape(uint32_t seed = 0);
        Tetris_Data::Shape* _shape;
        std::array<uint8_t, 4> _rotatedShape;
        uint16_t rotation = 0;
        int16_t x;
        int16_t y;
        bool loaded = false;
        uint8_t id;
    };

  public:
    Tetris();

    /**
     * @brief Starts the game
     */
    void run();

  private:
    uint16_t border_height;
    uint16_t border_width;
    std::vector<std::vector<bool>> playField;
    Piece currentPiece;
    Piece nextPiece;
    uint32_t score = 0;
    bool checkCollision(Piece& piece, Tetris_Data::Direction direction);
    void draw();
    uint8_t clearLines();
    void gameOver();
    void reset();
    Timer gameTick;
    uint16_t blocksize_x;
    uint16_t blocksize_y;
    void rotateCoordinates(int16_t& x, int16_t& y);
    uint16_t game_speed = Tetris_Data::Constants::GAME_SPEED;
    uint8_t linesCleared = 0;

    /**
     * @brief The queue of pieces to be displayed next
     */
    std::vector<Piece> pieceQueue;

    /**
     * @brief The x coordinate of the next piece in the queue
     */
    int16_t next_x;

    /**
     * @brief The y coordinate of the next piece in the queue
     */
    int16_t next_y;

    /**
     * @brief Draws the next piece in the queue between the playfield and the top of the game screen, depending on the game rotation
     */
    void drawNextPiece();

    /**
     * @brief Generates a new piece and adds it to the queue, while removing the first piece in the queue
     */
    void generateNextPiece();

    UI::SystemMessage systemMessage;
};

#endif
