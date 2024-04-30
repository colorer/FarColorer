#include "FarEditorSet.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include <farcolor.hpp>
#include "DlgBuilder.hpp"
#include "HrcSettingsForm.h"
#include "FarHrcSettings.h"
#include "SettingsControl.h"
#include "tools.h"

std::shared_ptr<spdlog::logger> logger;

VOID CALLBACK ColorThread(PVOID lpParam, BOOLEAN TimerOrWaitFired);

FarEditorSet::FarEditorSet()
{
  setEmptyLogger();

  UnicodeString module(Info.ModuleName);
  auto pos = module.lastIndexOf('\\');
  pos = module.lastIndexOf('\\', 0, pos);
  pluginPath = std::make_unique<UnicodeString>(module, 0, pos);

  hTimerQueue = CreateTimerQueue();
  ReloadBase();
}

FarEditorSet::~FarEditorSet()
{
  DeleteTimerQueue(hTimerQueue);
  dropAllEditors(false);
}

VOID CALLBACK ColorThread(PVOID /*lpParam*/, BOOLEAN /*TimerOrWaitFired*/)
{
  Info.AdvControl(&MainGuid, ACTL_SYNCHRO, 0, nullptr);
}

void FarEditorSet::menuConfigure()
{
  struct FarMenuItem shMenu[5] = {};
  shMenu[0].Text = GetMsg(mMainSettings);
  shMenu[1].Text = GetMsg(mUserHrcSetting);
  shMenu[2].Text = GetMsg(mLog);
  shMenu[3].Flags = MIF_SEPARATOR;
  shMenu[4].Text = GetMsg(mReloadAll);
  int menu_id = 0;
  int prev_id = 0;

  while (true) {
    shMenu[prev_id].Flags = MIF_SELECTED;
    menu_id = (int) Info.Menu(&MainGuid, &ConfigMenu, -1, -1, 0, FMENU_AUTOHIGHLIGHT | FMENU_WRAPMODE, GetMsg(mSettings), nullptr, L"settingsmenu",
                              nullptr, nullptr, shMenu, std::size(shMenu));
    switch (menu_id) {
      case -1:
        return;
        break;
      case 0:
        configure();
        break;
      case 1:
        configureHrc(true);
        break;
      case 2:
        configureLogging();
        break;
      case 4:
        TestLoadBase(Opt.CatalogPath, Opt.UserHrdPath, Opt.UserHrcPath, Opt.HrdName, Opt.HrdNameTm, true,
                     Opt.TrueModOn ? HRC_MODE::HRCM_BOTH : HRC_MODE::HRCM_CONSOLE);
        break;
      default:
        return;
        break;
    }
    shMenu[prev_id].Flags ^= MIF_SELECTED;
    prev_id = menu_id;
  }
}

FarEditorSet::MENU_ACTION FarEditorSet::showMenu(bool plugin_enabled, bool editor_enabled)
{
  if (plugin_enabled && editor_enabled) {
    int iMenuItems[] = {mListTypes,         mMatchPair,      mSelectBlock, mSelectPair,      mListFunctions, mFindErrors, mSelectRegion,
                        mCurrentRegionName, mLocateFunction, -1,           mUpdateHighlight, mReloadBase,    mConfigure};
    const size_t menu_size = std::size(iMenuItems);
    FarMenuItem menuElements[menu_size] {};

    menuElements[0].Flags = MIF_SELECTED;

    for (int i = 0; i < menu_size; i++) {
      if (iMenuItems[i] == -1) {
        menuElements[i].Flags |= MIF_SEPARATOR;
      }
      else {
        menuElements[i].Text = GetMsg(iMenuItems[i]);
      }
    }

    intptr_t menu_id =
        Info.Menu(&MainGuid, &PluginMenu, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu", nullptr, nullptr, menuElements, menu_size);
    if (menu_id != -1)
      return static_cast<MENU_ACTION>(menu_id);
  }
  else if (!plugin_enabled) {
    FarMenuItem menuElements[1] {};
    menuElements[0].Flags = MIF_SELECTED;
    menuElements[0].Text = GetMsg(mConfigure);
    intptr_t menu_id = Info.Menu(&MainGuid, &PluginMenu, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu", nullptr, nullptr, menuElements,
                                 std::size(menuElements));
    if (menu_id != -1)
      return MENU_ACTION::CONFIGURE;
  }
  else {
    FarMenuItem menuElements[4] {};
    menuElements[0].Text = GetMsg(mListTypes);
    menuElements[1].Flags |= MIF_SEPARATOR;
    menuElements[2].Text = GetMsg(mReloadBase);
    menuElements[3].Text = GetMsg(mConfigure);
    intptr_t menu_id = Info.Menu(&MainGuid, &PluginMenu, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu", nullptr, nullptr, menuElements,
                                 std::size(menuElements));

    switch (menu_id) {
      case 0:
        return MENU_ACTION::LIST_TYPE;
      case 2:
        return MENU_ACTION::RELOAD_BASE;
      case 3:
        return MENU_ACTION::CONFIGURE;
      default:
        return MENU_ACTION::NO_ACTION;
    }
  }

  return MENU_ACTION::NO_ACTION;
}

void FarEditorSet::execMenuAction(MENU_ACTION action, FarEditor* editor)
{
  if (!editor && action != MENU_ACTION::CONFIGURE) {
    throw Exception("Can't find current editor in array.");
  }
  try {
    switch (action) {
      case MENU_ACTION::LIST_TYPE:
        chooseType();
        break;
      case MENU_ACTION::MATCH_PAIR:
        editor->matchPair();
        break;
      case MENU_ACTION::SELECT_BLOCK:
        editor->selectBlock();
        break;
      case MENU_ACTION::SELECT_PAIR:
        editor->selectPair();
        break;
      case MENU_ACTION::LIST_FUNCTION:
        editor->listFunctions();
        break;
      case MENU_ACTION::FIND_ERROR:
        editor->listErrors();
        break;
      case MENU_ACTION::SELECT_REGION:
        editor->selectRegion();
        break;
      case MENU_ACTION::CURRENT_REGION_NAME:
        editor->getNameCurrentScheme();
        break;
      case MENU_ACTION::LOCATE_FUNCTION:
        editor->locateFunction();
        break;
      case MENU_ACTION::UPDATE_HIGHLIGHT:
        editor->updateHighlighting();
        break;
      case MENU_ACTION::RELOAD_BASE:
        ReloadBase();
        break;
      case MENU_ACTION::CONFIGURE:
        menuConfigure();
        break;
      default:
        break;
    }
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());
    UnicodeString msg("openMenu: ");
    msg.append(e.what());
    showExceptionMessage(&msg);
    disableColorer();
  }
}

void FarEditorSet::openMenu()
{
  FarEditor* editor = getCurrentEditor();

  auto menu_id = showMenu(Opt.rEnabled && editor, editor && editor->isColorerEnable());
  execMenuAction(menu_id, editor);
}

void FarEditorSet::viewFile(const UnicodeString& path)
{
  try {
    if (!Opt.rEnabled) {
      throw Exception("FarColorer is disabled");
    }

    // Creates store of text lines
    TextLinesStore textLinesStore;
    textLinesStore.loadFile(&path, true);
    // Base editor to make primary parse
    BaseEditor baseEditor(parserFactory.get(), &textLinesStore);
    std::unique_ptr<StyledHRDMapper> regionMap;
    try {
      auto hrd_name = UnicodeString(Opt.HrdName);
      regionMap = parserFactory->createStyledMapper(&DConsole, &hrd_name);
    } catch (ParserFactoryException& e) {
      spdlog::error("{0}", e.what());
      regionMap = parserFactory->createStyledMapper(&DConsole, nullptr);
    }
    baseEditor.setRegionMapper(regionMap.get());
    auto type = baseEditor.chooseFileType(&path);
    baseEditor.setFileType(type);

    FileType* def = parserFactory->getHrcLibrary().getFileType(UnicodeString(name_DefaultScheme));
    int maxBlockSize = def->getParamValueInt(UnicodeString(param_MaxBlockSize), 300);
    maxBlockSize = baseEditor.getFileType()->getParamValueInt(UnicodeString(param_MaxBlockSize), maxBlockSize);
    baseEditor.setMaxBlockSize(maxBlockSize);

    // initial event
    baseEditor.lineCountEvent((int) textLinesStore.getLineCount());
    // computing background color
    unsigned int background = 0x1F;
    const StyledRegion* rd = StyledRegion::cast(regionMap->getRegionDefine(UnicodeString(region_DefText)));

    if (rd != nullptr && rd->isForeSet && rd->isBackSet) {
      background = rd->fore + (rd->back << 4);
    }

    // File viewing in console window
    TextConsoleViewer viewer(&baseEditor, &textLinesStore, (unsigned short) background);
    viewer.view();
  } catch (Exception& e) {
    auto error_mes = UnicodeString(e.what());
    showExceptionMessage(&error_mes);
  }
}

