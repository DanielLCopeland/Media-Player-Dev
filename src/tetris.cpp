/**
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
 * 
 * @file tetris.cpp
 * @author Daniel Copeland
 * @brief 
 * @date 2024-11-29
 * 
 * @copyright Copyright (c) 2024
 * 
 *
 */

#include <tetris.h>

Tetris::Tetris()
{
    /* Get block size, rounded to the lowest whole number */
    switch (Tetris_Data::Constants::GAME_ROTATION) {
        case 90:
            blocksize_y = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_Y2 - 1) - (Tetris_Data::Constants::VIEWPORT_Y1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_WIDTH);
            blocksize_x = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_X2 - 1) - (Tetris_Data::Constants::VIEWPORT_X1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_HEIGHT);
            border_width = blocksize_x * Tetris_Data::Constants::PLAYFIELD_HEIGHT + blocksize_x;
            border_height = Tetris_Data::Constants::VIEWPORT_Y2 - Tetris_Data::Constants::VIEWPORT_Y1;
            next_x = ((Tetris_Data::Constants::RESOLUTION_WIDTH - Tetris_Data::Constants::VIEWPORT_X2) / 2) + Tetris_Data::Constants::VIEWPORT_X2;
            next_y = (Tetris_Data::Constants::RESOLUTION_HEIGHT) / 2;
            break;
        case 180:
            blocksize_y = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_X2 - 1) - (Tetris_Data::Constants::VIEWPORT_X1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_WIDTH);
            blocksize_x = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_Y2 - 1) - (Tetris_Data::Constants::VIEWPORT_Y1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_HEIGHT);
            border_width = Tetris_Data::Constants::VIEWPORT_X2 - Tetris_Data::Constants::VIEWPORT_X1;
            border_height = blocksize_y * Tetris_Data::Constants::PLAYFIELD_HEIGHT + blocksize_y;
            next_x = (Tetris_Data::Constants::RESOLUTION_WIDTH) / 2;
            next_y = ((Tetris_Data::Constants::RESOLUTION_HEIGHT - Tetris_Data::Constants::VIEWPORT_Y2) / 2) + Tetris_Data::Constants::VIEWPORT_Y2;
            break;
        case 270:
            blocksize_y = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_Y2 - 1) - (Tetris_Data::Constants::VIEWPORT_Y1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_WIDTH);
            blocksize_x = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_X2 - 1) - (Tetris_Data::Constants::VIEWPORT_X1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_HEIGHT);
            border_width = blocksize_x * Tetris_Data::Constants::PLAYFIELD_HEIGHT + blocksize_x;
            border_height = Tetris_Data::Constants::VIEWPORT_Y2 - Tetris_Data::Constants::VIEWPORT_Y1;
            next_x = ((Tetris_Data::Constants::RESOLUTION_WIDTH - Tetris_Data::Constants::VIEWPORT_X2) / 2) + Tetris_Data::Constants::VIEWPORT_X2;
            next_y = (Tetris_Data::Constants::RESOLUTION_HEIGHT) / 2;
            break;
        default:
            blocksize_y = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_X2 - 1) - (Tetris_Data::Constants::VIEWPORT_X1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_WIDTH);
            blocksize_x = static_cast<uint16_t>(((Tetris_Data::Constants::VIEWPORT_Y2 - 1) - (Tetris_Data::Constants::VIEWPORT_Y1 + 1)) /
                                                Tetris_Data::Constants::PLAYFIELD_HEIGHT);
            border_width = Tetris_Data::Constants::VIEWPORT_X2 - Tetris_Data::Constants::VIEWPORT_X1;
            border_height = blocksize_y * Tetris_Data::Constants::PLAYFIELD_HEIGHT + blocksize_y;
            next_x = (Tetris_Data::Constants::RESOLUTION_WIDTH) / 2;
            next_y = ((Tetris_Data::Constants::RESOLUTION_HEIGHT - Tetris_Data::Constants::VIEWPORT_Y2) / 2) + Tetris_Data::Constants::VIEWPORT_Y2;
            break;
    }

    /* Reserve memory for the piece queue */
    pieceQueue.reserve(Tetris_Data::Constants::PIECE_QUEUE_SIZE);

    uint32_t seed = millis();

    /* Load up the piece queue with unique pieces */
    for (uint8_t i = 0; i < Tetris_Data::Constants::PIECE_QUEUE_SIZE; i++) {
        
        Piece piece;
        pieceQueue.push_back(piece);
        pieceQueue[i].loadShape(seed);

        /* Make sure the next piece is unique by comparing the shape id with each piece in the queue */
        while (true) {
            bool unique = true;
            for (uint8_t j = 0; j < i; j++) {
                if (pieceQueue[i].getID() == pieceQueue[j].getID()) {
                    unique = false;
                    break;
                }
            }
            if (unique) {
                break;
            }
            pieceQueue[i].loadShape(seed++);
        }
        pieceQueue[i].setRotation(Tetris_Data::Constants::GAME_ROTATION);
    }
    currentPiece = pieceQueue[0];
    nextPiece = pieceQueue[1];
};

