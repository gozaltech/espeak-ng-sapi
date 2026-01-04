#ifndef ESPEAK_SAPI_PROFILE_DIALOG_HPP
#define ESPEAK_SAPI_PROFILE_DIALOG_HPP

#include <windows.h>
#include <string>
#include <vector>
#include "config_manager.hpp"
#include "MainDialog.hpp"

namespace Espeak {
namespace configurator {

class ProfileDialog {
public:
    static bool Show(HINSTANCE hInstance, HWND parent,
                     const std::vector<VoiceItem>& available_voices,
                     const std::vector<std::string>& available_variants,
                     config::VoiceProfile& profile);

    ProfileDialog(HWND hwnd, HINSTANCE hInstance,
                  const std::vector<VoiceItem>& available_voices,
                  const std::vector<std::string>& available_variants,
                  config::VoiceProfile& profile);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnInitDialog();
    void OnCommand(WPARAM wParam);
    void OnOK();
    void OnCancel();
    void OnBaseVoiceChanged();

    static std::string GenerateProfileId();

    HWND hwnd_;
    [[maybe_unused]] HINSTANCE hInstance_;
    const std::vector<VoiceItem>& available_voices_;
    const std::vector<std::string>& available_variants_;
    config::VoiceProfile& profile_;
    bool result_;
};
}
}
#endif
