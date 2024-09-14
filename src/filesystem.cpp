/**
 * @file filesystem.cpp
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

#include <filesystem.h>

Filesystem::Filesystem()
{
    /* Check if the SD card is present */
    if (sdfs->isReady()) {
        openDir({ "/", "/", "", FILETYPE_DIR, 0, LOCAL_FILE, false });
        cwd.reserve(SUBDIRECTORY_LIMIT);

        /* If the PLAYLIST_DIR doesn't exist, create it */
        if (!sdfs->exists(PLAYLIST_DIR)) {
            sdfs->mkdir(PLAYLIST_DIR);
        }

        log_i("SD card initialized");
    }
}

Filesystem::~Filesystem()
{
    root.close();
    indexFile.close();
}

/* Checks for the existence of "dir", checks to see if it's a directory, and if so, opens it with "root" and returns true, otherwise returns false. */
bool
Filesystem::openDir(MediaData dir)
{
    if (!sdfs->isReady()) {
        log_e("SD card error.  Can't open directory.");
        log_e("SD card error code: %d", sdfs->sdErrorCode());
        return false;
    }

    if (cwd.size() == SUBDIRECTORY_LIMIT) {
        log_i("Subdirectory limit reached.  Can't open directory.");
        return false;
    }

    /* Now we need to open the directory and check to see if it exists */
    if (root.open(dir.getPath(), O_RDONLY) && root.isDir()) {
        log_i("Opened directory %s", dir.getPath());

        /* Clear the cwd_media vector */
        cwd.clear();
        Path path(&dir);
        path.get(&cwd);

        generateIndex(SORT_NONE, SORT_ORDER_NONE);
        return true;
    }

    else
        log_e("Failed to open directory %s", dir.getPath());

    return false;
}

/* Reads the current working directory and generates an index file in the current working directory of all the files and directories in it.
It also generates a checksum of the root directory and compares it to the checksum recorded in the index file. If they match, it will skip generating the
index file. */
void
Filesystem::generateIndex(uint8_t sortType, uint8_t sortOrder, bool force)
{
    log_i("Generating index file...");
    SystemMessage message;
    FsFile file;
    MediaData mediadata;

    /* Open the index file */
    std::string indexPath = getPath();
    if (indexPath != "/")
        indexPath += "/";
    indexPath += INDEX_FILE;

    if (!sdfs->isReady()) {
        log_e("SD card error.  Can't generate index file.");
        log_e("SD card error code: %d", sdfs->sdErrorCode());
        return;
    }

    /* Check if the INDEX_FILE file exists */
    if (!sdfs->exists(indexPath.c_str())) {
        indexFile.open(indexPath.c_str(), O_RDWR | O_CREAT);
    } else {
        indexFile.open(indexPath.c_str(), O_RDWR);
    }
    log_i("Creating file vector memory object...");
    //FileVector* fileVector = new FileVector(&indexFile);
    fileVector.setIndexFile(&indexFile);
    log_i("Opened index file %s", indexPath.c_str());

    /* If it does exist, check to see if the directory has changed since the last time we generated the index file, and also check to see if the recorded
    checksum makes sense */
    uint32_t checksum = getChecksum();
    if (checksum == fileVector.getChecksum() && !force) {
        log_i("Checksums match, no need to regenerate index file");
        return;
    }

    log_i("Checksums don't match or generation forced, regenerating index file...");

    /* If we have to regenerate the index file, we need to clear the file first */
    size_t oldNumFiles = fileVector.size();
    fileVector.clear();

    /* Write the new checksum to the index file */
    fileVector.setChecksum(checksum);

    /* Rewind the directory */
    root.rewindDirectory();

    /* Now let's write the file data to the index file */
    while (sdfs->isReady() && file.openNext(&root, O_RDONLY)) {
        serviceLoop();

        /* Get the filename */
        char filenameBuffer[256] = ("");
        file.getName(filenameBuffer, 256);
        mediadata.filename = filenameBuffer;

        /* Get the path */
        mediadata.path = getPath();

        /* Get the type */
        if (file.isDirectory()) {
            mediadata.type = FILETYPE_DIR;
        }

        else {
            std::string extension = mediadata.filename.substr(mediadata.filename.find_last_of(".") + 1);
            if (extension == "mp3")
                mediadata.type = FILETYPE_MP3;
            else if (extension == "wav")
                mediadata.type = FILETYPE_WAV;
            else if (extension == "flac")
                mediadata.type = FILETYPE_FLAC;
            else if (extension == "ogg")
                mediadata.type = FILETYPE_OGG;
            else if (extension == "m3u")
                mediadata.type = FILETYPE_M3U;

            /* If none of the file types match what we need, then break this loop iteration and move on to the next file.
            We don't need to see the user's random non-media files in the file browser. */
            else
                continue;
        }

        mediadata.source = LOCAL_FILE;
        mediadata.loaded = true;

        /* Find a place in the file index to insert the new element */
        size_t i;
        switch (sortType) {
            case SORT_FILENAME:
                if (fileVector.size() == 0) {
                    i = 0;
                } else {
                    for (i = 0; i < fileVector.size(); i++) {
                        serviceLoop();
                        if ((sortOrder == SORT_ORDER_ASCENDING && mediadata.filename <= fileVector.at(i).filename) ||
                            (sortOrder == SORT_ORDER_DESCENDING && mediadata.filename >= fileVector.at(i).filename)) {
                            break;
                        }
                    }
                }
                break;

            case SORT_DIR:
                if (fileVector.size() == 0) {
                    i = 0;
                } else {
                    for (i = 0; i < fileVector.size(); i++) {
                        serviceLoop();
                        if ((sortOrder == SORT_ORDER_ASCENDING && mediadata.type == FILETYPE_DIR && fileVector.at(i).type != FILETYPE_DIR) ||
                            (sortOrder == SORT_ORDER_DESCENDING && mediadata.type != FILETYPE_DIR && fileVector.at(i).type == FILETYPE_DIR)) {
                            break;
                        }
                    }
                }
                break;

            default:
                i = fileVector.size();
                break;
        }

        fileVector.insert(mediadata, i, false);
        if (sortType == SORT_FILENAME || sortType == SORT_DIR) {
            message.show("Sorting...\n" + std::to_string((size_t) (((float) fileVector.size() / (float) oldNumFiles) * 100)) + "% complete", 0, false);
        } else {
            message.show("Generating index...\n" + std::to_string(fileVector.size()) + " files found", 0, false);
        }
    }

    log_i("Wrote %d files to index file", fileVector.size());
    file.close();
}

