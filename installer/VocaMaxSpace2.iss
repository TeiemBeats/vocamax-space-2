; VocaMax Space 2 - Windows 10/11 installer (Inno Setup 6)
; By Mark33s Beats Lab
;
; Build the plugin first (see README.md), then compile this script with ISCC.
; Source paths are relative to this .iss file.

#define MyAppName "VocaMax Space 2"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "Mark33s Beats Lab"
#define MyVst3Bundle "..\build\VocaMaxSpace2_artefacts\Release\VST3\VocaMax Space 2.vst3"
#define MyStandaloneExe "..\build\VocaMaxSpace2_artefacts\Release\Standalone\VocaMax Space 2.exe"

[Setup]
AppId={{6F3B2C7A-9E4D-4A18-9C2E-0B7D5A1C3E88}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=VocaMaxSpace2_Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
RestartApplications=no
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\VocaMax Space 2.exe

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
; VST3 bundle -> C:\Program Files\Common Files\VST3
Source: "{#MyVst3Bundle}\*"; DestDir: "{commoncf}\VST3\VocaMax Space 2.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone app (works without a DAW)
Source: "{#MyStandaloneExe}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\VocaMax Space 2.exe"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\VocaMax Space 2.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\VocaMax Space 2.exe"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
// Warn (but do not block) if a DAW is open, because it may hold the plugin DLL.
function InitializeSetup(): Boolean;
begin
  Result := True;

  // Never block silent / automated installs.
  if WizardSilent then
    exit;

  if MsgBox('Close your DAW before installing so it can pick up the new plugin. Continue anyway?', mbConfirmation, MB_YESNO) = IDNO then
    Result := False;
end;