void FarEditorSet::FillTypeMenu(ChooseTypeMenu* Menu, FileType* CurFileType) const
{
  UnicodeString group = DAutodetect;
  FileType* type = nullptr;
  HrcLibrary& hrcLibrary= parserFactory->getHrcLibrary();

  if (!CurFileType) {
    Menu->SetSelected(1);
  }

  for (int idx = 0;; idx++) {
    type = hrcLibrary.enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (group.compare(type->getGroup()) != 0) {
      Menu->AddGroup(UStr::to_stdwstr(type->getGroup()).c_str());
      group = type->getGroup();
    }

    size_t i;
    const UnicodeString* v;
    v = dynamic_cast<FileType*>(type)->getParamValue(UnicodeString(param_Favorite));
    if (v && v->compare(UnicodeString(value_True)) == 0) {
      i = Menu->AddFavorite(type);
    }
    else {
      i = Menu->AddItem(type);
    }
    if (type == CurFileType) {
      Menu->SetSelected(i);
    }
  }
}

INT_PTR WINAPI KeyDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  wchar_t wkey[2];

  auto* record = static_cast<INPUT_RECORD*>(Param2);
  if (Msg == DN_CONTROLINPUT && record->EventType == KEY_EVENT) {
    int key = record->Event.KeyEvent.wVirtualKeyCode;
    if (key == VK_ESCAPE || key == VK_RETURN) {
      return FALSE;
    }
    if (key > 31 && key != VK_F1) {
      FSF.FarInputRecordToName(static_cast<const INPUT_RECORD*>(Param2), wkey, 2);
      if (key > 255) {
        wchar_t* c = FSF.XLat(wkey, 0, 1, 0);
        wkey[0] = *c;
        FSF.FarNameToInputRecord(wkey, record);
      }
      key = record->Event.KeyEvent.wVirtualKeyCode;
      if ((key >= 48 && key <= 57) || (key >= 65 && key <= 90)) {
        Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, 2, Upper(wkey));
      }
      return TRUE;
    }
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool FarEditorSet::chooseType()
{
  FarEditor* fe = getCurrentEditor();
  if (!fe) {
    return false;
  }

  ChooseTypeMenu menu(GetMsg(mAutoDetect), GetMsg(mFavorites), GetMsg(mDisable));
  FillTypeMenu(&menu, fe->getFileType());

  wchar_t bottom[20];
  std::wstring key;
  _snwprintf(bottom, 20, GetMsg(mTotalTypes), parserFactory->getHrcLibrary().getFileTypesCount());
  struct FarKey BreakKeys[3] = {VK_INSERT, 0, VK_DELETE, 0, VK_F4, 0};
  intptr_t BreakCode;
  while (true) {
    intptr_t i = Info.Menu(&MainGuid, &FileChooseMenu, -1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT, GetMsg(mSelectSyntax), bottom,
                           L"filetypechoose", BreakKeys, &BreakCode, menu.getItems(), menu.getItemsCount());

    if (i >= 0) {
      if (BreakCode == 0) {
        if (i != 0 && !menu.IsFavorite(i)) {
          auto f = menu.GetFileType(i);
          menu.MoveToFavorites(i);
          addParamAndValue(f, UnicodeString(param_Favorite), UnicodeString(value_True));
        }
        else {
          menu.SetSelected(i);
        }
      }
      else if (BreakCode == 1) {
        if (i != 0 && menu.IsFavorite(i)) {
          menu.DelFromFavorites(i);
        }
        else {
          menu.SetSelected(i);
        }
      }
      else if (BreakCode == 2) {
        if (i == 0) {
          menu.SetSelected(i);
          continue;
        }

        FarDialogItem KeyAssignDlgData[] = {
            {DI_DOUBLEBOX, 3, 1, 30, 4, 0, nullptr, nullptr, 0, GetMsg(mKeyAssignDialogTitle)},
            {DI_TEXT, -1, 2, 0, 2, 0, nullptr, nullptr, 0, GetMsg(mKeyAssignTextTitle)},
            {DI_EDIT, 5, 3, 28, 3, 0, nullptr, nullptr, DIF_FOCUS | DIF_DEFAULTBUTTON, L""},
        };

        const UnicodeString* v;
        v = menu.GetFileType(i)->getParamValue(UnicodeString(param_HotKey));
        if (v && v->length()) {
          key = std::wstring(UStr::to_stdwstr(v));
          KeyAssignDlgData[2].Data = key.c_str();
        }

        HANDLE hDlg = Info.DialogInit(&MainGuid, &AssignKeyDlg, -1, -1, 34, 6, L"keyassign", KeyAssignDlgData, std::size(KeyAssignDlgData), 0, 0,
                                      KeyDialogProc, nullptr);
        intptr_t res = Info.DialogRun(hDlg);

        if (res != -1) {
          KeyAssignDlgData[2].Data =
              static_cast<const wchar_t*>(trim(reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, 2, nullptr))));
          auto ftype = menu.GetFileType(i);
          auto param_name = UnicodeString(param_HotKey);
          auto hotkey = UnicodeString(KeyAssignDlgData[2].Data);
          addParamAndValue(ftype, param_name, hotkey);
          menu.RefreshItemCaption(i);
        }
        menu.SetSelected(i);
        Info.DialogFree(hDlg);
      }
      else {
        if (i == 0) {
          UnicodeString* s = getCurrentFileName();
          fe->chooseFileType(s);
          applySettingsToEditor(fe);
          delete s;
          break;
        }
        fe->setFileType(menu.GetFileType(i));
        applySettingsToEditor(fe);
        break;
      }
    }
    else {
      break;
    }
  }

  FarHrcSettings p(this, parserFactory.get());
  p.writeUserProfile();
  return true;
}

const UnicodeString* FarEditorSet::getHRDescription(const UnicodeString& name, const UnicodeString& _hrdClass) const
{
  const UnicodeString* descr = nullptr;
  if (parserFactory != nullptr) {
    descr = &parserFactory->getHrdNode(_hrdClass, name).hrd_description;
  }

  if (descr == nullptr) {
    descr = &name;
  }

  return descr;
}

