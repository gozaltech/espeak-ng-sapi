#include <stdexcept>
#include <new>
#include <algorithm>
#include "IEnumSpObjectTokensImpl.hpp"
#include "espeak_wrapper.h"
#include "config_manager.hpp"
#include "voice_utils.hpp"
#include "error_handler.hpp"
#include "debug_log.h"

namespace Espeak {
namespace sapi {

IEnumSpObjectTokensImpl::IEnumSpObjectTokensImpl(bool initialize)
    : index_(0)
{
    if (!initialize) {
        return;
    }

    config::ConfigManager& config_mgr = config::ConfigManager::getInstance();
    [[maybe_unused]] bool loaded = config_mgr.load();
    config::Configuration cfg = config_mgr.getConfig();

    DEBUG_LOG("IEnumSpObjectTokensImpl: Loading config (default_only=%d, enabled_voices=%zu, profiles=%zu)",
              cfg.default_only, cfg.enabled_voices.size(), cfg.voice_profiles.size());

    EspeakEngine& engine = EspeakEngine::getInstance();
    if (engine.initialize()) {
        std::vector<VoiceInfo> espeak_voices = engine.getVoices();

        if (cfg.default_only) {
            for (const auto& voice : espeak_voices) {
                std::string voice_id = extractBaseVoiceName(voice.identifier, voice.name);

                if (voice_id == "en" || voice_id == "en-us") {
                    addVoice(voice, cfg.global_variant);
                    DEBUG_LOG("IEnumSpObjectTokensImpl: Added default voice '%s'", voice_id.c_str());
                    break;
                }
            }
        } else {
            for (const auto& voice : espeak_voices) {
                std::string voice_id = extractBaseVoiceName(voice.identifier, voice.name);

                if (isVoiceEnabled(voice_id, cfg.enabled_voices)) {
                    addVoice(voice, cfg.global_variant);
                    DEBUG_LOG("IEnumSpObjectTokensImpl: Added enabled voice '%s'", voice_id.c_str());
                }
            }
        }

        for (const auto& profile : cfg.voice_profiles) {
            if (profile.enabled) {
                for (const auto& voice : espeak_voices) {
                    std::string voice_id = extractBaseVoiceName(voice.identifier, voice.name);

                    if (voice_id == profile.base_voice) {
                        addVoiceProfile(voice, profile.name, profile.variant, profile.id);
                        DEBUG_LOG("IEnumSpObjectTokensImpl: Added voice profile '%s' (%s+%s)",
                                  profile.name.c_str(), profile.base_voice.c_str(), profile.variant.c_str());
                        break;
                    }
                }
            }
        }
    }

    if (sapi_voices_.empty()) {
        DEBUG_LOG("IEnumSpObjectTokensImpl: No voices configured, adding fallback 'en' voice");
        sapi_voices_.emplace_back("en", "en", false, 0, "en");
    }

    DEBUG_LOG("IEnumSpObjectTokensImpl: Total SAPI voices available: %zu", sapi_voices_.size());
}

IEnumSpObjectTokensImpl::ISpObjectTokenPtr IEnumSpObjectTokensImpl::create_token(
    const voice_attributes& attr) const
{
    std::wstring token_id = std::wstring(SPCAT_VOICES) + L"\\TokenEnums\\eSpeak-NG\\" + attr.get_name();
    com::object<voice_token> obj_data_key(attr);
    com::interface_ptr<ISpDataKey> int_data_key(obj_data_key);

    ISpObjectTokenInitPtr int_token_init(CLSID_SpObjectToken);
    if (!int_token_init) {
        throw std::runtime_error("Unable to create an object token");
    }

    if (FAILED(int_token_init->InitFromDataKey(SPCAT_VOICES, token_id.c_str(), int_data_key.get(false)))) {
        throw std::runtime_error("Unable to initialize an object token");
    }

    ISpObjectTokenPtr int_token = int_token_init;
    return int_token;
}

STDMETHODIMP IEnumSpObjectTokensImpl::Next(ULONG celt, ISpObjectToken** pelt, ULONG* pceltFetched)
{
    return com::safe_com_call([&]() -> HRESULT {
        if (celt == 0) {
            return E_INVALIDARG;
        }
        if (!pelt) {
            return E_POINTER;
        }
        if (!pceltFetched && celt > 1) {
            return E_POINTER;
        }

        if (pceltFetched) {
            *pceltFetched = 0;
        }

        std::vector<ISpObjectTokenPtr> tokens;
        tokens.reserve(celt);

        const std::size_t max_index = sapi_voices_.size();
        const std::size_t next_index = (std::min)(index_ + static_cast<std::size_t>(celt), max_index);

        for (std::size_t i = index_; i < next_index; ++i) {
            tokens.push_back(create_token(sapi_voices_[i]));
        }

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            tokens[i].AddRef();
            pelt[i] = tokens[i].GetInterfacePtr();
        }

        if (pceltFetched) {
            *pceltFetched = static_cast<ULONG>(tokens.size());
        }

        index_ += tokens.size();
        return (tokens.size() == celt) ? S_OK : S_FALSE;
    }, "IEnumSpObjectTokens::Next");
}

STDMETHODIMP IEnumSpObjectTokensImpl::Skip(ULONG celt)
{
    const std::size_t remaining = sapi_voices_.size() - index_;
    const std::size_t num_skipped = (std::min)(remaining, static_cast<std::size_t>(celt));
    index_ += num_skipped;
    return (num_skipped == celt) ? S_OK : S_FALSE;
}

STDMETHODIMP IEnumSpObjectTokensImpl::Reset()
{
    index_ = 0;
    return S_OK;
}

STDMETHODIMP IEnumSpObjectTokensImpl::GetCount(ULONG* pulCount)
{
    if (!pulCount) {
        return E_POINTER;
    }
    *pulCount = static_cast<ULONG>(sapi_voices_.size());
    return S_OK;
}

STDMETHODIMP IEnumSpObjectTokensImpl::Item(ULONG Index, ISpObjectToken** ppToken)
{
    return com::safe_com_call([&]() -> HRESULT {
        if (!ppToken) {
            return E_POINTER;
        }
        *ppToken = nullptr;

        if (Index >= sapi_voices_.size()) {
            return SPERR_NO_MORE_ITEMS;
        }

        ISpObjectTokenPtr int_token = create_token(sapi_voices_[Index]);
        int_token.AddRef();
        *ppToken = int_token.GetInterfacePtr();
        return S_OK;
    }, "IEnumSpObjectTokens::Item");
}

STDMETHODIMP IEnumSpObjectTokensImpl::Clone(IEnumSpObjectTokens** ppEnum)
{
    return com::safe_com_call([&]() -> HRESULT {
        if (!ppEnum) {
            return E_POINTER;
        }
        *ppEnum = nullptr;

        com::object<IEnumSpObjectTokensImpl> obj(false);
        obj->sapi_voices_ = sapi_voices_;
        obj->index_ = index_;
        com::interface_ptr<IEnumSpObjectTokens> int_ptr(obj);
        *ppEnum = int_ptr.get();
        return S_OK;
    }, "IEnumSpObjectTokens::Clone");
}

void IEnumSpObjectTokensImpl::addVoice(const ::Espeak::VoiceInfo& voice, const std::string& global_variant)
{
    bool is_female = isGenderFemale(voice.gender);
    std::string display_name = voice.name.empty() ? voice.identifier : voice.name;

    std::string voice_id = extractBaseVoiceName(voice.identifier, voice.name);

    if (!global_variant.empty()) {
        voice_id = voice_id + "+" + global_variant;
        display_name = display_name + " (" + global_variant + ")";
    }

    std::string clean_languages = cleanLanguageString(voice.languages);

    sapi_voices_.emplace_back(display_name, clean_languages, is_female, voice.age, voice_id);
}

void IEnumSpObjectTokensImpl::addVoiceProfile(const ::Espeak::VoiceInfo& base_voice, const std::string& profile_name,
                                               const std::string& variant, const std::string& profile_id)
{
    bool is_female = isGenderFemale(base_voice.gender);

    std::string voice_id = extractBaseVoiceName(base_voice.identifier, base_voice.name);

    if (!variant.empty()) {
        voice_id = voice_id + "+" + variant;
    }

    std::string clean_languages = cleanLanguageString(base_voice.languages);

    sapi_voices_.emplace_back(profile_name, clean_languages, is_female, base_voice.age, voice_id);
}

bool IEnumSpObjectTokensImpl::isVoiceEnabled(std::string_view voice_id,
                                              const std::vector<std::string>& enabled_list) const
{
    return std::find(enabled_list.begin(), enabled_list.end(), voice_id) != enabled_list.end();
}
}
}
