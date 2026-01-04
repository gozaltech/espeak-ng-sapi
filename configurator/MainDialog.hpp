#ifndef ESPEAK_SAPI_MAIN_DIALOG_HPP
#define ESPEAK_SAPI_MAIN_DIALOG_HPP

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "config_manager.hpp"
#include "espeak_wrapper.h"

namespace Espeak {
namespace configurator {

struct VoiceItem {
    std::string identifier;
    std::string display_name;
    bool checked;
};

class MainDialog {
public:
    static INT_PTR Show(HINSTANCE hInstance);

    MainDialog(HWND hwnd, HINSTANCE hInstance);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnInitDialog();
    void OnCommand(WPARAM wParam);
    void OnHScroll(WPARAM wParam, LPARAM lParam);
    void OnNotify(NMHDR* pnmhdr);

    void LoadConfiguration();
    void SaveConfiguration();
    void PopulateVoiceList();
    void PopulateVariantCombo();
    void PopulateProfileList();
    void UpdateIntonationLabel();
    void UpdateWordgapLabel();
    void UpdateUIState();

    void OnSelectAll();
    void OnDeselectAll();
    void OnDefaultOnlyChanged();
    void OnAddProfile();
    void OnRemoveProfile();
    void OnOpenFolder();
    void OnOK();
    void OnCancel();

    HWND hwnd_;
    HINSTANCE hInstance_;
    config::Configuration config_;
    std::vector<VoiceItem> available_voices_;
    std::vector<std::string> available_variants_;

    HWND hCheckDefaultOnly_;
    HWND hListVoices_;
    HWND hComboVariant_;
    HWND hSliderIntonation_;
    HWND hLabelIntonation_;
    HWND hSliderWordgap_;
    HWND hLabelWordgap_;
    HWND hCheckRateboost_;
    HWND hListProfiles_;
    HWND hBtnRemoveProfile_;
};
}
}
#endif
