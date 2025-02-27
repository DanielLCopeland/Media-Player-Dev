/**
 * @file playlist_engine.h
 *
 * @brief Playlist Controller class. Feed it a playlist MediaData struct
 * (FILETYPE_M3U) and it will return back the files in the playlist in order.
 * It has a few methods to allow you to move through the playlist or change
 * the order of the playback (shuffle, etc).
 *
 * Callbacks allow the controller to load the next file in the playlist
 * into the transport.
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

#ifndef playlist_engine2_h
#define playlist_engine2_h

#include <SdFat.h>
#include <card_manager.h>
#include <utilities.h>
#include <sqlite3.h>
#include <system.h>
#include <transport.h>
#include <ui/common.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
#include <regex>
#include <string>

#define PLAYLIST_DIR_PATH "/playlists"
#define PLAYLIST_DB_PATH "/playlists/.playlists.db"

class MediaData;
class Transport;

class Playlist_Engine2
{
    enum error_t
    {
        ERROR_NONE,
        ERROR_FAILURE,
        ERROR_NOT_FOUND,
        ERROR_ALREADY_EXISTS,
        ERROR_INVALID,
    };

    enum instance_type_t
    {
        MAIN,
        SUB,
    };

    enum playlist_mode_t
    {
        NORMAL,
        SHUFFLE,
    };

    enum playlist_state_t
    {
        STOPPED,
        PLAYING,
        PAUSED,
    };

  public:
    Playlist_Engine2(Playlist_Engine2* _playlist_engine);
    ~Playlist_Engine2();

    void get_list(std::vector<MediaData>* data, uint32_t index, uint32_t count);

    error_t begin();
    bool end();
    playlist_state_t is_playing() { return status; }
    error_t next();
    error_t previous();
    void eject();
    error_t load(std::string name);
    void loop();
    void play();
    void stop();
    bool is_loaded() { return _is_loaded; }
    size_t size();
    error_t get(size_t from, size_t to, std::vector<MediaData>& items);
    MediaData get_current_track() { return current_track; }
    error_t set_current_track(size_t track);
    bool is_driver() { return instance_type; }
    bool track_exists(size_t track);
    error_t add_track(MediaData track);
    error_t remove_track(size_t track);
    uint16_t available();
    error_t get_track(size_t id, MediaData& track);
    error_t add_playlist(MediaData playlist, std::string name);
    error_t remove_playlist(size_t playlist);
    error_t create_playlist(std::string name);
    static Playlist_Engine2* get_handle() {
        if (!_handle) {
            _handle = new Playlist_Engine2();
        }
        return _handle;
    }

  private:
    Playlist_Engine2();
    error_t create_playlist_db();
    instance_type_t instance_type;
    playlist_state_t status;
    playlist_mode_t playlist_mode;
    static Playlist_Engine2* _handle;
    Playlist_Engine2* _playlist_engine = nullptr;
    size_t current_track_id = 0;
    std::string current_playlist;
    bool _is_loaded = false;
    MediaData current_track = MediaData();
    Playlist_Engine2* _playlist_engine_main;
};

#endif