void
Tetris::run()
{
    /* Clear the playfield */
    reset();

    /* Main game loop */
    while (true) {

        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, SHORTPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::LEFT)) {
                currentPiece.setX(*currentPiece.getX() - 1);
            }
        }
        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, SHORTPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::RIGHT)) {
                currentPiece.setX(*currentPiece.getX() + 1);
            }
        }
        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, SHORTPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::DOWN)) {
                currentPiece.setY(*currentPiece.getY() + 1);
            }
        }
        if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, SHORTPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::ROTATION_LEFT)) {
                currentPiece.rotateLeft();
            }
        }

        if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            break;
        }

        /* Check for longpress events */
        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, LONGPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::DOWN)) {
                currentPiece.setY(*currentPiece.getY() + 1);
                Buttons::get_handle()->repeat(BUTTON_PLAY);
            }
        }
        if (Buttons::get_handle()->getButtonEvent(BUTTON_UP, LONGPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::LEFT)) {
                currentPiece.setX(*currentPiece.getX() - 1);
                Buttons::get_handle()->repeat(BUTTON_UP);
            }
        }
        if (Buttons::get_handle()->getButtonEvent(BUTTON_DOWN, LONGPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::RIGHT)) {
                currentPiece.setX(*currentPiece.getX() + 1);
                Buttons::get_handle()->repeat(BUTTON_DOWN);
            }
        }
        if (Buttons::get_handle()->getButtonEvent(BUTTON_STOP, LONGPRESS)) {
            if (!checkCollision(currentPiece, Tetris_Data::Direction::ROTATION_LEFT)) {
                currentPiece.rotateLeft();
                Buttons::get_handle()->repeat(BUTTON_STOP);
            }
        }

        /* Check for a collision with the playfield */
        if (gameTick.check(Tetris_Data::Constants::GAME_SPEED)) {

            /* Drop the piece down one */
            if (!checkCollision(currentPiece, Tetris_Data::Direction::DOWN)) {
                currentPiece.setY(*currentPiece.getY() + 1);
            } else {

                /* If we can't move the piece down, then we need to lock it in place */
                for (uint8_t i = 0; i < 4; i++) {
                    uint8_t byte = currentPiece.getShape()[i];
                    for (uint8_t j = 0; j < 4; j++) {
                        if (byte & (0x01 << j)) {
                            playField[*currentPiece.getX() + i][*currentPiece.getY() + j] = true;
                        }
                    }
                }

                /* Check for any lines that need to be cleared */
                uint8_t lines = clearLines();
                if (lines) {
                    linesCleared += lines;
                    if (linesCleared >= Tetris_Data::Constants::LINES_PER_LEVEL) {
                        if (game_speed > 50)
                            game_speed -= 50;
                        linesCleared -= Tetris_Data::Constants::LINES_PER_LEVEL;
                    }
                    if (lines >= 4) {
                        for (uint8_t i = 0; i < lines; i++) {
                            score += 100;
                            lines -= 4;
                        }
                        score += 10 * lines;
                    } else {
                        score += 10 * lines;
                    }
                }

                /* Check for a collision with the new piece at the top of the playfield before spawning it */
                if (checkCollision(nextPiece, Tetris_Data::Direction::DOWN)) {
                    /* If we can't spawn the new piece, then the game is over */
                    systemMessage.show("Game Over!\nScore: " + std::to_string(score), 5000, false);
                    break;
                }

                /* Load the next piece */
                //currentPiece = nextPiece;
                //nextPiece.loadShape(millis());
                gameTick.reset();
                generateNextPiece();
            }
        }

        draw();
    }
}

