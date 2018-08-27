
#define MyAppName "Miosix Toolchain"
#define MyAppVersion "GCC 4.7.3r001"
#define MyAppURL "http://miosix.org"
#define MyAppGUID "{{5270879A-9707-4BCB-930F-2FC7B5621061}"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
AppId={#MyAppGUID}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName=C:\arm-miosix-eabi
; Forcefully install in this directory (GCC hates having spaces in the path)
DisableDirPage=yes
DefaultGroupName={#MyAppName}
; Allow user to disable adding stuff to the start menu
AllowNoIcons=yes
; Produce an installer named MiosixToolchainInstaller.exe
OutputBaseFilename=MiosixToolchainInstaller
Compression=lzma
; Compress everything into one lzma stream
SolidCompression=yes
LicenseFile=license.txt
; The change in %PATH% takes effect after a restart
AlwaysRestart=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Source is where the InnoSetup compiler finds the directory to install
Source: "..\gcc\arm-miosix-eabi\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
; Add stuff to the start menu
Name: "{group}\QSTlink2"; Filename: "{app}\bin\qstlink2.exe"; WorkingDir: "{userdocs}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

; Add C:\arm-miosix-eabi to %PATH%, found on stackoverflow
; http://stackoverflow.com/questions/3304463/how-do-i-modify-the-path-environment-variable-when-running-an-inno-setup-install

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment";    \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};C:\arm-miosix-eabi\bin"; \
    Check: NeedsAddPath('C:\arm-miosix-eabi\bin')

[Code]

function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKLM,'SYSTEM\CurrentControlSet\Control\Session Manager\Environment','Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  // look for the path with leading and trailing semicolon
  Result := Pos(';' + UpperCase(Param) + ';', ';' + UpperCase(OrigPath) + ';') = 0;
end;

// Make the installer uninstall the previous version
// http://stackoverflow.com/questions/2000296/innosetup-how-to-automatically-uninstall-previous-installed-version

function FindUninstaller(): String;
var
  UnistallerRegKey1: String;
  UnistallerRegKey2: String;
begin
  // The uninstaller path can be in four places, according to stackoverflow
  UnistallerRegKey1 := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\{#emit SetupSetting("AppId")}_is1');
  UnistallerRegKey2 := ExpandConstant('Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{#emit SetupSetting("AppId")}_is1');
  if RegQueryStringValue(HKLM, UnistallerRegKey1, 'UninstallString', Result) then
  begin
    Exit;
  end;
  if RegQueryStringValue(HKCU, UnistallerRegKey1, 'UninstallString', Result) then
  begin
    Exit;
  end;
  if RegQueryStringValue(HKLM, UnistallerRegKey2, 'UninstallString', Result) then
  begin
    Exit;
  end;
  if RegQueryStringValue(HKCU, UnistallerRegKey2, 'UninstallString', Result) then
  begin
    Exit;
  end;
  Result := '';
end;

function InitializeSetup(): Boolean;  
var
  Uninstaller : String;
  ResultCode: Integer;
  i: Integer;
begin
  // FileExists doesn't like quotes
  Uninstaller := RemoveQuotes(FindUninstaller());
  Log('Uninstaller variable is set to "'+Uninstaller+'"');
  if not FileExists(Uninstaller) then
  begin
    Result := True;
    Exit;
  end;
  if MsgBox('A previous version is already installed. Replace it?', mbInformation, MB_YESNO) <> IDYES then
  begin
    Result := False;
    Exit;
  end;
  if Exec(Uninstaller, '/SILENT /NORESTART /SUPPRESSMSGBOXES','', SW_SHOW, ewWaitUntilTerminated, ResultCode) = False then
  begin
    MsgBox('Error: the uninstaller failed', mbError, MB_OK);
    Result := False;
    Exit;
  end;
  // Workaround for "the uninstaller returns before the uninstaller is deleted"
  // http://stackoverflow.com/questions/18902060/disk-caching-issue-with-inno-setup
  i := 0;
  repeat
    Sleep(500);
    i := i + 1;
  until not FileExists(Uninstaller) or (i >= 30);
  if (i >= 30) then
  begin
    MsgBox('Error: the previous uninstaller was not deleted', mbError, MB_OK);
    Result := False;
    Exit;
  end;
  Log('Uninstaller completed');
  Result := True;
end;