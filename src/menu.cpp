/**
 * @file menu.h
 *
 * @brief Menu system
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

#include <menu.h>

void
mainMenu()
{
    using namespace MENUDATA::MAIN_m;

    UI::ListSelection mainMenu;
    items selection;

    while (true) {
        selection = (items) mainMenu.get(menu, SIZE);

        switch (selection) {
            case WIFI:
                networkMenu();
                break;

            case BLUETOOTH:
                bluetoothMenu();
                break;

            case SYSTEM:
                systemMenu();
                break;

            case PLAYLIST_EDITOR:
                playlistEditor_mainMenu();
                break;

            case INFO:
                infoScreen();
                break;

            case UI::UI_EXIT:
                return;
        }
    }
}

void
infoScreen()
{
    UI::SystemMessage message;
    Timer timer;

    /* Get the MAC address */
    size_t mac = ESP.getEfuseMac();
    /* Convert to HEX */
    char macStr[13];
    sprintf(macStr, "%012X", mac);

    /* Uptime */
    size_t uptime = 0;
    std::string upTimeStr = "";

    /* Free heap */
    std::string freeHeapStr;

    while (!timer.check(SYSTEM_INFO_DISPLAY_TIME_MS) && !Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
        uptime = millis() / 1000;
        upTimeStr = std::to_string(uptime / 3600) + "h " + std::to_string((uptime % 3600) / 60) + "m " + std::to_string(uptime % 60) + "s";
        freeHeapStr = std::to_string(ESP.getFreeHeap() / 1024);

        message.show("Free RAM: " + freeHeapStr + "kB\nMAC: " + macStr + "\nUptime: " + upTimeStr, 0, false);

        if (Buttons::get_handle()->getButtonEvent(BUTTON_PLAY, LONGPRESS)) {
            UI::BinarySelector binarySelector("Tetris", "Snake");
            if (binarySelector.get()) {
                Snake snake;
                snake.run();
            } else {
                Tetris tetris;
                tetris.run();
            }
            break;
        }
    }
    return;
}

void
systemMenu()
{
    using namespace MENUDATA::SYSTEM_m;

    UI::ListSelection systemMenu;
    UI::SystemMessage notify;

    items selection;

    while (true) {
        selection = (items) systemMenu.get(menu, SIZE);

        switch (selection) {
            case AUDIO:
                audioMenu();
                break;

            case USB_TRANSFER:
                usbMenu();
                break;

            case SCREENSAVER:
                screensaverMenu();
                break;

            case REBOOT:
                notify.show("Rebooting...", 2000, false);
                reboot();
                break;

            case RESET:
                notify.show("Resetting...", 2000, false);
                log_i("Resetting preferences...");
                Config_Manager::get_handle()->resetPreferences();
                notify.show("Rebooting...", 0, false);
                reboot();
                break;

            case DATETIME:
                dateTimeMenu();
                break;

            case UI::UI_EXIT:
                return;
                break;
        }
    }
}

void
dateTimeMenu()
{
    using namespace MENUDATA::DATETIME_m;

    UI::ListSelection dateTimeMenu;
    TableData tzData(timezones, timezones_length, timezones_num_columns);
    UI::TextInput textInput;
    UI::SystemMessage notify;
    std::string input = "";
    std::string currentTime = "";
    std::string currentDate = "";
    items menuSelection;
    uint16_t timezoneSelection = 0;

    while (true) {
        menuSelection = (items) dateTimeMenu.get(menu, SIZE);

        switch (menuSelection) {
            case TIME:

                input = textInput.get("Time: (HH:MM:SS)", Config_Manager::get_handle()->getCurrentDateTime("%H:%M:%S"), 8, UI::INPUT_FORMAT_TIME);

                // Set the time
                if (Config_Manager::get_handle()->setTime(input))
                    notify.show("Time set!", 2000, false);
                else
                    notify.show("Invalid time!", 2000, false);
                break;

            case DATE:

                input = textInput.get("Date: (YYYY-MM-DD)", Config_Manager::get_handle()->getCurrentDateTime("%Y-%m-%d"), 10, UI::INPUT_FORMAT_DATE);

                // Set the date
                if (Config_Manager::get_handle()->setDate(input))
                    notify.show("Date set!", 2000, false);
                else
                    notify.show("Invalid date!", 2000, false);
                break;

            case TIMEZONE:
                timezoneSelection = dateTimeMenu.get(&tzData);
                if (timezoneSelection != UI::UI_EXIT) {
                    Config_Manager::get_handle()->setTimezone(tzData.get(timezoneSelection, 1));   // Row = timezoneSelection, Column = 1
                    notify.show("Timezone set!", 2000, false);
                }
                break;

            case ALARM:
                alarmMenu();
                break;

            default:
                return;
        }
    }
}

