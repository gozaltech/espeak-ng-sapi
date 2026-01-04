#pragma once

#include <cstring>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>

namespace Espeak {
namespace utils {

namespace fs = std::filesystem;

constexpr wchar_t APP_DIR_NAME[] = L"espeak-ng-sapi";

[[nodiscard]] inline fs::path getSpecialFolderPath(int csidl)
{
    wchar_t path[MAX_PATH];
    if (SHGetFolderPathW(nullptr, csidl, nullptr, 0, path) == S_OK) {
        return fs::path(path);
    }
    return {};
}

[[nodiscard]] inline fs::path getAppDataPath()
{
    return getSpecialFolderPath(CSIDL_APPDATA);
}

[[nodiscard]] inline fs::path getProgramDataPath()
{
    return getSpecialFolderPath(CSIDL_COMMON_APPDATA);
}

[[nodiscard]] inline fs::path getEspeakConfigDir()
{
    fs::path appdata = getAppDataPath();
    if (appdata.empty()) {
        return {};
    }
    return appdata / APP_DIR_NAME;
}

[[nodiscard]] inline fs::path getEspeakDataDir()
{
    fs::path programdata = getProgramDataPath();
    if (programdata.empty()) {
        return {};
    }
    return programdata / APP_DIR_NAME / "data";
}

[[nodiscard]] inline fs::path getEspeakVoicesDir()
{
    fs::path data_dir = getEspeakDataDir();
    if (data_dir.empty()) {
        return {};
    }
    return data_dir / "voices" / "!v";
}

[[nodiscard]] inline std::wstring string_to_wstring(std::string_view s)
{
    if (s.empty()) {
        return {};
    }
    const int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (size_needed <= 0) {
        return {};
    }
    std::wstring result(static_cast<size_t>(size_needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &result[0], size_needed);
    return result;
}

[[nodiscard]] inline std::string wstring_to_string(std::wstring_view s)
{
    if (s.empty()) {
        return {};
    }
    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, s.data(),
                                                 static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size_needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                        result.data(), size_needed, nullptr, nullptr);
    return result;
}

[[nodiscard]] inline std::string wstring_to_string(const wchar_t* s, std::size_t n)
{
    if (!s || n == 0) {
        return {};
    }
    const int size_needed = WideCharToMultiByte(CP_UTF8, 0, s, static_cast<int>(n),
                                                 nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size_needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s, static_cast<int>(n),
                        result.data(), size_needed, nullptr, nullptr);
    return result;
}

template<typename T>
class out_ptr
{
public:
    template<typename F>
    explicit out_ptr(F f)
        : ptr_(nullptr)
        , deleter_(std::make_unique<deleter_impl<F>>(f))
    {
    }

    ~out_ptr()
    {
        release();
    }

    out_ptr(const out_ptr&) = delete;
    out_ptr& operator=(const out_ptr&) = delete;

    [[nodiscard]] T* get() const noexcept
    {
        return ptr_;
    }

    T** address() noexcept
    {
        release();
        return &ptr_;
    }

private:
    class deleter_base
    {
    public:
        virtual ~deleter_base() = default;
        virtual void destroy(T* p) const noexcept = 0;
    };

    template<typename F>
    class deleter_impl : public deleter_base
    {
    public:
        explicit deleter_impl(F f) : func_(f) {}

        void destroy(T* p) const noexcept override
        {
            func_(p);
        }

    private:
        F func_;
    };

    void release() noexcept
    {
        if (ptr_) {
            deleter_->destroy(ptr_);
            ptr_ = nullptr;
        }
    }

    T* ptr_;
    std::unique_ptr<deleter_base> deleter_;
};
}
}
