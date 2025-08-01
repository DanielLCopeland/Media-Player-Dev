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

File_Explorer::~File_Explorer() {}

void
File_Explorer::init(MediaData& dir, std::function<void(uint32_t, uint32_t)> status_callback)
{
    log_i("Initializing file explorer");
    directory_stack.clear();
    if (open_dir(dir, status_callback) != ERROR_NONE) {
        log_e("Failed to initialize file explorer");
    } else {
        ready = true;
    }
}

void
File_Explorer::init(std::function<void(uint32_t, uint32_t)> status_callback)
{
    MediaData dir = { "/", "/", "", FILETYPE_DIR, 0, LOCAL_FILE, true };
    init(dir, status_callback);
}

File_Explorer::error_t
File_Explorer::open_dir(MediaData& mediadata, std::function<void(uint32_t, uint32_t)> status_callback)
{
    log_i("Opening directory %s", mediadata.getPath());

    if (mediadata.type != FILETYPE_DIR) {
        log_e("Invalid directory");
        return ERROR_INVALID;
    }

    if (generate_index(mediadata, status_callback) != ERROR_NONE) {
        log_e("Failed to generate index");
        return ERROR_FAILURE;
    }

    fill_dir_stack(mediadata);
    return ERROR_NONE;
}

File_Explorer::error_t
File_Explorer::exit_dir()
{
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

File_Explorer::error_t
File_Explorer::generate_index(MediaData& mediadata, std::function<void(uint32_t, uint32_t)> status_callback)
{
    DIR* _dir_handle = opendir(mediadata.getPath());
    if (!_dir_handle || mediadata.type != FILETYPE_DIR || mediadata.loaded == false) {
        log_e("Failed to open directory");
        return ERROR_FAILURE;
    }

    std::string path = mediadata.getPath();
    std::string _db_path = std::string(mediadata.getPath());
    if (_db_path != "/") {
        _db_path += "/";
    }
    _db_path += DB_FILE;

    sqlite3* sqlite_db;
    MD5Context md5;
    unsigned char _checksum[MD5_DIGEST_LENGTH];
    char checksum_str[MD5_DIGEST_STRING_LEN] = ("");
    char sql[DB_BUF_SIZE] = ("");

    /* Calculate the checksum of the directory */
    _num_files = 0;
    MD5Init(&md5);
    struct dirent* entry;
    while ((entry = readdir(_dir_handle)) != nullptr) {
        std::string filename = entry->d_name;
        std::string extension = filename.substr(filename.find_last_of(".") + 1);

        if (MediaData::get_file_extensions().find(extension) == MediaData::get_file_extensions().end() && entry->d_type != DT_DIR || filename == DB_FILE) {
            continue;
        }

        MD5Update(&md5, reinterpret_cast<const unsigned char*>(filename.c_str()), filename.size());
        _num_files++;

        if (status_callback != nullptr) {
            status_callback(_num_files, 0);
        }
    }
    MD5Final(_checksum, &md5);
    closedir(_dir_handle);

    if (sqlite3_open(_db_path.c_str(), &sqlite_db) != SQLITE_OK) {
        log_e("Failed to open database file: %s", _db_path.c_str());
        log_e("DB Path: %s", _db_path.c_str());
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }

    /* Retrieve the checksum from the database and compare it to the checksum of the directory */
    char db_checksum[MD5_DIGEST_STRING_LEN] = ("");
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(&checksum_str[i], 2, "%02x", _checksum[i]);
    }
    sql[0] = '\0';
    snprintf(sql, DB_BUF_SIZE, "SELECT checksum FROM meta WHERE id = 1");
    if (sqlite3_exec(sqlite_db, sql, db_callback_get_checksum, &db_checksum, NULL) != SQLITE_OK) {
        log_e("Failed to get checksum. Attempting to regenerate database");
        log_e("SQL: %s", sql);
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
        sqlite3_close(sqlite_db);
    }
    checksum_str[MD5_DIGEST_STRING_LEN - 1] = '\0';
    db_checksum[MD5_DIGEST_STRING_LEN - 1] = '\0';
    if (strcmp(checksum_str, db_checksum) == 0) {
        log_i("Checksums match, no need to regenerate database");
        log_i("Checksum: %s", checksum_str);
        log_i("DB Checksum: %s", db_checksum);
        sqlite3_close(sqlite_db);
        return ERROR_NONE;
    } else {
        log_i("Checksums do not match, regenerating database");
        log_i("Checksum: %s", checksum_str);
        log_i("DB Checksum: %s", db_checksum);
        sqlite3_close(sqlite_db);
    }

    struct stat buffer;
    if (stat(_db_path.c_str(), &buffer) == 0 && unlink(_db_path.c_str()) != 0) {
        log_e("Failed to remove database file");
        log_e("DB Path: %s", _db_path.c_str());
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }

    /* Open the database file for writing */
    if (sqlite3_open(_db_path.c_str(), &sqlite_db) != SQLITE_OK) {
        log_e("Failed to open database file");
        log_e("DB Path: %s", _db_path.c_str());
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
        sqlite3_free(sqlite_db);
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }
    log_i("Generating database schema");
    /* Generate the database schema */
    if (create_db(sqlite_db) != ERROR_NONE) {
        log_e("Failed to create database schema");
        sqlite3_free(sqlite_db);
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }

    log_i("Writing to database");
    _dir_handle = opendir(path.c_str());
    if (!_dir_handle) {
        log_e("Failed to open directory");
        closedir(_dir_handle);
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }
    MD5Init(&md5);
    uint32_t id = 0;

    /* Retrieve every filename in the directory and write its attributes to the database, and calculate the checksum */
    while ((entry = readdir(_dir_handle)) != nullptr) {

        std::string filename = entry->d_name;
        std::string extension = filename.substr(filename.find_last_of(".") + 1);

        if (MediaData::get_file_extensions().find(extension) == MediaData::get_file_extensions().end() && entry->d_type != DT_DIR || filename == DB_FILE) {
            continue;
        }

        uint8_t type;
        if (entry->d_type == DT_DIR) {
            type = FILETYPE_DIR;
        } else {
            auto it = MediaData::get_file_extensions().find(extension);
            type = (it != MediaData::get_file_extensions().end()) ? it->second : FILETYPE_UNKNOWN;
        }

        MD5Update(&md5, reinterpret_cast<const unsigned char*>(filename.c_str()), filename.size());

        /* Write record to database */
        sql[0] = '\0';
        snprintf(sql,
                 DB_BUF_SIZE,
                 "INSERT INTO files (id, filename, path, type) VALUES (%d, '%s', '%s', %d)",
                 id,
                 escape_single_quotes(filename).c_str(),
                 escape_single_quotes(mediadata.getPath()).c_str(),
                 type);
        if (sqlite3_exec(sqlite_db, sql, NULL, NULL, NULL) != SQLITE_OK) {
            log_e("Failed to execute SQL statement");
            log_e("SQL: %s", sql);
            log_e("Error: %s", sqlite3_errmsg(sqlite_db));
            MD5Final(_checksum, &md5);
            closedir(_dir_handle);
            sqlite3_close(sqlite_db);
            return ERROR_FAILURE;
        }

        if (status_callback != nullptr) {
            status_callback(id, _num_files);
        }

        id++;
    }
    closedir(_dir_handle);
    /* Write the checksum to the meta table */
    MD5Final(_checksum, &md5);
    checksum_str[0] = '\0';
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(&checksum_str[i], 2, "%02x", _checksum[i]);
    }
    checksum_str[MD5_DIGEST_STRING_LEN - 1] = '\0';
    sql[0] = '\0';
    snprintf(sql, DB_BUF_SIZE, "INSERT INTO meta (id, checksum, sort_order) VALUES (1, '%s', 0)", checksum_str);
    if (sqlite3_exec(sqlite_db, sql, NULL, NULL, NULL) != SQLITE_OK) {
        log_e("Failed to execute SQL statement");
        log_e("SQL: %s", sql);
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }
    sqlite3_close(sqlite_db);
    log_i("Computed checksum: %s", checksum_str);
    log_i("Wrote %d files to database", id);
    return ERROR_NONE;
}

