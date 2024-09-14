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

#include <playlist_engine.h>

/* Constructor for the PlaylistEngine engine when used as a standalone controller object */
PlaylistEngine::PlaylistEngine(std::function<bool(MediaData)> loadCallback,
                               std::function<bool()> playCallback,
                               std::function<void()> stopCallback,
                               std::function<uint8_t()> statusCallback)
  : loadCallback(loadCallback)
  , playCallback(playCallback)
  , statusCallback(statusCallback)
  , stopCallback(stopCallback)
  , urlRegex("^(http|https)://.*")
  , localRegex("^((/[a-zA-Z0-9-_.]+)+|/)(.mp3\\b|.flac|\\b.wav|\\b.ogg|\\b)$")
  , callBacksEnabled(true)
{
}
/* Constructor for the PlaylistEngine engine when used as a member of another class, used
to implement an editor or serve as a data source for tracks */
PlaylistEngine::PlaylistEngine(PlaylistEngine* mainEngine)
  : urlRegex("^(http|https)://.*")
  , localRegex("^((/[a-zA-Z0-9-_.]+)+|/)(.mp3\\b|.flac|\\b.wav|\\b.ogg|\\b)$")
  , callBacksEnabled(false)
  , _mainEngine(mainEngine)
{
}

bool
PlaylistEngine::load(MediaData playlist)
{

    if (playlist.type != FILETYPE_M3U) {
        return false;
    }

    /* Attempt to open the playlist file */
    if (!sdfs->isReady() || !file.open(playlist.getPath())) {
        eject();
        return false;
    }
    file.close();

    if (_playlist == nullptr) {
        _playlist = new MediaData(playlist);
    }

    getTrackList();

    /* If we have tracks to play, initialize everything */
    if (trackList.size()) {
        *this->_playlist = playlist;
        this->currentTrack = 0;
        this->mode = NORMAL;
        this->enabled = true;

        if (callBacksEnabled) {
            /* Load the transport with the first track */
            if (!loadCallback(getTrack(currentTrack))) {
                log_e("PlaylistEngine: load failed");
                delete _playlist;
                _playlist = nullptr;
                return false;
            }
        }
        return true;
    } else {
        *this->_playlist = playlist;
        this->currentTrack = 0;
        this->mode = NORMAL;
        this->enabled = false;
        return true;
    }
    delete _playlist;
    _playlist = nullptr;
    return false;
}

bool
PlaylistEngine::checkLine(std::string line)
{

    if (std::regex_match(line, localRegex)) {
        return true;
    } else {
        /* Convert the line to lowercase */
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        if (std::regex_match(line, urlRegex)) {
            return true;
        }
    }

    return false;
}

/*
Iterate through the file and count up the number of tracks.  Use checkLine()
to determine if the line is a valid track. Use the end of the file or newline
as a delimiter. If there is no valid data in the file, return false. We also
have to deal with any possible carriage returns in the file.
*/
bool
PlaylistEngine::getTrackList()
{
    if (sdfs->isReady() && _playlist) {
        if (sdfs->exists(_playlist->getPath())) {
            file.open(_playlist->getPath());
        } else {
            eject();
            log_e("PlaylistEngine: could not open playlist file");
            return false;
        }
    } else {
        log_e("PlaylistEngine: could not open playlist file");
        return false;
    }
    trackList.clear();
    file.seek(0);
    std::string line;

    while (sdfs->isReady() && file.available()) {

        if (trackList.size() >= PLAYLIST_TRACK_LIMIT) {
            break;
        }

        serviceLoop();
        line.clear();
        uint32_t startPointer = file.position();

        while (file.peek() != '\n' && file.available()) {
            line += file.read();
        }

        /* Strip out any carriage returns */
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        if (checkLine(line)) {
            uint32_t endPointer = file.position();
            uint16_t trackCount = trackList.size();
            trackList.push_back(TrackData{ trackCount, startPointer, endPointer });
        }

        file.seek(file.position() + 1);
    }
    log_d("PlaylistEngine: Found %d tracks", trackList.size());
    if (!sdfs->isReady()) {
        log_e("PlaylistEngine: SD card not ready");
        eject();
        return false;
    }
    file.close();
    return true;
}

MediaData
PlaylistEngine::getTrack(size_t track)
{
    if (!sdfs->isReady()) {
        log_e("PlaylistEngine: SD card not ready");
        eject();
        return { "", "", "", FILETYPE_UNKNOWN, 0, NO_SOURCE_LOADED, false };
    }

    if (track >= trackList.size()) {
        return { "", "", "", FILETYPE_UNKNOWN, 0, NO_SOURCE_LOADED, false };
    }

    if (_playlist) {
        if (_playlist) {
            if (sdfs->exists(_playlist->getPath())) {
                file.open(_playlist->getPath());
            }
        } else {
            log_e("PlaylistEngine: could not open playlist file");
            return { "", "", "", FILETYPE_UNKNOWN, 0, NO_SOURCE_LOADED, false };
        }
    }

    file.seek(trackList[track].startPointer);
    std::string line;
    while (sdfs->isReady() && (char) file.peek() != '\n' && file.available()) {
        line += (char) file.read();
    }

    /* Strip out any carriage returns */
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

    /* Parse the line and determine if it's a URL or local file */
    if (std::regex_match(line, urlRegex)) {
        return { "", "", line, FILETYPE_UNKNOWN, 0, REMOTE_FILE, true };
    } else if (std::regex_match(line, localRegex)) {

        /*  Extract the path, filename, and file type data */
        std::string path = line.substr(0, line.find_last_of("/"));
        std::string filename = line.substr(line.find_last_of("/") + 1);
        std::string ext = filename.substr(filename.find_last_of(".") + 1);

        if (sdfs->isReady() && ext == "mp3") {
            file.close();
            return { filename, path, "", FILETYPE_MP3, 0, LOCAL_FILE, true };
        } else if (sdfs->isReady() && ext == "flac") {
            file.close();
            return { filename, path, "", FILETYPE_FLAC, 0, LOCAL_FILE, true };
        } else if (sdfs->isReady() && ext == "wav") {
            file.close();
            return { filename, path, "", FILETYPE_WAV, 0, LOCAL_FILE, true };
        } else if (sdfs->isReady() && ext == "ogg") {
            file.close();
            return { filename, path, "", FILETYPE_OGG, 0, LOCAL_FILE, true };
        }
    }
    eject();
    return { "", "", "", FILETYPE_UNKNOWN, 0, NO_SOURCE_LOADED, false };
}

MediaData
PlaylistEngine::getCurrentTrack()
{
    return getTrack(currentTrack);
}

bool
PlaylistEngine::next()
{
    if (trackList.size() == 0 || !enabled) {
        return false;
    }
    if (currentTrack < trackList.size() - 1) {
        currentTrack++;
        if (callBacksEnabled) {
            stopCallback();
            if (!loadCallback(getTrack(currentTrack))) {
                log_e("PlaylistEngine: load failed");
                /* Keep trying to load the next track until we find one that works */
                next();
            }
            if (playing) {
                playCallback();
            }
        }
        return true;
    } else {
        stop();
    }

    return false;
}

bool
PlaylistEngine::previous()
{
    if (trackList.size() == 0 || !enabled) {
        return false;
    }
    if (currentTrack > 0) {
        currentTrack--;
        if (callBacksEnabled) {
            stopCallback();
            if (!loadCallback(getTrack(currentTrack))) {
                return false;
            }
            if (playing) {
                playCallback();
            }
        }
        return true;
    }

    return false;
}

bool
PlaylistEngine::shuffle()
{
    if (trackList.size() == 0 || !enabled) {
        return false;
    }

    std::random_shuffle(trackList.begin(), trackList.end());
    currentTrack = 0;
    return true;
}

void
PlaylistEngine::eject()
{
    enabled = false;
    playing = false;
    trackList.clear();
    file.close();
    if (_playlist) {
        delete _playlist;
        _playlist = nullptr;
    }
    _mainEngine = nullptr;
}

void
PlaylistEngine::loop()
{

    if (!sdfs->isReady() && !enabled || trackList.size() == 0 || !_playlist || playing == false) {
        return;
    }

    {
        if (callBacksEnabled && statusCallback() == TRANSPORT_STOPPED && playing && enabled) {
            if (!next()) {
                stop();
                return;
            }
            if (!loadCallback(getCurrentTrack())) {
                stop();
                log_e("PlaylistEngine: load failed");
                return;
            }
            playCallback();
        }
    }
}

void
PlaylistEngine::play()
{
    if (trackList.size() == 0 || !enabled) {
        return;
    }
    playing = true;
    if (statusCallback() == TRANSPORT_STOPPED && enabled)
        playCallback();
}

void
PlaylistEngine::stop()
{
    if (trackList.size() == 0 || !enabled) {
        return;
    }
    playing = false;
    stopCallback();
}

