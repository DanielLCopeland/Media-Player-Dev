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
 * @file vfs.h
 * @author Daniel Copeland
 * @brief
 * @date 2024-12-09
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef vfs_h
#define vfs_h

#include <SdFat.h>
#include <card_manager.h>
#include <esp_vfs.h>
#include <vector>

#define PATH_MAX 512

struct file_descriptor
{
    int fd;
    FsFile* handle;
    char path[PATH_MAX];
};

static std::vector<file_descriptor> file_descriptors;

static FsFile*
vfs_get_file_handle(int fd)
{
    for (auto& f : file_descriptors) {
        if (f.fd == fd) {
            return f.handle;
        }
    }

    return nullptr;
}

static ssize_t
vfs_write(int fd, const void* data, size_t size)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        return -1;
    }

    if (file->isOpen()) {
        file->clearWriteError();
        size_t ret = file->write(data, size);
        if (file->getWriteError()) {
            char filename[PATH_MAX];
            file->getName(filename, PATH_MAX);
            return -1;
        }
        return ret;
    } else {
        return -1;
    }
}

static ssize_t
vfs_read(int fd, void* dst, size_t size)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        return -1;
    }

    if (file->isOpen()) {
        size_t ret = file->read(dst, size);
        return ret;
    } else {
        return -1;
    }
}

static int
vfs_open(const char* path, int flags, int mode)
{
    if (!Card_Manager::get_handle()->isReady() || strlen(path) > PATH_MAX) {
        return -1;
    }

    file_descriptor file;
    file.handle = new FsFile();

    if (!file.handle->open(path, O_RDWR | O_CREAT)) {
        delete file.handle;
        return -1;
    }
    /* Generate a unique file descriptor that is not already in use */
    int fd = 1;
    for (auto& f : file_descriptors) {
        if (f.fd == fd) {
            fd++;
        }
    }

    file.fd = fd;
    file_descriptors.push_back(file);
    strcpy(file.path, path);

    return fd;
}

static int
vfs_close(int fd)
{
    for (auto it = file_descriptors.begin(); it != file_descriptors.end(); ++it) {
        if (it->fd == fd) {
            it->handle->close();
            delete it->handle;
            file_descriptors.erase(it);
            return 1;
        }
    }

    return -1;
}

static int
vfs_fstat(int fd, struct stat* st)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        return -1;
    }

    if (file->isOpen()) {
        st->st_blksize = 512;
        st->st_size = file->size();
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
        st->st_mtime = 0;
        st->st_atime = 0;
        st->st_ctime = 0;
        return 0;
    } else {
        return -1;
    }
}

static off_t
vfs_lseek(int fd, off_t offset, int mode)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        return -1;
    }

    if (file->isOpen()) {
        switch (mode) {
            case SEEK_SET:
                return file->seekSet(offset);
            case SEEK_CUR:
                return file->seekCur(offset);
            case SEEK_END:
                return file->seekEnd(offset);
            default:
                return -1;
        }
    } else {
        return -1;
    }
}

static int
vfs_link(const char* oldpath, const char* newpath)
{
    if (!Card_Manager::get_handle()->isReady() || strlen(oldpath) > PATH_MAX || strlen(newpath) > PATH_MAX) {
        return -1;
    }

    return -1;
}

static int
vfs_unlink(const char* path)
{
    if (!Card_Manager::get_handle()->isReady() || strlen(path) > PATH_MAX) {
        return -1;
    }

    /* Go through all open file descriptors and close the file if it is open */
    for (auto it = file_descriptors.begin(); it != file_descriptors.end(); ++it) {
        if (strcmp(it->path, path) == 0) {
            it->handle->close();
            delete it->handle;
            file_descriptors.erase(it);
            break;
        }
    }

    if (!Card_Manager::get_handle()->remove(path)) {
        return -1;
    }

    return 1;
}

static int
vfs_rename(const char* oldpath, const char* newpath)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }

    /* Go through all open file descriptors and close the file if it is open */
    for (auto it = file_descriptors.begin(); it != file_descriptors.end(); ++it) {
        if (strcmp(it->path, oldpath) == 0) {
            it->handle->close();
            delete it->handle;
            file_descriptors.erase(it);
            break;
        }
    }

    if (!Card_Manager::get_handle()->rename(oldpath, newpath)) {
        return -1;
    }

    return 1;
}

static int
vfs_truncate(const char* path, off_t length)
{
    FsFile file;

    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }

    if (!Card_Manager::get_handle()->exists(path)) {
        return -1;
    }

    if (!file.open(path, O_RDWR)) {
        return -1;
    }

    if (!file.truncate(length)) {
        return -1;
    }

    return 1;
}

static int
vfs_access(const char* path, int mode)
{
    if (!Card_Manager::get_handle()->isReady()) {
        return -1;
    }

    if (Card_Manager::get_handle()->exists(path)) {
        return 0;
    } else {
        return -1;
    }
}

static int
vfs_fsync(int fd)
{
    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        return -1;
    }

    if (file->isOpen()) {
        file->sync();
        return 0;
    } else {
        return -1;
    }
}

static esp_vfs_t sdfat_vfs = {
    .flags = ESP_VFS_FLAG_DEFAULT,
    .write = &vfs_write,
    .lseek = &vfs_lseek,
    .read = &vfs_read,
    .open = &vfs_open,
    .close = &vfs_close,
    .fstat = &vfs_fstat,
    .link = &vfs_link,
    .unlink = &vfs_unlink,
    .rename = &vfs_rename,
    .fsync = &vfs_fsync,
    .access = &vfs_access,
    .truncate = &vfs_truncate
};

#endif /* vfs_h */