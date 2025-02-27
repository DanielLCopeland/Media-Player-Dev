/**
 * @file playlist_engine.cpp
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

#include <playlist_engine2.h>

Playlist_Engine2* Playlist_Engine2::_handle = nullptr;

Playlist_Engine2::Playlist_Engine2()
{
    instance_type = instance_type_t::MAIN;
}

Playlist_Engine2::Playlist_Engine2(Playlist_Engine2* instance)
{
    instance_type = instance_type_t::SUB;
    _playlist_engine = instance;
}

Playlist_Engine2::~Playlist_Engine2()
{
    if (instance_type == instance_type_t::MAIN) {
        if (_handle != nullptr) {
            delete _handle;
            _handle = nullptr;
        }
    }
}

Playlist_Engine2::error_t
Playlist_Engine2::begin()
{
    return ERROR_NONE;
}

Playlist_Engine2::error_t
Playlist_Engine2::add_playlist(MediaData playlist, std::string name)
{

    sqlite3* db;
    FsFile playlist_file;
    DIR* file = nullptr;

    /* Check if the playlist folder exists, if not, create it */
    file = opendir(PLAYLIST_DIR_PATH);
    if (file == nullptr) {
        if (mkdir(PLAYLIST_DIR_PATH, 0777) == -1) {
            log_e("Failed to create playlist directory: %s", PLAYLIST_DIR_PATH);
            return ERROR_FAILURE;
        }
    } else {
        closedir(file);
    }

    /* Open the database */
    if (sqlite3_open(PLAYLIST_DB_PATH, &db) != SQLITE_OK) {
        log_e("Failed to open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return ERROR_FAILURE;
    }

    /** Parse the playlist file and add entries to the database */
    std::regex url_pattern(R"(https?://[^\s/$.?#].[^\s]*)");
    std::string path = playlist.getPath();
    std::ifstream file_stream(path.c_str());
    if (!file_stream.is_open()) {
        log_e("Failed to open the file: %s", path.c_str());
        return ERROR_FAILURE;
    }

    /* Create the table if it doesn't exist */
    char sql[256];
    sprintf(sql,
            "CREATE TABLE IF NOT EXISTS %s (id INTEGER PRIMARY KEY, filename TEXT, path TEXT, url TEXT, type INTEGER, source INTEGER);",
            escape_single_quotes(name).c_str());
    if (sqlite3_exec(db, sql, NULL, 0, NULL) != SQLITE_OK) {
        log_e("Failed to create table: %s", sqlite3_errmsg(db));
        file_stream.close();
        sqlite3_close(db);
        return ERROR_FAILURE;
    }

    if (load(name) != ERROR_NONE) {
        log_e("Failed to load playlist: %s", name.c_str());
        sqlite3_close(db);
        file_stream.close();
        return ERROR_FAILURE;
    }

    std::string line;
    while (std::getline(file_stream, line)) {
        std::sregex_iterator begin(line.begin(), line.end(), url_pattern);
        std::sregex_iterator end;

        for (std::sregex_iterator i = begin; i != end; ++i) {
            std::smatch match = *i;
            std::string url = match.str();
            log_i("Found URL: %s", url.c_str());
            // Add the URL to the database
            if (add_track(MediaData("", "", url, FILETYPE_M3U, 0, REMOTE_FILE, true)) == ERROR_NONE) {
                log_i("Added URL to playlist: %s", name.c_str());
            } else {
                log_e("Failed to add URL to playlist: %s", name.c_str());
                file_stream.close();
                sqlite3_close(db);
                return ERROR_FAILURE;
            }
        }
    }
    file_stream.close();
    sqlite3_close(db);
    return ERROR_NONE;
}

