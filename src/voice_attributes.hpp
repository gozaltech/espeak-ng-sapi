#pragma once

#include <string>
#include <array>
#include "utils.hpp"
#include "debug_log.h"

namespace Espeak {
namespace sapi {

class voice_attributes
{
public:
    explicit voice_attributes(
        std::string name = "en",
        std::string lang = "en",
        bool is_female = false,
        int age = 0,
        std::string identifier = "") noexcept
        : name_(std::move(name))
        , language_(std::move(lang))
        , is_female_(is_female)
        , age_(age)
        , identifier_(identifier.empty() ? name_ : std::move(identifier))
    {
    }

    [[nodiscard]] std::wstring get_name() const
    {
        return utils::string_to_wstring(name_);
    }

    [[nodiscard]] std::string get_identifier_utf8() const
    {
        return identifier_;
    }

    [[nodiscard]] std::string get_language_utf8() const
    {
        return language_;
    }

    [[nodiscard]] std::wstring get_age() const
    {
        if (age_ == 0) {
            return L"Adult";
        } else if (age_ < 18) {
            return L"Child";
        } else if (age_ < 60) {
            return L"Adult";
        } else {
            return L"Senior";
        }
    }

    [[nodiscard]] std::wstring get_gender() const
    {
        return is_female_ ? L"Female" : L"Male";
    }

    [[nodiscard]] std::wstring get_language() const
    {
        std::wstring locale_name = utils::string_to_wstring(language_);
        LCID lcid = 0;

        DEBUG_LOG("  Voice language field: '%s', trying locale: '%S', length=%zu",
                  language_.c_str(), locale_name.c_str(), locale_name.length());

        DEBUG_LOG("    Trying neutral locale: '%S'", locale_name.c_str());
        lcid = LocaleNameToLCID(locale_name.c_str(), LOCALE_ALLOW_NEUTRAL_NAMES);
        if (lcid != 0) {
            std::array<wchar_t, 10> lcid_str{};
            swprintf_s(lcid_str.data(), lcid_str.size(), L"%x", LANGIDFROMLCID(lcid));
            DEBUG_LOG("    Success with neutral locale, LCID: %S", lcid_str.data());
            return lcid_str.data();
        }

        size_t hyphen_pos = locale_name.find(L'-');
        if (hyphen_pos != std::wstring::npos) {
            std::wstring lang_only = locale_name.substr(0, hyphen_pos);
            DEBUG_LOG("    Trying language part only: '%S'", lang_only.c_str());

            lcid = LocaleNameToLCID(lang_only.c_str(), LOCALE_ALLOW_NEUTRAL_NAMES);
            if (lcid != 0) {
                std::array<wchar_t, 10> lcid_str{};
                swprintf_s(lcid_str.data(), lcid_str.size(), L"%x", LANGIDFROMLCID(lcid));
                DEBUG_LOG("    Success with language part, LCID: %S", lcid_str.data());
                return lcid_str.data();
            }
        }

        if (hyphen_pos == std::wstring::npos && locale_name.length() == 2) {
            std::wstring lang_code = locale_name;
            std::wstring country_code = lang_code;
            for (auto& c : country_code) {
                c = towupper(c);
            }
            std::wstring constructed_locale = lang_code + L"-" + country_code;
            DEBUG_LOG("    Trying constructed as fallback: '%S'", constructed_locale.c_str());

            lcid = LocaleNameToLCID(constructed_locale.c_str(), 0);
            if (lcid != 0) {
                std::array<wchar_t, 10> lcid_str{};
                swprintf_s(lcid_str.data(), lcid_str.size(), L"%x", LANGIDFROMLCID(lcid));
                DEBUG_LOG("    Success with constructed fallback, LCID: %S", lcid_str.data());
                return lcid_str.data();
            }
        }

        DEBUG_LOG("    All attempts failed, defaulting to 409 (en-US)");
        return L"409";
    }

private:
    std::string name_;
    std::string language_;
    bool is_female_;
    int age_;
    std::string identifier_;
};
}
}