Tetris_Data::Shape*
Tetris::Piece::getRandomShape(uint32_t seed)
{
    /* Random number generator */
    /* Use the milliseconds since boot as a seed, plus whatever seed was passed in */
    uint32_t _seed = millis() + seed;
    std::mt19937 gen(_seed);
    std::uniform_int_distribution<uint8_t> dis(0, 6);

    switch (dis(gen)) {
        case 0:
            return Tetris_Data::I;
            id = 0;
        case 1:
            return Tetris_Data::J;
            id = 1;
        case 2:
            return Tetris_Data::L;
            id = 2;
        case 3:
            return Tetris_Data::O;
            id = 3;
        case 4:
            return Tetris_Data::S;
            id = 4;
        case 5:
            return Tetris_Data::T;
            id = 5;
        case 6:
            return Tetris_Data::Z;
            id = 6;
    }
    return nullptr;
}

Tetris::Piece::Piece()
{
    this->_shape = nullptr;
    _rotatedShape = { 0, 0, 0, 0 };
}

void
Tetris::Piece::rotateLeft()
{
    this->rotation = (this->rotation + 90);
    if (this->rotation >= 360) {
        this->rotation = 0;
    }
    setRotation(this->rotation);
}

void
Tetris::Piece::rotateRight()
{
    if (this->rotation == 0) {
        this->rotation = 270;
    } else {
        this->rotation -= 90;
    }
    setRotation(this->rotation);
}

void
Tetris::Piece::setRotation(uint16_t rotation)
{
    if (rotation >= 360) {
        rotation = 0;
    }

    this->rotation = rotation;
    switch (this->rotation) {
        case 90:
            for (uint8_t i = 0; i < 4; i++) {
                _rotatedShape[i] = _shape[i];
            }
            break;
        case 180:
            for (uint8_t i = 0; i < 4; i++) {
                _rotatedShape[i] = _shape[i + 4];
            }
            break;
        case 270:
            for (uint8_t i = 0; i < 4; i++) {
                _rotatedShape[i] = _shape[i + 8];
            }
            break;
        default:
            for (uint8_t i = 0; i < 4; i++) {
                _rotatedShape[i] = _shape[i + 12];
            }
            break;
    }
}

bool
Tetris::Piece::loadShape(uint32_t seed)
{

    this->_shape = getRandomShape(seed);
    this->setRotation(Tetris_Data::Constants::GAME_ROTATION);

    for (uint8_t i = 0; i < 4; i++) {
        _rotatedShape[i] = _shape[i];
    }

    x = Tetris_Data::Constants::PLAYFIELD_WIDTH / 2 - 2;
    y = 0;

    return true;
}

bool
Tetris::checkCollision(Piece& piece, Tetris_Data::Direction direction)
{
    Piece _piece = piece;
    int16_t x = *_piece.getX();
    int16_t y = *_piece.getY();

    switch (direction) {
        case Tetris_Data::Direction::LEFT:
            x--;
            break;
        case Tetris_Data::Direction::RIGHT:
            x++;
            break;
        case Tetris_Data::Direction::DOWN:
            y++;
            break;
        case Tetris_Data::Direction::ROTATION_LEFT:
            _piece.rotateLeft();
            break;
        case Tetris_Data::Direction::ROTATION_RIGHT:
            _piece.rotateRight();
            break;
    }

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t byte = _piece.getShape()[i];
        for (uint8_t j = 0; j < 4; j++) {
            if (byte & (0x01 << j)) {
                if (x + i >= Tetris_Data::Constants::PLAYFIELD_WIDTH || x + i < 0) {
                    return true;
                }
                if (y + j >= Tetris_Data::Constants::PLAYFIELD_HEIGHT) {
                    return true;
                }
                if (playField[x + i][y + j]) {
                    return true;
                }
            }
        }
    }
    return false;
}

