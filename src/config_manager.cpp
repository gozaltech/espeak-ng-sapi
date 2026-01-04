#include "config_manager.hpp"
#include "debug_log.h"
#include "utils.hpp"
#include <nlohmann/json.hpp>
#include <windows.h>
#include <fstream>
#include <sstream>

using json = nlohmann::ordered_json;

namespace Espeak {
namespace config {

namespace {

void parseVoicesSection(const json& j, Configuration& config) {
    if (j.contains("voices")) {
        auto& voices = j["voices"];
        config.default_only = voices.value("default_only", true);

        if (voices.contains("enabled") && voices["enabled"].is_array()) {
            for (const auto& voice : voices["enabled"]) {
                if (voice.is_string()) {
                    config.enabled_voices.push_back(voice.get<std::string>());
                }
            }
        }
    }
}

void parseGlobalSettingsSection(const json& j, Configuration& config) {
    if (j.contains("global_settings")) {
        auto& settings = j["global_settings"];
        config.global_variant = settings.value("variant", "");
        config.intonation = settings.value("intonation", 50);
        config.wordgap = settings.value("wordgap", 0);
        config.rateboost = settings.value("rateboost", false);
    }
}

void parseVoiceProfilesSection(const json& j, Configuration& config) {
    if (j.contains("voice_profiles") && j["voice_profiles"].is_array()) {
        for (const auto& profile : j["voice_profiles"]) {
            VoiceProfile vp;
            vp.id = profile.value("id", "");
            vp.name = profile.value("name", "");
            vp.base_voice = profile.value("base_voice", "");
            vp.variant = profile.value("variant", "");
            vp.enabled = profile.value("enabled", true);

            if (!vp.id.empty() && !vp.base_voice.empty()) {
                config.voice_profiles.push_back(vp);
            }
        }
    }
}

void clampConfigValues(Configuration& config) {
    if (config.intonation < limits::INTONATION_MIN) config.intonation = limits::INTONATION_MIN;
    if (config.intonation > limits::INTONATION_MAX) config.intonation = limits::INTONATION_MAX;

    if (config.wordgap < limits::WORDGAP_MIN) config.wordgap = limits::WORDGAP_MIN;
    if (config.wordgap > limits::WORDGAP_MAX) config.wordgap = limits::WORDGAP_MAX;
}

void parseConfiguration(const json& j, Configuration& config) {
    parseVoicesSection(j, config);
    parseGlobalSettingsSection(j, config);
    parseVoiceProfilesSection(j, config);
    clampConfigValues(config);
}
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
    : configChangedEvent_(CreateEventW(nullptr, FALSE, FALSE, L"Global\\EspeakSAPIConfigChangedEvent"))
{
    config_ = createDefaultConfig();

    if (!configChangedEvent_) {
        DEBUG_LOG("ConfigManager: Failed to create config change event, error=%d", GetLastError());
    } else {
        DEBUG_LOG("ConfigManager: Config change event created/opened successfully");
    }
}

std::wstring ConfigManager::getConfigPath() {
    utils::fs::path config_dir = utils::getEspeakConfigDir();
    if (config_dir.empty()) {
        DEBUG_LOG("ConfigManager: Failed to get AppData path");
        return L"";
    }

    utils::fs::path config_file = config_dir / "config.json";

    std::error_code ec;
    utils::fs::create_directories(config_dir, ec);
    if (ec) {
        DEBUG_LOG("ConfigManager: Failed to create directory: %s", ec.message().c_str());
    }

    return config_file.wstring();
}

Configuration ConfigManager::createDefaultConfig() {
    Configuration config;
    config.version = "1.0";
    config.default_only = true;
    config.enabled_voices.clear();
    config.global_variant = "";
    config.intonation = 50;
    config.voice_profiles.clear();
    return config;
}

bool ConfigManager::load() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::wstring config_path = getConfigPath();
    if (config_path.empty()) {
        DEBUG_LOG("ConfigManager: Invalid config path");
        config_ = createDefaultConfig();
        return false;
    }

    if (!utils::fs::exists(config_path)) {
        DEBUG_LOG("ConfigManager: Config file doesn't exist, using defaults");
        config_ = createDefaultConfig();
        return false;
    }

    std::ifstream file(config_path);
    if (!file.is_open()) {
        DEBUG_LOG("ConfigManager: Failed to open config file");
        config_ = createDefaultConfig();
        return false;
    }