void
alarmMenu()
{
    using namespace MENUDATA::ALARM_m;

    UI::ListSelection alarmMenu;
    UI::TextInput textInput;
    UI::FileBrowser* fileBrowser = new UI::FileBrowser();
    UI::SystemMessage notify;
    MediaData mediadata;
    std::string input = "";
    items selection;

    while (true) {
        selection = (items) alarmMenu.get(menu, SIZE);

        switch (selection) {
            case ENABLE:
                Config_Manager::get_handle()->enableAlarm();
                notify.show("Alarm enabled!", 2000, false);
                break;

            case DISABLE:
                Config_Manager::get_handle()->disableAlarm();
                notify.show("Alarm disabled!", 2000, false);
                break;

            case SET:

                input = textInput.get("Time: (HH:MM:SS)", Config_Manager::get_handle()->getAlarmTime().c_str(), 8, UI::INPUT_FORMAT_TIME);

                // Set the alarm time
                if (Config_Manager::get_handle()->setAlarmTime(input))
                    notify.show("Alarm time set!", 2000, false);
                else
                    notify.show("Invalid time!", 2000, false);

                break;

            case MEDIA:
                mediadata = fileBrowser->get();
                if (mediadata.loaded) {
                    Config_Manager::get_handle()->saveAlarmMedia(mediadata);
                    notify.show("Alarm media set!", 2000, false);
                }
                break;

            default:
                delete fileBrowser;
                return;
        }
    }
}

void
reboot()
{
    ESP.restart();
}

void
wifiMenu()
{
    using namespace MENUDATA::WIFI_m;

    UI::ListSelection wifiToggleMenu;
    UI::SystemMessage notify;
    Timer timeout;
    items selection;

    while (true) {
        selection = (items) wifiToggleMenu.get(menu, SIZE);

        if (selection == ENABLE) {
            Config_Manager::get_handle()->enableWifi();

            while (WiFi.status() != WL_CONNECTED) {
                notify.show("Connecting", 0, true);
                if (WiFi.status() == WL_CONNECTED) {
                    notify.show("Connected!", 2000, false);
                    log_i("Connected to SSID: %s", Config_Manager::get_handle()->getWifiSSID().c_str());
                    break;
                }

                if (timeout.check(WIFI_CONNECTION_TIMEOUT_MS)) {
                    notify.show("Connection timed out!", 2000, false);
                    log_e("Connection to SSID timed out!");
                    Config_Manager::get_handle()->disableWifi();
                    break;
                }
            }

        }

        else if (selection == DISABLE) {
            Config_Manager::get_handle()->disableWifi();
            notify.show("Disconnected!", 2000, false);
        }

        else {
            return;
        }
    }
}

void
dhcpToggleMenu()
{
    using namespace MENUDATA::DHCP_TOGGLE_m;

    UI::ListSelection dhcpToggleMenu;
    UI::SystemMessage notify;
    items selection;

    selection = (items) dhcpToggleMenu.get(menu, SIZE);

    if (selection == ENABLE) {
        Transport::get_handle()->playUIsound(load_item, load_item_len);
        Config_Manager::get_handle()->enableDHCP();
        notify.show("DHCP enabled!", 2000, false);
        log_i("DHCP enabled!");
    }

    else if (selection == DISABLE) {
        Config_Manager::get_handle()->disableDHCP();
        notify.show("DHCP disabled!", 2000, false);
        log_i("DHCP disabled!");
    }

    else {
        return;
    }
}