INT_PTR WINAPI SettingDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  auto* fes = reinterpret_cast<FarEditorSet*>(Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, nullptr));

  if (DN_BTNCLICK == Msg && fes->settingWindow.okButtonConfig == Param1) {
    const auto* temp = static_cast<const wchar_t*>(
        trim(reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, fes->settingWindow.catalogEdit, nullptr))));
    const auto* userhrd = static_cast<const wchar_t*>(
        trim(reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, fes->settingWindow.hrdEdit, nullptr))));
    const auto* userhrc = static_cast<const wchar_t*>(
        trim(reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, fes->settingWindow.hrcEdit, nullptr))));

    int CurPosCons = (int) Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, fes->settingWindow.hrdCons, nullptr);
    int CurPosTm = (int) Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, fes->settingWindow.hrdTM, nullptr);

    int k = static_cast<int>(Info.SendDlgMessage(hDlg, DM_GETCHECK, fes->settingWindow.turnOff, nullptr));

    if (k == BSTATE_UNCHECKED) {
      // не проверяем настройки у отключенного плагина
      return false;
    }
    return !fes->TestLoadBase(temp, userhrd, userhrc, UStr::to_stdwstr(&fes->hrd_con_instances.at(CurPosCons)->hrd_name).c_str(),
                              UStr::to_stdwstr(&fes->hrd_rgb_instances.at(CurPosTm)->hrd_name).c_str(), false, FarEditorSet::HRC_MODE::HRCM_BOTH);
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool FarEditorSet::configure()
{
  try {
    PluginDialogBuilder Builder(Info, MainGuid, PluginConfig, mSetup, L"config", SettingDialogProc, this);
    Builder.AddCheckbox(mTurnOff, &Opt.rEnabled);
    settingWindow.turnOff = Builder.GetLastID();
    Builder.AddSeparator();
    Builder.AddText(mCatalogFile);
    Builder.AddEditField(Opt.CatalogPath, MAX_PATH, 65, L"catalog");
    settingWindow.catalogEdit = Builder.GetLastID();
    Builder.AddText(mUserHrcFile);
    Builder.AddEditField(Opt.UserHrcPath, MAX_PATH, 65, L"userhrc");
    settingWindow.hrcEdit = Builder.GetLastID();
    Builder.AddText(mUserHrdFile);
    Builder.AddEditField(Opt.UserHrdPath, MAX_PATH, 65, L"userhrd");
    settingWindow.hrdEdit = Builder.GetLastID();
    Builder.AddSeparator(mStyleConf);

    Builder.StartColumns();

    std::vector<const wchar_t*> console_style;
    unsigned long flag_disable = 0;
    int current_style;
    std::unique_ptr<HrdNode> cons, rgb;
    hrd_con_instances.clear();
    if (Opt.rEnabled) {
      hrd_con_instances = parserFactory->enumHrdInstances(DConsole);
      current_style = getHrdArrayWithCurrent(Opt.HrdName, &hrd_con_instances, &console_style);
    }
    else {
      cons = std::make_unique<HrdNode>();
      cons->hrd_name = UnicodeString(Opt.HrdName);
      hrd_con_instances.push_back(cons.get());
      console_style.push_back(Opt.HrdName);
      current_style = 0;
      flag_disable = DIF_DISABLE;
    }
    Builder.AddText(mHRDName);
    Builder.AddComboBox(&current_style, nullptr, 30, console_style.data(), console_style.size(), DIF_LISTWRAPMODE | DIF_DROPDOWNLIST | flag_disable);
    settingWindow.hrdCons = Builder.GetLastID();

    Builder.AddCheckbox(mTrueMod, &Opt.TrueModOn);

    std::vector<const wchar_t*> rgb_style;
    flag_disable = 0;
    int current_rstyle;
    hrd_rgb_instances.clear();
    if (Opt.rEnabled) {
      hrd_rgb_instances = parserFactory->enumHrdInstances(DRgb);
      current_rstyle = getHrdArrayWithCurrent(Opt.HrdNameTm, &hrd_rgb_instances, &rgb_style);
    }
    else {
      rgb = std::make_unique<HrdNode>();
      rgb->hrd_name = UnicodeString(Opt.HrdName);
      hrd_rgb_instances.push_back(rgb.get());
      rgb_style.push_back(Opt.HrdNameTm);
      current_rstyle = 0;
      flag_disable = DIF_DISABLE;
    }
    Builder.AddText(mHRDNameTrueMod);
    Builder.AddComboBox(&current_rstyle, nullptr, 30, rgb_style.data(), rgb_style.size(), DIF_LISTWRAPMODE | DIF_DROPDOWNLIST | flag_disable);
    settingWindow.hrdTM = Builder.GetLastID();

    Builder.ColumnBreak();

    Builder.AddCheckbox(mPairs, &Opt.drawPairs);
    Builder.AddCheckbox(mSyntax, &Opt.drawSyntax);
    Builder.AddCheckbox(mOldOutline, &Opt.oldOutline);
    Builder.AddCheckbox(mChangeBackgroundEditor, &Opt.ChangeBgEditor);

    Builder.AddCheckbox(mCross, &Opt.drawCross, 0, true);
    Builder.AddText(mCrossText);
    const wchar_t* cross_style[] = {GetMsg(mCrossVert), GetMsg(mCrossHoriz), GetMsg(mCrossBoth)};
    int cross_style_id = Opt.CrossStyle - 1;
    Builder.AddComboBox(&cross_style_id, nullptr, 25, cross_style, std::size(cross_style), DIF_LISTWRAPMODE | DIF_DROPDOWNLIST);

    Builder.EndColumns();
    Builder.AddSeparator(mPerfomance);
    auto intbox = Builder.AddIntEditField(&Opt.ThreadBuildPeriod, 6);
    Builder.AddTextBefore(intbox, mBuildPeriod);

    Builder.AddOKCancel(mOk, mCancel);
    settingWindow.okButtonConfig = Builder.GetLastID() - 1;

    if (Builder.ShowDialog()) {
      if (flag_disable == 0) {
        wcsncpy(Opt.HrdName, UStr::to_stdwstr(&hrd_con_instances.at(current_style)->hrd_name).c_str(), std::size(Opt.HrdName));
        wcsncpy(Opt.HrdNameTm, UStr::to_stdwstr(&hrd_rgb_instances.at(current_rstyle)->hrd_name).c_str(), std::size(Opt.HrdNameTm));
      }
      if (cross_style_id + 1 != Opt.CrossStyle)
        Opt.CrossStyle = cross_style_id + 1;
      SaveSettings();
      if (Opt.rEnabled) {
        ReloadBase();
      }
      else {
        disableColorer();
      }
      hrd_rgb_instances.clear();
      hrd_con_instances.clear();
    }

    Info.EditorControl(CurrentEditor, ECTL_REDRAW, 0, nullptr);
    return true;
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());

    UnicodeString msg("configure: ");
    msg.append(UnicodeString(e.what()));
    showExceptionMessage(&msg);
    disableColorer();
    return false;
  }
}

int FarEditorSet::editorInput(const INPUT_RECORD& Rec)
{
  if (ignore_event || Rec.EventType != KEY_EVENT)
    return 0;
  if (Opt.rEnabled) {
    FarEditor* editor = getCurrentEditor();
    if (editor && editor->isColorerEnable()) {
      auto res = editor->editorInput(Rec);
      if (editor->hasWork())
        addEventTimer();
      else
        removeEventTimer();
      return res;
    }
  }
  return 0;
}

int FarEditorSet::editorEvent(const struct ProcessEditorEventInfo* pInfo)
{
  if (ignore_event)
    return 0;
  // check whether all the editors cleaned
  if (!Opt.rEnabled && !farEditorInstances.empty() && pInfo->Event == EE_GOTFOCUS) {
    dropCurrentEditor(true);
    return 0;
  }

  if (!Opt.rEnabled) {
    return 0;
  }

  try {
    FarEditor* editor = nullptr;
    switch (pInfo->Event) {
      case EE_REDRAW: {
        editor = getCurrentEditor();
        if (!editor) {
          editor = addCurrentEditor();
        }
        if (editor && editor->isColorerEnable()) {
          auto res = editor->editorEvent(pInfo->Event, pInfo->Param);
          if (editor->hasWork())
            addEventTimer();
          else
            removeEventTimer();
          return res;
        }
        return 0;
      } break;
      case EE_CHANGE: {
        //запрещено вызывать EditorControl (getCurrentEditor)
        auto it_editor = farEditorInstances.find(pInfo->EditorID);
        if (it_editor != farEditorInstances.end() && it_editor->second->isColorerEnable()) {
          auto res = it_editor->second->editorEvent(pInfo->Event, pInfo->Param);
          if (it_editor->second->hasWork())
            addEventTimer();
          else
            removeEventTimer();
          return res;
        }
        else {
          return 0;
        }
      } break;
      case EE_READ: {
        editor = getCurrentEditor();
        if (!editor) {
          editor = addCurrentEditor();
        }
        return 0;
      } break;
      case EE_CLOSE: {
        auto it_editor = farEditorInstances.find(pInfo->EditorID);
        if (it_editor != farEditorInstances.end()) {
          delete it_editor->second;
          farEditorInstances.erase(pInfo->EditorID);
          removeEventTimer();
        }
        return 0;
      } break;
      case EE_GOTFOCUS: {
        addEventTimer();
        return 0;
      } break;
      case EE_KILLFOCUS: {
        removeEventTimer();
        return 0;
      } break;
      default:
        break;
    }
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());

    UnicodeString msg("editorEvent: ");
    msg.append(UnicodeString(e.what()));
    showExceptionMessage(&msg);
    disableColorer();
  }

  return 0;
}

