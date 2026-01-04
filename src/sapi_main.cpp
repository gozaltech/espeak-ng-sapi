#include <new>
#include <sapi.h>
#include "com.hpp"
#include "registry.hpp"
#include "ISpTTSEngineImpl.hpp"
#include "IEnumSpObjectTokensImpl.hpp"
#include "error_handler.hpp"
#include "debug_log.h"
#include <array>

namespace {

HINSTANCE g_dll_handle = nullptr;
Espeak::com::class_object_factory g_cls_obj_factory;

const std::wstring token_enums_path = L"Software\\Microsoft\\Speech\\Voices\\TokenEnums";

[[nodiscard]] std::wstring clsid_to_string(const GUID& clsid)
{
    std::array<wchar_t, 64> buf{};
    StringFromGUID2(clsid, buf.data(), static_cast<int>(buf.size()));
    return std::wstring(buf.data());
}

void register_token_enumerator()
{
    using namespace Espeak::sapi;
    using namespace Espeak::registry;

    const std::wstring clsid_str = clsid_to_string(__uuidof(IEnumSpObjectTokensImpl));

#ifdef _WIN64
    REGSAM access = KEY_CREATE_SUB_KEY | KEY_SET_VALUE | KEY_WOW64_64KEY;
#else
    REGSAM access = KEY_CREATE_SUB_KEY | KEY_SET_VALUE | KEY_WOW64_32KEY;
#endif

    key enums_key(HKEY_LOCAL_MACHINE, token_enums_path, access, true);
    key enum_key(enums_key, L"eSpeak-NG", KEY_SET_VALUE, true);

    enum_key.set(L"eSpeak-NG Voices");
    enum_key.set(L"CLSID", clsid_str);
}

void unregister_token_enumerator() noexcept
{
    using namespace Espeak::registry;

    try {
#ifdef _WIN64
        REGSAM access = KEY_ALL_ACCESS | KEY_WOW64_64KEY;
#else
        REGSAM access = KEY_ALL_ACCESS | KEY_WOW64_32KEY;
#endif
        key enums_key(HKEY_LOCAL_MACHINE, token_enums_path, access);
        enums_key.delete_subkey(L"eSpeak-NG");
    }
    catch (const std::exception& e) {
        [[maybe_unused]] const char* what = e.what();
        DEBUG_LOG("DllUnregisterServer: Exception during cleanup: %s", what);
    }
    catch (...) {
        DEBUG_LOG("DllUnregisterServer: Unknown exception during cleanup");
    }
}
}

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_dll_handle = hInstance;
        DisableThreadLibraryCalls(hInstance);

        try {
            g_cls_obj_factory.register_class<Espeak::sapi::IEnumSpObjectTokensImpl>();
            g_cls_obj_factory.register_class<Espeak::sapi::ISpTTSEngineImpl>();
        }
        catch (const std::exception& e) {
            [[maybe_unused]] const char* what = e.what();
            DEBUG_LOG("DllMain: Failed to register classes: %s", what);
            return FALSE;
        }
        catch (...) {
            DEBUG_LOG("DllMain: Failed to register classes: Unknown exception");
            return FALSE;
        }
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    return g_cls_obj_factory.create(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow()
{
    return Espeak::com::object_counter::is_zero() ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer()
{
    return Espeak::com::safe_com_call([]() -> HRESULT {
        Espeak::com::class_registrar r(g_dll_handle);
        r.register_class<Espeak::sapi::IEnumSpObjectTokensImpl>();
        r.register_class<Espeak::sapi::ISpTTSEngineImpl>();
        register_token_enumerator();
        return S_OK;
    }, "DllRegisterServer");
}

STDAPI DllUnregisterServer()
{
    return Espeak::com::safe_com_call([]() -> HRESULT {
        unregister_token_enumerator();
        Espeak::com::class_registrar r(g_dll_handle);
        r.unregister_class<Espeak::sapi::IEnumSpObjectTokensImpl>();
        r.unregister_class<Espeak::sapi::ISpTTSEngineImpl>();
        return S_OK;
    }, "DllUnregisterServer");
}
