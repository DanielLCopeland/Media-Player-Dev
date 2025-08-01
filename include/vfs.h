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
#include <AudioTools/Concurrency/Mutex.h>
#include <card_manager.h>
#include <dirent.h>
#include <esp_vfs.h>
#include <vector>

#define PATH_MAX 512

struct vfs_dir
{
    FsFile dir;
    FsFile file;
    dirent entry;
};

struct file_descriptor
{
    int fd;
    FsFile* handle;
    char path[PATH_MAX];
};

static std::vector<file_descriptor> file_descriptors;
static audio_tools::Mutex file_mutex;

static FsFile*
vfs_get_file_handle(int fd)
{
    //while (!file_mutex.try_lock()) {
        // Wait for the mutex to be available
    //    vTaskDelay(10);
    //}
    for (auto& f : file_descriptors) {
        if (f.fd == fd) {
            //file_mutex.unlock();
            return f.handle;
        }
    }
    //file_mutex.unlock();
    return nullptr;
}

static ssize_t
vfs_write(int fd, const void* data, size_t size)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        file_mutex.unlock();
        return -1;
    }

    if (file->isOpen()) {
        file->clearWriteError();
        size_t ret = file->write(data, size);
        if (file->getWriteError()) {
            char filename[PATH_MAX];
            file->getName(filename, PATH_MAX);
            file_mutex.unlock();
            return -1;
        }
        file_mutex.unlock();
        return ret;
    } else {
        file_mutex.unlock();
        return -1;
    }
}

static ssize_t
vfs_read(int fd, void* dst, size_t size)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        file_mutex.unlock();
        return -1;
    }

    if (file->isOpen()) {
        size_t ret = file->read(dst, size);
        file_mutex.unlock();
        return ret;
    } else {
        file_mutex.unlock();
        return -1;
    }
}

static int
vfs_open(const char* path, int flags, int mode)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady() || strlen(path) > PATH_MAX) {
        file_mutex.unlock();
        return -1;
    }

    file_descriptor file;
    file.handle = new FsFile();

    if (!file.handle->open(path, O_RDWR | O_CREAT)) {
        delete file.handle;
        file_mutex.unlock();
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

    file_mutex.unlock();
    return fd;
}

static int
vfs_close(int fd)
{
    file_mutex.lock();

    for (auto it = file_descriptors.begin(); it != file_descriptors.end(); ++it) {
        if (it->fd == fd) {
            it->handle->close();
            delete it->handle;
            file_descriptors.erase(it);
            file_mutex.unlock();
            return 1;
        }
    }

    file_mutex.unlock();
    return -1;
}

static int
vfs_fstat(int fd, struct stat* st)
{
    file_mutex.lock();
    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        file_mutex.unlock();
        return -1;
    }

    if (file->isOpen()) {
        st->st_blksize = 512;
        st->st_size = file->size();
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
        st->st_mtime = 0;
        st->st_atime = 0;
        st->st_ctime = 0;
        file_mutex.unlock();
        return 0;
    } else {
        file_mutex.unlock();
        return -1;
    }
}

int
vfs_stat(const char* path, struct stat* st)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    FsFile file;
    // log_i("Checking file: %s", path);

    if (!Card_Manager::get_handle()->exists(path)) {
        file_mutex.unlock();
        return -1;
    }

    file = Card_Manager::get_handle()->open(path, O_RDONLY);

    st->st_blksize = 512;
    st->st_size = file.size();
    st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
    st->st_mtime = 0;
    st->st_atime = 0;
    st->st_ctime = 0;
    // log_i("File size: %d", st->st_size);
    // log_i("File size from SDFat: %d", file.size());
    file.close();
    file_mutex.unlock();
    return 0;
}

static off_t
vfs_lseek(int fd, off_t offset, int mode)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        file_mutex.unlock();
        return -1;
    }

    int ret = 0;
    if (file->isOpen()) {
        switch (mode) {
            case SEEK_SET:
                ret = file->seekSet(offset);
                file_mutex.unlock();
                return ret;
            case SEEK_CUR:
                ret = file->seekCur(offset);
                file_mutex.unlock();
                return ret;
            case SEEK_END:
                ret = file->seekEnd(offset);
                file_mutex.unlock();
                return ret;
            default:
                ret = -1;
                file_mutex.unlock();
                return ret;
        }
    } else {
        file_mutex.unlock();
        return -1;
    }
}

