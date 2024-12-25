/**
 * @file gfx.h
 *
 * @brief Bitmaps and sprites
 *
 * @author Dan Copeland, Isaac Copeland
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

#ifndef gfx_h
#define gfx_h

/* File icons */

/* 'folder', 7x7px */
const unsigned char bitmap_folder[] PROGMEM = { 0xe0, 0xf0, 0xfe, 0x82, 0x82, 0x82, 0xfe };

/* 'note', 7x7px */
const unsigned char bitmap_note[] PROGMEM = { 0x3e, 0x22, 0x3e, 0x22, 0x22, 0xee, 0xee };

/* 'playlist', 7x7px */
const unsigned char bitmap_playlist[] PROGMEM = { 0xe8, 0xa8, 0xe8, 0x88, 0x8e, 0x00, 0xaa };

/* Transport status indicator sprite */
const unsigned char bitmap_sprite_playing_1[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x01, 0x40, 0x00, 0x01, 0x00, 0x06, 0xc1, 0x00,
                                                          0x0a, 0xa3, 0x00, 0x13, 0x93, 0x00, 0x12, 0x90, 0x00, 0x32, 0x98, 0x00, 0x52, 0x94, 0x00,
                                                          0xd2, 0x96, 0x00, 0xc9, 0x26, 0x00, 0x50, 0x14, 0x00, 0x20, 0x08, 0x00, 0x20, 0x08, 0x00,
                                                          0x40, 0x04, 0x00, 0x4c, 0x64, 0x00, 0x41, 0x04, 0x00, 0x20, 0x08, 0x00, 0x1f, 0xf0, 0x00 };

const unsigned char bitmap_sprite_playing_2[] PROGMEM = { 0x00, 0x03, 0x00, 0x00, 0x02, 0x80, 0x00, 0x02, 0x40, 0x00, 0x02, 0x20, 0x06, 0xc6, 0x20,
                                                          0x0a, 0xa6, 0x20, 0x13, 0x90, 0x20, 0x12, 0x90, 0x60, 0x32, 0x98, 0x60, 0x52, 0x94, 0x00,
                                                          0xd2, 0x96, 0x00, 0xc9, 0x26, 0x00, 0x50, 0x14, 0x00, 0x20, 0x08, 0x00, 0x20, 0x08, 0x00,
                                                          0x40, 0x04, 0x00, 0x4c, 0x64, 0x00, 0x41, 0x04, 0x00, 0x20, 0x08, 0x00, 0x1f, 0xf0, 0x00 };

const unsigned char bitmap_sprite_stopped_1[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xc0, 0x00,
                                                          0x0a, 0xa0, 0x00, 0x12, 0x90, 0x00, 0x12, 0x90, 0x00, 0x12, 0x90, 0x00, 0x12, 0x90, 0x00,
                                                          0x12, 0x90, 0x00, 0x09, 0x20, 0x00, 0x10, 0x10, 0x00, 0x20, 0x08, 0x00, 0x20, 0x08, 0x00,
                                                          0x44, 0x44, 0x00, 0x44, 0x44, 0x00, 0x41, 0x04, 0x00, 0x20, 0x08, 0x00, 0x1f, 0xf0, 0x00 };

const unsigned char bitmap_sprite_stopped_2[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xc0, 0x00,
                                                          0x0a, 0xa0, 0x00, 0x12, 0x90, 0x00, 0x12, 0x90, 0x00, 0x12, 0x90, 0x00, 0x12, 0x90, 0x00,
                                                          0x12, 0x90, 0x00, 0x09, 0x20, 0x00, 0x10, 0x10, 0x00, 0x20, 0x08, 0x00, 0x20, 0x08, 0x00,
                                                          0x40, 0x04, 0x00, 0x4c, 0x64, 0x00, 0x41, 0x04, 0x00, 0x20, 0x08, 0x00, 0x1f, 0xf0, 0x00 };

/* Create an array of arrays to hold the above bitmaps */
const unsigned char* const bunny_playing[] PROGMEM = { bitmap_sprite_playing_1, bitmap_sprite_playing_2};
const uint8_t bunny_playing_num_frames = 2;

const unsigned char* const bunny_stopped[] PROGMEM = { bitmap_sprite_stopped_1, bitmap_sprite_stopped_2};
const uint8_t bunny_stopped_num_frames = 2;

/* Bluetooth connection icon */
const unsigned char bitmap_bluetooth [] PROGMEM = { 0x00, 0x38, 0x44, 0xc6, 0xc6, 0x44, 0x00 };

/* Wifi connection icon */

/* 'wifi_1', 6x7px */
const unsigned char bitmap_wifi_1[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40 };

/* 'wifi_2', 6x7px */
const unsigned char bitmap_wifi_2[] PROGMEM = { 0x00, 0x00, 0x00, 0xc0, 0x20, 0x90, 0x50 };

/* 'wifi_3', 6x7px */
const unsigned char bitmap_wifi_3[] PROGMEM = { 0x00, 0xe0, 0x10, 0xc8, 0x24, 0x94, 0x54 };

/* 'wifi_error', 6x7px */
const unsigned char bitmap_wifi_error[] PROGMEM = { 0x04, 0x04, 0x04, 0xc0, 0x24, 0x90, 0x50 };

/* Volume icon */

/* 'volume_full', 7x7px */
const unsigned char bitmap_volume_full[] PROGMEM = { 0x08, 0x44, 0xd4, 0xd4, 0xd4, 0x44, 0x08 };

/* 'volume_low', 7x7px */
const unsigned char bitmap_volume_low[] PROGMEM = { 0x00, 0x40, 0xc0, 0xc0, 0xc0, 0x40, 0x00 };

/* 'volume_mid', 7x7px */
const unsigned char bitmap_volume_mid[] PROGMEM = { 0x00, 0x40, 0xd0, 0xd0, 0xd0, 0x40, 0x00 };

/* 'volume_muted', 7x7px */
const unsigned char bitmap_volume_muted[] PROGMEM = { 0x00, 0x40, 0xd4, 0xc8, 0xd4, 0x40, 0x00 };

/* Misc bitmaps */

/* 'Startup bitmap', 36x36px */
const unsigned char bitmap_startup_logo[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00,
    0x00, 0x01, 0xfc, 0x00, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 0x01, 0xff, 0x80, 0x00, 0x00,
    0x01, 0xf1, 0x80, 0x00, 0x00, 0x00, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0xe0, 0x40, 0x00, 0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x70, 0x20, 0x00, 0x00, 0x00,
    0x30, 0x20, 0x00, 0x30, 0x00, 0x38, 0x00, 0x00, 0xfe, 0x07, 0xfc, 0x00, 0x01, 0xff, 0x1f, 0xfc, 0x00, 0x01, 0xff, 0xff, 0xfe, 0x00, 0x03, 0xff, 0xff, 0xfe,
    0x00, 0x03, 0xbf, 0xff, 0xfe, 0x00, 0x07, 0x3f, 0xff, 0xfe, 0x00, 0x07, 0xff, 0xff, 0xfc, 0x00, 0x03, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x7f, 0xff, 0xf8, 0x00,
    0x01, 0xff, 0xff, 0xf8, 0x00, 0x0f, 0x87, 0xff, 0xf8, 0x00, 0x00, 0x07, 0xfc, 0x38, 0x00, 0x00, 0x1f, 0xf0, 0x78, 0x00, 0x00, 0x7f, 0x01, 0xf8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif