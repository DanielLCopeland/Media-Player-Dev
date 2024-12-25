/**
 * @file playlist_controller.h
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
#ifndef playlist_engine_h
#define playlist_engine_h

#include <SdFat.h>
#include <functional>
#include <regex>
#include <string>
#include <system.h>
#include <card_manager.h>
#include <transport.h>
#include <ui/common.h>
#include <vector>

#define TEMP_FILE "~playlist.tmp"

class MediaData;
class Transport;
class CardManager;
extern CardManager* sdfs;

/* We must limit the size of the playlist to prevent memory issues */
#define PLAYLIST_TRACK_LIMIT 1500

class PlaylistEngine
{
    enum PlaylistMode
    {
        NORMAL,
        SHUFFLE,
    };

  public:
    /* Main constructor, pass in the callbacks to load, play, stop, and get the status of the transport */
    PlaylistEngine(std::function<bool(MediaData)> loadCallback,
                   std::function<bool()> playCallback,
                   std::function<void()> stopCallback,
                   std::function<uint8_t()> statusCallback);

    /*
     * An additional overload to allow using the methods in this class as a viewer/editor.
     * We pass in a pointer to the main playlist engine so we can determine the current track
     * playing and stop the transport if the current track or playlist is being edited.
     */
    PlaylistEngine(PlaylistEngine* mainEngine);
    ~PlaylistEngine() { eject(); }
    bool isPlaying() { return playing; }
    bool isLoaded();
    bool load(MediaData playlist);
    bool next();
    bool previous();
    MediaData* getLoadedMedia()
    {
        if (isLoaded())
            return _playlist;
        else
            return nullptr;
    }
    bool setMode(PlaylistMode mode)
    {
        switch (mode) {
            case NORMAL:
                if (reset()) {
                    this->mode = mode;
                    return true;
                }
                break;

            case SHUFFLE:
                if (shuffle()) {
                    this->mode = mode;
                    return true;
                }
                break;

            default:
                return false;
                break;
        }
        return false;
    }
    PlaylistMode getMode() { return mode; }

    bool isEnabled() { return enabled; }
    void eject();
    void loop();
    void play();
    void stop();
    size_t size() { return trackList.size(); }

    /* Returns a vector of strings containing items in the playlist */
    bool get(size_t from, size_t to, std::vector<MediaData>& items);

    MediaData getCurrentTrack();
    uint16_t getCurrentTrackIndex()
    {
        if (callBacksEnabled) {
            return currentTrack;
        } else if (this->_mainEngine) {
            return this->_mainEngine->getCurrentTrackIndex();
        } else {
            return 0;
        }
    }
    bool setCurrentTrack(size_t track);

    /* How to tell if the playlist object is the main driver of the transport or being used as a viewer/editor */
    bool isDriver() { return callBacksEnabled; }

    bool addTrack(MediaData track);
    bool removeTrack(size_t track);

    uint16_t available()
    {
        if (trackList.size() > 0)
            return (trackList.size() - 1) - currentTrack;
        else
            return 0;
    }

    /* A browser which allows you to view the playlist and select a track */
    uint16_t view(bool showIndex = false);

    MediaData getTrack(size_t track);

  private:
    /* Randomize the order of the playlist */
    bool shuffle();

    /* Reset the playlist to the original order */
    bool reset()
    {
        if (trackList.size() == 0) {
            return false;
        }
        for (size_t i = 0; i < trackList.size(); i++)
            trackList[i].shuffleindex = i;
    }

    bool callBacksEnabled = false;
    bool getTrackList();
    bool checkLine(std::string line);
    uint16_t currentTrack = 0;
    MediaData* _playlist = nullptr;
    FsFile file;
    bool playing = false;
    bool enabled = false;
    PlaylistMode mode = NORMAL;
    PlaylistEngine* _mainEngine = nullptr;

    struct TrackData
    {
        uint16_t shuffleindex;
        uint32_t startPointer;
        uint32_t endPointer;
    };

    std::vector<TrackData> trackList;

    /* Callbacks */
    std::function<bool(MediaData)> loadCallback;
    std::function<bool()> playCallback;
    std::function<uint8_t()> statusCallback;
    std::function<void()> stopCallback;

    std::regex urlRegex;
    std::regex localRegex;
};

#endif