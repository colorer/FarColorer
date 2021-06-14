#ifndef _FAREDITORSET_H_
#define _FAREDITORSET_H_

#include <colorer/common/Colorer.h>
#include <colorer/handlers/LineRegionsSupport.h>
#include <colorer/viewer/TextConsoleViewer.h>
#include <spdlog/logger.h>
#include "ChooseTypeMenu.h"
#include "FarEditor.h"
#include "FarHrcSettings.h"
#include "pcolorer.h"

// registry keys
const wchar_t cRegEnabled[] = L"Enabled";
const wchar_t cRegHrdName[] = L"HrdName";
const wchar_t cRegHrdNameTm[] = L"HrdNameTm";
const wchar_t cRegCatalog[] = L"Catalog";
const wchar_t cRegPairsDraw[] = L"PairsDraw";
const wchar_t cRegSyntaxDraw[] = L"SyntaxDraw";
const wchar_t cRegOldOutLine[] = L"OldOutlineView";
const wchar_t cRegTrueMod[] = L"TrueMod";
const wchar_t cRegChangeBgEditor[] = L"ChangeBgEditor";
const wchar_t cRegUserHrdPath[] = L"UserHrdPath";
const wchar_t cRegUserHrcPath[] = L"UserHrcPath";
const wchar_t cRegLogPath[] = L"LogPath";
const wchar_t cRegLogLevel[] = L"LogLevel";
const wchar_t cRegLogEnabled[] = L"LogEnabled";
const wchar_t cRegCrossDraw[] = L"CrossDraw";
const wchar_t cRegCrossStyle[] = L"CrossStyle";
const wchar_t cThreadBuildPeriod[] = L"ThreadBuildPeriod";

// values of registry keys by default
const bool cEnabledDefault = true;
const wchar_t cHrdNameDefault[] = L"default";
const wchar_t cHrdNameTmDefault[] = L"default";
const wchar_t cCatalogDefault[] = L"";
const bool cPairsDrawDefault = true;
const bool cSyntaxDrawDefault = true;
const bool cOldOutLineDefault = true;
const bool cTrueMod = false;
const bool cChangeBgEditor = false;
const wchar_t cUserHrdPathDefault[] = L"";
const wchar_t cUserHrcPathDefault[] = L"";
const wchar_t cLogPathDefault[] = L"";
const wchar_t cLogLevelDefault[] = L"INFO";
const bool cLogEnabledDefault = false;
const int cCrossDrawDefault = 2;
const int cCrossStyleDefault = 3;
const int cThreadBuildPeriodDefault = 200;

const CString DConsole = CString("console");
const CString DRgb = CString("rgb");
const CString Ddefault = CString("<default>");
const CString DAutodetect = CString("autodetect");

enum {
  IDX_CH_BOX,
  IDX_CH_CAPTIONLIST,
  IDX_CH_SCHEMAS,
  IDX_CH_PARAM_LIST,
  IDX_CH_PARAM_VALUE_CAPTION,
  IDX_CH_PARAM_VALUE_LIST,
  IDX_CH_DESCRIPTION,
  IDX_CH_OK,
  IDX_CH_CANCEL
};

enum ERROR_TYPE { ERR_NO_ERROR = 0, ERR_BASE_LOAD = 1, ERR_FARSETTINGS_ERROR = 2 };

LONG_PTR WINAPI SettingDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
LONG_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);

struct Options
{
  int rEnabled;
  int drawPairs;
  int drawSyntax;
  int oldOutline;
  int TrueModOn;
  int ChangeBgEditor;
  int drawCross;
  int CrossStyle;
  int LogEnabled;
  int ThreadBuildPeriod;
  wchar_t HrdName[20];
  wchar_t HrdNameTm[20];
  wchar_t CatalogPath[MAX_PATH];
  wchar_t UserHrdPath[MAX_PATH];
  wchar_t UserHrcPath[MAX_PATH];
  wchar_t LogPath[MAX_PATH];
  wchar_t logLevel[10];
};

struct SettingWindow
{
  int okButtonConfig;
  int catalogEdit;
  int hrcEdit;
  int hrdEdit;
  int hrdCons;
  int hrdTM;
};

class HrcSettingsForm;
/**
 * FAR Editors container.
 * Manages all library resources and creates FarEditor class instances.
 * @ingroup far_plugin
 */
class FarEditorSet
{
  friend HrcSettingsForm;

 public:
  /** Creates set and initialises it with PluginStartupInfo structure */
  FarEditorSet();
  /** Standard destructor */
  ~FarEditorSet();

  /** Shows editor actions menu */
  void openMenu(int MenuId = -1);

