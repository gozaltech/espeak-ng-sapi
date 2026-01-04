#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Espeak {

struct VoiceInfo {
    std::string name;
    std::string identifier;
    std::string languages;
    int gender;
    int age;
};

using SpeakCallback = std::function<bool(const short* audio, int sample_count, void* user_data)>;

class EspeakEngine {
public:
    static EspeakEngine& getInstance();

    EspeakEngine(const EspeakEngine&) = delete;
    EspeakEngine& operator=(const EspeakEngine&) = delete;

    [[nodiscard]] bool initialize();

    [[nodiscard]] std::vector<VoiceInfo> getVoices() const;

    [[nodiscard]] bool setVoice(const std::string& voice_name);

    [[nodiscard]] bool speak(const std::string& text,
                             int rate,
                             int pitch,
                             int volume,
                             int intonation,
                             int wordgap,
                             bool rateboost,
                             SpeakCallback callback,
                             void* user_data);

    void stop() noexcept;

private:
    EspeakEngine();
    ~EspeakEngine();

    bool initialized_;
    std::string current_voice_;
};
}
