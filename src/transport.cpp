/**
 * @file transport.cpp
 *
 * @brief Manages loading/playing/stopping/pausing files and streams
 *
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

#include <transport.h>

Transport* Transport::_handle = nullptr;

Transport::Transport()
  : ringBuffer(AUDIO_BUFFER_SIZE)
{
}

void
Transport::begin()
{
    resetMetadata();
    if (!loadedMedia) {
        loadedMedia = new MediaData();
    }

    /* Configure the I2S output */
    log_i("Configuring I2S output");
    I2SConfig i2s_config = I2SConfig(TX_MODE);
    i2s_config.sample_rate = 44100;
    i2s_config.bits_per_sample = 16;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.channels = 2;
    i2s_config.buffer_count = 6;
    i2s_config.buffer_size = 512;
    i2s_config.auto_clear = true;
    i2s_config.pin_bck = 18;
    i2s_config.pin_data = 17;
    i2s_config.pin_ws = 8;
    out_i2s.begin(i2s_config);
    log_i("I2S configuration: sample rate: %d, bits per sample: %d, channels: %d", i2s_config.sample_rate, i2s_config.bits_per_sample, i2s_config.channels);

    /* Initialize the volume stream */
    log_i("Initializing volume stream");
    volume_stream.setOutput(out_i2s);
    volume_stream.setVolume(0.0);
    volume_stream.begin();

    /* Initialize the equalizer */
    log_i("Initializing equalizer");
    if (!eq) {
        eq = new EqualizerController(volume_stream);
    }

    /* Configure decoder objects */
    log_i("Creating decoder objects");
    mp3_decoder.setOutput(output);
    flac_decoder.setOutput(output);
    wav_decoder.setOutput(output);
    opus_decoder.setOutput(output);
    mp3_decoder.begin();
    flac_decoder.begin();
    wav_decoder.begin();
    opus_decoder.begin();

    /* Configure the FFT */
    log_i("Configuring FFT");
    auto tcfg = fft.defaultConfig();
    tcfg.copyFrom(i2s_config);
    tcfg.length = 512;

    /* Start the FFT */
    log_i("Starting FFT");
    fft.begin(tcfg);
    fft.reset();

    log_i("Configuring stream mirror");
    output.add(*eq);
    output.add(fft);

    log_i("Starting stream mirror");
    output.begin();

    log_i("Creating spectrum analyzer object");
    spectrumAnalyzer = new SpectrumAnalyzer(&fft, SPECTRUM_ANALYZER_NUM_BANDS, SPECTRUM_ANALYZER_PEAK_DECAY_MS, SPECTRUM_ANALYZER_PEAK_DECAY_RATE_MS);

    log_i("Starting spectrum analyzer");
    spectrumAnalyzer->begin();
    spectrumAnalyzer->clear();
    fft.reset();

    metadata_output.setCallback(Transport::metadataCallback);
    // metadata_output.begin();

    // url_stream.setMetadataCallback(Transport::metadataCallback);
    url_stream.setWaitForData(false);
    status = TRANSPORT_IDLE;

}