void
networkMenu()
{
    using namespace MENUDATA::NETWORK_m;

    UI::ListSelection networkMenu;
    Timer timeout;
    UI::SystemMessage notify;
    UI::TextInput textInput;
    items selection;

    while (true) {
        selection = (items) networkMenu.get(menu, SIZE);

        switch (selection) {
            case TOGGLE:

                wifiMenu();
                break;

            case DHCP:

                dhcpToggleMenu();
                break;

            case SEARCH:

                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                log_i("Starting SSID scanner");
                ssidScanner();
                break;

            case SSID:

                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                log_i("Current SSID: %s", Config_Manager::get_handle()->getWifiSSID().c_str());
                Config_Manager::get_handle()->setWifiSSID(textInput.get("SSID:", Config_Manager::get_handle()->getWifiSSID(), 255, UI::INPUT_FORMAT_TEXT));
                break;

            case PASSWORD:

                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                Config_Manager::get_handle()->setWifiPassword(textInput.get("Password:", Config_Manager::get_handle()->getWifiPassword(), 255, UI::INPUT_FORMAT_PASSWORD));
                break;

            case IP_ADDRESS:

                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                log_i("Current IP: %s", WiFi.localIP().toString().c_str());

                if (Config_Manager::get_handle()->isDHCPEnabled()) {
                    notify.show("DHCP is enabled!\n\nCurrent IP:\n" + Config_Manager::get_handle()->getIP(), 4000, false);
                    break;
                }

                if (!Config_Manager::get_handle()->setIP(textInput.get("IP Address", Config_Manager::get_handle()->getIP(), 15, UI::INPUT_FORMAT_IPADDRESS))) {
                    notify.show("Invalid IP address!", 2000, false);
                    break;
                }

            case NETMASK:

                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                log_i("Current netmask: %s", WiFi.subnetMask().toString().c_str());

                if (Config_Manager::get_handle()->isDHCPEnabled()) {
                    notify.show("DHCP is enabled!\n\nCurrent netmask:\n" + Config_Manager::get_handle()->getNetmask(), 4000, false);
                    break;
                }

                if (!Config_Manager::get_handle()->setNetmask(textInput.get("Netmask:", Config_Manager::get_handle()->getNetmask(), 15, UI::INPUT_FORMAT_IPADDRESS))) {
                    notify.show("Invalid netmask!", 2000, false);
                    break;
                }
                break;

            case GATEWAY:

                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                log_i("Current gateway: %s", WiFi.gatewayIP().toString().c_str());

                if (Config_Manager::get_handle()->isDHCPEnabled()) {
                    notify.show("DHCP is enabled!\n\nCurrent gateway:\n" + Config_Manager::get_handle()->getGateway(), 4000, false);
                    break;
                }

                if (!Config_Manager::get_handle()->setGateway(textInput.get("Gateway:", Config_Manager::get_handle()->getGateway(), 15, UI::INPUT_FORMAT_IPADDRESS))) {
                    notify.show("Invalid gateway!", 2000, false);
                    break;
                }
                break;

            case DNS:

                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                log_i("Current DNS: %s", WiFi.dnsIP().toString().c_str());

                if (Config_Manager::get_handle()->isDHCPEnabled()) {
                    notify.show("DHCP is enabled!\n\nCurrent DNS:\n" + Config_Manager::get_handle()->getDNS(), 4000, false);
                    break;
                }

                if (!Config_Manager::get_handle()->setDNS(textInput.get("DNS:", Config_Manager::get_handle()->getDNS(), 15, UI::INPUT_FORMAT_IPADDRESS))) {
                    notify.show("Invalid DNS!", 2000, false);
                    break;
                }
                break;

            case NTP_CONFIG:
                Transport::get_handle()->playUIsound(folder_open, folder_open_len);
                ntpConfigMenu();
                break;

            case UI::UI_EXIT:
                return;
                break;
        }
    }
}