    try {
        json j;
        file >> j;

        Configuration new_config;
        new_config.version = j.value("version", "1.0");

        parseConfiguration(j, new_config);

        config_ = new_config;

        DEBUG_LOG("ConfigManager: Successfully loaded config (default_only=%d, enabled_voices=%zu, profiles=%zu)",
                  config_.default_only, config_.enabled_voices.size(), config_.voice_profiles.size());
        return true;
    }
    catch (const json::exception& e) {
        DEBUG_LOG("ConfigManager: JSON parse error: %s", e.what());
        config_ = createDefaultConfig();
        return false;
    }
    catch (const std::exception& e) {
        DEBUG_LOG("ConfigManager: Error loading config: %s", e.what());
        config_ = createDefaultConfig();
        return false;
    }
}

bool ConfigManager::save(const Configuration& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::wstring config_path = getConfigPath();
    if (config_path.empty()) {
        DEBUG_LOG("ConfigManager: Invalid config path");
        return false;
    }

    try {
        json j;
        j["version"] = config.version;

        j["voices"]["default_only"] = config.default_only;
        j["voices"]["enabled"] = config.enabled_voices;

        j["global_settings"]["variant"] = config.global_variant;
        j["global_settings"]["intonation"] = config.intonation;
        j["global_settings"]["wordgap"] = config.wordgap;
        j["global_settings"]["rateboost"] = config.rateboost;

        json profiles = json::array();
        for (const auto& profile : config.voice_profiles) {
            json p;
            p["id"] = profile.id;
            p["name"] = profile.name;
            p["base_voice"] = profile.base_voice;
            p["variant"] = profile.variant;
            p["enabled"] = profile.enabled;
            profiles.push_back(p);
        }
        j["voice_profiles"] = profiles;

        std::ofstream file(config_path);
        if (!file.is_open()) {
            DEBUG_LOG("ConfigManager: Failed to open config file for writing");
            return false;
        }

        file << j.dump(2);
        file.close();

        config_ = config;

        signalConfigChanged();

        DEBUG_LOG("ConfigManager: Successfully saved config");
        return true;
    }
    catch (const json::exception& e) {
        DEBUG_LOG("ConfigManager: JSON serialization error: %s", e.what());
        return false;
    }
    catch (const std::exception& e) {
        DEBUG_LOG("ConfigManager: Error saving config: %s", e.what());
        return false;
    }
}

Configuration ConfigManager::getConfig() {
    std::lock_guard<std::mutex> lock(mutex_);
    checkAndReload();
    return config_;
}

void ConfigManager::checkAndReload() {
    if (configChangedEvent_) {
        DWORD result = WaitForSingleObject(configChangedEvent_.get(), 0);

        if (result == WAIT_OBJECT_0) {
            DEBUG_LOG("ConfigManager: Config change event signaled, reloading...");

            std::wstring config_path = getConfigPath();
            if (config_path.empty()) {
                return;
            }

            if (!utils::fs::exists(config_path)) {
                DEBUG_LOG("ConfigManager: Config file not found during reload");
                return;
            }

            std::ifstream file(config_path);
            if (!file.is_open()) {
                DEBUG_LOG("ConfigManager: Failed to open config file during reload");
                return;
            }

            try {
                json j;
                file >> j;

                Configuration new_config;
                new_config.version = j.value("version", "1.0");

                parseConfiguration(j, new_config);

                config_ = new_config;
                DEBUG_LOG("ConfigManager: Config reloaded successfully (intonation=%d, wordgap=%d, rateboost=%d)",
                          config_.intonation, config_.wordgap, config_.rateboost);
            }
            catch (const json::exception& e) {
                DEBUG_LOG("ConfigManager: JSON error during reload: %s", e.what());
            }
            catch (...) {
                DEBUG_LOG("ConfigManager: Unknown error during reload");
            }
        }
    }
}

void ConfigManager::signalConfigChanged() {
    if (configChangedEvent_) {
        if (SetEvent(configChangedEvent_.get())) {
            DEBUG_LOG("ConfigManager: Config change event signaled to all processes");
        } else {
            DEBUG_LOG("ConfigManager: Failed to signal config change event, error=%d", GetLastError());
        }
    }
}
}
}
