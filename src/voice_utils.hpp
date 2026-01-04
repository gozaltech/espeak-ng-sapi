#pragma once

#include <string>
#include <string_view>
#include <filesystem>

namespace Espeak {
namespace sapi {

namespace fs = std::filesystem;

inline std::string extractBaseVoiceName(std::string_view voice_identifier,
                                        std::string_view voice_name)
{
    std::string voice_id = voice_identifier.empty() ? std::string(voice_name) : std::string(voice_identifier);

    fs::path path_obj(voice_id);
    if (path_obj.has_filename()) {
        return path_obj.filename().string();
    }

    return voice_id;
}

inline std::string cleanLanguageString(std::string_view languages)
{
    if (languages.empty()) {
        return {};
    }

    if (static_cast<unsigned char>(languages[0]) < 0x20) {
        return std::string(languages.substr(1));
    }

    return std::string(languages);
}

inline bool isGenderFemale(int gender_code)
{
    return gender_code == 2;
}
}
}
