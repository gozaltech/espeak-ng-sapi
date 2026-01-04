#pragma once

#include <windows.h>
#include "debug_log.h"

namespace Espeak {
namespace com {

template<typename F>
inline HRESULT safe_com_call(F&& func, const char* context_label) {
    try {
        return func();
    }
    catch (const std::bad_alloc&) {
        DEBUG_LOG("%s: Out of memory", context_label);
        return E_OUTOFMEMORY;
    }
    catch (const std::exception& e) {
        [[maybe_unused]] const char* what = e.what();
        DEBUG_LOG("%s: Exception - %s", context_label, what);
        return E_UNEXPECTED;
    }
    catch (...) {
        DEBUG_LOG("%s: Unexpected exception", context_label);
        return E_UNEXPECTED;
    }
}
}
}
