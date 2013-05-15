;
; Install script for Colorer Far plugin
;
; Version should be passed via /Dversion=x.xx
; Beta status should be passed via /Dbeta=" Beta"
; Date is passed via /Ddate=(xxxx-xx-xx)
; Outfile could also be passed as /Doutfile=xxxxxxxx.xxx
; Directory to add files is added as /Drootdir=xxx/xxx
;
; Placed into the Public Domain by:
;   anatoly techtonik <techtoni@gmail.com>
;
; ---[processing command line parameters]
;
; !define VERSION 4.22


!ifndef PLUGIN
  !define PLUGIN "Colorer"
!endif

!ifndef VERSION
  !error "Missing version parameter /Dversion=x.xx"
!endif

!ifndef BETA
  !define BETA ""
!endif

!ifndef DATE
  !define /date DATE "%Y-%m-%d"
!endif

; ---[/command line parameters]

!define NAME "${PLUGIN} ${VERSION}${BETA} Far Plugin"

!ifndef OUTFILE
  OutFile "Far${PLUGIN}-${VERSION}.exe"
!endif
!ifdef OUTFILE
  OutFile "${OUTFILE}"
!endif

Name "${NAME}"
SetCompressor /solid lzma

; AddBrandingImage (left|right|top|bottom) (width|height) [padding]
; SetBrandingImage [/IMGID=item_id_in_dialog] [/RESIZETOFIT] path_to_bitmap_file.bmp

Caption "Colorer Far Plugin ${VERSION}${BETA} ${DATE}"
Icon colorernsis.ico
ShowInstDetails show

VIAddVersionKey "ProductName" "${NAME}"
VIAddVersionKey "Comments" "NSIS scripting by techtonik"
VIAddVersionKey "CompanyName" "http://colorer.sf.net"
VIAddVersionKey "LegalCopyright" "(c) Igor Russkih and co."
VIAddVersionKey "FileDescription" "${PLUGIN} Far Plugin"
VIAddVersionKey "FileVersion" "${VERSION}${BETA}"

VIProductVersion "${VERSION}"


!include "MUI.nsh"

!define MUI_ICON "colorernsis.ico"
;!define MUI_UI "shelf/sdbarker_mod.exe"
!define MUI_UI "${NSISDIR}\Contrib\UIs\sdbarker_tiny.exe"
!define MUI_INSTFILESPAGE_COLORS "FFFFFF 000000"

!define MUI_WELCOMEFINISHPAGE_INI "makeadist.ini"
!define MUI_WELCOMEPAGE_TITLE "Colorer Far Manager Plugin Installer"
;!define MUI_WELCOMEPAGE_TITLE_3LINES "xxx"
;!define MUI_WELCOMEPAGE_TEXT "Syntax highlighting plugin for Far Manager originally developed by Igor Russkih and now maintained by a team of contributors as an open source project at:\n\nhttp://colorer.sf.net"
!define MUI_WELCOMEPAGE_TEXT "Syntax highlighting plugin for Far Manager originally developed by Igor Russkih and now maintained by a team of contributors."


!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES


!insertmacro MUI_LANGUAGE "English"

DirText "Setup will install ${NAME} in the following folder.$\rClick install to start the installation."


!include "FileFunc.nsh"

Function .onInit
  # Detecting Far2 directory
  ReadRegStr $INSTDIR HKCU Software\Far2 InstallDir
  IfErrors TryFar2HKLM
  ${DirState} "$INSTDIR" $R0    # -1 if no directory
  IntCmp $R0 -1 0 0 AddSuffix

TryFar2HKLM:
  ReadRegStr $INSTDIR HKLM Software\Far2 InstallDir
  IfErrors TryFar2Path
  ${DirState} "$INSTDIR" $R0    # -1 if no directory
  IntCmp $R0 -1 0 0 AddSuffix

TryFar2Path:
  StrCpy $INSTDIR "$PROGRAMFILES\Far2"
  ${DirState} "$INSTDIR" $R0    # -1 if no directory
  IntCmp $R0 -1 0 0 AddSuffix

  # Trying Far 1.x
  ReadRegStr $INSTDIR HKCU Software\Far InstallDir
  IfErrors TryFarHKLM
  ${DirState} "$INSTDIR" $R0    # -1 if no directory
  IntCmp $R0 -1 0 0 AddSuffix

TryFarHKLM:
  ReadRegStr $INSTDIR HKLM Software\Far InstallDir
  IfErrors TryFarPath
  ${DirState} "$INSTDIR" $R0    # -1 if no directory
  IntCmp $R0 -1 0 0 AddSuffix

TryFarPath:
  StrCpy $INSTDIR "$PROGRAMFILES\Far"
  ${DirState} "$INSTDIR" $R0    # -1 if no directory
  IntCmp $R0 -1 0 0 AddSuffix

  StrCpy $INSTDIR "$PROGRAMFILES\Far2"

  AddSuffix:
    StrCpy $INSTDIR "$INSTDIR\Plugins\${PLUGIN}"
FunctionEnd

Section "Install Section"
  !ifndef ROOTDIR
    !define ROOTDIR "colorer"
  !endif
  SetOutPath $INSTDIR
  File /r "${ROOTDIR}\*.*"
SectionEnd
