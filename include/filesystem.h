/**
 * @file filesystem.h
 *
 * @brief Manages all interactions with the SD card
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

#ifndef filesystem_h
#define filesystem_h

/* SD Card Pins */
#define SD_CS_PIN 38

/* SD Card Config */
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(20), &SPI)

#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <card_manager.h>
#include <hash.h>
#include <string>
#include <system.h>
#include <transport.h>
#include <ui/common.h>
#include <vector>

#ifndef INDEX_FILE
#define INDEX_FILE ".fileindex.tmp"
#endif

/* How many subdirectories deep we can go and add to cwd vector.  Trying to keep an eye on memory usage. */
#define SUBDIRECTORY_LIMIT 20

enum sort_type
{
    SORT_FILENAME,
    SORT_DIR,
    SORT_NONE
};

enum sort_order
{
    SORT_ORDER_ASCENDING,
    SORT_ORDER_DESCENDING,
    SORT_ORDER_NONE
};

class SystemConfig;
class MediaData;
class FileVector;
class CardManager;

extern CardManager* sdfs;

/****************************************************
 *
 * FileVector: A class for reading and writing a vector
 * of MediaData structs to a file.  Replicates the
 * functionality of the std::vector class.
 *
 ****************************************************/

class FileVector
{
  public:
    FileVector(FsFile* fileHandle);
    FileVector() { _fileHandle = nullptr; }
    uint32_t getChecksum();
    bool setChecksum(uint32_t checksum);
    bool validateChecksum();
    uint16_t elementLength(MediaData& mediadata);
    bool insert(MediaData mediadata, uint32_t index, bool useFreeSpace = false);
    bool erase(uint32_t index);
    void clear();
    MediaData at(uint32_t index);
    uint32_t size();
    bool pop_back();
    bool push_back(MediaData mediadata, bool useFreeSpace = false);
    std::vector<MediaData> getVector(size_t start, size_t numElements);
    bool setIndexFile(FsFile* fileHandle)
    {
        if (fileHandle->isOpen()) {
            _fileHandle = fileHandle;
            getNumElements();
            return true;
        } else {
            return false;
        }
    }

    /* This class is used to allow us to use the [] operator to access elements of
    the FileVector and assign new elements to it as if it were an array. */
    class Proxy
    {
      public:
        Proxy(FileVector& vec, size_t index)
          : vec(vec)
          , index(index)
        {
        }
        operator MediaData() const;
        Proxy& operator=(const MediaData& mediadata);

      private:
        FileVector& vec;
        size_t index;
    };

    Proxy operator[](size_t index) { return Proxy(*this, index); }

    bool swap(uint32_t indexA, uint32_t indexB);

  private:
    MediaData readMediaData(uint32_t index);
    MediaData readMediaData();
    uint32_t readFirstElementPosition();
    bool writeFirstElementPosition(uint32_t position);
    bool writeMediaData(MediaData mediadata);
    bool writeMediaData(uint32_t index, MediaData mediadata);
    void seekToIndex(uint32_t index);
    uint32_t getNumElements();

    FsFile* _fileHandle = nullptr;
    uint32_t numElements;

    /* This struct is used to keep track of the start and end of each element in
    the file.  It is used to quickly access elements without having to count
    up from the beginning of the file. It is filled once when the FileVector is
    created and then used to access elements. */
    struct location
    {
        uint32_t start;
        uint32_t end;
    };
    std::vector<location> _locations;
};

/****************************************************
 *
 * Convenience classes for the Filesystem class
 *
 ****************************************************/

/* Path: Converts a MediaData struct representing a directory to a full vector of MediaData structs representing the hierarchy all the way down to the root
directory. */
class Path
{
  public:
    Path(MediaData* dir);
    void get(std::vector<MediaData>* path);
    void set(MediaData* dir);

  private:
    MediaData* dir;
    bool isRootDir();
};

class Filesystem
{
  public:
    Filesystem();

    ~Filesystem();

    /* Closes all open files and directories and unmounts the SD card. */
    void close();

    /* Checks for the existense of "dir", checks to see if it's a directory, and
    if so, opens it and returns true, otherwise returns false. */
    bool openDir(MediaData dir);

    /* Returns the number of files in the current working directory. Will only
    return a number greater than zero after generateIndex() or openDir() has
    been called. */
    uint32_t numFiles();

    /* Returns the full path of the current working directory by returning the
    back element of the cwd vector. */
    MediaData getPath();

    /* Exits the current working directory and opens the parent directory.  If we
    are already in the root directory, do nothing and return false.  If we are
    in a subdirectory, remove the last element from the cwd vector and open the
    parent directory and return true. */
    bool exitDir();

    /* Changes the current working directory to the directory given in "dir".  If
    the directory doesn't exist, it will return false, otherwise it will return
    true. */
    bool setRoot(MediaData dir);

    /* Returns a set of files in the current working directory starting at "start"
    and ending at "start + numFiles".  If the end is greater than the number of
    files in the cwd, it will return the files from start to the end of the
    cwd.  If the start is greater than the number of files in the cwd, it will
    return an empty vector. */
    std::vector<MediaData> getFiles(uint32_t start, uint32_t numFiles);

    /* Reads the current working directory and generates an index file in the
    current working directory of all the files and directories in it. It
    generates a checksum of the root directory and compares it to checksum
    recorded in the index file. If they don't match, it will generate a new
    index file. If force is true, it will generate a new index file regardless of
    the checksum. */
    void generateIndex(uint8_t sortType, uint8_t sortOrder, bool force = false);

    /* Returns the checksum of all the files in the current working directory */
    uint32_t getChecksum();

  private:
    FsFile root;
    FsFile indexFile;
    std::vector<MediaData> cwd;
    std::string playlistFilename;
    FileVector fileVector;
};

#endif