bool
PlaylistEngine::removeTrack(size_t track)
{
    if (track >= trackList.size()) {
        return false;
    }

    if (!sdfs->isReady() || !_playlist) {
        return false;
    }

    /* Erase the track from the playlist file by using a temporary file. Open
    it in the same directory as the playlist file. */
    FsFile tempFile;

    /* Open a dir file to get the path */
    FsFile dir;
    std::string path = _playlist->path;
    if (!sdfs->isReady() || !dir.open(path.c_str())) {
        log_e("PlaylistEngine: could not open directory");
        return false;
    }

    if (_playlist) {

        if (!sdfs->isReady() || !tempFile.open(&dir, TEMP_FILE, O_RDWR | O_TRUNC | O_CREAT)) {
            log_e("PlaylistEngine: could not open temp file");
            return false;
        }

        if (sdfs->isReady() && dir.exists(_playlist->filename.c_str())) {
            file.open(&dir, _playlist->filename.c_str(), O_RDONLY);
        } else {
            log_e("PlaylistEngine: could not open playlist file");
            return false;
        }
    }

    else {
        log_e("PlaylistEngine: _playlist is null");
        return false;
    }
    file.open(_playlist->getPath());
    file.seek(0);
    std::string line;
    size_t trackCount = 0;

    while (sdfs->isReady() && file.available()) {

        line.clear();

        if (trackCount == track) {
            /* Skip to the track we want to remove */
            while ((char) file.peek() != '\n' && file.available()) {
                line += file.read();
            }
            file.seek(file.position() + 1);
            if (checkLine(line)) {
                trackCount++;
            }
            continue;
        }

        while (sdfs->isReady() && (char) file.peek() != '\n' && file.available()) {
            line += (char) file.read();
        }
        if (!sdfs->isReady()) {
            log_e("PlaylistEngine: SD card not ready");
            tempFile.close();
            eject();
            return false;
        }

        /* Strip out any carriage returns */
        if (checkLine(line)) {
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            tempFile.write(line.c_str(), line.length());
            tempFile.write("\n", 1);
            line.clear();
            file.seek(file.position() + 1);
            trackCount++;
        }
    }

    /* Delete the original playlist file and rename the temp file */
    file.close();
    dir.remove(_playlist->filename.c_str());

    tempFile.rename(&dir, _playlist->filename.c_str());
    tempFile.close();
    dir.close();

    if (!getTrackList()) {
        log_e("PlaylistEngine: could not get track list");
        eject();
        return false;
    }

    /* If the current track is bigger than the tracklist size, set it to the last track */
    if (currentTrack >= trackList.size() && trackList.size() > 0) {
        currentTrack = trackList.size() - 1;
    } else if (trackList.size() == 0) {
        currentTrack = 0;
    }

    return true;
}

bool
PlaylistEngine::addTrack(MediaData track)
{
    if (track.type != FILETYPE_MP3 && track.type != FILETYPE_FLAC && track.type != FILETYPE_WAV && track.type != FILETYPE_OGG) {
        return false;
    }

    if (!sdfs->isReady() || !_playlist) {
        return false;
    }

    if (!(checkLine(track.getPath()) || checkLine(track.url))) {
        return false;
    }

    /* Open directory file */
    FsFile dir;
    std::string path = _playlist->path;
    if (!dir.open(path.c_str())) {
        log_e("PlaylistEngine: could not open directory");
        return false;
    }

    /* Open the playlist file */
    if (!file.open(&dir, _playlist->filename.c_str(), O_RDWR | O_APPEND)) {
        log_e("PlaylistEngine: could not open playlist file");
        return false;
    }

    uint32_t startPointer = 0;
    uint32_t endPointer = 0;

    /* Write the track to the end of the playlist file */
    file.seek(file.size());
    file.write("\n", 1);
    startPointer = file.position();
    file.write(track.getPath());
    endPointer = file.position();

    /* Add the track to the trackList vector */
    uint16_t trackCount = trackList.size();
    trackList.push_back(TrackData{ trackCount, startPointer, endPointer });
    file.close();
    return true;
}

bool
PlaylistEngine::get(size_t from, size_t to, std::vector<MediaData>& items)
{
    if (!enabled || trackList.size() == 0 || !_playlist || from >= trackList.size() || from > to) {
        return false;
    }

    if (to > trackList.size()) {
        to = trackList.size();
    }

    for (size_t i = from; i < to; i++) {
        items.push_back(getTrack(i));
    }
    return true;
}

uint16_t
PlaylistEngine::view(bool showIndex)
{
    if (!enabled || trackList.size() == 0 || !_playlist) {
        return UI_EXIT;
    }

    ListSelection listSelection;
    SystemMessage systemMessage;
    uint16_t selection = 0;

    if (!enabled || trackList.size() == 0 || !_playlist) {
        systemMessage.show("No playlist loaded!", 2000, false);
    }

    return listSelection.get(this, showIndex);
}

bool
PlaylistEngine::setCurrentTrack(size_t track)
{
    if (!enabled || track >= trackList.size() || !_playlist) {
        return false;
    }

    currentTrack = track;
    return true;
}

bool
PlaylistEngine::isLoaded()
{
    if (_playlist) {
        return true;
    }
    return false;
}