bool FarEditorSet::TestLoadBase(const wchar_t* catalogPath, const wchar_t* userHrdPath, const wchar_t* userHrcPath, const wchar_t* hrdCons,
                                const wchar_t* hrdTm, const bool full, const HRC_MODE hrc_mode)
{
  bool res = true;
  const wchar_t* marr[2] = {GetMsg(mName), GetMsg(mReloading)};
  Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], 2, 0);

  std::unique_ptr<ParserFactory> parserFactoryLocal = nullptr;
  std::unique_ptr<RegionMapper> regionMapperLocal = nullptr;

  std::unique_ptr<UnicodeString> catalogPathS(PathToFullS(catalogPath, false));
  std::unique_ptr<UnicodeString> userHrdPathS(PathToFullS(userHrdPath, false));
  std::unique_ptr<UnicodeString> userHrcPathS(PathToFullS(userHrcPath, false));

  std::unique_ptr<UnicodeString> tpath;
  if (!catalogPathS || !catalogPathS->length()) {
    auto* path = new UnicodeString(*pluginPath);
    path->append(UnicodeString(FarCatalogXml));
    tpath.reset(path);
  }
  else {
    tpath = std::move(catalogPathS);
  }

  try {
    parserFactoryLocal = std::make_unique<ParserFactory>();
    parserFactoryLocal->loadCatalog(tpath.get());
    auto& hrcLibraryLocal = parserFactoryLocal->getHrcLibrary();
    auto def_type = hrcLibraryLocal.getFileType(UnicodeString(name_DefaultScheme));
    FarHrcSettings p(this, parserFactoryLocal.get());
    p.loadUserHrd(userHrdPathS.get());
    p.loadUserHrc(userHrdPathS.get());
    p.readPluginHrcSettings(pluginPath.get());
    p.readUserProfile(def_type);

    if (hrc_mode == HRC_MODE::HRCM_CONSOLE || hrc_mode == HRC_MODE::HRCM_BOTH) {
      try {
        auto uhrdCons = UnicodeString(hrdCons);
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, &uhrdCons);
      } catch (ParserFactoryException& e) {
        spdlog::error("{0}", e.what());
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, nullptr);
      }
    }

    if (hrc_mode == HRC_MODE::HRCM_RGB || hrc_mode == HRC_MODE::HRCM_BOTH) {
      try {
        auto uhrdTm = UnicodeString(hrdTm);
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, &uhrdTm);
      } catch (ParserFactoryException& e) {
        spdlog::error("{0}", e.what());
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, nullptr);
      }
    }

    if (full) {
      for (int idx = 0;; idx++) {
        FileType* type = hrcLibraryLocal.enumerateFileTypes(idx);

        if (type == nullptr) {
          break;
        }

        UnicodeString tname;

        if (type->getGroup() != nullptr) {
          tname.append(type->getGroup());
          tname.append(": ");
        }

        tname.append(type->getDescription());
        std::wstring str_message = UStr::to_stdwstr(&tname);
        marr[1] = str_message.c_str();
        Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], 2, 0);
        if (idx % 5 == 0)
          Info.EditorControl(-1, ECTL_REDRAW, 0, nullptr);
        type->getBaseScheme();
      }
    }
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());
    auto error_mes = UnicodeString(e.what());
    showExceptionMessage(&error_mes);
    res = false;
  }

  return res;
}

void FarEditorSet::ReloadBase()
{
  ignore_event = true;
  try {
    removeEventTimer();
    ReadSettings();
    applyLogSetting();
    if (!Opt.rEnabled) {
      return;
    }

    const wchar_t* marr[2] = {GetMsg(mName), GetMsg(mReloading)};
    Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], std::size(marr), 0);
    dropAllEditors(true);
    regionMapper.reset();
    parserFactory.reset();

    UnicodeString hrdClass;
    UnicodeString hrdName;

    if (Opt.TrueModOn) {
      hrdClass = DRgb;
      hrdName = UnicodeString(Opt.HrdNameTm);
    }
    else {
      hrdClass = DConsole;
      hrdName = UnicodeString(Opt.HrdName);
    }

    parserFactory = std::make_unique<ParserFactory>();
    parserFactory->loadCatalog(sCatalogPathExp.get());
    HrcLibrary& hrcLibrary = parserFactory->getHrcLibrary();
    defaultType = hrcLibrary.getFileType(UnicodeString(name_DefaultScheme));
    FarHrcSettings p(this, parserFactory.get());
    p.loadUserHrd(sUserHrdPathExp.get());
    p.loadUserHrc(sUserHrcPathExp.get());
    p.readPluginHrcSettings(pluginPath.get());
    p.readUserProfile();

    try {
      regionMapper = parserFactory->createStyledMapper(&hrdClass, &hrdName);
    } catch (ParserFactoryException& e) {
      spdlog::error("{0}", e.what());
      regionMapper = parserFactory->createStyledMapper(&hrdClass, nullptr);
    }
    //устанавливаем фон редактора при каждой перезагрузке схем.
    SetBgEditor();
    addEventTimer();
  } catch (SettingsControlException& e) {
    spdlog::error("{0}", e.what());
    auto error_mes = UnicodeString(e.what());
    showExceptionMessage(&error_mes);
    err_status = ERR_FARSETTINGS_ERROR;
    disableColorer();
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());
    auto error_mes = UnicodeString(e.what());
    showExceptionMessage(&error_mes);
    err_status = ERR_BASE_LOAD;
    disableColorer();
  }
  ignore_event = false;
}

size_t FarEditorSet::getEditorCount() const
{
  return farEditorInstances.size();
}

FarEditor* FarEditorSet::addCurrentEditor()
{
  EditorInfo ei {sizeof(EditorInfo)};
  if (!Info.EditorControl(CurrentEditor, ECTL_GETINFO, 0, &ei)) {
    return nullptr;
  }

  UnicodeString* s = getCurrentFileName();
  auto* editor = new FarEditor(&Info, parserFactory.get(), s);
  delete s;
  std::pair<intptr_t, FarEditor*> pair_editor(ei.EditorID, editor);
  farEditorInstances.emplace(pair_editor);
  applySettingsToEditor(editor);
  return editor;
}

UnicodeString* FarEditorSet::getCurrentFileName()
{
  LPWSTR FileName = nullptr;
  size_t FileNameSize = Info.EditorControl(CurrentEditor, ECTL_GETFILENAME, 0, nullptr);

  if (FileNameSize) {
    FileName = new wchar_t[FileNameSize];
    Info.EditorControl(CurrentEditor, ECTL_GETFILENAME, FileNameSize, FileName);
  }

  UnicodeString fnpath(FileName);
  auto slash_idx = fnpath.lastIndexOf('\\');

  if (slash_idx == -1) {
    slash_idx = fnpath.lastIndexOf('/');
  }
  auto* s = new UnicodeString(fnpath, slash_idx + 1);
  delete[] FileName;
  return s;
}

FarEditor* FarEditorSet::getCurrentEditor()
{
  EditorInfo ei {sizeof(EditorInfo)};
  if (!Info.EditorControl(CurrentEditor, ECTL_GETINFO, 0, &ei)) {
    return nullptr;
  }
  auto if_editor = farEditorInstances.find(ei.EditorID);
  if (if_editor != farEditorInstances.end()) {
    return if_editor->second;
  }
  else {
    return nullptr;
  }
}

const wchar_t* FarEditorSet::GetMsg(int msg)
{
  return (Info.GetMsg(&MainGuid, msg));
}

void FarEditorSet::disableColorer()
{
  Opt.rEnabled = 0;
  if (!(err_status & ERR_FARSETTINGS_ERROR)) {
    SettingsControl ColorerSettings;
    ColorerSettings.Set(0, cRegEnabled, Opt.rEnabled);
  }
  removeEventTimer();

  dropCurrentEditor(true);

  regionMapper.reset();
  parserFactory.reset();
}

