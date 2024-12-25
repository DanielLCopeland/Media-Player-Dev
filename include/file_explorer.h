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
 * @date 2024-02-02
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef file_explorer_h
#define file_explorer_h

#define SQLITE_TEMP_STORE 3

#include <SdFat.h>
#include <callbacks.h>
#include <card_manager.h>
#include <hash.h>
#include <sqlite3.h>
#include <string>
#include <system.h>
#include <ulog_sqlite.h>
#include <vector>

#define DB_FILE            ".index.db"
#define ROOT_DIR           "/"
#define FS_MOUNT_POINT     "/sdfat"
#define SUBDIRECTORY_LIMIT 20
#define DB_BUF_SIZE        1024

extern CardManager* sdfs;

enum file_explorer_error_t
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

class File_Explorer
{
  public:
    File_Explorer();
    ~File_Explorer();

    void init(MediaData& dir);
    void init();
    void close()
    {
        ready = false;
        directory_stack.clear();
    }

    /**
     * @brief Returns a vector of strings containing the names of the files in the directory
     *
     * @param mediadata Metadata class containing the path to the directory or file
     * @param index The index of the first file to return
     * @param count The number of files to return
     * @param files Vector of strings to store the filenames
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t get(MediaData& mediadata, uint32_t index, uint32_t count, std::vector<std::string>& files);

    /**
     * @brief Opens a directory
     *
     * @param mediadata Metadata class containing the path to the directory
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t open_dir(MediaData& mediadata);

    /**
     * @brief Removes a file or directory
     *
     * @param mediadata Metadata class containing the path to the directory or file
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t remove(MediaData& mediadata);

    /**
     * @brief Creates a directory
     *
     * @param mediadata Metadata class containing the path to the directory
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t create_dir(MediaData& mediadata);

    /**
     * @brief Creates an empty file using the path and filename in the metadata class
     *
     * @param mediadata Metadata class containing the path to the file
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t create_file(MediaData& mediadata);

    /**
     * @brief Renames a file or directory
     *
     * @param mediadata Metadata class containing the path to the file or directory
     * @param new_name The new name of the file or directory
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t rename(MediaData& mediadata, std::string new_name);

    /**
     * @brief Copies a file or directory
     *
     * @param source Metadata class containing the path to the source file or directory
     * @param destination Metadata class containing the path to the destination file or directory
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t copy(MediaData& source, MediaData& destination);

    /**
     * @brief Moves a file or directory
     *
     * @param source Metadata class containing the path to the source file or directory
     * @param destination Metadata class containing the path to the destination file or directory
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t move(MediaData& source, MediaData& destination);

    /**
     * @brief Changes the current working directory to the parent directory
     *
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t exit_dir();

    /**
     * @brief Changes the current working directory to the specified directory
     *
     * @param mediadata External metadata class to pass back the current working directory
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t get_current_dir(MediaData& mediadata);

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
    uint32_t get_depth() { return directory_stack.size(); }

    /**
     * @brief Creates a checksum of all the filenames in the current directory
     *
     * @return uint32_t
     */
    file_explorer_error_t get_checksum();

  private:
    file_explorer_error_t generate_index(MediaData& mediadata);
    void fill_dir_stack(MediaData& mediadata);
    std::vector<MediaData> directory_stack;
    bool ready;
    bool is_root_dir(MediaData& mediadata);
    file_explorer_error_t create_db(sqlite3* db);

    MediaData db_get_mediadata(uint32_t id);
    void db_put_mediadata(MediaData& mediadata);
    std::string get_db_path(MediaData& mediadata);
    /**
     * @note The following describes the layout of the database file
     *
     * Columns:
     *
     * 1. id - The unique identifier for the file or directory
     * 2. filename - The name of the file or directory
     * 3. path - The path to the file or directory
     * 4. url - The URL of the file or directory
     * 5. type - The type of the file or directory
     * 6. port - The port number for the file or directory
     * 7. source - The source of the file or directory
     * 8. checksum - The checksum of the filename added to the previous checksums
     */

    /**
     * @brief writes a row to the database
     *
     * @param mediadata Metadata class containing the data to write
     * @return FE::file_explorer_error_e
     */
    file_explorer_error_t write_row(MediaData& mediadata);
};

#endif /* file_explorer_h */