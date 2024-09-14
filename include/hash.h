/**
 * @file hash.h
 *
 * @brief Hashing function.  Used in this program for checksums, mainly.
 *
 * @author Dan Copeland, Paul Hsieh
 * (http://www.azillionmonkeys.com/qed/hash.html)
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

#include <Arduino.h>

#ifndef hash_h
#define hash_h

#ifndef get16bits
#define get16bits(d) ((((uint32_t) (((const uint8_t*) (d))[1])) << 8) + (uint32_t) (((const uint8_t*) (d))[0]))
#endif

class Hash
{
  public:
    /* Hashes the given data */
    Hash(uint32_t* data);

    /* Returns the computed hash of the value given in the constructor */
    uint32_t get();

    /* Applies a salt to the hash and returns the result */
    uint32_t salted(uint32_t salt);

  private:
    uint32_t *_data;
};

#endif