  void menuConfigure();
  /** Shows plugin's configuration dialog */
  bool configure();
  /** Views current file with internal viewer */
  void viewFile(const String& path);
  HANDLE openFromMacro(const struct OpenInfo* oInfo);
  HANDLE openFromCommandLine(const struct OpenInfo* oInfo);
  void* execMacro(FARMACROAREA area, OpenMacroInfo* params);

  /** Dispatch editor event in the opened editor */
  int editorEvent(const struct ProcessEditorEventInfo* pInfo);
  /** Dispatch editor input event in the opened editor */
  int editorInput(const INPUT_RECORD& Rec);

  /** Get the description of HRD, or parameter name if description=null */
  const String* getHRDescription(const String& name, const CString& _hrdClass) const;

  /** Reads all registry settings into variables */
  void ReadSettings();
  /**
   * trying to load the database on the specified path
   */
  enum HRC_MODE { HRCM_CONSOLE, HRCM_RGB, HRCM_BOTH };
  bool TestLoadBase(const wchar_t* catalogPath, const wchar_t* userHrdPath, const wchar_t* userHrcPath, const String* hrdCons, const String* hrdTm,
                    const int full, const HRC_MODE hrc_mode);

  bool GetPluginStatus() const
  {
    return Opt.rEnabled;
  }

  bool isEnable() const
  {
    return Opt.rEnabled;
  }

  /** Disables all plugin processing*/
  void disableColorer();
  void enableColorer();

  bool SetBgEditor() const;
  void LoadUserHrd(const String* filename, ParserFactory* pf);
  void LoadUserHrc(const String* filename, ParserFactory* pf);

  /** Shows hrc configuration dialog */
  bool configureHrc(bool call_from_editor);

  /** Show logging configuration dialog*/
  bool configureLogging();

  void showExceptionMessage(const wchar_t* message);
  void applyLogSetting();
  size_t getEditorCount() const;

  SettingWindow settingWindow;

  std::vector<const HRDNode*> hrd_con_instances;
  std::vector<const HRDNode*> hrd_rgb_instances;

 private:
  /** add current active editor and return him. */
  FarEditor* addCurrentEditor();
  /** Returns currently active editor. */
  FarEditor* getCurrentEditor();
  /**
   * Reloads HRC database.
   * Drops all currently opened editors and their
   * internal structures. Prepares to work with newly
   * loaded database. Read settings from registry
   */
  void ReloadBase();

  /** Shows dialog of file type selection */
  bool chooseType();
  /** FAR localized messages */
  const wchar_t* GetMsg(int msg);
  /** Applies the current settings for editors*/
  void ApplySettingsToEditors();
  /** writes settings in the registry*/
  void SaveSettings() const;
  void SaveLogSettings() const;

  /** Kills all currently opened editors*/
  void dropAllEditors(bool clean);
  /** kill the current editor*/
  void dropCurrentEditor(bool clean);

  void setEmptyLogger();
  void addEventTimer();
  void removeEventTimer();

  void* macroSettings(FARMACROAREA area, OpenMacroInfo* params);
  void* macroMenu(FARMACROAREA area, OpenMacroInfo* params);
  void* macroTypes(FARMACROAREA area, OpenMacroInfo* params);
  void* macroBrackets(FARMACROAREA area, OpenMacroInfo* params);
  void* macroRegion(FARMACROAREA area, OpenMacroInfo* params);
  void* macroFunctions(FARMACROAREA area, OpenMacroInfo* params);
  void* macroErrors(FARMACROAREA area, OpenMacroInfo* params);
  void* macroEditor(FARMACROAREA area, OpenMacroInfo* params);
  void* macroParams(FARMACROAREA area, OpenMacroInfo* params);

  void disableColorerInEditor();
  void enableColorerInEditor();
  void FillTypeMenu(ChooseTypeMenu* Menu, FileType* CurFileType) const;
  String* getCurrentFileName();

  int getHrdArrayWithCurrent(const wchar_t* current, std::vector<const HRDNode*>* hrd_instances, std::vector<const wchar_t*>* out_array);
  // filetype "default"
  FileTypeImpl* defaultType;
  std::unordered_map<intptr_t, FarEditor*> farEditorInstances;
  std::unique_ptr<ParserFactory> parserFactory;
  std::unique_ptr<RegionMapper> regionMapper;
  HRCParser* hrcParser;

  /** registry settings */
  Options Opt;

  /** UNC path */
  std::unique_ptr<SString> sCatalogPathExp;
  std::unique_ptr<SString> sUserHrdPathExp;
  std::unique_ptr<SString> sUserHrcPathExp;

  std::unique_ptr<SString> pluginPath;
  int CurrentMenuItem;

  unsigned int err_status;

  std::unique_ptr<Colorer> colorer_lib;
  std::shared_ptr<spdlog::logger> log;

  HANDLE hTimer = NULL;
  HANDLE hTimerQueue = NULL;

  bool ignore_event = false;
};

#endif
