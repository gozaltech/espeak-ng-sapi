#include "MainDialog.hpp"
#include "ProfileDialog.hpp"
#include "resource.h"
#include "../src/utils.hpp"
#include "../src/voice_utils.hpp"
#include "../src/win32_utils.hpp"
#include <commctrl.h>
#include <shellapi.h>
#include <algorithm>
#include <memory>

namespace Espeak {
namespace configurator {

INT_PTR MainDialog::Show(HINSTANCE hInstance) {
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), nullptr,
                          DialogProc, 0);
}

MainDialog::MainDialog(HWND hwnd, HINSTANCE hInstance)
    : hwnd_(hwnd)
    , hInstance_(hInstance)
    , hCheckDefaultOnly_(nullptr)
    , hListVoices_(nullptr)
    , hComboVariant_(nullptr)
    , hSliderIntonation_(nullptr)
    , hLabelIntonation_(nullptr)
    , hListProfiles_(nullptr)
    , hBtnRemoveProfile_(nullptr)
{
}

INT_PTR CALLBACK MainDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainDialog* pThis = reinterpret_cast<MainDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (msg == WM_INITDIALOG) {
        auto dialog = std::make_unique<MainDialog>(hwnd, reinterpret_cast<HINSTANCE>(lParam));
        pThis = dialog.get();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog.release()));
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

        case WM_NOTIFY:
            pThis->OnNotify(reinterpret_cast<NMHDR*>(lParam));
            return TRUE;

        case WM_HSCROLL:
            pThis->OnHScroll(wParam, lParam);
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;

        case WM_DESTROY:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            std::unique_ptr<MainDialog>{pThis};
            return TRUE;
    }

    return FALSE;
}