void
ntpConfigMenu()
{
    using namespace MENUDATA::NTP_m;

    UI::ListSelection ntpConfigMenu;
    UI::ListSelection timezoneMenu;

    UI::SystemMessage notify;
    UI::TextInput textInput;
    std::string text;
    TableData tzData(timezones, timezones_length, timezones_num_columns);
    uint16_t timezoneSelection;
    items selection;

    while (true) {
        selection = (items) ntpConfigMenu.get(menu, SIZE);

        switch (selection) {
            case SERVER:

                text = textInput.get("NTP Server:", Config_Manager::get_handle()->getNTPServer(), 255, UI::INPUT_FORMAT_SERVADDR);
                notify.show("Setting NTP server..." + text, 0, false);
                if (Config_Manager::get_handle()->setNTPServer(text)) {
                    notify.show("NTP server set!", 2000, false);
                } else {
                    notify.show("Invalid server!", 2000, false);
                }
                break;

            case INTERVAL:

                text = textInput.get("Interval (1-1440 min):", std::to_string(Config_Manager::get_handle()->getNTPInterval()), 4, UI::INPUT_FORMAT_NUMERIC);
                if (Config_Manager::get_handle()->setNTPInterval(atoi(text.c_str()))) {
                    notify.show("NTP interval set!", 2000, false);
                } else {
                    notify.show("Invalid interval!\nMust be between\n1-1440 minutes!", 2000, false);
                }
                break;

            case TIMEZONE:

                timezoneSelection = timezoneMenu.get(&tzData);
                if (timezoneSelection != UI::UI_EXIT) {
                    Config_Manager::get_handle()->setTimezone(tzData.get(timezoneSelection, 1)); /* Row = timezoneSelection, Column = 1 */
                    notify.show("Timezone set!", 2000, false);
                }
                break;

            case UPDATE:

                if (WiFi.status() != WL_CONNECTED) {
                    notify.show("WiFi not connected!", 2000, false);
                    break;
                }
                /* Update the system time */
                Config_Manager::get_handle()->updateNTP();
                notify.show("Started update!", 2000, false);
                break;

            case UI::UI_EXIT:
                return;
                break;
        }
    }
}

void
playlistEditor_mainMenu()
{
    using namespace MENUDATA::PLAYLIST_EDITOR_m;

    UI::ListSelection playlistEditor;
    UI::TextInput textInput;
    std::string filename;
    std::string path;
    UI::FileBrowser* fileBrowser_load;
    UI::FileBrowser* fileBrowser_remove;
    UI::SystemMessage notify;
    MediaData mediadata;
    PlaylistEngine* _playlistEngine = nullptr;
    items selection;

    if (playlistEngine) {
        _playlistEngine = new PlaylistEngine(playlistEngine);
    } else {
        return;
    }
    fileBrowser_remove = new UI::FileBrowser();
    fileBrowser_remove->setRoot(PLAYLIST_DIR);
    fileBrowser_load = new UI::FileBrowser();
    fileBrowser_load->setRoot(PLAYLIST_DIR);

    while (true) {
        selection = (items) playlistEditor.get(menu, SIZE);

        switch (selection) {

            case LOAD:
                if (!Card_Manager::get_handle()->isReady()) {
                    _playlistEngine->eject();
                    notify.show("SD card error!", 2000, false);
                    break;
                }
                fileBrowser_load->refresh();
                mediadata = fileBrowser_load->get();
                if (mediadata.loaded) {
                    /* If the playlist is loaded in the main engine, stop the transport */
                    if (playlistEngine->getCurrentTrack() == mediadata) {
                        Transport::get_handle()->stop();
                        playlistEngine->eject();
                    }
                    if (_playlistEngine->load(mediadata)) {
                        notify.show("Playlist loaded!", 1000, false);
                    } else {
                        notify.show("Error!", 1000, false);
                    }
                }
                break;

            case EDIT:

                if (!Card_Manager::get_handle()->isReady()) {
                    _playlistEngine->eject();
                    notify.show("SD card error!", 2000, false);
                    break;
                }

                if (!_playlistEngine->isLoaded()) {
                    notify.show("Not loaded!", 1000, false);
                    break;
                }
                playlistEditor_trackMenu(_playlistEngine);
                break;

            case ADD:
                filename = textInput.get("Filename:", "", 255, UI::INPUT_FORMAT_TEXT);

                /* If the filename is empty, return */
                if (filename.empty()) {
                    break;
                }

                /* Add the .m3u extension if it doesn't exist */
                if (filename.find(".m3u") == std::string::npos) {
                    filename += ".m3u";
                }

                if (!Card_Manager::get_handle()->isReady()) {
                    notify.show("SD card error!", 2000, false);
                    break;
                }

                /* Create the playlist */
                if (Card_Manager::get_handle()->isReady() && !Card_Manager::get_handle()->exists(path.c_str())) {
                    /* Create the file */
                    FsFile dir;
                    FsFile file;
                    dir.open(PLAYLIST_DIR);
                    file.open(&dir, filename.c_str(), O_RDWR | O_TRUNC | O_CREAT);
                    file.close();
                    dir.close();
                    notify.show("Playlist created!", 1000, false);
                } else {
                    notify.show("Playlist exists!", 1000, false);
                }
                break;

            case REMOVE:

                while (true) {

                    if (!Card_Manager::get_handle()->isReady()) {
                        _playlistEngine->eject();
                        notify.show("SD card error!", 2000, false);
                        break;
                    }

                    fileBrowser_remove->refresh();
                    mediadata = fileBrowser_remove->get();
                    if (mediadata.loaded) {
                        if (_playlistEngine->isLoaded() && mediadata == *_playlistEngine->getLoadedMedia()) {
                            _playlistEngine->eject();
                        }
                        if (playlistEngine->isLoaded() && mediadata == *playlistEngine->getLoadedMedia()) {
                            playlistEngine->eject();
                        }
                        if (Card_Manager::get_handle()->remove((std::string(PLAYLIST_DIR) + "/" + mediadata.filename).c_str())) {
                            notify.show("Playlist deleted!", 1000, false);
                        } else {
                            notify.show("Error!", 1000, false);
                        }
                    } else {
                        break;
                    }
                }
                break;

            default:
                delete _playlistEngine;
                delete fileBrowser_remove;
                delete fileBrowser_load;
                return;
                break;
        }
    }
    if (!Card_Manager::get_handle()->isReady()) {
        _playlistEngine->eject();
        notify.show("SD card error!", 2000, false);
    }
    return;
}