uint32_t
Filesystem::getChecksum()
{
    log_i("Getting checksum...");

    uint32_t checksum = 0;
    FsFile file;
    if (!sdfs->isReady()) {
        log_e("SD card error.  Can't generate index file.");
        log_e("SD card error code: %d", sdfs->sdErrorCode());
        return checksum;
    }
    root.rewindDirectory();

    /* Read all the files in the directory into the checksum */
    while (sdfs->isReady() && file.openNext(&root, O_RDONLY)) {
        serviceLoop();

        /* Get the filename */
        char filenameBuffer[256] = ("");
        file.getName(filenameBuffer, 256);

        /* If the name is INDEX_FILE, skip it */
        if (strcmp(filenameBuffer, INDEX_FILE) == 0)
            continue;

        std::string extension = filenameBuffer;
        extension = extension.substr(extension.find_last_of(".") + 1);

        /* If the file is not a media file, skip it */
        if (extension != "mp3" && extension != "wav" && extension != "flac" && extension != "ogg" && extension != "m3u" && !file.isDirectory())
            continue;

        /* Add each byte of the filename to the checksum */
        for (uint8_t i = 0; i < strlen(filenameBuffer); i++)
            checksum += filenameBuffer[i];
    }
    file.close();

    log_i("Calculating checksum...");
    Hash *hash;
    hash = new Hash(&checksum);
    uint32_t newChecksum = hash->get();
    
    delete hash;
    return newChecksum;
}

/* 
Returns the number of files in the current working directory. Will only return a number 
greater than zero after generateIndex() or openDir() has been called.
 */
uint32_t
Filesystem::numFiles()
{
    if (!sdfs->isReady() || !indexFile.isOpen()) {
        log_e("SD card error.  Can't get number of files.");
        return 0;
    }
    return fileVector.size();
}

