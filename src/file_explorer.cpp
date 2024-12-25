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
 * @file file_explorer.cpp
 * @author Daniel Copeland
 * @brief
 * @date 2024-02-02
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <file_explorer.h>

File_Explorer::File_Explorer()
{
    ready = false;
    sqlite3_initialize();
}

File_Explorer::~File_Explorer()
{

}

void
File_Explorer::init(MediaData& dir)
{
    log_i("Initializing file explorer");
    ready = true;
    directory_stack.clear();
    if (sdfs->isReady()) {
        if (open_dir(dir) == ERROR_NONE) {
        } else {
            log_e("Failed to initialize file explorer");
            ready = false;
        }
    }
}

void
File_Explorer::init()
{
    MediaData dir = { "/", "/", "", FILETYPE_DIR, 0, LOCAL_FILE, true };
    init(dir);
}

file_explorer_error_t
File_Explorer::open_dir(MediaData& mediadata)
{
    log_i("Opening directory %s", mediadata.getPath());

    if (mediadata.type != FILETYPE_DIR) {
        log_e("Invalid directory");
        return ERROR_INVALID;
    }

    if (ready) {
        ready = false;
    } else {
        close();
        log_e("File explorer not ready");
        return ERROR_FAILURE;
    }

    /* Always want to do a sanity check and make sure the SD card is still there before working with it */
    if (!sdfs->isReady()) {
        close();
        log_e("SD card not ready");
        return ERROR_FAILURE;
    }

    if (generate_index(mediadata) != ERROR_NONE) {
        log_e("Failed to generate index");
        return ERROR_FAILURE;
    }

    fill_dir_stack(mediadata);
    return ERROR_NONE;
    ready = true;
}

file_explorer_error_t
File_Explorer::exit_dir()
{
    if (!ready || !sdfs->isReady()) {
        return ERROR_FAILURE;
    }

    if (directory_stack.size() <= 1) {
        return ERROR_ROOT_DIR;
    }

    MediaData temp = directory_stack.back();
    directory_stack.pop_back();
    MediaData new_dir = directory_stack.back();
    if (open_dir(new_dir) != ERROR_NONE) {
        directory_stack.push_back(temp);
        return ERROR_FAILURE;
    }

    return ERROR_NONE;
}

bool
File_Explorer::is_root_dir(MediaData& mediadata)
{
    return mediadata.path == "/" && mediadata.filename == "/";
}

void
File_Explorer::fill_dir_stack(MediaData& mediadata)
{
    directory_stack.clear();

    if (is_root_dir(mediadata)) {
        directory_stack.push_back(mediadata);
        return;
    }

    std::string temp_path = mediadata.path != "/" ? mediadata.path + "/" + mediadata.filename : mediadata.path + mediadata.filename;

    directory_stack.push_back(MediaData({ "/", "/", "", FILETYPE_DIR, 0, LOCAL_FILE, true }));

    size_t pos = 0;
    std::string segment;
    while ((pos = temp_path.find('/')) != std::string::npos) {
        segment = temp_path.substr(0, pos);
        if (!segment.empty()) {
            std::string f = segment;
            std::string p =
              directory_stack.back().path != "/" && directory_stack.back().filename != "/" ? directory_stack.back().path + "/" + directory_stack.back().filename
              : directory_stack.back().path == "/" && directory_stack.back().filename != "/" ? directory_stack.back().path + directory_stack.back().filename
                                                                                             : directory_stack.back().path;
            directory_stack.push_back(MediaData({ f, p, "", FILETYPE_DIR, 0, LOCAL_FILE, true }));
        }
        temp_path.erase(0, pos + 1);
    }

    /* Add the last segment */
    if (!temp_path.empty()) {
        std::string f = temp_path;
        std::string p =
          directory_stack.back().path != "/" && directory_stack.back().filename != "/"   ? directory_stack.back().path + "/" + directory_stack.back().filename
          : directory_stack.back().path == "/" && directory_stack.back().filename != "/" ? directory_stack.back().path + directory_stack.back().filename
                                                                                         : directory_stack.back().path;
        directory_stack.push_back(MediaData({ f, p, "", FILETYPE_DIR, 0, LOCAL_FILE, true }));
    }
}

