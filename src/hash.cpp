/**
 * @file hash.h
 *
 * @brief Hash class, used for checksumming and verifying integrity of the
 * filesystem
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

#include <hash.h>

Hash::Hash(uint32_t* data) : _data(data)

{
}

uint32_t
Hash::get()
{
    uint8_t len = sizeof(uint32_t);

    uint32_t hash = len, tmp;
    uint8_t rem;

    if (_data == NULL)
        return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (; len > 0; len--) {
        hash += get16bits(_data);
        tmp = (get16bits(_data + 2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        _data += 2 * sizeof(uint16_t);
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3:
            hash += get16bits(_data);
            hash ^= hash << 16;
            hash ^= _data[sizeof(uint16_t)] << 18;
            hash += hash >> 11;
            break;
        case 2:
            hash += get16bits(_data);
            hash ^= hash << 11;
            hash += hash >> 17;
            break;
        case 1:
            hash += *_data;
            hash ^= hash << 10;
            hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

uint32_t Hash::salted(uint32_t salt)
{
    return get() ^ salt;
}