Playlist_Engine2::error_t
Playlist_Engine2::load(std::string name)
{
    eject();

    std::string _playlist_name;
    sqlite3* db;
    char sql[256];
    sprintf(sql, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", escape_single_quotes(name).c_str());

    /* Open the database and verify the table exists */
    if (sqlite3_open(PLAYLIST_DB_PATH, &db) != SQLITE_OK) {
        log_e("Failed to open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return ERROR_FAILURE;
    }

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        log_e("Failed to prepare statement: %s", sqlite3_errmsg(db));
        return ERROR_FAILURE;
    }

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = true;
    }

    if (!exists) {
        log_e("Playlist does not exist: %s", name.c_str());
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return ERROR_NOT_FOUND;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    current_playlist = name;
    _is_loaded = true;
    return ERROR_NONE;
}

void
Playlist_Engine2::eject()
{
    current_playlist = "";
    current_track = MediaData();
    _is_loaded = false;
}

Playlist_Engine2::error_t
Playlist_Engine2::next()
{
    eject();
    if (!Card_Manager::get_handle()->isReady()) {
        log_e("SD card not ready!");
        return ERROR_FAILURE;
    }

    if (!_is_loaded) {
        log_e("No playlist loaded!");
        return ERROR_INVALID;
    }

    /* Get the next track from the currently loaded playlist, if it exists */
    MediaData track;
    Playlist_Engine2::error_t err = get_track(current_track_id + 1, track);
    if (err != ERROR_NONE) {
        log_e("Failed to retrieve next track: %d", err);
        return err;
    } else {
        current_track_id++;
        current_track = track;
        return ERROR_NONE;
    }
}

Playlist_Engine2::error_t
Playlist_Engine2::previous()
{
    eject();

    if (!_is_loaded) {
        log_e("No playlist loaded!");
        return ERROR_INVALID;
    }

    if (current_track_id == 0) {
        log_e("Already at the beginning of the playlist!");
        return ERROR_INVALID;
    }

    /* Get the previous track from the currently loaded playlist, if it exists */
    MediaData track;
    Playlist_Engine2::error_t err = get_track(current_track_id - 1, track);
    if (err != ERROR_NONE) {
        log_e("Failed to retrieve previous track: %d", err);
        return err;
    } else {
        current_track_id--;
        current_track = track;
        return ERROR_NONE;
    }
}

Playlist_Engine2::error_t
Playlist_Engine2::get_track(size_t id, MediaData& track)
{
    if (!_is_loaded) {
        log_e("No playlist loaded!");
        return ERROR_INVALID;
    }

    /* Get the track from the currently loaded playlist, if it exists */
    sqlite3* db;
    char sql[256];
    sprintf(sql, "SELECT * FROM %s WHERE id = %d;", escape_single_quotes(current_playlist).c_str(), id);

    /* Open the database and retrieve the track */
    if (sqlite3_open(PLAYLIST_DB_PATH, &db) != SQLITE_OK) {
        log_e("Failed to open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return ERROR_FAILURE;
    }

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        log_e("Failed to prepare statement: %s", sqlite3_errmsg(db));
        return ERROR_FAILURE;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        track.filename = std::string((const char*) sqlite3_column_text(stmt, 1));
        track.path = std::string((const char*) sqlite3_column_text(stmt, 2));
        track.url = std::string((const char*) sqlite3_column_text(stmt, 3));
        track.type = (uint8_t) sqlite3_column_int(stmt, 4);
        track.source = (uint8_t) sqlite3_column_int(stmt, 5);
    } else {
        log_e("Failed to retrieve track: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return ERROR_NOT_FOUND;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    track.loaded = true;
    return ERROR_NONE;
}

Playlist_Engine2::error_t
Playlist_Engine2::set_current_track(size_t track)
{
    if (!Card_Manager::get_handle()->isReady()) {
        log_e("SD card not ready!");
        return ERROR_FAILURE;
    }

    if (!_is_loaded) {
        log_e("No playlist loaded!");
        return ERROR_INVALID;
    }

    if (track_exists(track)) {

        /* If we made it this far, the track exists. Set it as the current track */
        if (get_track(track, current_track) == ERROR_NONE) {
            current_track_id = track;
            return ERROR_NONE;
        } else {
            return ERROR_FAILURE;
        }
    } else {
        return ERROR_NOT_FOUND;
    }
}

bool
Playlist_Engine2::track_exists(size_t track)
{
    if (!Card_Manager::get_handle()->isReady()) {
        log_e("SD card not ready!");
        return false;
    }

    if (!_is_loaded) {
        log_e("No playlist loaded!");
        return false;
    }

    sqlite3* db;
    char sql[256];

    /* Open the database and verify the track exists */
    if (sqlite3_open(PLAYLIST_DB_PATH, &db) != SQLITE_OK) {
        log_e("Failed to open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return ERROR_FAILURE;
    }

    sprintf(sql, "SELECT COUNT(*) FROM %s WHERE id = %zu;", escape_single_quotes(current_playlist).c_str(), track);
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        log_e("Failed to prepare statement: %s", sqlite3_errmsg(db));
        return false;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return true;
    } else {
        log_e("Track not found: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }
}

Playlist_Engine2::error_t
Playlist_Engine2::add_track(MediaData track)
{

    if (!_is_loaded) {
        log_e("No playlist loaded!");
        return ERROR_INVALID;
    }

    sqlite3* db;
    char sql[256];

    /* Open the database and add the track */
    if (sqlite3_open(PLAYLIST_DB_PATH, &db) != SQLITE_OK) {
        log_e("Failed to open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return ERROR_FAILURE;
    }

    sprintf(sql,
            "INSERT INTO %s (filename, path, url, type, source) VALUES ('%s','%s', '%s', %d, %d);",
            escape_single_quotes(current_playlist).c_str(),
            track.filename.c_str(),
            track.path.c_str(),
            track.url.c_str(),
            track.type,
            track.source);
    if (sqlite3_exec(db, sql, NULL, 0, NULL) != SQLITE_OK) {
        log_e("Failed to insert track: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return ERROR_FAILURE;
    }
    return ERROR_NONE;
}