MediaData
Filesystem::getPath()
{
    MediaData path = cwd.back();
    path.source = LOCAL_FILE;
    return path;
}

/*
Exits the current working directory and opens the parent directory.  If we are already in the 
root directory, do nothing and return false.  If we are in a subdirectory, remove the last element
from the cwd vector and open the parent directory and return true.
*/
bool
Filesystem::exitDir()
{
    /* Pop the last element off the cwd vector */
    if (cwd.size() > 1) {
        cwd.pop_back();
        std::string path = getPath();
        log_i("Exiting directory %s", path.c_str());
        root.open(path.c_str(), O_RDONLY);
        generateIndex(SORT_NONE, SORT_ORDER_NONE);

        return true;

    }

    else {
        log_i("Already in root directory.");
        return false;
    }
}

/* Returns a set of files in the current working directory starting at "start" and ending at "start + numFiles" */
std::vector<MediaData>
Filesystem::getFiles(uint32_t start, uint32_t numFiles)
{
    std::vector<MediaData> files;

    if (!sdfs->isReady() || !indexFile.isOpen()) {
        log_e("SD card error.  Can't get files.");
        return files;
    }

    for (uint32_t i = start; i < start + numFiles; i++) {
        files.push_back(fileVector[i]);
    }

    return files;
}

void
Filesystem::close()
{
    root.close();
    sdfs->end();
}

bool
Filesystem::setRoot(MediaData dir)
{
    if (openDir(dir)) {
        return true;
    } else
        return false;
}

/****************************************************
 *
 * FileVector: A class for reading and writing a vector of MediaData structs to a file.  Replicates the functionality of the std::vector class.
 *
 ****************************************************/

FileVector::FileVector(FsFile* fileHandle) : _fileHandle(fileHandle), numElements(0)
{
    getNumElements();
}

uint32_t
FileVector::getNumElements()
{

    numElements = 0;
    uint32_t currentPosition = 0;
    uint16_t mediaDataLength = 0;
    _locations.clear();

    _fileHandle->seekSet(0);
    _fileHandle->seekCur(sizeof(uint32_t)); /* Skip the checksum */

    /* Read the first element position */
    _fileHandle->read(&currentPosition, sizeof(uint32_t));

    while (currentPosition && currentPosition < _fileHandle->size()) { /*Make sure we don't read past the end of the file or string of elements */

        serviceLoop();

        location _location;

        numElements++;
        _fileHandle->seekSet(currentPosition);
        _location.start = _fileHandle->position();
        /* Read the length of the element */
        _fileHandle->read(&mediaDataLength, sizeof(uint16_t));
        /* Jump to the end of the element minus 32 bits - the length we just read - the terminator */
        _fileHandle->seekCur(mediaDataLength - sizeof(uint32_t) - sizeof(uint16_t) - sizeof(char));
        _location.end = currentPosition + mediaDataLength;
        /* Read the next element position */
        _fileHandle->read(&currentPosition, sizeof(uint32_t));

        _locations.push_back(_location);
    }
    log_i("Found %d elements in file", numElements);
    return numElements;
}

MediaData
FileVector::readMediaData(uint32_t index)
{
    MediaData mediadata;

    seekToIndex(index);

    return readMediaData();
}

bool
FileVector::writeMediaData(uint32_t index, MediaData mediadata)
{
    seekToIndex(index);

    return writeMediaData(mediadata);
}