void FarEditorSet::enableColorer()
{
  Opt.rEnabled = 1;
  SaveSettings();
  ReloadBase();
}

void FarEditorSet::applySettingsToEditor(FarEditor* editor)
{
  if (editor->isColorerEnable()) {
    editor->setTrueMod(Opt.TrueModOn);
    editor->setRegionMapper(regionMapper.get());
    editor->setDrawPairs(Opt.drawPairs);
    editor->setDrawSyntax(Opt.drawSyntax);
    editor->setOutlineStyle(Opt.oldOutline);
    editor->setCrossState(Opt.drawCross, Opt.CrossStyle);
  }
}

void FarEditorSet::dropCurrentEditor(bool clean)
{
  EditorInfo ei {sizeof(EditorInfo)};
  Info.EditorControl(CurrentEditor, ECTL_GETINFO, 0, &ei);
  auto it_editor = farEditorInstances.find(ei.EditorID);
  if (it_editor != farEditorInstances.end()) {
    if (clean) {
      it_editor->second->cleanEditor();
    }
    delete it_editor->second;
    farEditorInstances.erase(ei.EditorID);
    ignore_event = true;
    Info.EditorControl(CurrentEditor, ECTL_REDRAW, 0, nullptr);
    ignore_event = false;
  }
}

void FarEditorSet::dropAllEditors(bool clean)
{
  if (clean) {
    //мы не имеем доступа к другим редакторам, кроме текущего
    dropCurrentEditor(clean);
  }
  farEditorInstances.clear();
}

void FarEditorSet::ReadSettings()
{
  SettingsControl ColorerSettings;

  ColorerSettings.Get(0, cRegHrdName, Opt.HrdName, std::size(Opt.HrdName), cHrdNameDefault);
  ColorerSettings.Get(0, cRegHrdNameTm, Opt.HrdNameTm, std::size(Opt.HrdNameTm), cHrdNameTmDefault);
  ColorerSettings.Get(0, cRegCatalog, Opt.CatalogPath, std::size(Opt.CatalogPath), cCatalogDefault);
  ColorerSettings.Get(0, cRegUserHrcPath, Opt.UserHrcPath, std::size(Opt.UserHrcPath), cUserHrcPathDefault);
  ColorerSettings.Get(0, cRegUserHrdPath, Opt.UserHrdPath, std::size(Opt.UserHrdPath), cUserHrdPathDefault);
  ColorerSettings.Get(0, cRegLogPath, Opt.LogPath, std::size(Opt.LogPath), cLogPathDefault);
  ColorerSettings.Get(0, cRegLogLevel, Opt.logLevel, std::size(Opt.LogPath), cLogLevelDefault);

  sCatalogPathExp.reset(PathToFullS(Opt.CatalogPath, false));
  if (!sCatalogPathExp || !sCatalogPathExp->length()) {
    auto* path = new UnicodeString(*pluginPath);
    path->append(UnicodeString(FarCatalogXml));
    sCatalogPathExp.reset(path);
  }
  sUserHrdPathExp.reset(PathToFullS(Opt.UserHrdPath, false));
  sUserHrcPathExp.reset(PathToFullS(Opt.UserHrcPath, false));

  Opt.rEnabled = ColorerSettings.Get(0, cRegEnabled, cEnabledDefault);
  Opt.drawPairs = ColorerSettings.Get(0, cRegPairsDraw, cPairsDrawDefault);
  Opt.drawSyntax = ColorerSettings.Get(0, cRegSyntaxDraw, cSyntaxDrawDefault);
  Opt.oldOutline = ColorerSettings.Get(0, cRegOldOutLine, cOldOutLineDefault);
  Opt.TrueModOn = ColorerSettings.Get(0, cRegTrueMod, cTrueMod);
  Opt.ChangeBgEditor = ColorerSettings.Get(0, cRegChangeBgEditor, cChangeBgEditor);
  Opt.LogEnabled = ColorerSettings.Get(0, cRegLogEnabled, cLogEnabledDefault);
  Opt.drawCross = ColorerSettings.Get(0, cRegCrossDraw, cCrossDrawDefault);
  Opt.CrossStyle = ColorerSettings.Get(0, cRegCrossStyle, cCrossStyleDefault);
  Opt.ThreadBuildPeriod = ColorerSettings.Get(0, cThreadBuildPeriod, cThreadBuildPeriodDefault);
}

void FarEditorSet::applyLogSetting()
{
  if (Opt.LogEnabled) {
    auto u_loglevel = UnicodeString(Opt.logLevel);
    auto level = spdlog::level::from_str(UStr::to_stdstr(&u_loglevel));
    if (level != spdlog::level::off) {
      try {
        std::string file_name = "farcolorer.log";
        if (Opt.LogPath[0] != '\0') {
          UnicodeString sLogPathExp(*PathToFullS(Opt.LogPath, false));
          file_name = std::string(UStr::to_stdstr(&sLogPathExp)).append("\\").append(file_name);
        }
        spdlog::drop_all();
        logger = spdlog::basic_logger_mt("main", file_name);
        spdlog::set_default_logger(logger);
        logger->set_level(level);
      } catch (std::exception& e) {
        setEmptyLogger();
        auto error_mes = UnicodeString(e.what());
        showExceptionMessage(&error_mes);
      }
    }
  }
  else {
    setEmptyLogger();
  }
}

void FarEditorSet::SaveSettings() const
{
  SettingsControl ColorerSettings;
  ColorerSettings.Set(0, cRegEnabled, Opt.rEnabled);
  ColorerSettings.Set(0, cRegHrdName, Opt.HrdName);
  ColorerSettings.Set(0, cRegHrdNameTm, Opt.HrdNameTm);
  ColorerSettings.Set(0, cRegCatalog, Opt.CatalogPath);
  ColorerSettings.Set(0, cRegPairsDraw, Opt.drawPairs);
  ColorerSettings.Set(0, cRegSyntaxDraw, Opt.drawSyntax);
  ColorerSettings.Set(0, cRegOldOutLine, Opt.oldOutline);
  ColorerSettings.Set(0, cRegTrueMod, Opt.TrueModOn);
  ColorerSettings.Set(0, cRegChangeBgEditor, Opt.ChangeBgEditor);
  ColorerSettings.Set(0, cRegUserHrdPath, Opt.UserHrdPath);
  ColorerSettings.Set(0, cRegUserHrcPath, Opt.UserHrcPath);
  ColorerSettings.Set(0, cRegCrossDraw, Opt.drawCross);
  ColorerSettings.Set(0, cRegCrossStyle, Opt.CrossStyle);
  ColorerSettings.Set(0, cThreadBuildPeriod, Opt.ThreadBuildPeriod);
}

void FarEditorSet::SaveLogSettings() const
{
  SettingsControl ColorerSettings;
  ColorerSettings.Set(0, cRegLogPath, Opt.LogPath);
  ColorerSettings.Set(0, cRegLogLevel, Opt.logLevel);
  ColorerSettings.Set(0, cRegLogEnabled, Opt.LogEnabled);
}

bool FarEditorSet::SetBgEditor() const
{
  if (Opt.rEnabled && Opt.ChangeBgEditor) {
    const StyledRegion* def_text = StyledRegion::cast(regionMapper->getRegionDefine(UnicodeString(region_DefText)));

    FarColor fc {};
    if (Opt.TrueModOn) {
      fc.Flags = 0;
      fc.BackgroundColor = revertRGB(def_text->back);
      fc.ForegroundColor = revertRGB(def_text->fore);
      fc.BackgroundRGBA.a = 0xFF;
      fc.ForegroundRGBA.a = 0xFF;
    }
    else {
      fc.Flags = FCF_4BITMASK;
      fc.BackgroundColor = def_text->back;
      fc.ForegroundColor = def_text->fore;
    }
    FarSetColors fsc {sizeof(FarSetColors), FSETCLR_REDRAW, COL_EDITORTEXT, 1, &fc};
    return Info.AdvControl(&MainGuid, ACTL_SETARRAYCOLOR, 0, &fsc) != 0;
  }
  return false;
}

