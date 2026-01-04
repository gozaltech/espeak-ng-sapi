#include <new>
#include <string>
#include <cmath>
#include <algorithm>
#include "utils.hpp"
#include "ISpTTSEngineImpl.hpp"
#include "config_manager.hpp"
#include "error_handler.hpp"
#include "debug_log.h"

namespace Espeak {
namespace sapi {

namespace {

constexpr WORD AUDIO_CHANNELS = 1;
constexpr DWORD AUDIO_SAMPLE_RATE = 22050;
constexpr WORD AUDIO_BITS_PER_SAMPLE = 16;

constexpr int MIN_RATE = -10;
constexpr int MAX_RATE = 10;
constexpr int RATE_TO_WPM_MULTIPLIER = 10;

constexpr int BASE_PITCH = 50;
constexpr int PITCH_ADJ_MULTIPLIER = 2;
constexpr int MIN_PITCH_ADJ = -50;
constexpr int MAX_PITCH_ADJ = 50;

constexpr int MIN_VOLUME = 0;
constexpr int MAX_VOLUME = 100;

struct SpeakContext {
    ISpTTSEngineSite* caller = nullptr;
    ULONGLONG bytes_written = 0;
    bool aborted = false;
};

inline bool checkAndHandleActionFlags(ISpTTSEngineSite* site, bool* aborted = nullptr) {
    const DWORD actions = site->GetActions();

    if (actions & SPVES_ABORT) {
        DEBUG_LOG("SAPI: ABORT requested");
        if (aborted) *aborted = true;
        return false;
    }

    if (actions & SPVES_SKIP) {
        DEBUG_LOG("SAPI: SKIP requested");
        site->CompleteSkip(0);
        if (aborted) *aborted = true;
        return false;
    }

    return true;
}

bool speak_callback(const short* audio, int sample_count, void* user) {
    auto* ctx = static_cast<SpeakContext*>(user);
    if (!ctx || !ctx->caller) {
        DEBUG_LOG("SAPI Callback: ERROR - No context or caller");
        return false;
    }

    if (!checkAndHandleActionFlags(ctx->caller, &ctx->aborted)) {
        return false;
    }

    DEBUG_LOG("SAPI Callback: Writing %d samples (%d bytes) to SAPI",
              sample_count, sample_count * 2);

    auto ptr = reinterpret_cast<const BYTE*>(audio);
    ULONG remaining = static_cast<ULONG>(sample_count * sizeof(short));

    while (remaining > 0) {
        if (!checkAndHandleActionFlags(ctx->caller, &ctx->aborted)) {
            return false;
        }

        ULONG written = remaining;
        HRESULT hr = ctx->caller->Write(ptr, remaining, &written);
        if (FAILED(hr)) {
            DEBUG_LOG("SAPI Callback: Write FAILED with HRESULT 0x%08X", hr);
            return false;
        }
        if (written > remaining) {
            DEBUG_LOG("SAPI Callback: Write error - written (%lu) > remaining (%lu)",
                      written, remaining);
            return false;
        }
        ctx->bytes_written += written;
        remaining -= written;
        ptr += written;
    }

    DEBUG_LOG("SAPI Callback: Successfully wrote %d samples", sample_count);
    return true;
}
}

ISpTTSEngineImpl::ISpTTSEngineImpl()
{
    [[maybe_unused]] bool initialized = EspeakEngine::getInstance().initialize();
}

STDMETHODIMP ISpTTSEngineImpl::SetObjectToken(ISpObjectToken* pToken)
{
    DEBUG_LOG("=== SetObjectToken Called ===");

    return com::safe_com_call([&]() -> HRESULT {
        if (!pToken) {
            DEBUG_LOG("SetObjectToken: ERROR - pToken is NULL");
            return E_INVALIDARG;
        }

        ISpDataKeyPtr attr;
        if (FAILED(pToken->OpenKey(L"Attributes", &attr))) {
            DEBUG_LOG("SetObjectToken: ERROR - Failed to open Attributes key");
            return E_INVALIDARG;
        }

        utils::out_ptr<wchar_t> name(CoTaskMemFree);
        if (FAILED(attr->GetStringValue(L"Name", name.address()))) {
            DEBUG_LOG("SetObjectToken: ERROR - Failed to get Name attribute");
            return E_INVALIDARG;
        }

        utils::out_ptr<wchar_t> voice_id(CoTaskMemFree);
        std::string espeak_voice_id;
        if (SUCCEEDED(attr->GetStringValue(L"VoiceId", voice_id.address()))) {
            espeak_voice_id = utils::wstring_to_string(voice_id.get());
        } else {
            espeak_voice_id = utils::wstring_to_string(name.get());
        }

        voice_name_ = espeak_voice_id;
        DEBUG_LOG("SetObjectToken: Display name = %S, Voice ID = %s",
                  name.get(), espeak_voice_id.c_str());

        if (!EspeakEngine::getInstance().setVoice(espeak_voice_id)) {
            DEBUG_LOG("SetObjectToken: WARNING - Failed to set voice, will use default");
        }

        token_ = pToken;
        DEBUG_LOG("SetObjectToken: SUCCESS");
        return S_OK;
    }, "ISpTTSEngine::SetObjectToken");
}

STDMETHODIMP ISpTTSEngineImpl::GetObjectToken(ISpObjectToken** ppToken)
{
    DEBUG_LOG("=== GetObjectToken Called ===");

    if (!ppToken) {
        DEBUG_LOG("GetObjectToken: ERROR - ppToken is NULL");
        return E_POINTER;
    }
    *ppToken = nullptr;

    if (token_) {
        token_.AddRef();
        *ppToken = token_.GetInterfacePtr();
        DEBUG_LOG("GetObjectToken: SUCCESS - Returned token");
        return S_OK;
    }
    DEBUG_LOG("GetObjectToken: ERROR - No token set");
    return E_UNEXPECTED;
}

STDMETHODIMP ISpTTSEngineImpl::GetOutputFormat(
    const GUID* /*pTargetFmtId*/,
    const WAVEFORMATEX* /*pTargetWaveFormatEx*/,
    GUID* pOutputFormatId,
    WAVEFORMATEX** ppCoMemOutputWaveFormatEx)
{
    DEBUG_LOG("=== GetOutputFormat Called ===");

    if (!pOutputFormatId) {
        DEBUG_LOG("GetOutputFormat: ERROR - pOutputFormatId is NULL");
        return E_POINTER;
    }
    if (!ppCoMemOutputWaveFormatEx) {
        DEBUG_LOG("GetOutputFormat: ERROR - ppCoMemOutputWaveFormatEx is NULL");
        return E_POINTER;
    }

    *pOutputFormatId = SPDFID_WaveFormatEx;
    *ppCoMemOutputWaveFormatEx = nullptr;

    auto* pwfex = static_cast<WAVEFORMATEX*>(CoTaskMemAlloc(sizeof(WAVEFORMATEX)));
    if (!pwfex) {
        DEBUG_LOG("GetOutputFormat: ERROR - Out of memory");
        return E_OUTOFMEMORY;
    }

    pwfex->wFormatTag = WAVE_FORMAT_PCM;
    pwfex->nChannels = AUDIO_CHANNELS;
    pwfex->nSamplesPerSec = AUDIO_SAMPLE_RATE;
    pwfex->wBitsPerSample = AUDIO_BITS_PER_SAMPLE;
    pwfex->nBlockAlign = pwfex->nChannels * pwfex->wBitsPerSample / 8;
    pwfex->nAvgBytesPerSec = pwfex->nSamplesPerSec * pwfex->nBlockAlign;
    pwfex->cbSize = 0;

    *ppCoMemOutputWaveFormatEx = pwfex;
    DEBUG_LOG("GetOutputFormat: SUCCESS - Channels=%d, Rate=%lu, Bits=%d",
              pwfex->nChannels, pwfex->nSamplesPerSec, pwfex->wBitsPerSample);
    return S_OK;
}

STDMETHODIMP ISpTTSEngineImpl::Speak(
    DWORD dwSpeakFlags,
    REFGUID /*rguidFormatId*/,
    const WAVEFORMATEX* /*pWaveFormatEx*/,
    const SPVTEXTFRAG* pTextFragList,
    ISpTTSEngineSite* pOutputSite)
{
    DEBUG_LOG("=== Speak Called ===");
    DEBUG_LOG("Speak Flags: 0x%08X", dwSpeakFlags);

    return com::safe_com_call([&]() -> HRESULT {
        if (!pTextFragList) {
            DEBUG_LOG("Speak: ERROR - pTextFragList is NULL");
            return E_INVALIDARG;
        }
        if (!pOutputSite) {
            DEBUG_LOG("Speak: ERROR - pOutputSite is NULL");
            return E_INVALIDARG;
        }
        long sapi_rate = 0;
        pOutputSite->GetRate(&sapi_rate);

        unsigned short sapi_volume = 100;
        pOutputSite->GetVolume(&sapi_volume);

        DEBUG_LOG("=== New Speech Request ===");
        DEBUG_LOG("Voice: %s", voice_name_.c_str());
        DEBUG_LOG("SAPI Rate: %d, SAPI Volume: %u", (int)sapi_rate, sapi_volume);

        ULONGLONG event_interest = 0;
        pOutputSite->GetEventInterest(&event_interest);
        const bool send_sentence_events = (event_interest & (1ULL << SPEI_SENTENCE_BOUNDARY)) != 0;
        const bool send_word_events = (event_interest & (1ULL << SPEI_WORD_BOUNDARY)) != 0;
        DEBUG_LOG("Event interest: 0x%llX (sentence: %d, word: %d)",
                  event_interest, send_sentence_events, send_word_events);

        SpeakContext ctx;
        ctx.caller = pOutputSite;
        ctx.bytes_written = 0;
        ctx.aborted = false;

        [[maybe_unused]] int frag_count = 0;
        for (const SPVTEXTFRAG* f = pTextFragList; f; f = f->pNext) {
            frag_count++;
        }
        DEBUG_LOG("Fragment count: %d", frag_count);

        [[maybe_unused]] int frag_num = 0;
        for (const SPVTEXTFRAG* frag = pTextFragList; frag; frag = frag->pNext) {
            frag_num++;
            DEBUG_LOG("--- Processing Fragment %d/%d ---", frag_num, frag_count);

            const DWORD actions = pOutputSite->GetActions();
            DEBUG_LOG("Actions flags: 0x%08X (ABORT=%d, SKIP=%d, RATE=%d, VOLUME=%d)",
                     actions,
                     !!(actions & SPVES_ABORT),
                     !!(actions & SPVES_SKIP),
                     !!(actions & SPVES_RATE),
                     !!(actions & SPVES_VOLUME));

            if (!checkAndHandleActionFlags(pOutputSite)) {
                break;
            }

            if (actions & SPVES_RATE) {
                pOutputSite->GetRate(&sapi_rate);
                DEBUG_LOG("  Rate updated via SPVES_RATE: %d", (int)sapi_rate);
            }
            if (actions & SPVES_VOLUME) {
                pOutputSite->GetVolume(&sapi_volume);
                DEBUG_LOG("  Volume updated via SPVES_VOLUME: %u", sapi_volume);
            }

            long current_rate = 0;
            HRESULT hr_rate = pOutputSite->GetRate(&current_rate);
            if (SUCCEEDED(hr_rate) && current_rate != sapi_rate) {
                DEBUG_LOG("  Rate mismatch detected! Cached=%d, Current=%d - using current",
                         (int)sapi_rate, (int)current_rate);
                sapi_rate = current_rate;
            }

            DEBUG_LOG("Fragment eAction: %d (SPVA_Speak=0, SPVA_Silence=1, "
                      "SPVA_Pronounce=2, SPVA_Bookmark=3, SPVA_SpellOut=4)",
                      frag->State.eAction);
            DEBUG_LOG("Fragment State: RateAdj=%d, Volume=%u, PitchAdj=%d",
                     frag->State.RateAdj, frag->State.Volume, frag->State.PitchAdj.MiddleAdj);

            if (frag->State.eAction == SPVA_Bookmark) {
                DEBUG_LOG("Fragment is a BOOKMARK");
                if (frag->ulTextLen > 0 && frag->pTextStart) {
                    std::wstring bookmark_text(frag->pTextStart, frag->ulTextLen);
                    DEBUG_LOG("Bookmark text: \"%S\"", bookmark_text.c_str());

                    long bookmark_id = 0;
                    try {
                        bookmark_id = std::stol(bookmark_text);
                    }
                    catch (const std::invalid_argument&) {
                        DEBUG_LOG("Bookmark: Invalid number format '%S', using 0", bookmark_text.c_str());
                    }
                    catch (const std::out_of_range&) {
                        DEBUG_LOG("Bookmark: Number out of range '%S', using 0", bookmark_text.c_str());
                    }

                    SPEVENT event = {};
                    event.eEventId = SPEI_TTS_BOOKMARK;
                    event.elParamType = SPET_LPARAM_IS_STRING;
                    event.ullAudioStreamOffset = ctx.bytes_written;
                    event.ulStreamNum = 0;
                    event.lParam = reinterpret_cast<LPARAM>(bookmark_text.c_str());
                    event.wParam = bookmark_id;
                    [[maybe_unused]] HRESULT hr = pOutputSite->AddEvents(&event, 1);
                    DEBUG_LOG("SAPI Event: Bookmark at byte offset %llu, id=%ld - Result: 0x%08X",
                              ctx.bytes_written, bookmark_id, hr);
                } else {
                    SPEVENT event = {};
                    event.eEventId = SPEI_TTS_BOOKMARK;
                    event.elParamType = SPET_LPARAM_IS_UNDEFINED;
                    event.ullAudioStreamOffset = ctx.bytes_written;
                    event.ulStreamNum = 0;
                    event.lParam = 0;
                    event.wParam = 0;
                    [[maybe_unused]] HRESULT hr = pOutputSite->AddEvents(&event, 1);
                    DEBUG_LOG("SAPI Event: Bookmark (empty) at byte offset %llu - Result: 0x%08X",
                              ctx.bytes_written, hr);
                }
                continue;
            }

            if (frag->State.eAction != SPVA_Speak && frag->State.eAction != SPVA_SpellOut) {
                DEBUG_LOG("Fragment skipped - not Speak or SpellOut action");
                continue;
            }

            if (frag->ulTextLen == 0 || !frag->pTextStart) {
                DEBUG_LOG("Fragment skipped - no text");
                continue;
            }

            const std::string text = utils::wstring_to_string(frag->pTextStart, frag->ulTextLen);
            DEBUG_LOG("Fragment text: \"%s\"", text.c_str());
            if (text.empty()) {
                DEBUG_LOG("Fragment skipped - empty after conversion");
                continue;
            }

            if (send_sentence_events) {
                SPEVENT event = {};
                event.eEventId = SPEI_SENTENCE_BOUNDARY;
                event.elParamType = SPET_LPARAM_IS_UNDEFINED;
                event.ullAudioStreamOffset = ctx.bytes_written;
                event.ulStreamNum = 0;
                event.lParam = frag->ulTextSrcOffset;
                event.wParam = frag->ulTextLen;
                [[maybe_unused]] HRESULT hr = pOutputSite->AddEvents(&event, 1);
                DEBUG_LOG("SAPI Event: Sentence boundary at byte offset %llu - Result: 0x%08X",
                          ctx.bytes_written, hr);
            }

            if (send_word_events) {
                const wchar_t* text_start = frag->pTextStart;
                ULONG text_len = frag->ulTextLen;

                bool in_word = false;
                ULONG word_start = 0;

                for (ULONG i = 0; i <= text_len; ++i) {
                    bool is_word_char = (i < text_len) &&
                                       (iswalnum(text_start[i]) || text_start[i] == L'\'' ||
                                        text_start[i] == L'-');

                    if (is_word_char && !in_word) {
                        word_start = i;
                        in_word = true;
                    } else if (!is_word_char && in_word) {
                        ULONG word_len = i - word_start;
                        SPEVENT event = {};
                        event.eEventId = SPEI_WORD_BOUNDARY;
                        event.elParamType = SPET_LPARAM_IS_UNDEFINED;
                        event.ullAudioStreamOffset = ctx.bytes_written;
                        event.ulStreamNum = 0;
                        event.lParam = frag->ulTextSrcOffset + word_start;
                        event.wParam = word_len;
                        [[maybe_unused]] HRESULT hr = pOutputSite->AddEvents(&event, 1);

                        std::wstring word_text(text_start + word_start, word_len);
                        DEBUG_LOG("SAPI Event: Word boundary \"%S\" at byte offset %llu - Result: 0x%08X",
                                  word_text.c_str(), ctx.bytes_written, hr);

                        in_word = false;
                    }
                }
            }

            int combined_rate = static_cast<int>(sapi_rate) + frag->State.RateAdj;
            combined_rate = std::clamp(combined_rate, MIN_RATE, MAX_RATE);
            int espeak_rate = combined_rate * RATE_TO_WPM_MULTIPLIER;

            int pitch_adj = frag->State.PitchAdj.MiddleAdj;
            int espeak_pitch = BASE_PITCH + std::clamp(pitch_adj * PITCH_ADJ_MULTIPLIER, MIN_PITCH_ADJ, MAX_PITCH_ADJ);

            int volume_adj = (sapi_volume - MAX_VOLUME) + (frag->State.Volume - MAX_VOLUME);
            int espeak_volume = std::clamp(static_cast<int>(sapi_volume) + volume_adj, MIN_VOLUME, MAX_VOLUME);

            config::ConfigManager& config_mgr = config::ConfigManager::getInstance();
            config::Configuration cfg = config_mgr.getConfig();
            int intonation = cfg.intonation;
            int wordgap = cfg.wordgap;
            bool rateboost = cfg.rateboost;

            DEBUG_LOG("--- Parameters ---");
            DEBUG_LOG("  Rate: SAPI=%d -> eSpeak-range=%d%s", combined_rate, espeak_rate, rateboost ? " (will boost x3 at WPM level)" : "");
            DEBUG_LOG("  Pitch: SAPI=%d -> eSpeak=%d", pitch_adj, espeak_pitch);
            DEBUG_LOG("  Volume: SAPI=%u -> eSpeak=%d", sapi_volume, espeak_volume);
            DEBUG_LOG("  Intonation: %d", intonation);
            DEBUG_LOG("  Word gap: %d", wordgap);

            if (!EspeakEngine::getInstance().speak(text, espeak_rate, espeak_pitch, espeak_volume,
                                                    intonation, wordgap, rateboost, speak_callback, &ctx)) {
                if (ctx.aborted) {
                    DEBUG_LOG("Speech aborted");
                    break;
                }
                DEBUG_LOG("Speech failed");
                return E_FAIL;
            }

            if (ctx.aborted) {
                break;
            }
        }

        DEBUG_LOG("=== Speak Completed Successfully ===");
        return S_OK;
    }, "ISpTTSEngine::Speak");
}
}
}