uint8_t
Tetris::clearLines()
{
    uint8_t linesCleared = 0;
    for (uint8_t i = 0; i < Tetris_Data::Constants::PLAYFIELD_HEIGHT; i++) {
        bool clear = true;
        for (uint8_t j = 0; j < Tetris_Data::Constants::PLAYFIELD_WIDTH; j++) {
            if (!playField[j][i]) {
                clear = false;
                break;
            }
        }

        if (clear) {
            linesCleared++;
            for (uint8_t j = 0; j < Tetris_Data::Constants::PLAYFIELD_WIDTH; j++) {
                playField[j].erase(playField[j].begin() + i);
                playField[j].insert(playField[j].begin(), false);
            }
        }
    }

    return linesCleared;
}

void
Tetris::reset()
{
    /* Reserves the memory for the playfield */
    playField.reserve(Tetris_Data::Constants::PLAYFIELD_WIDTH);
    for (uint8_t i = 0; i < Tetris_Data::Constants::PLAYFIELD_WIDTH; i++) {
        playField.push_back(std::vector<bool>());
        playField[i].reserve(Tetris_Data::Constants::PLAYFIELD_HEIGHT);
    }
    for (uint8_t i = 0; i < Tetris_Data::Constants::PLAYFIELD_WIDTH; i++) {
        playField[i].clear();
        for (uint8_t j = 0; j < Tetris_Data::Constants::PLAYFIELD_HEIGHT; j++) {
            playField[i].push_back(false);
        }
    }
    score = 0;
    /* Load the first piece */
    //currentPiece->loadShape(millis());
    //currentPiece->setRotation(Tetris_Data::Constants::GAME_ROTATION);
    //nextPiece->loadShape(millis() + 1);
    //nextPiece->setRotation(Tetris_Data::Constants::GAME_ROTATION);
}

void
Tetris::draw()
{
    display->clearDisplay();

    /* Iterate through the playfield and draw the pieces */
    for (uint8_t i = 0; i < Tetris_Data::Constants::PLAYFIELD_WIDTH; i++) {
        for (uint8_t j = 0; j < Tetris_Data::Constants::PLAYFIELD_HEIGHT; j++) {
            if (playField[i][j]) {
                int16_t x = i;
                int16_t y = j;
                rotateCoordinates(x, y);
                display->drawRect(x, y, blocksize_x, blocksize_y, SSD1306_WHITE);
            }
        }
    }

    /* Draw the current piece */
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t byte = currentPiece.getShape()[i];
        for (uint8_t j = 0; j < 4; j++) {
            if (byte & (0x01 << j)) {
                int16_t x = i + *currentPiece.getX();
                int16_t y = j + *currentPiece.getY();
                rotateCoordinates(x, y);
                display->drawRect(x, y, blocksize_x, blocksize_y, SSD1306_WHITE);
            }
        }
    }

    /* Draw a transparent rectangle around the playfield */
    display->drawRect(Tetris_Data::Constants::VIEWPORT_X1, Tetris_Data::Constants::VIEWPORT_Y1, border_width, border_height, SSD1306_WHITE);
    drawNextPiece();
    display->display();
}

