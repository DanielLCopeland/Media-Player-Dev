/**
 * @file animation.h
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

#ifndef animation_h
#define animation_h

#include <ui/common.h>

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

    /* Draw the animation at the specified location */
    void draw(size_t x, size_t y, size_t width, size_t height);

    /* Set the frames for the animation from a bitmap array */
    void setFrames(const unsigned char* const frames[], uint8_t numFrames);

    /* Set the order in which the frames are displayed */
    void setSequence(uint8_t* sequence, uint8_t len);

    /* Set the duration of each frame in milliseconds */
    void setDuration(uint16_t duration);

  private:

    /* The pointer to the array of bitmap frames */
    unsigned char** _frames = nullptr;

    uint8_t _numFrames = 0;
    uint8_t _currentFrame = 0;
    Timer _animationTimer;
    std::vector<uint8_t> _sequence;

    /* The duration of each frame in milliseconds */
    uint16_t _duration = 100;
};


#endif