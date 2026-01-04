#pragma once

#include <vector>
#include <string_view>
#include <windows.h>
#include <sapi.h>
#include <sapiddk.h>
#include <sperror.h>
#include <comdef.h>
#include <comip.h>

#include "com.hpp"
#include "voice_attributes.hpp"
#include "voice_token.hpp"
#include "espeak_wrapper.h"

namespace Espeak {
namespace sapi {

class __declspec(uuid("{8F2D6C4B-1E9A-4F3D-A2B8-5C7E9D3A6F1B}")) IEnumSpObjectTokensImpl :
    public IEnumSpObjectTokens
{
public:
    explicit IEnumSpObjectTokensImpl(bool initialize = true);

    IEnumSpObjectTokensImpl(const IEnumSpObjectTokensImpl&) = delete;
    IEnumSpObjectTokensImpl& operator=(const IEnumSpObjectTokensImpl&) = delete;

    STDMETHOD(Next)(ULONG celt, ISpObjectToken** pelt, ULONG* pceltFetched) override;
    STDMETHOD(Skip)(ULONG celt) override;
    STDMETHOD(Reset)() override;
    STDMETHOD(Clone)(IEnumSpObjectTokens** ppEnum) override;
    STDMETHOD(Item)(ULONG Index, ISpObjectToken** ppToken) override;
    STDMETHOD(GetCount)(ULONG* pulCount) override;

protected:
    [[nodiscard]] void* get_interface(REFIID riid) noexcept
    {
        return com::try_primary_interface<IEnumSpObjectTokens>(this, riid);
    }

private:
    _COM_SMARTPTR_TYPEDEF(ISpObjectToken, __uuidof(ISpObjectToken));
    _COM_SMARTPTR_TYPEDEF(ISpObjectTokenInit, __uuidof(ISpObjectTokenInit));

    [[nodiscard]] ISpObjectTokenPtr create_token(const voice_attributes& attr) const;

    void addVoice(const ::Espeak::VoiceInfo& voice, const std::string& global_variant);
    void addVoiceProfile(const ::Espeak::VoiceInfo& base_voice, const std::string& profile_name,
                         const std::string& variant, const std::string& profile_id);
    bool isVoiceEnabled(std::string_view voice_id, const std::vector<std::string>& enabled_list) const;

    std::size_t index_;
    std::vector<voice_attributes> sapi_voices_;
};
}
}