void
playlistEditor_trackMenu(PlaylistEngine* _playlistEngine)
{
    using namespace MENUDATA::PLAYLIST_EDITOR_EDIT_m;

    UI::ListSelection playlistEditor;
    UI::FileBrowser* fileBrowser = nullptr;
    UI::SystemMessage notify;
    MediaData mediadata;
    items selection;
    uint16_t trackSelection;

    fileBrowser = new UI::FileBrowser();

    while (Card_Manager::get_handle()->isReady()) {
        selection = (items) playlistEditor.get(menu, SIZE);

        switch (selection) {
            case ADDTRACK:

                if (!Card_Manager::get_handle()->isReady()) {
                    _playlistEngine->eject();
                    notify.show("SD card error!", 2000, false);
                    break;
                }

                mediadata = fileBrowser->get();
                if (mediadata.loaded) {
                    if (_playlistEngine->addTrack(mediadata)) {
                        notify.show("Track added!", 1000, false);
                    } else {
                        notify.show("Error!", 1000, false);
                    }
                }
                break;

            case REMOVETRACK:

                if (!Card_Manager::get_handle()->isReady()) {
                    _playlistEngine->eject();
                    notify.show("SD card error!", 2000, false);
                    break;
                }

                while (Card_Manager::get_handle()->isReady()) {
                    if (_playlistEngine && _playlistEngine->size() > 0) {
                        trackSelection = _playlistEngine->view();
                        if (trackSelection != UI::UI_EXIT) {
                            if (_playlistEngine->removeTrack(trackSelection)) {
                                notify.show("Track removed!", 1000, false);
                            } else {
                                notify.show("Error!", 1000, false);
                            }
                        } else {
                            break;
                        }
                    } else {
                        notify.show("No Tracks!", 1000, false);
                        break;
                    }
                }
                break;

            default:
                delete fileBrowser;
                return;
                break;
        }
    }
    if (!Card_Manager::get_handle()->isReady()) {
        _playlistEngine->eject();
        notify.show("SD card error!", 2000, false);
    }
    return;
}

