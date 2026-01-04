#pragma once

#include <windows.h>
#include <cstdio>
#include <ctime>

#define ENABLE_DEBUG_LOG 0

#if ENABLE_DEBUG_LOG

namespace DebugLog {

inline void GetLogPath(wchar_t* path, size_t size) {
    wchar_t userProfile[MAX_PATH];
    DWORD result = GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH);

    if (result > 0 && result < MAX_PATH) {
        swprintf_s(path, size, L"%s\\EspeakSAPI_debug.log", userProfile);
    } else {
        wchar_t tempPath[MAX_PATH];
        if (GetTempPathW(MAX_PATH, tempPath) > 0) {
            swprintf_s(path, size, L"%sEspeakSAPI_debug.log", tempPath);
        } else {
            wcscpy_s(path, size, L"EspeakSAPI_debug.log");
        }
    }
}

inline void Log(const char* format, ...) {
    wchar_t logPath[MAX_PATH];
    GetLogPath(logPath, MAX_PATH);

    FILE* file = nullptr;
    if (_wfopen_s(&file, logPath, L"a") == 0 && file) {
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);

        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

        fprintf(file, "[%s] ", timeStr);

        va_list args;
        va_start(args, format);
        vfprintf(file, format, args);
        va_end(args);

        fprintf(file, "\n");
        fclose(file);
    }
}
}

#define DEBUG_LOG(...) DebugLog::Log(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif
