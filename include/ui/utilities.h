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

#ifndef utilities_h
#define utilities_h

#include <ui/common.h>

namespace UI {

class Vector2D
{
  public:
    Vector2D() : x(0), y(0) {}
    Vector2D(float x, float y) : x(x), y(y) {}

    float getX() { return x; }
    float getY() { return y; }

    void setX(float x) { this->x = x; }
    void setY(float y) { this->y = y; }

    Vector2D operator+(Vector2D &other) { return Vector2D(x + other.x, y + other.y); }
    Vector2D operator-(Vector2D &other) { return Vector2D(x - other.x, y - other.y); }
    Vector2D operator*(float scalar) { return Vector2D(x * scalar, y * scalar); }
    Vector2D operator/(float scalar) { return Vector2D(x / scalar, y / scalar); }
    Vector2D operator=(Vector2D &other) { return Vector2D(other.x, other.y); }


  private:
    float x;
    float y;
};

class Camera;

/* This class represents a 3D vectorThe X, Y, and Z values are
 * represented as floats with 0.0 being the center. They are then scaled and
 * using the camera's position and rotation, the final 2D position is calculated. */
class Vector3D
{
  public:
    Vector3D() : _x(0), _y(0), _z(0), _camera(nullptr), _resolution(nullptr), _scale(nullptr) {}
    Vector3D(float x, float y, float z) : _x(x), _y(y), _z(z), _camera(nullptr), _resolution(nullptr), _scale(nullptr) {}
    Vector3D(float x, float y, float z, Camera *camera, Vector2D resolution, float scale) : _x(x), _y(y), _z(z), _camera(nullptr), _resolution(nullptr), _scale(nullptr) {}

    /* Convert the 3D vector to a 2D vector */
    Vector2D to2D() const;

    void setResolution(Vector2D resolution) { this->_resolution = &resolution; }
    void setCamera(Camera *camera) { this->_camera = camera; }
    void setScale(float *scale) { this->_scale = scale; } 

    Vector3D get() { return *this; }
    float getX() { return _x; }
    float getY() { return _y; }
    float getZ() { return _z; }

    void set(Vector3D vector) { _x = vector.getX(); _y = vector.getY(); _z = vector.getZ(); }
    void setX(float x) { this->_x = x; }
    void setY(float y) { this->_y = y; }
    void setZ(float z) { this->_z = z; }

    Vector3D operator+(Vector3D &other) { return Vector3D(_x + other._x, _y + other._y, _z + other._z, _camera, *_resolution, *_scale); }
    Vector3D operator-(Vector3D &other) { return Vector3D(_x - other._x, _y - other._y, _z - other._z, _camera, *_resolution, *_scale); }
    Vector3D operator*(float scalar) { return Vector3D(_x * scalar, _y * scalar, _z * scalar, _camera, *_resolution, *_scale); }
    Vector3D operator/(float scalar) { return Vector3D(_x / scalar, _y / scalar, _z / scalar, _camera, *_resolution, *_scale); }
    Vector3D operator=(Vector3D &other) { return Vector3D(other._x, other._y, other._z, _camera, *_resolution, *_scale); }

  private:
    float _x;
    float _y;
    float _z;

    /* For converting 3D to 2D */
    float *_scale = nullptr;

    Vector2D *_resolution = nullptr;

    Camera *_camera = nullptr;
  
};

class Rot3D
{
  public:
    Rot3D(float x, float y, float z) : x(x), y(y), z(z) {}

    void setX(float x) { this->x = x; }
    void setY(float y) { this->y = y; }
    void setZ(float z) { this->z = z; }

    float getX() { return x; }
    float getY() { return y; }
    float getZ() { return z; }

  private:
    float x;
    float y;
    float z;
};

class Camera
{
  public:
    Camera(Vector3D position, Rot3D rotation) : position(position), rotation(rotation) {}
    void lookAt(Vector3D target);
    Vector3D getPosition() { return position; }
    Rot3D getRotation() { return rotation; }
    void setPosition(Vector3D position) { this->position = position; }
    void setRotation(Rot3D rotation) { this->rotation = rotation; }

  private:
    Vector3D position = Vector3D(0.5, 0.5, 0.5);
    Rot3D rotation = Rot3D(0, 0, 0);

};

class Display : public Adafruit_SSD1306
{
  public:
    void drawLine3D(Vector3D start, Vector3D end);
    void drawRect3D(Vector3D start, Vector3D end);
    void drawCircle3D(Vector3D center, float radius);
    void drawTriangle3D(Vector3D a, Vector3D b, Vector3D c);
    void drawCube3D(Vector3D center, Rot3D rotation, float size);

  private:

    Camera camera = Camera(Vector3D(0.5, 0.5, 0.5, &camera, Vector2D(128, 64), 1.0), Rot3D(0, 0, 0));

};

} // namespace UI

#endif