static int
vfs_link(const char* oldpath, const char* newpath)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady() || strlen(oldpath) > PATH_MAX || strlen(newpath) > PATH_MAX) {
        file_mutex.unlock();
        return -1;
    }

    file_mutex.unlock();
    return -1;
}

static int
vfs_unlink(const char* path)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady() || strlen(path) > PATH_MAX) {
        file_mutex.unlock();
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
        file_mutex.unlock();
        return -1;
    }

    file_mutex.unlock();
    return 0;
}

static int
vfs_rename(const char* oldpath, const char* newpath)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
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
        file_mutex.unlock();
        return -1;
    }

    file_mutex.unlock();
    return 1;
}

static int
vfs_truncate(const char* path, off_t length)
{
    file_mutex.lock();

    FsFile file;

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    if (!Card_Manager::get_handle()->exists(path)) {
        file_mutex.unlock();
        return -1;
    }

    if (!file.open(path, O_RDWR)) {
        file_mutex.unlock();
        return -1;
    }

    if (!file.truncate(length)) {
        file_mutex.unlock();
        return -1;
    }

    return 1;
}

static int
vfs_access(const char* path, int mode)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    if (Card_Manager::get_handle()->exists(path)) {
        file_mutex.unlock();
        return 0;
    } else {
        file_mutex.unlock();
        return -1;
    }
}

static int
vfs_fsync(int fd)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    FsFile* file = vfs_get_file_handle(fd);

    if (file == nullptr) {
        file_mutex.unlock();
        return -1;
    }

    if (file->isOpen()) {
        file->sync();
        file_mutex.unlock();
        return 0;
    } else {
        file_mutex.unlock();
        return -1;
    }
}

/** Directory functions **/
static DIR*
vfs_opendir(const char* name)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return nullptr;
    }
    vfs_dir* vdir = new vfs_dir;
    if (!vdir->dir.open(name, O_RDONLY)) {
        delete vdir;
        file_mutex.unlock();
        return nullptr;
    }
    file_mutex.unlock();
    return reinterpret_cast<DIR*>(vdir);
}

static struct dirent*
vfs_readdir(DIR* pdir)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return nullptr;
    }
    vfs_dir* vdir = reinterpret_cast<vfs_dir*>(pdir);
    // FIXME:
    if (!vdir->file.openNext(&vdir->dir, O_RDONLY)) {
        file_mutex.unlock();
        return nullptr;
    }
    vdir->file.getName(vdir->entry.d_name, sizeof(vdir->entry.d_name));
    vdir->entry.d_type = vdir->file.isDir() ? DT_DIR : DT_REG;
    file_mutex.unlock();
    return &vdir->entry;
}

static int
vfs_closedir(DIR* pdir)
{
    file_mutex.lock();

    vfs_dir* vdir = reinterpret_cast<vfs_dir*>(pdir);
    vdir->dir.close();
    vdir->file.close();
    delete vdir;
    file_mutex.unlock();
    return 0;
}

static int
vfs_mkdir(const char* path, mode_t mode)
{
    file_mutex.lock();

    if (!Card_Manager::get_handle()->isReady()) {
        file_mutex.unlock();
        return -1;
    }

    if (!Card_Manager::get_handle()->mkdir(path)) {
        file_mutex.unlock();
        return -1;
    }

    file_mutex.unlock();
    return 0;
}

static esp_vfs_t sdfat_vfs = { .flags = ESP_VFS_FLAG_DEFAULT,
                               .write = &vfs_write,
                               .lseek = &vfs_lseek,
                               .read = &vfs_read,
                               .pread = NULL,
                               .pwrite = NULL,
                               .open = &vfs_open,
                               .close = &vfs_close,
                               .fstat = &vfs_fstat,
                               .stat = &vfs_stat,
                               .link = &vfs_link,
                               .unlink = &vfs_unlink,
                               .rename = &vfs_rename,
                               .opendir = &vfs_opendir,
                               .readdir = &vfs_readdir,
                               .closedir = &vfs_closedir,
                               .mkdir = &vfs_mkdir,
                               .fsync = &vfs_fsync,
                               .access = &vfs_access,
                               .truncate = &vfs_truncate };

#endif /* vfs_h */