File_Explorer::error_t
File_Explorer::create_db(sqlite3* db)
{
    char* error_message = NULL;

    char sql[DB_BUF_SIZE] = ("");
    strcpy(sql, "CREATE TABLE IF NOT EXISTS meta (id INTEGER PRIMARY KEY, checksum TEXT, sort_order INTEGER)");
    if (sqlite3_exec(db, sql, NULL, NULL, &error_message) != SQLITE_OK) {
        log_e("Failed to create table 'meta'");
        log_e("SQL: %s", sql);
        log_e("Error: %s", error_message);
        sqlite3_free(error_message);
        return ERROR_FAILURE;
    }

    sql[0] = '\0';
    strcpy(sql, "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, filename TEXT, path TEXT, type INTEGER)");
    if (sqlite3_exec(db, sql, NULL, NULL, &error_message) != SQLITE_OK) {
        log_e("Failed to create table 'files'");
        log_e("SQL: %s", sql);
        log_e("Error: %s", error_message);
        sqlite3_free(error_message);
        return ERROR_FAILURE;
    }

    sql[0] = '\0';
    strcpy(sql, "CREATE UNIQUE INDEX file_index ON files (filename)");
    if (sqlite3_exec(db, sql, NULL, NULL, &error_message) != SQLITE_OK) {
        log_e("Failed to execute SQL statement");
        log_e("SQL: %s", sql);
        log_e("Error: %s", error_message);
        sqlite3_free(error_message);
        return ERROR_FAILURE;
    }

    sql[0] = '\0';
    strcpy(sql, "PRAGMA journal_mode = WAL");
    if (sqlite3_exec(db, sql, NULL, NULL, &error_message) != SQLITE_OK) {
        log_e("Failed to execute SQL statement");
        log_e("SQL: %s", sql);
        log_e("Error: %s", error_message);
        sqlite3_free(error_message);
        return ERROR_FAILURE;
    }

    sqlite3_free(error_message);
    return ERROR_NONE;
}

