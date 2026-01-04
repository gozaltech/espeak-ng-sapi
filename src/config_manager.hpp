#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <windows.h>
#include "win32_utils.hpp"

namespace Espeak {
namespace config {

namespace limits {
    constexpr int INTONATION_MIN = 0;
    constexpr int INTONATION_MAX = 100;
    constexpr int WORDGAP_MIN = 0;
    constexpr int WORDGAP_MAX = 100;
}

struct VoiceProfile {
    std::string id;
    std::string name;
    std::string base_voice;
    std::string variant;
    bool enabled;

    VoiceProfile() : enabled(true) {}

    VoiceProfile(std::string id_, std::string name_, std::string base_voice_,
                 std::string variant_, bool enabled_ = true)
        : id(std::move(id_))
        , name(std::move(name_))
        , base_voice(std::move(base_voice_))
        , variant(std::move(variant_))
        , enabled(enabled_)
    {}
};

struct Configuration {
    std::string version;
    std::vector<std::string> enabled_voices;
    bool default_only;
    std::string global_variant;
    int intonation;
    int wordgap;
    bool rateboost;
    std::vector<VoiceProfile> voice_profiles;

    Configuration()
        : version("1.0")
        , default_only(true)
        , intonation(50)
        , wordgap(0)
        , rateboost(false)
    {}
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    [[nodiscard]] bool load();

    [[nodiscard]] bool save(const Configuration& config);

    [[nodiscard]] Configuration getConfig();

    [[nodiscard]] static std::wstring getConfigPath();

    [[nodiscard]] static Configuration createDefaultConfig();

private:
    ConfigManager();

    void checkAndReload();

    void signalConfigChanged();

    Configuration config_;
    mutable std::mutex mutex_;
    utils::unique_handle configChangedEvent_;
};
}
}
