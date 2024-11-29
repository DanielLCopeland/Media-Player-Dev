/**
 * @file utilities.h
 *
 * @brief Various utility functions and classes for the UI library.
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

UI::Vector2D
UI::Vector3D::to2D() const
{
    if (!_camera || !_resolution || !_scale) {
        return Vector2D();
    }

    // Translate the point based on the camera position
    float translatedX = _x - _camera->getPosition().getX();
    float translatedY = _y - _camera->getPosition().getY();
    float translatedZ = _z - _camera->getPosition().getZ();

    // Apply rotation based on the camera rotation
    float cosX = cos(_camera->getRotation().getX());
    float sinX = sin(_camera->getRotation().getX());
    float cosY = cos(_camera->getRotation().getY());
    float sinY = sin(_camera->getRotation().getY());
    float cosZ = cos(_camera->getRotation().getZ());
    float sinZ = sin(_camera->getRotation().getZ());

    // Rotate around X axis
    float rotatedX = translatedX;
    float rotatedY = translatedY * cosX - translatedZ * sinX;
    float rotatedZ = translatedY * sinX + translatedZ * cosX;

    // Rotate around Y axis
    float tempX = rotatedX * cosY + rotatedZ * sinY;
    float tempZ = -rotatedX * sinY + rotatedZ * cosY;
    rotatedX = tempX;
    rotatedZ = tempZ;

    // Rotate around Z axis
    tempX = rotatedX * cosZ - rotatedY * sinZ;
    float tempY = rotatedX * sinZ + rotatedY * cosZ;
    rotatedX = tempX;
    rotatedY = tempY;

    float scale = &_scale ? *_scale : 1.0;

    // Apply scale
    rotatedX *= scale;
    rotatedY *= scale;

    // Convert to 2D coordinates based on screen resolution
    float screenX = (_resolution->getX() / 2) + rotatedX;
    float screenY = (_resolution->getY() / 2) - rotatedY;

    return Vector2D(screenX, screenY);
}
