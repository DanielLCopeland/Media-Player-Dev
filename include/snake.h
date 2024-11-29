/**
 * @file snake.h
 *
 * @brief Small snake game, meant to be hidden as an easter egg.
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

#ifndef snake_h
#define snake_h

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include <deque>
#include <random>
#include <ui/common.h>
#include <buttons.h>
#include <system.h>
#include <timer.h>

extern Buttons* buttons;
extern Adafruit_SSD1306* display;

class Snake
{

enum Direction : uint8_t
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

enum CollisionType : uint8_t
{
    COLLISION_FOOD,
    COLLISION_WALL,
    COLLISION_SELF,
    COLLISION_NONE
};

enum Constants : uint16_t
{
    SNAKE_SPEED = 300, /* ms */
    PLAYFIELD_WIDTH = UI::SCREEN_WIDTH,
    PLAYFIELD_HEIGHT = UI::SCREEN_HEIGHT,
    SCALE_FACTOR = 2 /* The size of the snake, food, and playfield. Must be a factor of the playfield width and height */
};

struct Item
{
    uint8_t x;
    uint8_t y;
};

    public:
        Snake();
        void run();
    
    private:
        void draw();
        void move();
        CollisionType checkCollision();
        void generateFood(uint32_t seed = 0);
        uint8_t direction;
        uint16_t score;
        Item food;
        std::deque<Item> snake;
        Timer timer;
        UI::SystemMessage message;
};

#endif