bool FarEditorSet::configureHrc(bool call_from_editor)
{
  if (!Opt.rEnabled) {
    return false;
  }

  FileType* ft = defaultType;
  if (call_from_editor) {
    auto* editor = getCurrentEditor();
    if (editor && editor->isColorerEnable())
      ft = editor->getFileType();
  }
  HrcSettingsForm form(this, ft);
  return form.Show();
}

void FarEditorSet::showExceptionMessage(const UnicodeString* message)
{
  auto str_mes = UStr::to_stdwstr(message);
  const wchar_t* exceptionMessage[3] = {GetMsg(mName), str_mes.c_str(), GetMsg(mDie)};
  Info.Message(&MainGuid, &ErrorMessage, FMSG_WARNING, L"exception", &exceptionMessage[0], std::size(exceptionMessage), 1);
}

bool FarEditorSet::configureLogging()
{
  const wchar_t* levelList[] = {L"error", L"warning", L"info", L"debug"};
  const auto level_count = std::size(levelList);

  int log_level = 0;

  for (size_t i = 0; i < level_count; ++i) {
    if (UnicodeString(levelList[i]) == UnicodeString(Opt.logLevel)) {
      log_level = (int) i;
      break;
    }
  }

  PluginDialogBuilder Builder(Info, MainGuid, LoggingConfig, mLogging, L"configlog");
  Builder.AddCheckbox(mLogTurnOff, &Opt.LogEnabled);
  Builder.AddSeparator();
  FarDialogItem* box = Builder.AddComboBox(&log_level, Opt.logLevel, 10, levelList, level_count, DIF_LISTWRAPMODE | DIF_DROPDOWNLIST);
  Builder.AddTextBefore(box, mLogLevel);
  Builder.AddText(mLogPath);
  Builder.AddEditField(Opt.LogPath, MAX_PATH, 28, L"logpath");
  Builder.AddOKCancel(mOk, mCancel);

  if (Builder.ShowDialog()) {
    SaveLogSettings();
    applyLogSetting();
  }

  return true;
}

HANDLE FarEditorSet::openFromCommandLine(const struct OpenInfo* oInfo)
{
  auto* ocli = (OpenCommandLineInfo*) oInfo->Data;
  // file name, which we received
  const wchar_t* file = ocli->CommandLine;

  wchar_t* nfile = PathToFull(file, true);
  if (nfile) {
    viewFile(UnicodeString(nfile));
  }

  delete[] nfile;
  return nullptr;
}

void FarEditorSet::setEmptyLogger()
{
  spdlog::drop_all();
  logger = spdlog::null_logger_mt("main");
  logger->set_level(spdlog::level::off);
  spdlog::set_default_logger(logger);
}

int FarEditorSet::getHrdArrayWithCurrent(const wchar_t* current, std::vector<const HrdNode*>* hrd_instances, std::vector<const wchar_t*>* out_array)
{
  size_t hrd_count = hrd_instances->size();
  auto current_style = 0;

  std::sort(hrd_instances->begin(), hrd_instances->end(), [](auto elmA, auto elmB) {
    UnicodeString strA;
    if (elmA->hrd_description.length() != 0) {
      strA = elmA->hrd_description;
    }
    else {
      strA = elmA->hrd_name;
    }

    UnicodeString strB;
    if (elmB->hrd_description.length() != 0) {
      strB = elmB->hrd_description;
    }
    else {
      strB = elmB->hrd_name;
    }
    return strA.compare(strB) == -1;
  });

  for (size_t i = 0; i < hrd_count; i++) {
    const HrdNode* hrd_node = hrd_instances->at(i);

    if (hrd_node->hrd_description.length() != 0) {
      out_array->push_back(_wcsdup(UStr::to_stdwstr(&hrd_node->hrd_description).c_str()));
    }
    else {
      out_array->push_back(_wcsdup(UStr::to_stdwstr(&hrd_node->hrd_name).c_str()));
    }

    if (UnicodeString(current).compare(hrd_node->hrd_name) == 0) {
      current_style = (int) i;
    }
  }

  return current_style;
}

void FarEditorSet::disableColorerInEditor()
{
  EditorInfo ei {sizeof(EditorInfo)};
  if (!Info.EditorControl(CurrentEditor, ECTL_GETINFO, 0, &ei)) {
    return;
  }

  auto it_editor = farEditorInstances.find(ei.EditorID);
  if (it_editor != farEditorInstances.end()) {
    it_editor->second->setFileType(nullptr);
  }else {
    auto* new_editor = new FarEditor(&Info, parserFactory.get(), nullptr);
    std::pair<intptr_t, FarEditor*> pair_editor(ei.EditorID, new_editor);
    farEditorInstances.emplace(pair_editor);
  }
}

void FarEditorSet::enableColorerInEditor()
{
  EditorInfo ei {sizeof(EditorInfo)};
  if (!Info.EditorControl(CurrentEditor, ECTL_GETINFO, 0, &ei)) {
    return;
  }

  auto it_editor = farEditorInstances.find(ei.EditorID);
  if (it_editor != farEditorInstances.end()) {
    delete it_editor->second;
    farEditorInstances.erase(it_editor);
  }

  if (auto* new_editor = addCurrentEditor()) {
    new_editor->editorEvent(EE_REDRAW, EEREDRAW_ALL);
  }
}

void FarEditorSet::addEventTimer()
{
  if (!hTimer) {
    CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK) ColorThread, nullptr, 200, Opt.ThreadBuildPeriod, 0);
  }
}

void FarEditorSet::removeEventTimer()
{
  if (hTimer)
    DeleteTimerQueueTimer(hTimerQueue, hTimer, nullptr);
  hTimer = nullptr;
}

void FarEditorSet::addParamAndValue(FileType* filetype, const UnicodeString& name, const UnicodeString& value, const FileType* def_filetype)
{
  if (filetype->getParamValue(name) == nullptr) {
    const UnicodeString* default_value;
    if (def_filetype) {
      default_value = def_filetype->getParamValue(name);
    }else {
      default_value = defaultType->getParamValue(name);
    }
    filetype->addParam(name, *default_value);
  }
  filetype->setParamValue(name, &value);
}

#pragma region macro_functions

HANDLE FarEditorSet::openFromMacro(const struct OpenInfo* oInfo)
{
  auto area = (FARMACROAREA) Info.MacroControl(&MainGuid, MCTL_GETAREA, 0, nullptr);
  auto* mi = (OpenMacroInfo*) oInfo->Data;
  return execMacro(area, mi);
}

void* FarEditorSet::execMacro(FARMACROAREA area, OpenMacroInfo* params)
{
  if (params->Values[0].Type != FMVT_STRING) {
    return nullptr;
  }

  UnicodeString command_type = UnicodeString(params->Values[0].String);

  if (UnicodeString("Settings").caseCompare(command_type, 0) == 0) {
    return macroSettings(area, params);
  }

  if (UnicodeString("Menu").caseCompare(command_type, 0) == 0) {
    return macroMenu(area, params);
  }

  // one parameter functions only up here
  if (params->Count == 1) {
    return nullptr;
  }

  if (UnicodeString("Types").caseCompare(command_type, 0) == 0) {
    return macroTypes(area, params);
  }

  if (UnicodeString("Brackets").caseCompare(command_type, 0) == 0) {
    return macroBrackets(area, params);
  }

  if (UnicodeString("Region").caseCompare(command_type, 0) == 0) {
    return macroRegion(area, params);
  }

  if (UnicodeString("Functions").caseCompare(command_type, 0) == 0) {
    return macroFunctions(area, params);
  }

  if (UnicodeString("Errors").caseCompare(command_type, 0) == 0) {
    return macroErrors(area, params);
  }

  if (UnicodeString("Editor").caseCompare(command_type, 0) == 0) {
    return macroEditor(area, params);
  }

  if (UnicodeString("ParamsOfType").caseCompare(command_type, 0) == 0) {
    return macroParams(area, params);
  }

  return nullptr;
}