void MainDialog::OnInitDialog() {
    hCheckDefaultOnly_ = GetDlgItem(hwnd_, IDC_CHECK_DEFAULT_ONLY);
    hListVoices_ = GetDlgItem(hwnd_, IDC_LIST_VOICES);
    hComboVariant_ = GetDlgItem(hwnd_, IDC_COMBO_VARIANT);
    hSliderIntonation_ = GetDlgItem(hwnd_, IDC_SLIDER_INTONATION);
    hLabelIntonation_ = GetDlgItem(hwnd_, IDC_LABEL_INTONATION);
    hSliderWordgap_ = GetDlgItem(hwnd_, IDC_SLIDER_WORDGAP);
    hLabelWordgap_ = GetDlgItem(hwnd_, IDC_LABEL_WORDGAP);
    hCheckRateboost_ = GetDlgItem(hwnd_, IDC_CHECK_RATEBOOST);
    hListProfiles_ = GetDlgItem(hwnd_, IDC_LIST_PROFILES);
    hBtnRemoveProfile_ = GetDlgItem(hwnd_, IDC_BTN_REMOVE_PROFILE);

    ListView_SetExtendedListViewStyle(hListVoices_, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    ListView_SetExtendedListViewStyle(hListProfiles_, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_WIDTH;
    lvc.cx = 180;
    ListView_InsertColumn(hListVoices_, 0, &lvc);
    ListView_InsertColumn(hListProfiles_, 0, &lvc);

    SendMessage(hSliderIntonation_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessage(hSliderIntonation_, TBM_SETTICFREQ, 10, 0);

    SendMessage(hSliderWordgap_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessage(hSliderWordgap_, TBM_SETTICFREQ, 10, 0);

    LoadConfiguration();
    PopulateVariantCombo();
    PopulateVoiceList();
    PopulateProfileList();
    UpdateUIState();
}

void MainDialog::OnCommand(WPARAM wParam) {
    WORD id = LOWORD(wParam);
    WORD code = HIWORD(wParam);

    switch (id) {
        case IDC_CHECK_DEFAULT_ONLY:
            if (code == BN_CLICKED) {
                OnDefaultOnlyChanged();
            }
            break;

        case IDC_BTN_SELECT_ALL:
            OnSelectAll();
            break;

        case IDC_BTN_DESELECT_ALL:
            OnDeselectAll();
            break;

        case IDC_BTN_ADD_PROFILE:
            OnAddProfile();
            break;

        case IDC_BTN_REMOVE_PROFILE:
            OnRemoveProfile();
            break;

        case IDC_BTN_OPEN_FOLDER:
            OnOpenFolder();
            break;

        case IDOK:
            OnOK();
            break;

        case IDCANCEL:
            OnCancel();
            break;
    }
}

void MainDialog::OnHScroll(WPARAM wParam, LPARAM lParam) {
    HWND hSlider = reinterpret_cast<HWND>(lParam);
    if (hSlider == hSliderIntonation_) {
        UpdateIntonationLabel();
    } else if (hSlider == hSliderWordgap_) {
        UpdateWordgapLabel();
    }
}

void MainDialog::OnNotify(NMHDR* pnmhdr) {
    if (pnmhdr->code == LVN_ITEMCHANGED) {
        [[maybe_unused]] NMLISTVIEW* pnmlv = reinterpret_cast<NMLISTVIEW*>(pnmhdr);

        if (pnmhdr->hwndFrom == hListProfiles_) {
            UpdateUIState();
        }
    }
}

void MainDialog::LoadConfiguration() {
    config::ConfigManager& mgr = config::ConfigManager::getInstance();
    [[maybe_unused]] bool loaded = mgr.load();
    config_ = mgr.getConfig();
    SendMessage(hCheckDefaultOnly_, BM_SETCHECK, config_.default_only ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hSliderIntonation_, TBM_SETPOS, TRUE, config_.intonation);
    UpdateIntonationLabel();
    SendMessage(hSliderWordgap_, TBM_SETPOS, TRUE, config_.wordgap);
    UpdateWordgapLabel();
    SendMessage(hCheckRateboost_, BM_SETCHECK, config_.rateboost ? BST_CHECKED : BST_UNCHECKED, 0);
}

void MainDialog::SaveConfiguration() {
    config_.default_only = (SendMessage(hCheckDefaultOnly_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    config_.enabled_voices.clear();
    int voiceCount = ListView_GetItemCount(hListVoices_);
    for (int i = 0; i < voiceCount && i < static_cast<int>(available_voices_.size()); i++) {
        if (ListView_GetCheckState(hListVoices_, i)) {
            config_.enabled_voices.push_back(available_voices_[i].identifier);
        }
    }

    int profileCount = ListView_GetItemCount(hListProfiles_);
    for (int i = 0; i < profileCount && i < static_cast<int>(config_.voice_profiles.size()); i++) {
        config_.voice_profiles[i].enabled = (ListView_GetCheckState(hListProfiles_, i) != FALSE);
    }

    int variantIdx = SendMessage(hComboVariant_, CB_GETCURSEL, 0, 0);
    if (variantIdx >= 0 && variantIdx < static_cast<int>(available_variants_.size())) {
        config_.global_variant = available_variants_[variantIdx];
    } else {
        config_.global_variant = "";
    }

    config_.intonation = SendMessage(hSliderIntonation_, TBM_GETPOS, 0, 0);
    config_.wordgap = SendMessage(hSliderWordgap_, TBM_GETPOS, 0, 0);
    config_.rateboost = (SendMessage(hCheckRateboost_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    config::ConfigManager& mgr = config::ConfigManager::getInstance();
    if (mgr.save(config_)) {
        MessageBox(hwnd_,
                   L"Configuration saved successfully.\n\n"
                   L"Changes take effect:\n"
                   L"\u2022 Intonation/Word gap/Rate boost: Immediately for new speech\n"
                   L"\u2022 Voice list/variants: Require restarting SAPI applications\n\n"
                   L"Applications using eSpeak-NG (screen readers, TTS software)\n"
                   L"will automatically detect parameter changes.",
                   L"Success",
                   MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBox(hwnd_,
                   L"Failed to save configuration file.\n\n"
                   L"Please check that the configuration directory is writable.",
                   L"Error",
                   MB_OK | MB_ICONERROR);
    }
}

void MainDialog::PopulateVoiceList() {
    ListView_DeleteAllItems(hListVoices_);
    available_voices_.clear();

    EspeakEngine& engine = EspeakEngine::getInstance();
    if (!engine.initialize()) {
        MessageBox(hwnd_, L"Failed to initialize eSpeak-NG engine.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::vector<VoiceInfo> espeak_voices = engine.getVoices();

    int index = 0;
    for (const auto& voice : espeak_voices) {
        std::string voice_id = sapi::extractBaseVoiceName(voice.identifier, voice.name);

        std::string display_name = voice.name.empty() ? voice_id : voice.name;

        bool checked = std::find(config_.enabled_voices.begin(), config_.enabled_voices.end(), voice_id)
                      != config_.enabled_voices.end();

        VoiceItem item;
        item.identifier = voice_id;
        item.display_name = display_name;
        item.checked = checked;
        available_voices_.push_back(item);

        std::wstring display_w(display_name.begin(), display_name.end());
        LVITEM lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(display_w.c_str());
        ListView_InsertItem(hListVoices_, &lvi);
        ListView_SetCheckState(hListVoices_, index, checked);

        index++;
    }
}

void MainDialog::PopulateVariantCombo() {
    SendMessage(hComboVariant_, CB_RESETCONTENT, 0, 0);
    available_variants_.clear();

    available_variants_.push_back("");
    SendMessage(hComboVariant_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"(None)"));

    EspeakEngine& engine = EspeakEngine::getInstance();
    if (engine.initialize()) {
        utils::fs::path data_path = utils::getEspeakVoicesDir();

        WIN32_FIND_DATAW find_data;
        utils::find_file_handle hFind((data_path / L"*").c_str(), find_data);

        if (hFind) {
            do {
                if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring variant_w = find_data.cFileName;
                    std::string variant_s = utils::wstring_to_string(variant_w);

                    available_variants_.push_back(variant_s);
                    SendMessage(hComboVariant_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(variant_w.c_str()));
                }
            } while (hFind.find_next(find_data));
        }
    }

    if (available_variants_.size() > 1) {
        std::vector<std::string> variants_to_sort(available_variants_.begin() + 1, available_variants_.end());
        std::sort(variants_to_sort.begin(), variants_to_sort.end());

        SendMessage(hComboVariant_, CB_RESETCONTENT, 0, 0);
        SendMessage(hComboVariant_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"(None)"));

        available_variants_.clear();
        available_variants_.push_back("");

        for (const auto& variant : variants_to_sort) {
            available_variants_.push_back(variant);
            std::wstring variant_w(variant.begin(), variant.end());
            SendMessage(hComboVariant_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(variant_w.c_str()));
        }
    }

    int selected_index = 0;
    if (!config_.global_variant.empty()) {
        for (size_t i = 0; i < available_variants_.size(); i++) {
            if (available_variants_[i] == config_.global_variant) {
                selected_index = static_cast<int>(i);
                break;
            }
        }
    }
    SendMessage(hComboVariant_, CB_SETCURSEL, selected_index, 0);
}

void MainDialog::PopulateProfileList() {
    ListView_DeleteAllItems(hListProfiles_);

    int index = 0;
    for (const auto& profile : config_.voice_profiles) {
        std::wstring display = std::wstring(profile.name.begin(), profile.name.end()) +
                              L" (" +
                              std::wstring(profile.base_voice.begin(), profile.base_voice.end()) +
                              L"+" +
                              std::wstring(profile.variant.begin(), profile.variant.end()) +
                              L")";

        LVITEM lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(display.c_str());
        ListView_InsertItem(hListProfiles_, &lvi);
        ListView_SetCheckState(hListProfiles_, index, profile.enabled);

        index++;
    }
}

void MainDialog::UpdateIntonationLabel() {
    int value = SendMessage(hSliderIntonation_, TBM_GETPOS, 0, 0);
    std::wstring text = std::to_wstring(value);
    SetWindowText(hLabelIntonation_, text.c_str());
}

void MainDialog::UpdateWordgapLabel() {
    int value = SendMessage(hSliderWordgap_, TBM_GETPOS, 0, 0);
    std::wstring text = std::to_wstring(value);
    SetWindowText(hLabelWordgap_, text.c_str());
}

void MainDialog::UpdateUIState() {
    bool defaultOnly = (SendMessage(hCheckDefaultOnly_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    EnableWindow(hListVoices_, !defaultOnly);
    EnableWindow(GetDlgItem(hwnd_, IDC_BTN_SELECT_ALL), !defaultOnly);
    EnableWindow(GetDlgItem(hwnd_, IDC_BTN_DESELECT_ALL), !defaultOnly);

    int profileSel = ListView_GetNextItem(hListProfiles_, -1, LVNI_SELECTED);
    EnableWindow(hBtnRemoveProfile_, profileSel != -1 && !config_.voice_profiles.empty());
}

void MainDialog::OnSelectAll() {
    int count = ListView_GetItemCount(hListVoices_);
    for (int i = 0; i < count; i++) {
        ListView_SetCheckState(hListVoices_, i, TRUE);
    }
}

void MainDialog::OnDeselectAll() {
    int count = ListView_GetItemCount(hListVoices_);
    for (int i = 0; i < count; i++) {
        ListView_SetCheckState(hListVoices_, i, FALSE);
    }
}

void MainDialog::OnDefaultOnlyChanged() {
    UpdateUIState();
}

void MainDialog::OnAddProfile() {
    config::VoiceProfile profile;
    if (ProfileDialog::Show(hInstance_, hwnd_, available_voices_, available_variants_, profile)) {
        config_.voice_profiles.push_back(profile);
        PopulateProfileList();
        UpdateUIState();
    }
}

void MainDialog::OnRemoveProfile() {
    int sel = ListView_GetNextItem(hListProfiles_, -1, LVNI_SELECTED);
    if (sel != -1 && sel < static_cast<int>(config_.voice_profiles.size())) {
        config_.voice_profiles.erase(config_.voice_profiles.begin() + sel);
        PopulateProfileList();
        UpdateUIState();
    }
}

void MainDialog::OnOpenFolder() {
    std::wstring config_path = config::ConfigManager::getConfigPath();
    if (!config_path.empty()) {
        utils::fs::path folder = utils::fs::path(config_path).parent_path();
        ShellExecute(hwnd_, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void MainDialog::OnOK() {
    SaveConfiguration();
    EndDialog(hwnd_, IDOK);
}

void MainDialog::OnCancel() {
    EndDialog(hwnd_, IDCANCEL);
}
}
}