File_Explorer::error_t
File_Explorer::get_list(std::vector<MediaData>* data, uint32_t index, uint32_t count)
{
    data->clear();

    if (!ready) {
        log_e("File explorer not ready");
        return ERROR_FAILURE;
    }

    if (index == 0 && count == 0) {
        log_e("Invalid index and count");
        return ERROR_INVALID;
    }

    std::string _s_order = (_sort_order == SORT_ASCENDING) ? "ASC" : "DESC";
    std::string _s_type = (_sort_type == SORT_NAME) ? "filename" : "type";

    std::string sql = "SELECT * FROM files ORDER BY " + _s_type + " " + _s_order + " LIMIT " + std::to_string(count) + " OFFSET " + std::to_string(index);
    MediaData cwd;
    if (get_current_dir(cwd) != ERROR_NONE) {
        log_e("Failed to get current directory");
        return ERROR_FAILURE;
    }

    sqlite3* sqlite_db;
    std::string _db_path = std::string(cwd.getPath()) + "/" + DB_FILE;

    if (sqlite3_open(_db_path.c_str(), &sqlite_db) != SQLITE_OK) {
        log_e("Failed to open database file");
        log_e("DB Path: %s", _db_path.c_str());
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }

    if (sqlite3_exec(sqlite_db, sql.c_str(), db_callback_get_files, data, NULL) != SQLITE_OK) {
        log_e("Failed to execute SQL statement");
        log_e("SQL: %s", sql.c_str());
        log_e("Error: %s", sqlite3_errmsg(sqlite_db));
        sqlite3_close(sqlite_db);
        return ERROR_FAILURE;
    }

    sqlite3_close(sqlite_db);
    return ERROR_NONE;
}

File_Explorer::error_t
File_Explorer::get_current_dir(MediaData& mediadata)
{
    if (directory_stack.empty()) {
        return ERROR_FAILURE;
    }

    mediadata = directory_stack.back();
    return ERROR_NONE;
}

MediaData
File_Explorer::get_file(uint32_t index)
{
    std::vector<MediaData> data;
    if (get_list(&data, index, 1) == ERROR_NONE) {
        return data[0];
    }
    return MediaData();
}