void* FarEditorSet::macroSettings(FARMACROAREA area, OpenMacroInfo* params)
{
  (void) area;

  if (params->Count == 1) {
    menuConfigure();
    return INVALID_HANDLE_VALUE;
  }

  if (FMVT_STRING != params->Values[1].Type) {
    return nullptr;
  }
  UnicodeString command = UnicodeString(params->Values[1].String);

  if (UnicodeString("Main").caseCompare(command, 0) == 0) {
    return configure() ? INVALID_HANDLE_VALUE : nullptr;
  }
  if (UnicodeString("Menu").caseCompare(command, 0) == 0) {
    menuConfigure();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("Log").caseCompare(command, 0) == 0) {
    return configureLogging() ? INVALID_HANDLE_VALUE : nullptr;
  }
  if (UnicodeString("Hrc").caseCompare(command, 0) == 0) {
    return configureHrc(area == MACROAREA_EDITOR) ? INVALID_HANDLE_VALUE : nullptr;
  }
  if (UnicodeString("Reload").caseCompare(command, 0) == 0) {
    ReloadBase();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("Status").caseCompare(command, 0) == 0) {
    auto cur_status = isEnable();
    if (params->Count > 2) {
      // change status
      bool val = static_cast<bool>(macroGetValue(params->Values + 2));
      if (val)
        enableColorer();
      else
        disableColorer();
    }
    return cur_status ? INVALID_HANDLE_VALUE : nullptr;
  }
  if (UnicodeString("SaveSettings").caseCompare(command, 0) == 0) {
    SaveSettings();
    SaveLogSettings();
    FarHrcSettings p(this, parserFactory.get());
    p.writeUserProfile();
    return INVALID_HANDLE_VALUE;
  }
  return nullptr;
}

void* FarEditorSet::macroMenu(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR)
    return nullptr;

  if (params->Count == 1) {
    openMenu();
    return INVALID_HANDLE_VALUE;
  }
  return nullptr;
}

void* FarEditorSet::macroTypes(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled || FMVT_STRING != params->Values[1].Type)
    return nullptr;
  UnicodeString command = UnicodeString(params->Values[1].String);

  if (UnicodeString("Menu").caseCompare(command, 0) == 0) {
    return chooseType() ? INVALID_HANDLE_VALUE : nullptr;
  }
  HrcLibrary& hrcLibrary= parserFactory->getHrcLibrary();
  if (UnicodeString("Set").caseCompare(command, 0) == 0) {
    if (params->Count > 2 && FMVT_STRING == params->Values[2].Type) {
      UnicodeString new_type = UnicodeString(params->Values[2].String);
      FarEditor* editor = getCurrentEditor();
      if (!editor || !editor->isColorerEnable())
        return nullptr;

      auto file_type = hrcLibrary.getFileType(&new_type);
      if (file_type) {
        editor->setFileType(file_type);
        applySettingsToEditor(editor);
        return INVALID_HANDLE_VALUE;
      }
      else
        return nullptr;
    }
    return nullptr;
  }
  if (UnicodeString("Get").caseCompare(command, 0) == 0) {
    FarEditor* editor = getCurrentEditor();
    if (!editor || !editor->isColorerEnable())
      return nullptr;

    auto file_type = editor->getFileType();
    if (file_type) {
      auto* out_params = new FarMacroValue[2];
      out_params[0].Type = FMVT_STRING;
      out_params[0].String = _wcsdup(UStr::to_stdwstr(file_type->getName()).c_str());
      out_params[1].Type = FMVT_STRING;
      out_params[1].String = _wcsdup(UStr::to_stdwstr(file_type->getGroup()).c_str());

      return macroReturnValues(out_params, 2);
    }
    else
      return nullptr;
  }
  if (UnicodeString("List").caseCompare(command, 0) == 0) {
    auto type_count = hrcLibrary.getFileTypesCount();
    FileType* type = nullptr;

    auto* array = new FarMacroValue[type_count];

    for (size_t idx = 0; idx < type_count; idx++) {
      type = hrcLibrary.enumerateFileTypes((unsigned int) idx);
      if (type == nullptr) {
        break;
      }
      array[idx].Type = FMVT_STRING;
      array[idx].String = _wcsdup(UStr::to_stdwstr(type->getName()).c_str());
    }
    auto* out_params = new FarMacroValue[1];
    out_params[0].Type = FMVT_ARRAY;
    out_params[0].Array.Values = array;
    out_params[0].Array.Count = type_count;
    return macroReturnValues(out_params, 1);
  }
  return nullptr;
}

void* FarEditorSet::macroBrackets(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled || FMVT_STRING != params->Values[1].Type)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor || !editor->isColorerEnable())
    return nullptr;

  UnicodeString command = UnicodeString(params->Values[1].String);
  if (UnicodeString("Match").caseCompare(command, 0) == 0) {
    editor->matchPair();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("SelectAll").caseCompare(command, 0) == 0) {
    editor->selectBlock();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("SelectIn").caseCompare(command, 0) == 0) {
    editor->selectPair();
    return INVALID_HANDLE_VALUE;
  }

  return nullptr;
}

void* FarEditorSet::macroRegion(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled || FMVT_STRING != params->Values[1].Type)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor || !editor->isColorerEnable())
    return nullptr;

  UnicodeString command = UnicodeString(params->Values[1].String);
  if (UnicodeString("Select").caseCompare(command, 0) == 0) {
    editor->selectRegion();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("Show").caseCompare(command, 0) == 0) {
    editor->getNameCurrentScheme();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("List").caseCompare(command, 0) == 0) {
    UnicodeString region, scheme;
    editor->getCurrentRegionInfo(region, scheme);
    auto* out_params = new FarMacroValue[2];
    out_params[0].Type = FMVT_STRING;
    out_params[0].String = _wcsdup(UStr::to_stdwstr(&region).c_str());
    out_params[1].Type = FMVT_STRING;
    out_params[1].String = _wcsdup(UStr::to_stdwstr(&scheme).c_str());

    return macroReturnValues(out_params, 2);
  }
  return nullptr;
}

void* FarEditorSet::macroFunctions(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled || FMVT_STRING != params->Values[1].Type)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor || !editor->isColorerEnable())
    return nullptr;

  UnicodeString command = UnicodeString(params->Values[1].String);
  if (UnicodeString("Show").caseCompare(command, 0) == 0) {
    editor->listFunctions();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("Find").caseCompare(command, 0) == 0) {
    editor->locateFunction();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("List").caseCompare(command, 0) == 0) {
    auto* outliner = editor->getFunctionOutliner();
    auto items_num = outliner->itemCount();

    auto* array_string = new FarMacroValue[items_num];
    auto* array_numline = new FarMacroValue[items_num];
    for (size_t idx = 0; idx < items_num; idx++) {
      OutlineItem* item = outliner->getItem(idx);
      UnicodeString* line = editor->getLine(item->lno);

      array_string[idx].Type = FMVT_STRING;
      array_string[idx].String = _wcsdup(UStr::to_stdwstr(line).c_str());

      array_numline[idx].Type = FMVT_INTEGER;
      array_numline[idx].Integer = item->lno + 1;
    }

    auto* out_params = new FarMacroValue[2];
    out_params[0].Type = FMVT_ARRAY;
    out_params[0].Array.Values = array_string;
    out_params[0].Array.Count = items_num;

    out_params[1].Type = FMVT_ARRAY;
    out_params[1].Array.Values = array_numline;
    out_params[1].Array.Count = items_num;

    return macroReturnValues(out_params, 2);
  }
  return nullptr;
}

void* FarEditorSet::macroErrors(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled || FMVT_STRING != params->Values[1].Type)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor || !editor->isColorerEnable())
    return nullptr;

  UnicodeString command = UnicodeString(params->Values[1].String);
  if (UnicodeString("Show").caseCompare(command, 0) == 0) {
    editor->listErrors();
    return INVALID_HANDLE_VALUE;
  }
  if (UnicodeString("List").caseCompare(command, 0) == 0) {
    auto* outliner = editor->getErrorOutliner();
    auto items_num = outliner->itemCount();

    auto* array_string = new FarMacroValue[items_num];
    auto* array_numline = new FarMacroValue[items_num];
    for (size_t idx = 0; idx < items_num; idx++) {
      OutlineItem* item = outliner->getItem(idx);
      UnicodeString* line = editor->getLine(item->lno);

      array_string[idx].Type = FMVT_STRING;
      array_string[idx].String = _wcsdup(UStr::to_stdwstr(line).c_str());

      array_numline[idx].Type = FMVT_INTEGER;
      array_numline[idx].Integer = item->lno + 1;
    }

    auto* out_params = new FarMacroValue[2];
    out_params[0].Type = FMVT_ARRAY;
    out_params[0].Array.Values = array_string;
    out_params[0].Array.Count = items_num;

    out_params[1].Type = FMVT_ARRAY;
    out_params[1].Array.Values = array_numline;
    out_params[1].Array.Count = items_num;

    return macroReturnValues(out_params, 2);
  }
  return nullptr;
}

