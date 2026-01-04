#include "ProfileDialog.hpp"
#include "resource.h"
#include "../src/utils.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <memory>
#include <array>

namespace Espeak {
namespace configurator {

bool ProfileDialog::Show(HINSTANCE hInstance, HWND parent,
                         const std::vector<VoiceItem>& available_voices,
                         const std::vector<std::string>& available_variants,
                         config::VoiceProfile& profile) {
    auto dialog = std::make_unique<ProfileDialog>(nullptr, hInstance, available_voices, available_variants, profile);
    ProfileDialog* pThis = dialog.get();

    [[maybe_unused]] INT_PTR result = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PROFILE_DIALOG),
                                                      parent, DialogProc, reinterpret_cast<LPARAM>(pThis));

    return pThis->result_;
}

ProfileDialog::ProfileDialog(HWND hwnd, HINSTANCE hInstance,
                              const std::vector<VoiceItem>& available_voices,
                              const std::vector<std::string>& available_variants,
                              config::VoiceProfile& profile)
    : hwnd_(hwnd)
    , hInstance_(hInstance)
    , available_voices_(available_voices)
    , available_variants_(available_variants)
    , profile_(profile)
    , result_(false)
{
}

INT_PTR CALLBACK ProfileDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ProfileDialog* pThis = reinterpret_cast<ProfileDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (msg == WM_INITDIALOG) {
        pThis = reinterpret_cast<ProfileDialog*>(lParam);
        pThis->hwnd_ = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->OnInitDialog();
        return TRUE;
    }

    if (!pThis) {
        return FALSE;
    }

    switch (msg) {
        case WM_COMMAND:
            pThis->OnCommand(wParam);
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
    }

    return FALSE;
}

void ProfileDialog::OnInitDialog() {
    HWND hComboBaseVoice = GetDlgItem(hwnd_, IDC_COMBO_BASE_VOICE);
    HWND hComboVariant = GetDlgItem(hwnd_, IDC_COMBO_PROFILE_VARIANT);

    for (const auto& voice : available_voices_) {
        std::wstring display_w(voice.display_name.begin(), voice.display_name.end());
        SendMessage(hComboBaseVoice, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display_w.c_str()));
    }
    if (!available_voices_.empty()) {
        SendMessage(hComboBaseVoice, CB_SETCURSEL, 0, 0);
    }

    for (size_t i = 1; i < available_variants_.size(); i++) {
        std::wstring variant_w(available_variants_[i].begin(), available_variants_[i].end());
        SendMessage(hComboVariant, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(variant_w.c_str()));
    }
    if (available_variants_.size() > 1) {
        SendMessage(hComboVariant, CB_SETCURSEL, 0, 0);
    }

    OnBaseVoiceChanged();
}

void ProfileDialog::OnCommand(WPARAM wParam) {
    WORD id = LOWORD(wParam);
    WORD code = HIWORD(wParam);

    switch (id) {
        case IDC_COMBO_BASE_VOICE:
            if (code == CBN_SELCHANGE) {
                OnBaseVoiceChanged();
            }
            break;

        case IDC_COMBO_PROFILE_VARIANT:
            if (code == CBN_SELCHANGE) {
                OnBaseVoiceChanged();
            }
            break;

        case IDOK:
            OnOK();
            break;

        case IDCANCEL:
            OnCancel();
            break;
    }
}

void ProfileDialog::OnOK() {
    HWND hEditName = GetDlgItem(hwnd_, IDC_EDIT_PROFILE_NAME);
    HWND hComboBaseVoice = GetDlgItem(hwnd_, IDC_COMBO_BASE_VOICE);
    HWND hComboVariant = GetDlgItem(hwnd_, IDC_COMBO_PROFILE_VARIANT);

    std::array<wchar_t, 256> name{};
    GetWindowText(hEditName, name.data(), static_cast<int>(name.size()));
    if (wcslen(name.data()) == 0) {
        MessageBox(hwnd_, L"Please enter a profile name.", L"Validation Error", MB_OK | MB_ICONWARNING);
        return;
    }

    int voiceIdx = SendMessage(hComboBaseVoice, CB_GETCURSEL, 0, 0);
    if (voiceIdx == CB_ERR || voiceIdx >= static_cast<int>(available_voices_.size())) {
        MessageBox(hwnd_, L"Please select a base voice.", L"Validation Error", MB_OK | MB_ICONWARNING);
        return;
    }

    int variantIdx = SendMessage(hComboVariant, CB_GETCURSEL, 0, 0);
    if (variantIdx == CB_ERR || variantIdx + 1 >= static_cast<int>(available_variants_.size())) {
        MessageBox(hwnd_, L"Please select a variant.", L"Validation Error", MB_OK | MB_ICONWARNING);
        return;
    }

    profile_.id = GenerateProfileId();
    std::wstring name_w(name.data());
    profile_.name = utils::wstring_to_string(name_w);

    profile_.base_voice = available_voices_[voiceIdx].identifier;
    profile_.variant = available_variants_[variantIdx + 1];

    profile_.enabled = true;

    result_ = true;
    EndDialog(hwnd_, IDOK);
}

void ProfileDialog::OnCancel() {
    result_ = false;
    EndDialog(hwnd_, IDCANCEL);
}

void ProfileDialog::OnBaseVoiceChanged() {
    HWND hEditName = GetDlgItem(hwnd_, IDC_EDIT_PROFILE_NAME);
    HWND hComboBaseVoice = GetDlgItem(hwnd_, IDC_COMBO_BASE_VOICE);
    HWND hComboVariant = GetDlgItem(hwnd_, IDC_COMBO_PROFILE_VARIANT);

    int voiceIdx = SendMessage(hComboBaseVoice, CB_GETCURSEL, 0, 0);
    int variantIdx = SendMessage(hComboVariant, CB_GETCURSEL, 0, 0);

    if (voiceIdx != CB_ERR && voiceIdx < static_cast<int>(available_voices_.size()) &&
        variantIdx != CB_ERR && variantIdx + 1 < static_cast<int>(available_variants_.size())) {

        std::wstring voice_name(available_voices_[voiceIdx].display_name.begin(),
                               available_voices_[voiceIdx].display_name.end());
        std::wstring variant_name(available_variants_[variantIdx + 1].begin(),
                                 available_variants_[variantIdx + 1].end());

        std::wstring profile_name = voice_name + L" " + variant_name;
        SetWindowText(hEditName, profile_name.c_str());
    }
}

std::string ProfileDialog::GenerateProfileId() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    std::ostringstream oss;
    oss << "profile-" << millis;
    return oss.str();
}
}
}
