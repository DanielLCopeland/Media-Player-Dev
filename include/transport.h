/**
 * @file transport.h
 *
 * @brief Manages loading/playing/stopping/pausing files and streams
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

#ifndef transport_h
#define transport_h
#define CONNECTION_TIMEOUT_MS 4000
#define PLAYTIME_UPDATE_INTERVAL_MS 1000

/* Audio buffer size in bytes. Decrease this if you have memory issues, increase if you have audio issues. This
buffer is used to transfer audio data from one task to another. We use mutexes to prevent data corruption. */
#define AUDIO_BUFFER_SIZE 1024 * 32
#define AUDIO_BUFFER_WRITE_CHUNK 1024 * 2 /* Chunks written from the main loop */
#define AUDIO_BUFFER_READ_CHUNK 1024 * 3 /* Chunks read by the audio task into the audio chain */

#include <AudioTools.h>
#include <AudioTools/AudioCodecs/AudioCodecs.h>
#include <AudioTools/AudioCodecs/CodecFLAC.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/AudioCodecs/CodecOpusOgg.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>
#include <AudioTools/CoreAudio/MusicalNotes.h>
#include <AudioTools/AudioLibs/AudioRealFFT.h>
#include <AudioTools/Concurrency/All.h>
#include <SdFat.h>
#include <WiFi.h>
#include <system.h>
#include <timer.h>

enum transport_status : uint8_t
{
    TRANSPORT_PLAYING,
    TRANSPORT_PAUSED,
    TRANSPORT_STOPPED,
    TRANSPORT_IDLE,
    TRANSPORT_BUFFERING,
    TRANSPORT_CONNECTING
};

enum spectrum_analyzer
{
    SPECTRUM_ANALYZER_NUM_BANDS = 7,
    SPECTRUM_ANALYZER_UPDATE_INTERVAL_MS = 30,
    SPECTRUM_ANALYZER_PEAK_DECAY_MS = 500,
    SPECTRUM_ANALYZER_PEAK_DECAY_RATE_MS = 50,
    SPECTRUM_ANALYZER_PEAK_VISBILITY_TIMEOUT_MS = 5000
};

enum filter_constants
{
    TRANSPORT_MIN_VOLUME = 0,
    TRANSPORT_MAX_VOLUME = 100,
    TRANSPORT_MIN_SYSTEM_VOLUME = 0,
    TRANSPORT_MAX_SYSTEM_VOLUME = 100,
    TRANSPORT_BASS_CENTER_FREQ = 200,
    TRANSPORT_MIN_BASS = 0,
    TRANSPORT_MAX_BASS = 100,
    TRANSPORT_MID_CENTER_FREQ = 3000,
    TRANSPORT_MIN_MID = 0,
    TRANSPORT_MAX_MID = 100,
    TRANSPORT_TREBLE_CENTER_FREQ = 8000,
    TRANSPORT_MIN_TREBLE = 0,
    TRANSPORT_MAX_TREBLE = 100,
    TRANSPORT_CONTROL_STEP_SIZE = 2
};

class MediaData;
class SystemConfig;

extern SystemConfig* systemConfig;

class Transport
{
  public:
    Transport();
    void begin();   /* Starts the stream and audio objects */
    bool load(MediaData media);
    bool play();
    void playUIsound(const unsigned char* uiSound, size_t length);
    void pause();
    void stop();
    void eject();

    std::string getLoadedFileName();
    std::string getLoadedURL();
    std::string getLoadedTitle();
    std::string getLoadedArtist();
    std::string getLoadedAlbum();
    std::string getLoadedGenre();
    std::string getLoadedYear();

    void resetMetadata();
    void loop();

    uint8_t getStatus();          /* returns the current status of the transport (PLAYING, PAUSED, STOPPED, or IDLE) */
    MediaData getLoadedMedia();   /* returns the data of the currently loaded media */

    void volumeUp();                  /* Increases the volume by 2, up to a maximum of 100 */
    void volumeDown();                /* Decreases the volume by 2, down to a minimum of 0 */
    uint8_t getVolume();              /* Returns the current volume level */
    void setVolume(uint8_t volume);   /* Sets the volume level */
    uint8_t getMinVolume();           /* Returns the minimum volume level */
    uint8_t getMaxVolume();           /* Returns the maximum volume level */

    void systemVolumeUp();                  /* Increases the system volume by 2, up to a maximum of 100 */
    void systemVolumeDown();                /* Decreases the system volume by 2, down to a minimum of 0 */
    uint8_t getSystemVolume();        /* Returns the current system volume level */
    void setSystemVolume(uint8_t volume);   /* Sets the system volume level */
    uint8_t getMinSystemVolume();          /* Returns the minimum system volume level */
    uint8_t getMaxSystemVolume();          /* Returns the maximum system volume level */

    /* Return the play time of the currently loaded media */
    size_t getPlayTime();
    void clearPlayTime();

    static std::function<void()> audio_writer(Transport* _transport);

    Timer debugTimer;

    /* Spectrum analyzer */
    class SpectrumAnalyzer
    {
      public:
        SpectrumAnalyzer(audio_tools::AudioRealFFT* fft, uint16_t bands, uint16_t decayTime_ms, uint16_t decayRate_ms);
        ~SpectrumAnalyzer();
        audio_tools::AudioRealFFT* _fft;
        uint16_t _bands;
        float* values;
        float* peak;
        uint32_t* peakHoldTime;
        uint32_t* lastPeakTime;
        bool* peakVisibilityFlag;
        float* lastPeakValue;
        float getVal(uint8_t band);
        void getVals(uint16_t* currentVal, uint16_t* currentPeak);
        float getPeak(uint8_t band);
        uint16_t getBands();
        uint16_t _decayTime_ms;
        uint16_t _decayRate_ms;
        void update();
        void begin();
        void clear();
        void clearValues();
        void decayPeaks();
        bool isPeakVisible(uint8_t band);
    }* spectrumAnalyzer = nullptr;
    static const uint16_t spectrum_analyzer_refresh_interval = 5;

