#include "espeak_wrapper.h"
#include "debug_log.h"
#include "utils.hpp"
#include <espeak-ng/speak_lib.h>
#include <windows.h>
#include <cstring>
#include <algorithm>

namespace Espeak {

namespace {

constexpr int BASE_RATE = 175;
constexpr int SLOW_RATE_SCALE_FACTOR = 95;
constexpr int FAST_RATE_SCALE_FACTOR = 275;
constexpr int MIN_RATE = 80;
constexpr int MAX_RATE = 450;
constexpr int RATE_BOOST_MULTIPLIER = 3;
constexpr int MAX_BOOSTED_RATE = 1350;
constexpr int RATE_SCALE_DIVISOR = 100;

constexpr int MIN_PITCH = 0;
constexpr int MAX_PITCH = 99;
constexpr int VOLUME_MULTIPLIER = 2;
constexpr int MIN_VOLUME = 0;
constexpr int MAX_VOLUME = 200;
constexpr int MIN_INTONATION = 0;
constexpr int MAX_INTONATION = 100;
constexpr int MIN_WORDGAP = 0;
constexpr int MAX_WORDGAP = 100;

struct CallbackContext {
    SpeakCallback callback;
    void* user_data;
    bool aborted;
};

thread_local CallbackContext* g_callback_context = nullptr;

int espeak_callback(short* wav, int numsamples, espeak_EVENT* events) {
    if (!g_callback_context || g_callback_context->aborted) {
        return 1;
    }

    if (numsamples > 0 && wav) {
        if (!g_callback_context->callback(wav, numsamples, g_callback_context->user_data)) {
            g_callback_context->aborted = true;
            return 1;
        }
    }

    if (events) {
        for (espeak_EVENT* event = events; event->type != espeakEVENT_LIST_TERMINATED; ++event) {
            if (event->type == espeakEVENT_MSG_TERMINATED) {
                break;
            }
        }
    }

    return 0;
}
}

EspeakEngine& EspeakEngine::getInstance() {
    static EspeakEngine instance;
    return instance;
}

EspeakEngine::EspeakEngine()
    : initialized_(false)
{
}

EspeakEngine::~EspeakEngine() {
    if (initialized_) {
        espeak_Terminate();
    }
}

bool EspeakEngine::initialize() {
    if (initialized_) {
        return true;
    }

    utils::fs::path data_path = utils::getEspeakDataDir();
    if (!data_path.empty()) {
        std::string data_path_utf8 = data_path.u8string();

        int sample_rate = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, data_path_utf8.c_str(), 0);
        if (sample_rate != -1) {
            DEBUG_LOG("EspeakEngine: Initialized with sample rate %d Hz using data path: %S", sample_rate, data_path.c_str());
            espeak_SetSynthCallback(espeak_callback);
            initialized_ = true;

            if (espeak_SetVoiceByName("en") == EE_OK) {
                current_voice_ = "en";
                DEBUG_LOG("EspeakEngine: Set default voice to 'en'");
            } else {
                DEBUG_LOG("EspeakEngine: Warning - Failed to set default voice");
            }

            return true;
        }
        DEBUG_LOG("EspeakEngine: Failed to initialize with ProgramData path, trying default");
    }

    int sample_rate = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, nullptr, 0);
    if (sample_rate == -1) {
        DEBUG_LOG("EspeakEngine: Failed to initialize espeak-ng");
        return false;
    }

    DEBUG_LOG("EspeakEngine: Initialized with sample rate %d Hz (default path)", sample_rate);

    espeak_SetSynthCallback(espeak_callback);

    initialized_ = true;

    if (espeak_SetVoiceByName("en") == EE_OK) {
        current_voice_ = "en";
        DEBUG_LOG("EspeakEngine: Set default voice to 'en'");
    } else {
        DEBUG_LOG("EspeakEngine: Warning - Failed to set default voice");
    }

    return true;
}