void
audioMenu()
{
    using namespace MENUDATA::AUDIO_m;

    UI::ListSelection audioMenu;
    UI::ValueSelector selector_bass = UI::ValueSelector("Bass",
                                                std::bind(&Transport::EqualizerController::getBass, Transport::get_handle()->eq),
                                                std::bind(&Transport::EqualizerController::bassUp, Transport::get_handle()->eq),
                                                std::bind(&Transport::EqualizerController::bassDown, Transport::get_handle()->eq),
                                                Transport::get_handle()->eq->getMinBass(),
                                                Transport::get_handle()->eq->getMaxBass());
    UI::ValueSelector selector_mid = UI::ValueSelector("Mid",
                                               std::bind(&Transport::EqualizerController::getMid, Transport::get_handle()->eq),
                                               std::bind(&Transport::EqualizerController::midUp, Transport::get_handle()->eq),
                                               std::bind(&Transport::EqualizerController::midDown, Transport::get_handle()->eq),
                                               Transport::get_handle()->eq->getMinMid(),
                                               Transport::get_handle()->eq->getMaxMid());
    UI::ValueSelector selector_treble = UI::ValueSelector("Treble",
                                                  std::bind(&Transport::EqualizerController::getTreble, Transport::get_handle()->eq),
                                                  std::bind(&Transport::EqualizerController::trebleUp, Transport::get_handle()->eq),
                                                  std::bind(&Transport::EqualizerController::trebleDown, Transport::get_handle()->eq),
                                                  Transport::get_handle()->eq->getMinTreble(),
                                                  Transport::get_handle()->eq->getMaxTreble());
    UI::ValueSelector selector_sysvol = UI::ValueSelector("UI Volume",
                                                  std::bind(&Transport::getSystemVolume, Transport::get_handle()),
                                                  std::bind(&Transport::systemVolumeUp, Transport::get_handle()),
                                                  std::bind(&Transport::systemVolumeDown, Transport::get_handle()),
                                                  Transport::get_handle()->getMinSystemVolume(),
                                                  Transport::get_handle()->getMaxSystemVolume());

    UI::SystemMessage notify;
    items selection;

    while (true) {
        selection = (items) audioMenu.get(menu, SIZE);

        switch (selection) {
            case BASS:
                selector_bass.get();
                break;

            case MID:
                selector_mid.get();
                break;

            case TREBLE:
                selector_treble.get();
                break;

            case SYSVOL:
                selector_sysvol.get();
                break;

            default:
                return;
                break;
        }
    }
    return;
}

void
ssidScanner()
{
    if (playlistEngine->isEnabled() && Transport::get_handle()->getStatus() == TRANSPORT_PLAYING && Transport::get_handle()->getLoadedMedia().source == REMOTE_FILE) {
        Transport::get_handle()->stop();
        playlistEngine->stop();
    }

    UI::SystemMessage notify;
    Timer SSIDscanTimeout;
    bool reEnableWiFi = false;

    if (Config_Manager::get_handle()->isWifiEnabled()) {
        Config_Manager::get_handle()->disableWifi();
        reEnableWiFi = true;
    }

    WiFi.scanDelete();
    WiFi.scanNetworks(true, false);
    notify.show("Scanning", 0, true);
    log_i("Scanning for networks...");
    while (WiFi.scanComplete() == -1 || WiFi.scanComplete() == -2 && !SSIDscanTimeout.check(WIFI_CONNECTION_TIMEOUT_MS)) {
        notify.show("Scanning", 0, true);
    }
    int16_t numNetworks = WiFi.scanComplete();
    log_i("Scan complete! Found %d networks", numNetworks);

    if (numNetworks <= 0) {
        notify.show("No networks found!", 2000, false);
        log_e("No networks found during SSID scan!");
        return;
    }

    else {
        /* Load up the SSIDs into a vector */
        std::vector<std::string> networkList;
        for (uint8_t i = 0; i < numNetworks && i < WIFI_MAX_DISPLAYED_NETWORKS; i++) {
            log_i("SSID: %s", WiFi.SSID(i).c_str());
            networkList.push_back(WiFi.SSID(i).c_str());
        }
        /* Sort and remove duplicates */
        std::sort(networkList.begin(), networkList.end());
        networkList.erase(std::unique(networkList.begin(), networkList.end()), networkList.end());
        /* Remove empty SSIDs */
        networkList.erase(std::remove(networkList.begin(), networkList.end(), ""), networkList.end());
        /* If the vector is left empty, return */
        if (networkList.size() == 0) {
            notify.show("No networks found!", 2000, false);
            log_e("No networks found after running filters!");
            if (reEnableWiFi)
                Config_Manager::get_handle()->enableWifi();
            return;
        }
        UI::ListSelection menu;
        uint16_t selection = menu.get(networkList);

        if (selection != UI::UI_EXIT) {
            Config_Manager::get_handle()->setWifiSSID(networkList[selection]);
            notify.show("SSID selected:\n\n" + networkList[selection], 2000, false);
            log_i("Selected SSID: %s", networkList[selection].c_str());
        }

        WiFi.scanDelete();
        if (reEnableWiFi)
            Config_Manager::get_handle()->enableWifi();
    }
}