    class EqualizerController : public audio_tools::Equilizer3Bands
    {
      private:
        bool eq_enabled = false;
        uint8_t control_step = 2;
        uint8_t _bass = 0;
        uint8_t _mid = 0;
        uint8_t _treble = 0;

        ConfigEquilizer3Bands cfg;

      public:
        EqualizerController(audio_tools::AudioStream& out)
          : audio_tools::Equilizer3Bands(out)
        {
            cfg.sample_rate = 44100;
            cfg.bits_per_sample = 16;
            cfg.channels = 2;
            cfg.gain_low = 0.5;
            cfg.gain_medium = 0.5;
            cfg.gain_high = 0.5;
            this->begin(cfg);
        }

        /* Convert to a 0.0-0.1 float value to set the EQ.  Use MAX and MIN constants to get the range */
        void setBass(uint8_t bass);
        void setMid(uint8_t mid);
        void setTreble(uint8_t treble);

        void bassUp()
        {
            if (this->_bass + control_step <= TRANSPORT_MAX_BASS) {
                this->_bass += control_step;
                setBass(this->_bass);
            }
        }
        void bassDown()
        {
            if (this->_bass - control_step >= TRANSPORT_MIN_BASS) {
                this->_bass -= control_step;
                setBass(this->_bass);
            }
        }
        void midUp()
        {
            if (this->_mid + control_step <= TRANSPORT_MAX_MID) {
                this->_mid += control_step;
                setMid(this->_mid);
            }
        }
        void midDown()
        {
            if (this->_mid - control_step >= TRANSPORT_MIN_MID) {
                this->_mid -= control_step;
                setMid(this->_mid);
            }
        }
        void trebleUp()
        {
            if (this->_treble + control_step <= TRANSPORT_MAX_TREBLE) {
                this->_treble += control_step;
                setTreble(this->_treble);
            }
        }
        void trebleDown()
        {
            if (this->_treble - control_step >= TRANSPORT_MIN_TREBLE) {
                this->_treble -= control_step;
                setTreble(this->_treble);
            }
        }

        uint8_t getBass() { return _bass; }
        uint8_t getMid() { return _mid; }
        uint8_t getTreble() { return _treble; }

        uint8_t getMinBass() { return TRANSPORT_MIN_BASS; }
        uint8_t getMaxBass() { return TRANSPORT_MAX_BASS; }
        uint8_t getMinMid() { return TRANSPORT_MIN_MID; }
        uint8_t getMaxMid() { return TRANSPORT_MAX_MID; }
        uint8_t getMinTreble() { return TRANSPORT_MIN_TREBLE; }
        uint8_t getMaxTreble() { return TRANSPORT_MAX_TREBLE; }

        void setEnabled(bool enabled) { eq_enabled = enabled; }
        bool isEnabled() { return eq_enabled; }

    } *eq = nullptr;

    audio_tools::URLStream* getURLStream() { return &url_stream; }

  private:
    /* Decoders */
    audio_tools::MP3DecoderHelix mp3_decoder;
    audio_tools::OpusOggDecoder opus_decoder;
    audio_tools::WAVDecoder wav_decoder;
    audio_tools::FLACDecoder flac_decoder;

    /* Audio objects */
    audio_tools::I2SStream out_i2s;
    audio_tools::VolumeStream volume_stream;
    audio_tools::AudioRealFFT fft;
    audio_tools::MultiOutput output;
    audio_tools::MemoryStream memory_stream = MemoryStream(0, 0);

    /* Metadata */
    static void metadataCallback(MetaDataType type, const char* str, int len);
    MetaDataOutput metadata_output;
    static char loadedAlbum[255];
    static char loadedArtist[255];
    static char loadedTitle[255];
    static char loadedGenre[255];
    static char loadedYear[255];

    Timer spectrumAnalyzerPeakDecayTimer;
    Timer SpectrumAnalyzerUpdateTimer;

    audio_tools::URLStream url_stream;
    //ICYStream url_stream;
    Timer connection_timeout_timer;

    audio_tools::Task audio_task;
    audio_tools::Task* connection_task = nullptr;

    FsFile audio_file;

    /* Timer to control the play time display */
    Timer playTimeUpdateTimer;

    size_t playTime = 0;   /* The current play time of the loaded media */

    MediaData* loadedMedia = nullptr;   /* Stores the data of the currently loaded file */
    transport_status status;            /* The current status of the transport (PLAYING, PAUSED, STOPPED, or IDLE) */

    uint8_t volume = 2;   /* The current volume level */
    uint8_t system_volume = 2; /* The volume of the menu sounds */

    const unsigned char* uiSound;     /* Pointer to a char array to play from for UI sounds */
    size_t uiSoundLength;             /* The length of the UI sound array */
    bool playingUISound = false;   /* True if a UI sound is currently playing */

    /* Lock this mutex before accessing the audio buffer, unlock when done.  If you don't,
    bad things will happen and you won't find the bug for days, don't ask me how I know. */
    audio_tools::Mutex mutex; 
    audio_tools::RingBuffer<uint8_t> ringBuffer; /* The audio buffer */
};

#endif