MediaData
FileVector::readMediaData()
{
    size_t offset = 0;
    uint16_t elementSize;
    uint8_t fsize;
    uint16_t psize;
    uint16_t usize;

    MediaData mediadata = { "", "", "", FILETYPE_DIR, 0, LOCAL_FILE, false };

    _fileHandle->read(&elementSize, sizeof(uint16_t));
    uint8_t readBuffer[elementSize];
    _fileHandle->read(readBuffer, elementSize);

    memcpy(&fsize, readBuffer + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    mediadata.filename = std::string((char*) readBuffer + offset, fsize).c_str();
    offset += fsize;
    memcpy(&psize, readBuffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    mediadata.path = std::string((char*) readBuffer + offset, psize).c_str();
    offset += psize;
    memcpy(&usize, readBuffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    mediadata.url = std::string((char*) readBuffer + offset, usize).c_str();
    offset += usize;
    memcpy(&mediadata.type, readBuffer + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&mediadata.port, readBuffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(&mediadata.source, readBuffer + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&mediadata.loaded, readBuffer + offset, sizeof(bool));
    offset += sizeof(bool);
    memcpy(&mediadata.nextElement, readBuffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    return mediadata;
}

bool
FileVector::writeMediaData(MediaData mediadata)
{
    uint8_t fsize = mediadata.filename.size() + sizeof(char);
    uint16_t psize = mediadata.path.size() + sizeof(char);
    uint16_t usize = mediadata.url.size() + sizeof(char);
    uint16_t length = elementLength(mediadata);

    /* 
    Create a buffer to take in all the variables as a single byte array so we can write them all at once

    The format of the elements in the buffer are as follows (all lengths are in bytes):

    2 bytes - length of element
    1 byte - length of filename
    n bytes - filename
     2 bytes - length of path
     n bytes - path
     2 bytes - length of url
     n bytes - url
     1 byte - type
     2 bytes - port
     1 byte - source
     1 byte - loaded
     */

    uint8_t writeBuffer[length - sizeof(char)];
    size_t offset = 0;   /* This is the offset into the writeBuffer array, which is incremented after each memcpy by the width of the element being copied */

    memcpy(writeBuffer, &length, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(writeBuffer + offset, &fsize, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(writeBuffer + offset, mediadata.filename.c_str(), fsize);
    offset += fsize;
    memcpy(writeBuffer + offset, &psize, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(writeBuffer + offset, mediadata.path.c_str(), psize);
    offset += psize;
    memcpy(writeBuffer + offset, &usize, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(writeBuffer + offset, mediadata.url.c_str(), usize);
    offset += usize;
    memcpy(writeBuffer + offset, &mediadata.type, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(writeBuffer + offset, &mediadata.port, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(writeBuffer + offset, &mediadata.source, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(writeBuffer + offset, &mediadata.loaded, sizeof(bool));
    offset += sizeof(bool);
    memcpy(writeBuffer + offset, &mediadata.nextElement, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (_fileHandle->write(writeBuffer, length - sizeof(char))) {
        _fileHandle->write("\n", 1);   /* Terminator */
        _fileHandle->flush();
        return true;
    }
    return false;
}

bool
FileVector::push_back(MediaData mediadata, bool useFreeSpace)
{
    _fileHandle->seekSet(sizeof(uint32_t));   /* Skip the checksum */

    uint32_t firstMediaDataPosition;
    _fileHandle->read(&firstMediaDataPosition, sizeof(uint32_t));
    _fileHandle->seekSet(firstMediaDataPosition);

    uint16_t length = elementLength(mediadata);
    uint32_t lastMediaDataPosition;
    MediaData lastMediaData;
    location _location;

    mediadata.nextElement = 0;

    if (numElements == 0) {
        _location.start = _fileHandle->position();
        _location.end = _fileHandle->position() + length;
        _locations.push_back(_location);
        return writeMediaData(mediadata) && ++numElements;
    }

    for (size_t i = 0; i < numElements; i++) {
        serviceLoop();
        lastMediaDataPosition = _fileHandle->position();
        lastMediaData = readMediaData();
        _fileHandle->seekSet(lastMediaData.nextElement);
    }

    if (useFreeSpace) {
        _fileHandle->seekSet(sizeof(uint32_t) + sizeof(uint32_t));   /* Skip the checksum and the first element position */

        while (_fileHandle->available()) {
            serviceLoop();
            uint8_t c = _fileHandle->peek();
            uint16_t foundSpace = 0;

            while (c != 0x0a && c == 0 && foundSpace != length) {   /* 0x0a is the newline character */
                _fileHandle->seekCur(1);
                c = _fileHandle->peek();
                foundSpace++;
            }

            if (foundSpace == length) {
                _fileHandle->seekCur(-foundSpace);
                lastMediaData.nextElement = _fileHandle->position();
                if (writeMediaData(mediadata) && _fileHandle->seekSet(lastMediaDataPosition) && writeMediaData(lastMediaData)) {
                    numElements++;
                    return true;
                }
                return false;
            }

            _fileHandle->seekCur(1);
        }
    }
    _fileHandle->seekEnd();
    lastMediaData.nextElement = _fileHandle->size();
    _location.start = _fileHandle->size();
    writeMediaData(mediadata);
    _location.end = _fileHandle->size();
    _locations.push_back(_location);

    return _fileHandle->seekSet(lastMediaDataPosition) && writeMediaData(lastMediaData) && ++numElements;
}

bool
FileVector::pop_back()
{
    if (erase(numElements - 1))
        return true;
    return false;
}

uint32_t
FileVector::size()
{
    return numElements;
}

void
FileVector::clear()
{
    /* Truncate the file to 0 bytes and set numElements to 0 */
    _locations.clear();
    _fileHandle->truncate(0);
    numElements = 0;
    uint32_t checksum = 0;
    _fileHandle->write(&checksum, sizeof(uint32_t));
    /* Set a pointer to the first element */
    uint32_t firstElementPosition = _fileHandle->position() + sizeof(uint32_t);
    _fileHandle->write(&firstElementPosition, sizeof(uint32_t));
}

bool
FileVector::insert(MediaData mediadata, uint32_t index, bool useFreeSpace)
{
    /* Early exit conditions */
    if ((numElements == 0 && index > 0) || (index > numElements))
        return false;
    if (index == numElements)
        return push_back(mediadata, useFreeSpace);

    uint16_t length;
    uint32_t lastMediaDataPosition = 0;
    uint32_t nextMediaDataPosition = 0;
    MediaData lastMediaData;
    MediaData nextMediaData;
    uint32_t currentPosition = 0;
    location location;

    _fileHandle->seekSet(0);
    _fileHandle->seekCur(sizeof(uint32_t));   /* Skip the checksum */
    _fileHandle->read(&currentPosition, sizeof(uint32_t));
    _fileHandle->seekSet(currentPosition);
    lastMediaDataPosition = _fileHandle->position();

    if (numElements == 0) {
        mediadata.nextElement = 0;
        return writeMediaData(mediadata) && ++numElements;
    }

    for (size_t i = 0; i < index; i++) {
        serviceLoop();
        lastMediaDataPosition = _fileHandle->position();
        _fileHandle->read(&length, sizeof(uint16_t));
        _fileHandle->seekCur(length - sizeof(uint32_t) - sizeof(uint16_t) - sizeof(char));
        _fileHandle->read(&currentPosition, sizeof(uint32_t));
        _fileHandle->seekSet(currentPosition);
    }

    /* Save the next element and its position */
    nextMediaDataPosition = _fileHandle->position();
    nextMediaData = readMediaData();

    /* Seek back to the last element */
    _fileHandle->seekSet(lastMediaDataPosition);
    lastMediaData = readMediaData();

    if (useFreeSpace) {
        /* Try to find enough space to write the new element.  If we can't, then we need to write the new element to the end of the file */
        length = elementLength(mediadata);
        _fileHandle->seekSet(0);
        _fileHandle->seekCur(sizeof(uint32_t) + sizeof(uint32_t));   /* Skip the checksum and the first element position */

        while (_fileHandle->available()) {
            serviceLoop();

            uint8_t c = _fileHandle->peek();
            uint16_t foundSpace = 0;

            if (c == 0) {
                /* See if there is enough space to write the new element */
                while (c != 0x0a && c == 0) {   /* 0x0a is the newline character */

                    _fileHandle->seekCur(1);
                    c = _fileHandle->peek();
                    foundSpace++;

                    if (foundSpace == length) {
                        _fileHandle->seekCur(-foundSpace);

                        if (index > 0) {
                            mediadata.nextElement = nextMediaDataPosition;
                            lastMediaData.nextElement = _fileHandle->position();
                        }

                        else {
                            /* Save the current position */
                            uint32_t currentPosition = _fileHandle->position();

                            /* Read the first element pointer and write it to the mediadata.nextElement */
                            mediadata.nextElement = readFirstElementPosition();

                            /* Rewrite the first element pointer */
                            if (!writeFirstElementPosition(currentPosition))
                                return false;

                            /* Seek back to where we were */
                            _fileHandle->seekSet(currentPosition);
                        }

                        if (writeMediaData(mediadata)) {
                            if (index > 0) {
                                _fileHandle->seekSet(lastMediaDataPosition);
                                return (writeMediaData(lastMediaData)) && ++numElements;
                            }

                            else
                                return ++numElements;

                        } else
                            return false;
                    }
                }

            }

            else
                _fileHandle->seekCur(1);
        }
    }

    /* If we've come this far, then we need to write the new element to the end of the file */

    if (index > 0) {
        mediadata.nextElement = nextMediaDataPosition;
        lastMediaData.nextElement = _fileHandle->size(); 
        location.start = _fileHandle->size();
    } else {
        /* Save the current position */
        uint32_t currentPosition = _fileHandle->size();

        /* Read the first element pointer and write it to the mediadata.nextElement */
        mediadata.nextElement = readFirstElementPosition();

        /* Rewrite the first element pointer */
        if (!writeFirstElementPosition(currentPosition))
            return false;
        location.start = currentPosition;
    }

    _fileHandle->seekEnd();
    location.end = _fileHandle->size() + elementLength(mediadata);
    

    if (writeMediaData(mediadata)) {
        _locations.insert(_locations.begin() + index, location);
        if (index > 0) {
            _fileHandle->seekSet(lastMediaDataPosition);
            return (writeMediaData(lastMediaData)) && ++numElements;
        }

        else
            return ++numElements;

    } else
        return false;
}

bool
FileVector::erase(uint32_t index)
{
    if (index >= numElements)
        return false;
    if (numElements == 0)
        return false;

    MediaData mediadata_current;
    MediaData mediadata_before;
    uint32_t mediadata_before_position;
    uint32_t mediadata_current_position;

    if (index > 0) {
        seekToIndex(index - 1);
        mediadata_before_position = _fileHandle->position();
        mediadata_before = readMediaData();
        _fileHandle->seekSet(mediadata_before.nextElement);
        mediadata_current_position = _fileHandle->position();
    }

    else {
        seekToIndex(0);
        mediadata_current_position = _fileHandle->position();
    }

    mediadata_current = readMediaData();

    _fileHandle->seekSet(mediadata_current_position);
    /* Fill the gap with zeros, including the newline character */
    for (uint16_t i = 0; i < elementLength(mediadata_current); i++) {
        _fileHandle->write("\0", 1);
    }

    if (index > 0) {
        mediadata_before.nextElement = mediadata_current.nextElement;
        _fileHandle->seekSet(mediadata_before_position);
        --numElements;
        return writeMediaData(mediadata_before);
    } else {
        --numElements;
        return writeFirstElementPosition(mediadata_current.nextElement);
    }
}

MediaData
FileVector::at(uint32_t index)
{
    if (numElements == 0) {
        return MediaData();
    }

    if (index > numElements) {
        return MediaData();
    }

    return readMediaData(index);
}

bool
FileVector::swap(uint32_t indexA, uint32_t indexB)
{
    if (indexA >= numElements || indexB >= numElements)
        return false;
    if (indexA == indexB)
        return true;

    /* Read the MediaData at the given indices */
    MediaData mediaDataA = readMediaData(indexA);
    MediaData mediaDataB = readMediaData(indexB);

    erase(indexA);
    insert(mediaDataB, indexA);
    erase(indexB);
    insert(mediaDataA, indexB);

    return true;
}

uint32_t
FileVector::getChecksum()
{
    uint32_t checksum = 0;

    _fileHandle->seekSet(0);
    _fileHandle->read(&checksum, sizeof(uint32_t));
    return checksum;
}

bool
FileVector::setChecksum(uint32_t checksum)
{
    _fileHandle->seekSet(0);
    _fileHandle->write(&checksum, sizeof(uint32_t));
    return true;
}

bool
FileVector::validateChecksum()
{
    /* Iterate through the file and calculate the checksum by adding each filename to the checksum */
    uint32_t checksum = 0;

    for (size_t i = 0; i < numElements; i++) {
        MediaData mediadata = at(i);

        /* Add each byte of the filename to the checksum */
        for (uint8_t j = 0; j < mediadata.filename.size(); j++) {
            checksum += mediadata.filename[j];
        }
    }

    Hash hash(&checksum);
    checksum = hash.get();

    if (checksum == getChecksum())
        return true;
    else
        return false;
}

uint16_t
FileVector::elementLength(MediaData& mediadata)
{
    uint8_t fsize = mediadata.filename.size() + sizeof(char);
    uint16_t psize = mediadata.path.size() + sizeof(char);
    uint16_t usize = mediadata.url.size() + sizeof(char);
    uint16_t length = sizeof(uint8_t) + fsize + sizeof(uint16_t) + psize + sizeof(uint16_t) + usize + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint8_t) +
                      sizeof(bool) + sizeof(uint32_t);   /* Pointer to the next element */

    /* Add the length of the length descriptor to the length */
    length += sizeof(uint16_t);
    length += sizeof(char);   /* @ terminator.  Can actually be any character as long as it's not 0.  Used for finding available space in the index file. */

    return length;
}

uint32_t
FileVector::readFirstElementPosition()
{
    uint32_t firstElementPosition;
    _fileHandle->seekSet(sizeof(uint32_t));
    _fileHandle->read(&firstElementPosition, sizeof(uint32_t));
    return firstElementPosition;
}

bool
FileVector::writeFirstElementPosition(uint32_t position)
{
    _fileHandle->seekSet(sizeof(uint32_t));
    return _fileHandle->write(&position, sizeof(uint32_t));
}

void
FileVector::seekToIndex(uint32_t index)
{
    uint32_t currentPosition = 0;
    uint16_t elementLength = 0;

    _fileHandle->seekSet(0);
    _fileHandle->seekCur(sizeof(uint32_t));   /* Skip the checksum */

    /* Read the first element position */
    _fileHandle->read(&currentPosition, sizeof(uint32_t));
    _fileHandle->seekSet(currentPosition);

    if (index >= _locations.size())
        return;

    _fileHandle->seekSet(_locations[index].start);

}

FileVector::Proxy::operator MediaData() const
{
    return vec.at(index);
}

FileVector::Proxy&
FileVector::Proxy::operator=(const MediaData& mediadata)
{
    if (index >= vec.size()) {
        vec.push_back(mediadata);
    } else {
        /* We need to remove the element currently at index and insert the new element at index */
        vec.erase(index);
        vec.insert(mediadata, index, true);
    }
    return *this;
}

/****************************************************
 *
 * Convenience classes for the Filesystem class
 *
 ****************************************************/

/* Path: Converts a MediaData struct representing a directory to a full vector of MediaData structs representing the
hierarchy all the way down to the root directory. */

Path::Path(MediaData* dir)
{
    this->dir = dir;
}

void
Path::set(MediaData* dir)
{
    this->dir = dir;
}

void
Path::get(std::vector<MediaData>* path)
{
    path->clear();

    if (isRootDir()) {
        path->push_back(*dir);
        return;
    }

    std::string tempPath = dir->path != "/" ? dir->path + "/" + dir->filename : dir->path + dir->filename;

    path->push_back(MediaData({ "/", "/", "", FILETYPE_DIR, 0, LOCAL_FILE, true }));

    size_t pos = 0;
    std::string segment;
    while ((pos = tempPath.find('/')) != std::string::npos) {
        segment = tempPath.substr(0, pos);
        if (!segment.empty()) {
            std::string tempF = segment;
            std::string tempP = path->back().path != "/" && path->back().filename != "/"   ? path->back().path + "/" + path->back().filename
                                : path->back().path == "/" && path->back().filename != "/" ? path->back().path + path->back().filename
                                                                                           : path->back().path;
            path->push_back(MediaData({ tempF, tempP, "", FILETYPE_DIR, 0, LOCAL_FILE, true }));
        }
        tempPath.erase(0, pos + 1);
    }

    /* Add the last segment */
    if (!tempPath.empty()) {
        std::string tempF = tempPath;
        std::string tempP = path->back().path != "/" && path->back().filename != "/"   ? path->back().path + "/" + path->back().filename
                            : path->back().path == "/" && path->back().filename != "/" ? path->back().path + path->back().filename
                                                                                       : path->back().path;
        path->push_back(MediaData({ tempF, tempP, "", FILETYPE_DIR, 0, LOCAL_FILE, true }));
    }
}

bool
Path::isRootDir()
{
    return dir->path == "/" && dir->filename == "/";
}