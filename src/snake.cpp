/**
 * @file snake.cpp
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

#include <snake.h>

Snake::Snake()
{
    this->snake.push_back(Item{ Constants::PLAYFIELD_WIDTH / 2, Constants::PLAYFIELD_HEIGHT / 2 });
    this->snake.push_back(Item{ Constants::PLAYFIELD_WIDTH / 2 - Constants::SCALE_FACTOR, Constants::PLAYFIELD_HEIGHT / 2 });
    this->snake.push_back(Item{ Constants::PLAYFIELD_WIDTH / 2 - Constants::SCALE_FACTOR * 2, Constants::PLAYFIELD_HEIGHT / 2 });

    /* Initialize the food */
    generateFood();

    /* Set the initial direction */
    this->direction = Direction::RIGHT;

    /* Set the initial score */
    this->score = 0;
}

void
Snake::run()
{

    while (true) {

        serviceLoop();

        /* Check for button presses and change snake direction, but don't allow the snake to reverse direction */
        if (buttons->getButtonEvent(BUTTON_UP, SHORTPRESS) && this->direction != Direction::DOWN) {
            this->direction = Direction::UP;
        } else if (buttons->getButtonEvent(BUTTON_DOWN, SHORTPRESS) && this->direction != Direction::UP) {
            this->direction = Direction::DOWN;
        } else if (buttons->getButtonEvent(BUTTON_PLAY, SHORTPRESS) && this->direction != Direction::RIGHT) {
            this->direction = Direction::LEFT;
        } else if (buttons->getButtonEvent(BUTTON_STOP, SHORTPRESS) && this->direction != Direction::LEFT) {
            this->direction = Direction::RIGHT;
        } else if (buttons->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            message.show("Exiting!\nScore: " + std::to_string(this->score), 2000, false);
            return;
        }

        if (timer.check(Constants::SNAKE_SPEED)) {
            Item snake_tail = this->snake.back();
            move();
            /* Check for collisions */
            switch (checkCollision()) {
                case CollisionType::COLLISION_FOOD:
                    generateFood();
                    snake.push_back(snake_tail); /* Grow the snake */
                    draw();
                    this->score++;
                    break;
                case CollisionType::COLLISION_WALL:
                    message.show("Collided with wall!\nScore: " + std::to_string(this->score), 2000, false);
                    return;
                    break;
                case CollisionType::COLLISION_SELF:
                    message.show("Collided with self!\nScore: " + std::to_string(this->score), 2000, false);
                    return;
                    break;
                case CollisionType::COLLISION_NONE:
                    draw();
            }
        }
    }
}

void
Snake::draw()
{

    /* Clear the display */
    display->clearDisplay();

    /* Draw the snake */
    for (auto it = this->snake.begin(); it != this->snake.end(); it++) {
        display->fillRect(it->x, it->y, Constants::SCALE_FACTOR, Constants::SCALE_FACTOR, WHITE);
    }

    /* Draw the food */
    display->fillRect(this->food.x, this->food.y, Constants::SCALE_FACTOR, Constants::SCALE_FACTOR, WHITE);

    /* Draw the walls, 2 pixels thick */
    for (uint8_t i = 0; i < Constants::SCALE_FACTOR; i++) {
        display->drawRect(i, i, Constants::PLAYFIELD_WIDTH - (i * 2), Constants::PLAYFIELD_HEIGHT - (i * 2), WHITE);
    }

    display->display();
}

void
Snake::generateFood(uint32_t seed)
{
    /* Random number generator */
    /* Use the milliseconds since boot as a seed */
    uint32_t _seed = millis() + seed;
    std::mt19937 gen(_seed);
    std::uniform_int_distribution<uint8_t> dis_x(Constants::SCALE_FACTOR, Constants::PLAYFIELD_WIDTH - Constants::SCALE_FACTOR);
    std::uniform_int_distribution<uint8_t> dis_y(Constants::SCALE_FACTOR, Constants::PLAYFIELD_HEIGHT - Constants::SCALE_FACTOR);

    /* Generate a new food item */
    this->food.x = dis_x(gen) / Constants::SCALE_FACTOR * Constants::SCALE_FACTOR;
    this->food.y = dis_y(gen) / Constants::SCALE_FACTOR * Constants::SCALE_FACTOR;

    /* Check if the food is on the snake */
    for (auto it = this->snake.begin(); it != this->snake.end(); it++) {
        if (this->food.x == it->x && this->food.y == it->y) {
            generateFood(_seed);
        }
    }
    /* Check if the food is on the walls */
    if (this->food.x >= Constants::PLAYFIELD_WIDTH - Constants::SCALE_FACTOR || this->food.x <= Constants::SCALE_FACTOR ||
        this->food.y >= Constants::PLAYFIELD_HEIGHT - Constants::SCALE_FACTOR || this->food.y <= Constants::SCALE_FACTOR) {
        generateFood(_seed);
    }
}

Snake::CollisionType
Snake::checkCollision()
{
    /* Check if the snake has collided with the food */
    if (this->snake.front().x == this->food.x && this->snake.front().y == this->food.y) {
        return CollisionType::COLLISION_FOOD;
    }

    /* Check if the snake has collided with itself */
    for (auto it = this->snake.begin() + 1; it != this->snake.end(); it++) {
        if (this->snake.front().x == it->x && this->snake.front().y == it->y) {
            return CollisionType::COLLISION_SELF;
        }
    }

    /* Check if the snake has collided with the walls */
    if (this->snake.front().x >= Constants::PLAYFIELD_WIDTH - Constants::SCALE_FACTOR || this->snake.front().x == 0 ||
        this->snake.front().y >= Constants::PLAYFIELD_HEIGHT - Constants::SCALE_FACTOR || this->snake.front().y == 0) {
        return CollisionType::COLLISION_WALL;
    }
    return CollisionType::COLLISION_NONE;
}

void
Snake::move()
{
    Item snake_head = this->snake.front();

    /* Move the snake by adding two pixels to the appropriate side */
    switch (this->direction) {
        case Direction::UP:
            snake_head.y -= Constants::SCALE_FACTOR;
            break;
        case Direction::DOWN:
            snake_head.y += Constants::SCALE_FACTOR;
            break;
        case Direction::LEFT:
            snake_head.x -= Constants::SCALE_FACTOR;
            break;
        case Direction::RIGHT:
            snake_head.x += Constants::SCALE_FACTOR;
            break;
    }

    /* Add the new head */
    this->snake.push_front(snake_head);

    /* Remove the tail */
    this->snake.pop_back();
}