std::vector<VoiceInfo> EspeakEngine::getVoices() const {
    std::vector<VoiceInfo> voices;

    if (!initialized_) {
        return voices;
    }

    const espeak_VOICE** voice_list = espeak_ListVoices(nullptr);
    if (!voice_list) {
        return voices;
    }

    for (int i = 0; voice_list[i] != nullptr; ++i) {
        const espeak_VOICE* voice = voice_list[i];

        VoiceInfo info;
        info.name = voice->name ? voice->name : "";
        info.identifier = voice->identifier ? voice->identifier : "";
        info.languages = voice->languages ? voice->languages : "";
        info.gender = voice->gender;
        info.age = voice->age;

        voices.push_back(info);
    }

    DEBUG_LOG("EspeakEngine: Found %zu voices", voices.size());
    return voices;
}

bool EspeakEngine::setVoice(const std::string& voice_name) {
    if (!initialized_) {
        return false;
    }

    espeak_ERROR result = espeak_SetVoiceByName(voice_name.c_str());
    if (result != EE_OK) {
        DEBUG_LOG("EspeakEngine: Failed to set voice '%s', error %d", voice_name.c_str(), result);
        return false;
    }

    current_voice_ = voice_name;
    DEBUG_LOG("EspeakEngine: Set voice to '%s'", voice_name.c_str());
    return true;
}

bool EspeakEngine::speak(const std::string& text,
                         int rate,
                         int pitch,
                         int volume,
                         int intonation,
                         int wordgap,
                         bool rateboost,
                         SpeakCallback callback,
                         void* user_data) {
    if (!initialized_) {
        DEBUG_LOG("EspeakEngine: Not initialized");
        return false;
    }

    if (text.empty()) {
        DEBUG_LOG("EspeakEngine: Empty text");
        return true;
    }

    int espeak_rate;
    if (rate < 0) {
        espeak_rate = BASE_RATE + (rate * SLOW_RATE_SCALE_FACTOR / RATE_SCALE_DIVISOR);
    } else {
        espeak_rate = BASE_RATE + (rate * FAST_RATE_SCALE_FACTOR / RATE_SCALE_DIVISOR);
    }
    espeak_rate = std::clamp(espeak_rate, MIN_RATE, MAX_RATE);

    if (rateboost) {
        espeak_rate = (std::min)(espeak_rate * RATE_BOOST_MULTIPLIER, MAX_BOOSTED_RATE);
    }

    espeak_SetParameter(espeakRATE, espeak_rate, 0);

    int espeak_pitch = std::clamp(pitch, MIN_PITCH, MAX_PITCH);
    espeak_SetParameter(espeakPITCH, espeak_pitch, 0);

    int espeak_volume = std::clamp(volume * VOLUME_MULTIPLIER, MIN_VOLUME, MAX_VOLUME);
    espeak_SetParameter(espeakVOLUME, espeak_volume, 0);

    int espeak_intonation = std::clamp(intonation, MIN_INTONATION, MAX_INTONATION);
    espeak_SetParameter(espeakRANGE, espeak_intonation, 0);

    int espeak_wordgap = std::clamp(wordgap, MIN_WORDGAP, MAX_WORDGAP);
    espeak_SetParameter(espeakWORDGAP, espeak_wordgap, 0);

    DEBUG_LOG("EspeakEngine: Speaking text (rate=%d->%dwpm%s, pitch=%d, volume=%d->%d, intonation=%d, wordgap=%d)",
              rate, espeak_rate, rateboost ? " (boosted x3)" : "", espeak_pitch, volume, espeak_volume, espeak_intonation, espeak_wordgap);

    CallbackContext ctx;
    ctx.callback = callback;
    ctx.user_data = user_data;
    ctx.aborted = false;
    g_callback_context = &ctx;

    espeak_ERROR result = espeak_Synth(text.c_str(), text.length() + 1,
                                       0, POS_CHARACTER, 0,
                                       espeakCHARS_UTF8, nullptr, nullptr);

    g_callback_context = nullptr;

    if (result != EE_OK) {
        DEBUG_LOG("EspeakEngine: Synthesis failed with error %d", result);
        return false;
    }

    if (ctx.aborted) {
        DEBUG_LOG("EspeakEngine: Synthesis aborted by callback");
        return false;
    }

    DEBUG_LOG("EspeakEngine: Synthesis completed successfully");
    return true;
}

void EspeakEngine::stop() noexcept {
    if (initialized_) {
        espeak_Cancel();
    }
}
}
