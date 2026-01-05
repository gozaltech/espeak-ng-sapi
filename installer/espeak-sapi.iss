#define MyAppName "eSpeak-NG SAPI"
#define MyAppVersion "1.2.1"
#define MyAppPublisher "gozaltech"
#define MyAppURL "https://gozaltech.org"

[Setup]
AppId={{A7E9C2D5-F4B8-4E3A-9D6F-1C8B5E2A7F9D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\espeak-ng-sapi
DisableWelcomePage=yes
DisableDirPage=yes
DisableReadyPage=yes
DisableProgramGroupPage=yes
OutputDir=..\output
OutputBaseFilename=EspeakSAPI_Setup_{#MyAppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={autopf}\espeak-ng-sapi\EspeakSAPI.dll

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
english.DonationTitle=Support eSpeak-NG SAPI
english.DonationSubtitle=Help us continue developing this SAPI wrapper
english.DonationMessage=Thank you for installing eSpeak-NG SAPI5 Wrapper!%n%nThis SAPI wrapper is developed and maintained by gozaltech. Your support helps us continue improving the wrapper and adding new features.%n%nIf you find this software useful, please consider supporting the development:
english.DonatePayPal=Donate via PayPal
english.DonateYooMoney=Donate via YooMoney
english.SberbankLabel=Or via Sberbank card number:

russian.DonationTitle=Поддержите eSpeak-NG SAPI
russian.DonationSubtitle=Помогите нам продолжать разработку этой SAPI-оболочки
russian.DonationMessage=Спасибо за установку SAPI-оболочки eSpeak-NG!%n%nЭта SAPI-оболочка разрабатывается и поддерживается gozaltech. Ваша поддержка помогает нам улучшать оболочку и добавлять новые функции.%n%nЕсли это программное обеспечение полезно для вас, пожалуйста, поддержите разработку:
russian.DonatePayPal=Пожертвовать через PayPal
russian.DonateYooMoney=Пожертвовать через YooMoney
russian.SberbankLabel=Или на карту Сбербанка:

[Files]
Source: "..\output\x86\EspeakSAPI.dll"; DestDir: "{autopf32}\espeak-ng-sapi"; Flags: ignoreversion regserver 32bit
Source: "..\output\x86\espeak-ng.dll"; DestDir: "{autopf32}\espeak-ng-sapi"; Flags: ignoreversion 32bit
Source: "..\output\x64\EspeakSAPI.dll"; DestDir: "{autopf}\espeak-ng-sapi"; Flags: ignoreversion regserver; Check: Is64BitInstallMode
Source: "..\output\x64\espeak-ng.dll"; DestDir: "{autopf}\espeak-ng-sapi"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\output\x64\EspeakSAPIConfig.exe"; DestDir: "{autopf}\espeak-ng-sapi"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "..\output\x86\EspeakSAPIConfig.exe"; DestDir: "{autopf32}\espeak-ng-sapi"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "..\output\espeak-ng-data\*"; DestDir: "{commonappdata}\espeak-ng-sapi\data"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start Menu shortcut - points to appropriate configurator based on system architecture
Name: "{autoprograms}\eSpeak-NG SAPI Configurator"; Filename: "{autopf}\espeak-ng-sapi\EspeakSAPIConfig.exe"; IconFilename: "{autopf}\espeak-ng-sapi\EspeakSAPIConfig.exe"; Check: Is64BitInstallMode
Name: "{autoprograms}\eSpeak-NG SAPI Configurator"; Filename: "{autopf32}\espeak-ng-sapi\EspeakSAPIConfig.exe"; IconFilename: "{autopf32}\espeak-ng-sapi\EspeakSAPIConfig.exe"; Check: not Is64BitInstallMode

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\TokenEnums\eSpeak-NG"; Flags: uninsdeletekey; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\TokenEnums\eSpeak-NG"; ValueType: string; ValueName: ""; ValueData: "eSpeak-NG Voices"; Check: Is64BitInstallMode
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Speech\Voices\TokenEnums\eSpeak-NG"; ValueType: string; ValueName: "CLSID"; ValueData: "{{8F2D6C4B-1E9A-4F3D-A2B8-5C7E9D3A6F1B}}"; Check: Is64BitInstallMode
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\TokenEnums\eSpeak-NG"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\TokenEnums\eSpeak-NG"; ValueType: string; ValueName: ""; ValueData: "eSpeak-NG Voices"
Root: HKLM32; Subkey: "SOFTWARE\Microsoft\Speech\Voices\TokenEnums\eSpeak-NG"; ValueType: string; ValueName: "CLSID"; ValueData: "{{8F2D6C4B-1E9A-4F3D-A2B8-5C7E9D3A6F1B}}"

[Run]
Filename: "regsvr32.exe"; Parameters: "/s ""{autopf}\espeak-ng-sapi\EspeakSAPI.dll"""; Flags: runhidden; Check: Is64BitInstallMode; Description: "Register 64-bit SAPI engine"
Filename: "regsvr32.exe"; Parameters: "/s ""{autopf32}\espeak-ng-sapi\EspeakSAPI.dll"""; Flags: runhidden; Description: "Register 32-bit SAPI engine"

[UninstallRun]
Filename: "regsvr32.exe"; Parameters: "/u /s ""{autopf}\espeak-ng-sapi\EspeakSAPI.dll"""; Flags: runhidden; Check: Is64BitInstallMode
Filename: "regsvr32.exe"; Parameters: "/u /s ""{autopf32}\espeak-ng-sapi\EspeakSAPI.dll"""; Flags: runhidden

[Code]
var
  DonationPage: TWizardPage;
  DonationLabel: TNewStaticText;
  PayPalButton: TNewButton;
  YooMoneyButton: TNewButton;
  SberbankLabel: TNewStaticText;
  SberbankEdit: TEdit;

procedure PayPalButtonClick(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('open', 'https://paypal.me/gozaltech', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
end;

procedure YooMoneyButtonClick(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('open', 'https://yoomoney.ru/to/4100117727255296', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
end;

procedure UpdateDonationPageLanguage();
var
  IsRussian: Boolean;
begin
  IsRussian := ActiveLanguage = 'russian';

  DonationPage.Caption := CustomMessage('DonationTitle');
  DonationPage.Description := CustomMessage('DonationSubtitle');
  DonationLabel.Caption := CustomMessage('DonationMessage');
  PayPalButton.Caption := CustomMessage('DonatePayPal');

  YooMoneyButton.Visible := IsRussian;
  SberbankLabel.Visible := IsRussian;
  SberbankEdit.Visible := IsRussian;

  if IsRussian then
  begin
    YooMoneyButton.Caption := CustomMessage('DonateYooMoney');
    SberbankLabel.Caption := CustomMessage('SberbankLabel');
  end;
end;

procedure InitializeWizard();
begin
  DonationPage := CreateCustomPage(wpSelectDir, 'Support eSpeak-NG', 'Help us continue developing this free text-to-speech engine');

  DonationLabel := TNewStaticText.Create(DonationPage);
  DonationLabel.Parent := DonationPage.Surface;
  DonationLabel.Left := 0;
  DonationLabel.Top := 20;
  DonationLabel.Width := DonationPage.SurfaceWidth;
  DonationLabel.Height := 120;
  DonationLabel.WordWrap := True;

  PayPalButton := TNewButton.Create(DonationPage);
  PayPalButton.Parent := DonationPage.Surface;
  PayPalButton.Left := 0;
  PayPalButton.Top := 160;
  PayPalButton.Width := 200;
  PayPalButton.Height := 35;
  PayPalButton.OnClick := @PayPalButtonClick;

  YooMoneyButton := TNewButton.Create(DonationPage);
  YooMoneyButton.Parent := DonationPage.Surface;
  YooMoneyButton.Left := PayPalButton.Left + PayPalButton.Width + 10;
  YooMoneyButton.Top := 160;
  YooMoneyButton.Width := 200;
  YooMoneyButton.Height := 35;
  YooMoneyButton.OnClick := @YooMoneyButtonClick;
  YooMoneyButton.Visible := False;

  SberbankEdit := TEdit.Create(DonationPage);
  SberbankEdit.Parent := DonationPage.Surface;
  SberbankEdit.Left := 0;
  SberbankEdit.Top := 220;
  SberbankEdit.Width := 200;
  SberbankEdit.Height := 25;
  SberbankEdit.Text := '2202203670189333';
  SberbankEdit.ReadOnly := True;
  SberbankEdit.Color := clBtnFace;
  SberbankEdit.Visible := False;

  SberbankLabel := TNewStaticText.Create(DonationPage);
  SberbankLabel.Parent := DonationPage.Surface;
  SberbankLabel.Left := 210;
  SberbankLabel.Top := 220;
  SberbankLabel.AutoSize := True;
  SberbankLabel.Visible := False;

  SberbankLabel.FocusControl := SberbankEdit;

  UpdateDonationPageLanguage();
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = DonationPage.ID then
  begin
    UpdateDonationPageLanguage();
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
end;