void
Transport::audio_writer(Transport* _transport)
{
    /* Loop forever, waiting for data to be available on the ring buffer */
    log_i("Audio task started, reporting from core %d", xPortGetCoreID());
    _transport->spectrumAnalyzer->clear();
    const uint16_t chunksize = AUDIO_BUFFER_READ_CHUNK;
    while (true) {
        if (_transport->ringBuffer.available()) {

            uint16_t bytes_available = _transport->ringBuffer.available();
            if (bytes_available > chunksize) {
                bytes_available = chunksize;
            }
            uint8_t data[bytes_available];
            _transport->mutex.lock();
            _transport->ringBuffer.readArray(data, bytes_available);
            _transport->mutex.unlock();
            _transport->mp3_decoder.write(data, bytes_available);
        }
        if (!_transport->ringBuffer.available()) {
            /* If nothing is available, send chunks of silence to the stream to
            keep the DAC alive and prevent pops and glitches */
            uint8_t silence[chunksize];
            memset(silence, 0, chunksize);
            _transport->output.write(silence, chunksize);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

uint8_t
Transport::getStatus()
{
    return status;
}

void
Transport::resetMetadata()
{
    loadedAlbum[0] = '\0';
    loadedArtist[0] = '\0';
    loadedTitle[0] = '\0';
    loadedGenre[0] = '\0';
    loadedYear[0] = '\0';
}

MediaData
Transport::getLoadedMedia()
{
    return *loadedMedia;
}

bool
Transport::load(MediaData media)
{
    if (!media.loaded)
        return false;

    /* Otherwise, load the file */
    else {
        /* If the source is a local media file, load it from the SD card */
        if (media.type != FILETYPE_M3U) {
            switch (media.source) {
                case LOCAL_FILE:
                    if (_file_descriptor) {
                        close(_file_descriptor);
                    }
                    _file_descriptor = open(media.getPath(), O_RDONLY);
                    bytes_read = 0;
                    if (_file_descriptor) {
                        *loadedMedia = media;
                        status = TRANSPORT_STOPPED;
                        resetMetadata();
                        clearPlayTime();
                        resetMetadata();
                        log_i("Loaded file: %s", media.filename.c_str());
                        return true;
                    } else {
                        log_e("Error loading file: %s", media.filename.c_str());
                        return false;
                    }
                    break;
                case REMOTE_FILE:
                    *loadedMedia = media;
                    resetMetadata();
                    clearPlayTime();
                    status = TRANSPORT_STOPPED;
                    return true;
                    break;
            }
        }
    }
    return false;
}

bool
Transport::play()
{
    /* Clear the buffer */
    mutex.lock();
    ringBuffer.clear();
    mutex.unlock();

    playingUISound = false;
    volume_stream.setVolume((float) volume / TRANSPORT_MAX_VOLUME);

    if (loadedMedia->loaded && loadedMedia->source == LOCAL_FILE && _file_descriptor) {
        status = TRANSPORT_PLAYING;
        log_i("Playing file: %s", loadedMedia->filename.c_str());
        return true;
    }

    else if (loadedMedia->loaded && loadedMedia->source == REMOTE_FILE && WiFi.status() == WL_CONNECTED) {
        if (connection_task != nullptr) {
            connection_task->remove();
            delete connection_task;
            connection_task = nullptr;
        }
        connection_task = new Task("connection_task", 8192, 1, 1);
        connection_task->begin([this] {
            url_stream.end();
            log_i("Connecting to stream: %s", loadedMedia->url.c_str());
            if (url_stream.begin(loadedMedia->url.c_str())) {
                status = TRANSPORT_PLAYING;
            } else {
                log_e("Error connecting to stream: %s", loadedMedia->url.c_str());
                status = TRANSPORT_STOPPED;
                url_stream.end();
            }
            connection_task->remove();
        });
        status = TRANSPORT_CONNECTING;
    }
    return false;
}

void
Transport::pause()
{
    status = TRANSPORT_PAUSED;
    spectrumAnalyzer->clear();
    log_i("Paused");
    if (loadedMedia->source == REMOTE_FILE) {
        url_stream.end();
    }
    if (connection_task) {
        connection_task->remove();
        delete connection_task;
        connection_task = nullptr;
    }
}

void
Transport::stop()
{

    if (status == TRANSPORT_PLAYING || status == TRANSPORT_PAUSED || status == TRANSPORT_IDLE) {

        status = TRANSPORT_STOPPED;
        bytes_read = 0;
        if (loadedMedia->source == LOCAL_FILE) {
            lseek(_file_descriptor, 0, SEEK_SET);
        }
        log_i("Stopped");
        clearPlayTime();
        spectrumAnalyzer->clear();
        if (loadedMedia->source == REMOTE_FILE) {
            clearPlayTime();
            url_stream.end();
        }
        if (connection_task) {
            connection_task->remove();
            delete connection_task;
            connection_task = nullptr;
        }
    }
}

void
Transport::eject()
{
    stop();
    resetMetadata();
    *loadedMedia = MediaData();
    close(_file_descriptor);
    _file_descriptor = -1;
    status = TRANSPORT_IDLE;
}

/****************************************************
 *
 * Play system sounds
 *
 ****************************************************/

void
Transport::playUIsound(const unsigned char* uiSound, size_t length)
{
    if (status == TRANSPORT_STOPPED || status == TRANSPORT_PAUSED || status == TRANSPORT_IDLE || playingUISound) {
        /* Clear the buffer */
        mutex.lock();
        ringBuffer.clear();
        mutex.unlock();
        memory_stream.setValue(uiSound, length);
        playingUISound = true;
        volume_stream.setVolume((float) system_volume / TRANSPORT_MAX_SYSTEM_VOLUME);
    }
}

/****************************************************
 *
 * Volume functions
 *
 ****************************************************/

void
Transport::volumeUp()
{
    if (volume < 100) {
        volume += 2;

        /* Convert the 1-100 volume to a 0-1 float */
        float vol = (float) volume / TRANSPORT_MAX_VOLUME;
        if (!playingUISound) {
            volume_stream.setVolume(vol);
        }
        Config_Manager::get_handle()->setVolume(volume);
    }
}

void
Transport::volumeDown()
{
    if (volume > 0) {
        volume -= 2;

        /* Convert the 1-100 volume to a 0-1 float */
        float vol = (float) volume / TRANSPORT_MAX_VOLUME;
        if (!playingUISound) {
            volume_stream.setVolume(vol);
        }
        Config_Manager::get_handle()->setVolume(volume);
    }
}

uint8_t
Transport::getVolume()
{
    return volume;
}

void
Transport::setVolume(uint8_t volume)
{
    this->volume = volume;

    /* Convert the 1-100 volume to a 0-1 float */
    float vol = (float) volume / TRANSPORT_MAX_VOLUME;
    if (!playingUISound) {
        volume_stream.setVolume(vol);
    }
    Config_Manager::get_handle()->setVolume(volume);
}

uint8_t
Transport::getMinVolume()
{
    return TRANSPORT_MIN_VOLUME;
}

uint8_t
Transport::getMaxVolume()
{
    return TRANSPORT_MAX_VOLUME;
}

uint8_t
Transport::getSystemVolume()
{
    return system_volume;
}

void
Transport::setSystemVolume(uint8_t volume)
{
    system_volume = volume;
    Config_Manager::get_handle()->setSystemVolume(volume);
}

uint8_t
Transport::getMinSystemVolume()
{
    return TRANSPORT_MIN_SYSTEM_VOLUME;
}

uint8_t
Transport::getMaxSystemVolume()
{
    return TRANSPORT_MAX_SYSTEM_VOLUME;
}

void
Transport::systemVolumeUp()
{
    if (system_volume < TRANSPORT_MAX_SYSTEM_VOLUME) {
        system_volume += 2;
        Config_Manager::get_handle()->setSystemVolume(system_volume);
    }
}

void
Transport::systemVolumeDown()
{
    if (system_volume > TRANSPORT_MIN_SYSTEM_VOLUME) {
        system_volume -= 2;
        Config_Manager::get_handle()->setSystemVolume(system_volume);
    }
}

/****************************************************
 *
 * Play time functions
 *
 ****************************************************/

size_t
Transport::getPlayTime()
{
    return playTime;
}

void
Transport::clearPlayTime()
{
    playTime = 0;
}

/****************************************************
 *
 * Looping functions
 *
 ****************************************************/

void
Transport::loop()
{
    if (spectrumAnalyzer && SpectrumAnalyzerUpdateTimer.check(spectrum_analyzer_refresh_interval)) {
        spectrumAnalyzer->update();
    }

    if (status == TRANSPORT_PLAYING) {

        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (playTimeUpdateTimer.check(1000)) {
            playTime++;
        }
        struct stat st;
        int32_t _bytes_available = 0;
        switch (loadedMedia->source) {

            case LOCAL_FILE:

                /* If the audio buffer has space, write the next AUDIO_BUFFER_CHUNK_SIZE
                bytes of the file to the buffer */

                fstat(_file_descriptor, &st);
                _bytes_available = st.st_size - bytes_read;
                if (ringBuffer.availableForWrite() > AUDIO_BUFFER_WRITE_CHUNK && _bytes_available) {
                    uint16_t chunkSize = 0;
                    if (_bytes_available < AUDIO_BUFFER_WRITE_CHUNK) {
                        chunkSize = _bytes_available;
                    } else {
                        chunkSize = AUDIO_BUFFER_WRITE_CHUNK;
                    }

                    uint8_t data[chunkSize];
                    uint32_t _bytes = read(_file_descriptor, data, chunkSize);

                    if (_bytes < 0) {
                        log_e("Error reading file %s", loadedMedia->filename.c_str());
                        stop();
                        break;
                    }
                    bytes_read += _bytes;
                    /* Write the data to the metadata output stream.  There's a bug in the metadata
                    library that causes it to crash.  Might be a memory leak or null pointer dereference.
                    For now, the workaround I've found is to call begin() and end() on the metadata_output
                    object before and after writing data on each pass of the main loop to make sure any
                    objects this library creates are properly initialized and destroyed. */
                    metadata_output.begin();
                    metadata_output.write(data, chunkSize);
                    metadata_output.end();

                    mutex.lock();
                    ringBuffer.writeArray(data, chunkSize);
                    mutex.unlock();
                }

                /* If the file has finished playing, stop the playback */
                if (_bytes_available <= 0) {
                    log_i("End of file %s", loadedMedia->filename.c_str());
                    stop();
                }
                break;

            case REMOTE_FILE:
                /* If the stream has finished playing, stop the playback */
                if (!url_stream.available()) {
                    log_i("No data available");
                    if (connection_timeout_timer.check(CONNECTION_TIMEOUT_MS)) {
                        log_e("Connection timeout");
                        connection_timeout_timer.reset();
                        stop();
                        break;
                    }
                    break;
                } else {
                    connection_timeout_timer.reset();
                }
                /* If the audio buffer has space, write the next AUDIO_BUFFER_CHUNK_SIZE
                bytes of the stream to the buffer */
                if (ringBuffer.availableForWrite() > AUDIO_BUFFER_WRITE_CHUNK) {
                    uint16_t chunkSize = url_stream.available();
                    if (chunkSize > AUDIO_BUFFER_WRITE_CHUNK) {
                        chunkSize = AUDIO_BUFFER_WRITE_CHUNK;
                    }
                    uint8_t data[chunkSize];
                    uint16_t read_bytes = url_stream.readBytes(data, chunkSize);
                    mutex.lock();
                    ringBuffer.writeArray(data, read_bytes);
                    mutex.unlock();
                }
                break;
        }
    }

    if (playingUISound) {

        vTaskDelay(10 / portTICK_PERIOD_MS);

        /* If the audio buffer has space, write the next AUDIO_BUFFER_CHUNK_SIZE bytes of the sound to the buffer */
        if (ringBuffer.availableForWrite() > AUDIO_BUFFER_WRITE_CHUNK && memory_stream.available()) {
            uint16_t chunkSize = 0;
            if (memory_stream.available() < AUDIO_BUFFER_WRITE_CHUNK) {
                chunkSize = memory_stream.available();
            } else {
                chunkSize = AUDIO_BUFFER_WRITE_CHUNK;
            }
            uint8_t data[chunkSize];
            for (int i = 0; i < chunkSize; i++) {
                data[i] = memory_stream.read();
            }
            mutex.lock();
            ringBuffer.writeArray(data, chunkSize);
            mutex.unlock();
        }

        /* If the sound has finished playing, stop the sound */
        if (!memory_stream.available()) {
            playingUISound = false;
            memory_stream.clear();
        }
    }
}

std::string
Transport::getLoadedFileName()
{
    if (loadedMedia->loaded) {
        return (std::string) loadedMedia->filename;
    } else {
        return "";
    }
}

std::string
Transport::getLoadedArtist()
{
    if (loadedMedia->loaded) {
        return (std::string) loadedArtist;
    } else {
        return "";
    }
}

std::string
Transport::getLoadedAlbum()
{
    if (loadedMedia->loaded) {
        return (std::string) loadedAlbum;
    } else {
        return "";
    }
}

std::string
Transport::getLoadedTitle()
{
    if (loadedMedia->loaded) {
        return (std::string) loadedTitle;
    } else {
        return "";
    }
}

std::string
Transport::getLoadedGenre()
{
    if (loadedMedia->loaded) {
        return (std::string) loadedGenre;
    } else {
        return "";
    }
}

std::string
Transport::getLoadedURL()
{
    if (loadedMedia->loaded) {
        return (std::string) loadedMedia->url;
    } else {
        return "";
    }
}

/****************************************************
 *
 * Spectrum analyzer
 *
 ****************************************************/

Transport::SpectrumAnalyzer::SpectrumAnalyzer(AudioRealFFT* fft, uint16_t bands, uint16_t decayTime_ms, uint16_t decayRate_ms)
  : _fft(fft)
  , _bands(bands)
  , _decayTime_ms(decayTime_ms)
  , _decayRate_ms(decayRate_ms)

{
}

void
Transport::SpectrumAnalyzer::begin()
{
    values = new float[_bands];
    peak = new float[_bands];
    peakHoldTime = new uint32_t[_bands];
    lastPeakTime = new uint32_t[_bands];
    lastPeakValue = new float[_bands];
    peakVisibilityFlag = new bool[_bands];

    /* Zero out the values */
    clear();
}

Transport::SpectrumAnalyzer::~SpectrumAnalyzer()
{
    delete[] values;
    delete[] peak;
    delete[] peakHoldTime;
    delete[] lastPeakValue;
    delete[] lastPeakTime;
    delete[] peakVisibilityFlag;
}

/* Read the fft data and update the current and peak values */
void
Transport::SpectrumAnalyzer::update()
{
    /* Reset the values array */
    memset(values, 0, sizeof(values) * _bands);

    /* Calculate the total number of FFT bins */
    size_t totalBins = _fft->size();

    if (totalBins > 0) {

        /* Calculate the number of bins per band */
        size_t binsPerBand = totalBins / _bands;

        /* Loop through each band and calculate the average value */
        for (size_t i = 0; i < _bands; i++) {
            /* Calculate the start and end bins for the current band */
            size_t startBin = i * binsPerBand;
            size_t endBin = (i + 1) * binsPerBand;

            /* Calculate the average value for the current band */
            for (size_t j = startBin; j < endBin; j++) {
                values[i] += _fft->magnitude(j);
            }

            values[i] /= binsPerBand;
        }

        /* Apply a bit of a smoothing effect to the values */
        for (size_t i = 0; i < _bands; i++) {
            values[i] = (values[i] + values[i - 1] + values[i + 1]) / 3;
        }
    }

    /* Now we will apply a simple decay to the peak values to enhance the visual effect */
    decayPeaks();

}

void
Transport::SpectrumAnalyzer::clear()
{
    for (size_t i = 0; i < _bands; i++) {
        values[i] = 0.0;
        peak[i] = 0.0;
        peakHoldTime[i] = 0;
        lastPeakTime[i] = 0;
        lastPeakValue[i] = 0.0;
        peakVisibilityFlag[i] = false;
    }
}

void
Transport::SpectrumAnalyzer::clearValues()
{
    memset(values, 0, sizeof(values) * _bands);
}

float
Transport::SpectrumAnalyzer::getVal(uint8_t band)
{
    return values[band];
}

float
Transport::SpectrumAnalyzer::getPeak(uint8_t band)
{
    return peak[band];
}

uint16_t
Transport::SpectrumAnalyzer::getBands()
{
    return _bands;
}

void
Transport::SpectrumAnalyzer::getVals(uint16_t* currentVal, uint16_t* currentPeak)
{
    for (int i = 0; i < _bands; i++) {
        currentVal[i] = (uint16_t) (values[i]);
        if (values[i] < 0.0) {
            currentVal[i] = 0;
        }
        currentPeak[i] = (uint16_t) (peak[i]);
        if (peak[i] < 0.0) {
            currentPeak[i] = 0;
        }
    }
}

bool
Transport::SpectrumAnalyzer::isPeakVisible(uint8_t band)
{
    return peakVisibilityFlag[band];
}

void
Transport::SpectrumAnalyzer::decayPeaks()
{
    for (size_t i = 0; i < _bands; i++) {
        if (values[i] > peak[i] + 0.1) {
            peak[i] = values[i];
            peakHoldTime[i] = millis();
            lastPeakTime[i] = millis();
            lastPeakValue[i] = values[i];
            peakVisibilityFlag[i] = true;
        }

        else if (peak[i] > 0 && millis() - peakHoldTime[i] > _decayTime_ms) {

            if (millis() - lastPeakTime[i] > _decayRate_ms) {
                peak[i] -= lastPeakValue[i] * 0.1;
                if (peak[i] < 0.1) {
                    peak[i] = 0.0;
                    peakVisibilityFlag[i] = false;
                }
                lastPeakTime[i] = millis();
            }
        }

        if (peak[i] < 0) {
            peak[i] = 0;
        }
    }
}

/****************************************************
 *
 * Graphic equalizer
 *
 ****************************************************/

// FIXME: cast to float before division
void
Transport::EqualizerController::setBass(uint8_t bass)
{
    _bass = bass;
    cfg.gain_low = (float) bass / TRANSPORT_MAX_BASS;
    Config_Manager::get_handle()->setBass(bass);
}

void
Transport::EqualizerController::setMid(uint8_t mid)
{
    _mid = mid;
    cfg.gain_medium = (float) mid / TRANSPORT_MAX_MID;
    Config_Manager::get_handle()->setMid(mid);
}

void
Transport::EqualizerController::setTreble(uint8_t treble)
{
    _treble = treble;
    cfg.gain_high = (float) treble / TRANSPORT_MAX_TREBLE;
    Config_Manager::get_handle()->setTreble(treble);
}

/****************************************************
 *
 * Metadata callback
 *
 ****************************************************/
char Transport::loadedAlbum[255];
char Transport::loadedArtist[255];
char Transport::loadedTitle[255];
char Transport::loadedGenre[255];
char Transport::loadedYear[255];

void
Transport::metadataCallback(MetaDataType info, const char* str, int len)
{
    log_i("Metadata callback: %d, %s, %d", info, str, len);
    if (len && info && str) {
        if (len > 254) {
            len = 254;
        }
        switch (info) {
            case Title:
                strncpy(loadedTitle, str, len);
                /* Check for a null terminator and add one if it's not there */
                break;
            case Artist:
                strncpy(loadedArtist, str, len);
                break;
            case Album:
                strncpy(loadedAlbum, str, len);
                break;
            case Genre:
                strncpy(loadedGenre, str, len);
                break;
        }
    }
}