#include <new>
#include <comdef.h>
#include "voice_token.hpp"
#include "ISpTTSEngineImpl.hpp"
#include "error_handler.hpp"
#include "debug_log.h"

namespace Espeak {
namespace sapi {

voice_token::voice_token(const voice_attributes& attr)
{
    const std::wstring name = attr.get_name();
    set(name);

    utils::out_ptr<wchar_t> clsid_str(CoTaskMemFree);
    StringFromCLSID(__uuidof(ISpTTSEngineImpl), clsid_str.address());
    set(L"CLSID", clsid_str.get());

    std::wstring language_lcid = attr.get_language();
    DEBUG_LOG("voice_token: Creating token for '%S', language_code='%s', LCID='%S'",
              name.c_str(), attr.get_language_utf8().c_str(), language_lcid.c_str());

    attributes_[L"Age"] = attr.get_age();
    attributes_[L"Vendor"] = L"eSpeak-NG";
    attributes_[L"Language"] = language_lcid;
    attributes_[L"Gender"] = attr.get_gender();
    attributes_[L"Name"] = name;
    attributes_[L"VoiceId"] = utils::string_to_wstring(attr.get_identifier_utf8());
}

STDMETHODIMP voice_token::OpenKey(LPCWSTR pszSubKeyName, ISpDataKey** ppSubKey)
{
    return com::safe_com_call([&]() -> HRESULT {
        if (!pszSubKeyName) {
            return E_INVALIDARG;
        }
        if (!ppSubKey) {
            return E_POINTER;
        }
        *ppSubKey = nullptr;

        if (!str_equal(pszSubKeyName, L"Attributes")) {
            return SPERR_NOT_FOUND;
        }

        com::object<ISpDataKeyImpl> obj;
        for (const auto& [key, value] : attributes_) {
            obj->set(key, value);
        }

        com::interface_ptr<ISpDataKey> int_ptr(obj);
        *ppSubKey = int_ptr.get();
        return S_OK;
    }, "voice_token::OpenKey");
}

STDMETHODIMP voice_token::EnumKeys(ULONG Index, LPWSTR* ppszSubKeyName)
{
    return com::safe_com_call([&]() -> HRESULT {
        if (!ppszSubKeyName) {
            return E_POINTER;
        }
        *ppszSubKeyName = nullptr;

        if (Index > 0) {
            return SPERR_NO_MORE_ITEMS;
        }

        *ppszSubKeyName = com::strdup(L"Attributes");
        return S_OK;
    }, "voice_token::EnumKeys");
}
}
}
