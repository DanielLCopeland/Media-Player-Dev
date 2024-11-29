/**
 * @file animation.cpp
 *
 * @brief Draws an animation on the screen. Part of the UI library.
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
#include <ui/animation.h>

Animation::Animation(const unsigned char* const frames[], uint8_t numFrames)
{

    _frames = new unsigned char*[numFrames];
    /* Set up a basic sequence of frames */
    for (uint8_t i = 0; i < numFrames; i++) {
        _sequence.push_back(i);
    }
    for (uint8_t i = 0; i < numFrames; i++) {
        _frames[i] = (unsigned char*) frames[i];
    }
    _numFrames = numFrames;
}

void
Animation::setSequence(uint8_t* sequence, uint8_t len)
{
    _sequence.clear();
    for (uint8_t i = 0; i < len; i++) {
        if (sequence[i] < _numFrames) {
            _sequence.push_back(sequence[i]);
        } else {
            _sequence.push_back(0);
        }
    }
}

void
Animation::setDuration(uint16_t duration)
{
    _duration = duration;
}

void
Animation::setFrames(const unsigned char* const frames[], uint8_t numFrames)
{
    if (_frames != nullptr) {
        delete[] _frames;
    }
    _frames = new unsigned char*[numFrames];
    for (uint8_t i = 0; i < numFrames; i++) {
        _frames[i] = (unsigned char*) frames[i];
    }
    _numFrames = numFrames;

    /* Set up a new default sequence */
    _sequence.clear();
    for (uint8_t i = 0; i < numFrames; i++) {
        _sequence.push_back(i);
    }
}

void
Animation::draw(size_t x, size_t y, size_t width, size_t height)
{
    if (_frames == nullptr) {
        return;
    }

    if (_animationTimer.check(_duration)) {
        _currentFrame++;
        if (_currentFrame >= _sequence.size()) {
            _currentFrame = 0;
        }
    }

    /* Draw the current frame */
    display->drawBitmap(x, y, _frames[_sequence[_currentFrame]], width, height, WHITE);
}