void
usbMenu()
{
    UI::SystemMessage notify;
    if (!Card_Manager::get_handle()->isReady()) {
        notify.show("SD card error!", 2000, false);
        return;
    }
    Transport::get_handle()->eject();
    playlistEngine->eject();

    USBMSC usb_msc;
    usb_msc.vendorID("BMA");             // max 8 chars
    usb_msc.productID("Media Player");   // max 16 chars
    usb_msc.productRevision("1.0");      // max 4 chars
    usb_msc.onStartStop(onStartStop);
    usb_msc.onRead(onRead);
    usb_msc.onWrite(onWrite);
    usb_msc.mediaPresent(true);
    usb_msc.begin(Card_Manager::get_handle()->card()->sectorCount(), 512);
    USB.onEvent(usbEventCallback);
    USB.manufacturerName("BMA");
    USB.productName("Media Player");
    USB.serialNumber("1.0");
    USB.begin();

    while (Card_Manager::get_handle()->isReady()) {
        notify.show("USB file transfer\nenabled. Press\nEXIT to end", 0, true);
        if (Buttons::get_handle()->getButtonEvent(BUTTON_EXIT, SHORTPRESS)) {
            notify.show("Unmounting SD card\n and restarting...", 2000, false);
            usb_msc.end();
            ESP.restart();
            return;
        }
    }

    /* If we made it to this point, something has gone wrong with the SD card */
    notify.show("SD card error!\nRestarting...", 2000, false);
    usb_msc.end();
    ESP.restart();
    return;
}

void
bluetoothMenu()
{
    using namespace MENUDATA::BLUETOOTH_m;

    UI::SystemMessage notify;
    UI::ListSelection bluetoothMenu;
    items selection;

    while (true) {
        selection = (items) bluetoothMenu.get(menu, SIZE);

        switch (selection) {
            case ENABLE:
                if (Bluetooth::get_handle()->getMode() == POWER_ON) {
                    notify.show("Bluetooth already\nenabled!", 2000, false);
                    break;
                }
                Bluetooth::get_handle()->powerOn();
                notify.show("Bluetooth enabled!", 2000, false);
                break;

            case DISABLE:
                if (Bluetooth::get_handle()->getMode() == POWER_OFF) {
                    notify.show("Bluetooth already\ndisabled!", 2000, false);
                    break;
                }
                Bluetooth::get_handle()->powerOff();
                notify.show("Bluetooth disabled!", 2000, false);
                break;

            default:
                return;
                break;
        }
    }
}

void
screensaverMenu()
{
    using namespace MENUDATA::SCREENSAVER_m;

    UI::ListSelection screensaverMenu;
    UI::SystemMessage notify;
    items selection;
    UI::TextInput text;
    uint16_t timeout = 0;

    while (true) {
        selection = (items) screensaverMenu.get(menu, SIZE);

        switch (selection) {
            case ENABLE:
                if (Config_Manager::get_handle()->isScreenSaverEnabled()) {
                    notify.show("Screensaver already\nenabled!", 2000, false);
                    break;
                }
                Config_Manager::get_handle()->enableScreenSaver();
                notify.show("Screensaver enabled!", 2000, false);
                break;

            case DISABLE:
                if (!Config_Manager::get_handle()->isScreenSaverEnabled()) {
                    notify.show("Screensaver already\ndisabled!", 2000, false);
                    break;
                }
                Config_Manager::get_handle()->disableScreenSaver();
                notify.show("Screensaver disabled!", 2000, false);
                break;

            case TIMEOUT:
                timeout = atoi(text.get("Timeout (1s-3600s):", std::to_string(Config_Manager::get_handle()->getScreenSaverTimeout()), 4, UI::INPUT_FORMAT_NUMERIC).c_str());
                if (timeout < 1 || timeout > 3600) {
                    notify.show("Invalid timeout!\nValid values are:\n1-3600", 2000, false);
                    break;
                }
                Config_Manager::get_handle()->setScreenSaverTimeout(timeout);
                notify.show("Timeout set to\n" + std::to_string(timeout) + "seconds.", 2000, false);
                break;

            default:
                return;
                break;
        }
    }
}