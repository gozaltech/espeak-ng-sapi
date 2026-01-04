#pragma once

#include <windows.h>
#include <utility>

namespace Espeak {
namespace utils {

class unique_handle
{
public:
    unique_handle() noexcept : handle_(nullptr) {}

    explicit unique_handle(HANDLE h) noexcept : handle_(h) {}

    ~unique_handle() noexcept
    {
        close();
    }

    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;

    unique_handle(unique_handle&& other) noexcept : handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }

    unique_handle& operator=(unique_handle&& other) noexcept
    {
        if (this != &other) {
            close();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept
    {
        return handle_;
    }

    [[nodiscard]] bool is_valid() const noexcept
    {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }

    explicit operator bool() const noexcept
    {
        return is_valid();
    }

private:
    void close() noexcept
    {
        if (is_valid()) {
            CloseHandle(handle_);
            handle_ = nullptr;
        }
    }

    HANDLE handle_;
};

class find_file_handle
{
public:
    find_file_handle() noexcept : handle_(INVALID_HANDLE_VALUE) {}

    explicit find_file_handle(const wchar_t* path, WIN32_FIND_DATAW& find_data) noexcept
        : handle_(FindFirstFileW(path, &find_data))
    {}

    ~find_file_handle() noexcept
    {
        close();
    }

    find_file_handle(const find_file_handle&) = delete;
    find_file_handle& operator=(const find_file_handle&) = delete;

    find_file_handle(find_file_handle&& other) noexcept : handle_(other.handle_)
    {
        other.handle_ = INVALID_HANDLE_VALUE;
    }

    find_file_handle& operator=(find_file_handle&& other) noexcept
    {
        if (this != &other) {
            close();
            handle_ = other.handle_;
            other.handle_ = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept
    {
        return handle_;
    }

    [[nodiscard]] bool is_valid() const noexcept
    {
        return handle_ != INVALID_HANDLE_VALUE;
    }

    explicit operator bool() const noexcept
    {
        return is_valid();
    }

    bool find_next(WIN32_FIND_DATAW& find_data) noexcept
    {
        return is_valid() && FindNextFileW(handle_, &find_data) != 0;
    }

private:
    void close() noexcept
    {
        if (is_valid()) {
            FindClose(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE handle_;
};
}
}