file_explorer_error_t
File_Explorer::generate_index(MediaData& mediadata)
{

    FsFile file_handle;
    FsFile dir_handle;
    sqlite3* sqlite_db;
    uint32_t checksum = 0;

    log_i("Checking checksum");
    while (sdfs->isReady() && file_handle.openNext(&dir_handle, O_RDONLY)) {
        serviceLoop();
        char filename_buffer[256] = ("");
        file_handle.getName(filename_buffer, 256);
        if (strcmp(filename_buffer, DB_FILE) == 0) {
            continue;
        }
        std::string extension = filename_buffer;
        extension = extension.substr(extension.find_last_of(".") + 1);
        if (extension != "mp3" && extension != "wav" && extension != "flac" && extension != "ogg" && extension != "m3u" && !file_handle.isDirectory()) {
            continue;
        }
        for (uint32_t i = 0; i < strlen(filename_buffer); i++) {
            checksum += filename_buffer[i];
        }
        checksum = hash(&checksum);
    }
    file_handle.close();

    log_i("Retrieving checksum from database");
    
    if (sdfs->isReady()) {
        int rc = sqlite3_open(get_db_path(mediadata).c_str(), &sqlite_db) != SQLITE_OK;
        if (rc != SQLITE_OK) {
            log_e("Failed to open database file: %s", get_db_path(mediadata).c_str());
            log_e("Error: %s", sqlite3_errmsg(sqlite_db));
            sqlite3_free(sqlite_db);
            sqlite3_close(sqlite_db);
            return ERROR_FAILURE;
        }
    }

    /* Get the checksum from the first row in the meta table */
    char* query_result = NULL;
    if (sdfs->isReady() && sqlite3_exec(sqlite_db, "SELECT checksum FROM meta WHERE id = 1", NULL, &query_result, NULL) != SQLITE_OK) {
        log_e("Failed to retrieve checksum from database");
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
    }

    if (query_result != NULL && (uint32_t)atoi(query_result) == checksum) {
        log_i("Checksums match, no need to regenerate database");
        sqlite3_free(query_result);
        sqlite3_close(sqlite_db);
        return ERROR_NONE;
    }
    sqlite3_free(query_result);

    /** Checksums do not match, regenerate the database */
    log_i("Checksums do not match, regenerating database");
    if (sdfs->isReady()) {
        if (sdfs->exists(get_db_path(mediadata).c_str()) && !sdfs->remove(get_db_path(mediadata).c_str())) {
            log_e("Failed to remove database file");
            sqlite3_close(sqlite_db);
            return ERROR_FAILURE;
        }
    } else {
        log_e("SD failure");
        sqlite3_free(sqlite_db);
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }
    /* Open the database file for writing */
    if (sdfs->isReady() && sqlite3_open(get_db_path(mediadata).c_str(), &sqlite_db) != SQLITE_OK) {
        log_e("Failed to open database file");
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
        sqlite3_free(sqlite_db);
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }
 
    /* Generate the database schema */
    if (sdfs->isReady() && create_db(sqlite_db) != ERROR_NONE) {
        log_e("Failed to create database schema");
        sqlite3_free(sqlite_db);
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }
        
    log_i("Writing to database");
    dir_handle.rewind();
    uint32_t id = 0;

    while (sdfs->isReady() && file_handle.openNext(&dir_handle, O_RDONLY)) {
        serviceLoop();
        char filename_buffer[256] = ("");
        file_handle.getName(filename_buffer, 256);
        if (strcmp(filename_buffer, DB_FILE) == 0) {
            continue;
        }
        std::string extension = filename_buffer;
        extension = extension.substr(extension.find_last_of(".") + 1);
        if (extension != "mp3" && extension != "wav" && extension != "flac" && extension != "ogg" && extension != "m3u" && !file_handle.isDirectory()) {
            continue;
        }

        uint8_t type;
        if (file_handle.isDirectory()) {
            type = FILETYPE_DIR;
        } else {
            if (extension == "mp3") {
                type = FILETYPE_MP3;
            } else if (extension == "wav") {
                type = FILETYPE_WAV;
            } else if (extension == "flac") {
                type = FILETYPE_FLAC;
            } else if (extension == "ogg") {
                type = FILETYPE_OGG;
            } else if (extension == "m3u") {
                type = FILETYPE_M3U;
            } else {
                type = FILETYPE_UNKNOWN;
            }
        }

        for (uint32_t i = 0; i < strlen(filename_buffer); i++) {
            checksum += filename_buffer[i];
        }

        log_i("Computing checksum");
        checksum = hash(&checksum);

        /* Write record to database */
        char sql[DB_BUF_SIZE] = ("");
        snprintf(sql, DB_BUF_SIZE, "INSERT INTO files (id, filename, path, type) VALUES (%d, '%s', '%s', %d)", id, filename_buffer, mediadata.getPath(), type);
        if (sdfs->isReady() && sqlite3_exec(sqlite_db, sql, NULL, NULL, NULL) != SQLITE_OK) {
            log_e("Failed to execute SQL statement");
            log_e("Error: %s", sqlite3_errmsg(sqlite_db));
            sqlite3_close(sqlite_db);
            return ERROR_FAILURE;
        }

        id++;
    }
    sqlite3_close(sqlite_db);
    file_handle.close();
    log_i("Wrote %d files to database", id);
    return ERROR_NONE;
}

std::string
File_Explorer::get_db_path(MediaData& mediadata)
{
    if (mediadata.path == "/" && mediadata.filename == "/") {
        return std::string(FS_MOUNT_POINT) + "/" + DB_FILE;
    } else {
        return std::string(FS_MOUNT_POINT) + mediadata.getPath() + "/" + DB_FILE;
    }
}

file_explorer_error_t
File_Explorer::create_db(sqlite3* db)
{
    char* error_message = NULL;
    if (sqlite3_exec(db, "CREATE TABLE meta (checksum INTEGER, sort_order INTEGER)", NULL, NULL, &error_message) != SQLITE_OK) {
        log_e("Failed to create table 'meta'");
        log_e("Error: %s", error_message);
        sqlite3_free(error_message);
        return ERROR_FAILURE;
    }
    
    if (sqlite3_exec(db, "CREATE TABLE files (id INTEGER PRIMARY KEY, filename TEXT, path TEXT, type INTEGER)", NULL, NULL, &error_message) != SQLITE_OK) {
        log_e("Failed to create table 'files'");
        log_e("Error: %s", error_message);
        sqlite3_free(error_message);
        return ERROR_FAILURE;
    }

    if (sqlite3_exec(db, "CREATE UNIQUE INDEX file_index ON files (filename)", NULL, NULL, &error_message) != SQLITE_OK) {
        log_e("Failed to execute SQL statement");
        log_e("Error: %s", error_message);
        sqlite3_free(error_message);
        return ERROR_FAILURE;
    }

    sqlite3_free(error_message);
    return ERROR_NONE;
}