void
Tetris::rotateCoordinates(int16_t& x, int16_t& y)
{
    int16_t virtual_x = x;
    int16_t virtual_y = y;
    float scaleX;
    float scaleY;
    int16_t viewport_width = (Tetris_Data::Constants::VIEWPORT_X2) - (Tetris_Data::Constants::VIEWPORT_X1);
    int16_t viewport_height = (Tetris_Data::Constants::VIEWPORT_Y2) - (Tetris_Data::Constants::VIEWPORT_Y1);

    /* Rotate coordinates based on the game rotation */
    int16_t rotatedX, rotatedY;
    switch (Tetris_Data::Constants::GAME_ROTATION) {
        case 90:
            rotatedX = Tetris_Data::Constants::PLAYFIELD_HEIGHT - 1 - virtual_y;
            rotatedY = virtual_x;
            rotatedX *= blocksize_x;
            rotatedY *= blocksize_y;
            scaleX = static_cast<float>((Tetris_Data::Constants::VIEWPORT_X2) - (Tetris_Data::Constants::VIEWPORT_X1)) / viewport_width;
            scaleY = static_cast<float>((Tetris_Data::Constants::VIEWPORT_Y2) - (Tetris_Data::Constants::VIEWPORT_Y1)) / viewport_height;
            break;

        case 180:
            rotatedX = Tetris_Data::Constants::PLAYFIELD_WIDTH - 1 - virtual_x;
            rotatedY = Tetris_Data::Constants::PLAYFIELD_HEIGHT - 1 - virtual_y;
            rotatedX *= blocksize_x;
            rotatedY *= blocksize_y;
            scaleX = static_cast<float>((Tetris_Data::Constants::VIEWPORT_Y2) - (Tetris_Data::Constants::VIEWPORT_Y1)) / viewport_width;
            scaleY = static_cast<float>((Tetris_Data::Constants::VIEWPORT_X2) - (Tetris_Data::Constants::VIEWPORT_X1)) / viewport_height;
            break;

        case 270:
            rotatedX = virtual_y;
            rotatedY = Tetris_Data::Constants::PLAYFIELD_WIDTH - 1 - virtual_x;
            rotatedX *= blocksize_x;
            rotatedY *= blocksize_y;
            scaleX = static_cast<float>((Tetris_Data::Constants::VIEWPORT_X2) - (Tetris_Data::Constants::VIEWPORT_X1)) / viewport_width;
            scaleY = static_cast<float>((Tetris_Data::Constants::VIEWPORT_Y2) - (Tetris_Data::Constants::VIEWPORT_Y1)) / viewport_height;
            break;

        default:
            rotatedX = virtual_x;
            rotatedY = virtual_y;
            rotatedX *= blocksize_x;
            rotatedY *= blocksize_y;
            scaleX = static_cast<float>((Tetris_Data::Constants::VIEWPORT_Y2) - (Tetris_Data::Constants::VIEWPORT_Y1)) / viewport_width;
            scaleY = static_cast<float>((Tetris_Data::Constants::VIEWPORT_X2) - (Tetris_Data::Constants::VIEWPORT_X1)) / viewport_height;
            break;
    }

    /* Scale coordinates to fit within the viewport */

    x = (static_cast<int16_t>(rotatedX * scaleX)) + 1; /* Offset by 1 to account for the border */
    y = (static_cast<int16_t>(rotatedY * scaleY)) + 1; /* Offset by 1 to account for the border */
}

void
Tetris::drawNextPiece()
{
    int16_t x;
    int16_t y;
    uint16_t rotation = *nextPiece.getRotation();
    nextPiece.setRotation(0);
    /* Draw the next piece */
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t byte = nextPiece.getShape()[i];
        for (uint8_t j = 0; j < 4; j++) {
            if (byte & (0x01 << j)) {
                switch (Tetris_Data::Constants::GAME_ROTATION) {
                    case 90:
                        x = (next_x - (blocksize_x * 2)) - (i * blocksize_x);
                        y = (next_y + (blocksize_y * 2)) - (j * blocksize_y);
                        break;
                    case 180:
                        x = (next_x - (blocksize_x * 2)) - (j * blocksize_x);
                        y = (next_y - (blocksize_y * 2)) + (i * blocksize_y);
                        break;
                    case 270:
                        x = (next_x - (blocksize_x * 2)) + (i * blocksize_x);
                        y = (next_y - (blocksize_y * 2)) + (j * blocksize_y);
                        break;
                    default:
                        x = (next_x - (blocksize_x * 2)) + (j * blocksize_x);
                        y = (next_y - (blocksize_y * 2)) - (i * blocksize_y);
                        break;
                }
                display->drawRect(x, y, blocksize_x, blocksize_y, SSD1306_WHITE);
            }
        }
    }
    nextPiece.setRotation(rotation);
}

void
Tetris::generateNextPiece()
{
    uint32_t seed = millis();

    /* Remove the first piece in the queue */
    pieceQueue.erase(pieceQueue.begin());

    /* Add a new piece to the end of the queue while ensuring it is unique */
    Piece piece;
    piece.loadShape(seed);
    pieceQueue.push_back(piece);
    while (true) {
        bool unique = true;
        for (uint8_t j = 0; j < pieceQueue.size() - 1; j++) {
            if (pieceQueue.back().getID() == pieceQueue[j].getID()) {
                unique = false;
                break;
            }
        }
        if (unique) {
            break;
        }
        piece.loadShape(seed++);
    }
    pieceQueue.back().setRotation(Tetris_Data::Constants::GAME_ROTATION);
    currentPiece = pieceQueue[0];
    nextPiece = pieceQueue[1];
}