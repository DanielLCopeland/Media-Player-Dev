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
 * @file file_explorer.h
 * @author Daniel Copeland
 * @brief
 * @date 2025-01-02
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef file_explorer_h
#define file_explorer_h

#define SQLITE_TEMP_STORE 3

#include <SdFat.h>
#include <utilities.h>
#include <card_manager.h>
#include <rom/md5_hash.h>
#include <sqlite3.h>
#include <string>
#include <system.h>
#include <callbacks.h>
#include <functional>
#include <vector>

#define DB_FILE            ".index.db"
#define ROOT_DIR           "/"
#define FS_MOUNT_POINT     "/sdfat"
#define SUBDIRECTORY_LIMIT 20
#define DB_BUF_SIZE        1024
#define MD5_DIGEST_LENGTH  16
#define MD5_DIGEST_STRING_LEN (MD5_DIGEST_LENGTH + 1)
#define FILENAME_BUFFER_LEN       256

class Card_Manager;
class File_Explorer;
class MediaData;

class File_Explorer
{
  public:
    enum error_t
    {
        ERROR_NONE,
        ERROR_FAILURE,
        ERROR_ROOT_DIR,
        ERROR_NOT_FOUND,
        ERROR_ALREADY_EXISTS,
        ERROR_INVALID,
        ERROR_UNKNOWN
    };

    enum db_column_t
    {
        DB_COL_ID,
        DB_COL_FILENAME,
        DB_COL_PATH,
        DB_COL_TYPE,
        DB_COL_CHECKSUM,
        DB_COL_COUNT
    };

    enum sort_order_t : uint8_t
    {
        SORT_ASCENDING,
        SORT_DESCENDING
    };

    enum sort_type_t : uint8_t
    {
        SORT_NAME,
        SORT_TYPE
    };

    File_Explorer();
    ~File_Explorer();

    void init(MediaData& dir, std::function<void(uint32_t, uint32_t)> status_callback = nullptr);
    void init(std::function<void(uint32_t, uint32_t)> status_callback = nullptr);
    error_t generate_index(MediaData& mediadata, std::function<void(uint32_t, uint32_t)> status_callback = nullptr);
    void close()
    {
        ready = false;
        directory_stack.clear();
    }

    /**
     * @brief Passes back a vector of MediaData objects containing the names of the files in the directory
     *
     * @param data Metadata class containing the path to the directory or file
     * @param index Index of the first file to return
     * @param count Number of files to return
     * @param sort_order Sort order of the files, either SORT_ASCENDING or SORT_DESCENDING
     */
    error_t get_list(std::vector<MediaData>* data, uint32_t index, uint32_t count, uint8_t sort_order, uint8_t sort_type);
    error_t get_list(std::vector<std::string>* data, uint32_t index, uint32_t count, uint8_t sort_order, uint8_t sort_type);

    /**
     * @brief Opens a directory
     *
     * @param mediadata Metadata class containing the path to the directory
     * @return FE::file_explorer_error_e
     */
    error_t open_dir(MediaData& mediadata, std::function<void(uint32_t, uint32_t)> status_callback = nullptr);

    /**
     * @brief Changes the current working directory to the parent directory
     *
     * @return FE::file_explorer_error_e
     */
    error_t exit_dir();

    /**
     * @brief Returns the current working directory
     *
     * @param mediadata External metadata class to pass back the current working directory
     * @return FE::file_explorer_error_e
     */
    error_t get_current_dir(MediaData& mediadata);

    /**
     * @brief Indicates whether the file explorer has successfully initialized and is ready to use
     *
     * @return bool
     */
    bool is_ready() { return ready; }

    /**
     * @brief Returns the number of subdirectories deep the current working directory is
     * relative to the root directory
     *
     * @return uint32_t
     */
    uint32_t depth()
    {
        uint32_t _depth = directory_stack.size();
        if (_depth > 0) {
            _depth--;
        }
        return _depth;
    }

    /**
     * @brief Returns the number of files in the current working directory
     *
     * @return uint32_t
     */
    uint32_t size() { return _num_files; }

  private:

    uint32_t _num_files = 0;
    void fill_dir_stack(MediaData& mediadata);
    std::vector<MediaData> directory_stack;
    bool ready;
    bool is_root_dir(MediaData& mediadata);
    error_t create_db(sqlite3* db);
    std::string get_db_path(MediaData& mediadata);
};

#endif /* file_explorer_h */