void* FarEditorSet::macroEditor(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled || FMVT_STRING != params->Values[1].Type)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor)
    return nullptr;
  UnicodeString command = UnicodeString(params->Values[1].String);

  if (UnicodeString("Status").caseCompare(command, 0) == 0) {
    auto cur_status = editor->isColorerEnable();
    if (params->Count > 2) {
      // change status
      bool val = static_cast<bool>(macroGetValue(params->Values + 2));
      if (val)
        enableColorerInEditor();
      else
        disableColorerInEditor();
    }
    return cur_status ? INVALID_HANDLE_VALUE : nullptr;
  }

  if (!editor->isColorerEnable())
    return nullptr;

  if (UnicodeString("CrossVisible").caseCompare(command, 0) == 0) {
    auto cur_style = editor->getVisibleCrossState();
    auto cur_status = editor->getCrossStatus();

    if (params->Count > 2) {
      // change status
      int val = static_cast<int>(macroGetValue(params->Values + 2));
      if (val >= static_cast<int>(FarEditor::CROSS_STATUS::CROSS_OFF) && val <= static_cast<int>(FarEditor::CROSS_STATUS::CROSS_INSCHEME))
        editor->setCrossState(val, Opt.CrossStyle);
    }
    if (params->Count > 3) {
      // change style
      int val = static_cast<int>(macroGetValue(params->Values + 3));
      if (val >= static_cast<int>(FarEditor::CROSS_STYLE::CSTYLE_VERT) && val <= static_cast<int>(FarEditor::CROSS_STYLE::CSTYLE_BOTH))
        editor->setCrossStyle(val);
    }

    auto* out_params = new FarMacroValue[2];
    out_params[0].Type = FMVT_INTEGER;
    out_params[0].Integer = cur_status;
    out_params[1].Type = FMVT_INTEGER;
    out_params[1].Integer = static_cast<int>(cur_style);

    return macroReturnValues(out_params, 2);
  }

  if (UnicodeString("Refresh").caseCompare(command, 0) == 0) {
    editor->updateHighlighting();
    return INVALID_HANDLE_VALUE;
  }

  if (UnicodeString("Pair").caseCompare(command, 0) == 0) {
    // current status
    auto cur_status = editor->isDrawPairs();
    if (params->Count > 2) {
      // change status
      bool val = static_cast<bool>(macroGetValue(params->Values + 2));
      editor->setDrawPairs(val);
    }
    return macroReturnInt(cur_status);
  }

  if (UnicodeString("Syntax").caseCompare(command, 0) == 0) {
    // current status
    auto cur_status = editor->isDrawSyntax();
    if (params->Count > 2) {
      // change status
      bool val = static_cast<bool>(macroGetValue(params->Values + 2));
      editor->setDrawSyntax(val);
    }
    return macroReturnInt(cur_status);
  }

  if (UnicodeString("Progress").caseCompare(command, 0) == 0) {
    auto* out_params = new FarMacroValue[1];
    out_params[0].Type = FMVT_INTEGER;
    out_params[0].Integer = editor->getParseProgress();
    return macroReturnValues(out_params, 1);
  }

  return nullptr;
}

void* FarEditorSet::macroParams(FARMACROAREA area, OpenMacroInfo* params)
{
  (void) area;
  if (!Opt.rEnabled || FMVT_STRING != params->Values[1].Type)
    return nullptr;

  HrcLibrary& hrcLibrary= parserFactory->getHrcLibrary();
  UnicodeString command = UnicodeString(params->Values[1].String);
  if (UnicodeString("List").caseCompare(command, 0) == 0) {
    if (params->Count > 2 && FMVT_STRING == params->Values[2].Type) {
      UnicodeString type = UnicodeString(params->Values[2].String);
      auto* file_type = dynamic_cast<FileType*>(hrcLibrary.getFileType(&type));
      if (file_type) {
        // max count params
        size_t size = file_type->getParamCount() + defaultType->getParamCount();
        auto* array_param = new FarMacroValue[size];
        auto* array_param_value = new FarMacroValue[size];

        size_t count = 0;
        std::vector<UnicodeString> type_params = file_type->enumParams();
        for (auto& type_param : type_params) {
          if (defaultType->getParamValue(type_param) == nullptr) {
            array_param[count].Type = FMVT_STRING;
            array_param[count].String = _wcsdup(UStr::to_stdwstr(&type_param).c_str());

            auto* value = file_type->getParamValue(type_param);
            if (value) {
              array_param_value[count].Type = FMVT_STRING;
              array_param_value[count].String = _wcsdup(UStr::to_stdwstr(value).c_str());
            }
            else {
              array_param_value[count].Type = FMVT_NIL;
            }
            count++;
          }
        }

        std::vector<UnicodeString> default_params = defaultType->enumParams();
        for (auto& default_param : default_params) {
          array_param[count].Type = FMVT_STRING;
          array_param[count].String = _wcsdup(UStr::to_stdwstr(&default_param).c_str());

          auto* value = file_type->getParamValue(default_param);
          if (value) {
            array_param_value[count].Type = FMVT_STRING;
            array_param_value[count].String = _wcsdup(UStr::to_stdwstr(value).c_str());
          }
          else {
            array_param_value[count].Type = FMVT_NIL;
          }
          count++;
        }

        auto* out_params = new FarMacroValue[2];
        out_params[0].Type = FMVT_ARRAY;
        out_params[0].Array.Values = array_param;
        out_params[0].Array.Count = size;

        out_params[1].Type = FMVT_ARRAY;
        out_params[1].Array.Values = array_param_value;
        out_params[1].Array.Count = size;

        return macroReturnValues(out_params, 2);
      }
    }
    return nullptr;
  }

  if (UnicodeString("Get").caseCompare(command, 0) == 0) {
    if (params->Count > 3 && FMVT_STRING == params->Values[2].Type && FMVT_STRING == params->Values[3].Type) {
      UnicodeString type = UnicodeString(params->Values[2].String);

      auto file_type = hrcLibrary.getFileType(&type);
      if (file_type) {
        UnicodeString param_name = UnicodeString(params->Values[3].String);
        auto* value = file_type->getParamValue(param_name);
        auto* out_params = new FarMacroValue[1];
        if (value) {
          out_params[0].Type = FMVT_STRING;
          out_params[0].String = _wcsdup(UStr::to_stdwstr(value).c_str());
        }
        else {
          out_params[0].Type = FMVT_NIL;
        }

        return macroReturnValues(out_params, 1);
      }
    }
    return nullptr;
  }

  if (UnicodeString("Set").caseCompare(command, 0) == 0) {
    if (params->Count > 3 && FMVT_STRING == params->Values[2].Type && FMVT_STRING == params->Values[3].Type) {
      UnicodeString type = UnicodeString(params->Values[2].String);
      auto* file_type = hrcLibrary.getFileType(&type);
      if (file_type) {
        UnicodeString param_name = UnicodeString(params->Values[3].String);
        if (params->Count > 4 && FMVT_STRING == params->Values[4].Type) {
          // replace value
          UnicodeString param_value = UnicodeString(params->Values[4].String);
          addParamAndValue(file_type, param_name, param_value);
        }
        else if (params->Count == 4) {
          // remove value
          file_type->setParamValue(param_name, nullptr);
        }
        else
          return nullptr;
        return INVALID_HANDLE_VALUE;
      }
    }
    return nullptr;
  }

  return nullptr;
}
#pragma endregion macro_functions
