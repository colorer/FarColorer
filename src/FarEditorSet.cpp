#include "FarEditorSet.h"
#include <colorer/parsers/CatalogParser.h>
#include <colorer/parsers/ParserFactoryException.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include <farcolor.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "DlgBuilder.hpp"
#include "SettingsControl.h"
#include "tools.h"

VOID CALLBACK ColorThread(PVOID lpParam, BOOLEAN TimerOrWaitFired);

FarEditorSet::FarEditorSet()
    : dialogFirstFocus(false),
      menuid(0),
      sTempHrdName(nullptr),
      sTempHrdNameTm(nullptr),
      parserFactory(nullptr),
      regionMapper(nullptr),
      hrcParser(nullptr),
      sCatalogPathExp(nullptr),
      sUserHrdPathExp(nullptr),
      sUserHrcPathExp(nullptr),
      CurrentMenuItem(0),
      err_status(ERR_NO_ERROR)
{
  setEmptyLogger();

  CString module(Info.ModuleName, 0);
  size_t pos = module.lastIndexOf('\\');
  pos = module.lastIndexOf('\\', pos);
  pluginPath = std::make_unique<SString>(CString(module, 0, pos));

  colorer_lib = std::make_unique<Colorer>();
  ReloadBase();

  hTimerQueue = CreateTimerQueue();
  CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK) ColorThread, nullptr, 500, 500, 0);
}

FarEditorSet::~FarEditorSet()
{
  dropAllEditors(false);
  regionMapper.reset();
  parserFactory.reset();
  DeleteTimerQueue(hTimerQueue);
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
        configureHrc();
        break;
      case 2:
        configureLogging();
        break;
      case 4:
        TestLoadBase(Opt.CatalogPath, Opt.UserHrdPath, Opt.UserHrcPath, true, Opt.TrueModOn ? FarEditorSet::HRCM_BOTH : FarEditorSet::HRCM_CONSOLE);
        break;
      default:
        return;
        break;
    }
    shMenu[prev_id].Flags ^= MIF_SELECTED;
    prev_id = menu_id;
  }
}

void FarEditorSet::openMenu(int MenuId)
{
  if (MenuId < 0) {
    const size_t menu_size = 13;
    int iMenuItems[menu_size] = {mListTypes,         mMatchPair,      mSelectBlock, mSelectPair,      mListFunctions, mFindErrors,     mSelectRegion,
                                 mCurrentRegionName, mLocateFunction, -1,           mUpdateHighlight, mReloadBase,    mConfigureHotkey};
    FarMenuItem menuElements[menu_size];
    memset(menuElements, 0, sizeof(menuElements));
    if (Opt.rEnabled) {
      menuElements[0].Flags = MIF_SELECTED;
    }
    for (int i = menu_size - 1; i >= 0; i--) {
      if (iMenuItems[i] == -1) {
        menuElements[i].Flags |= MIF_SEPARATOR;
      }
      else {
        menuElements[i].Text = GetMsg(iMenuItems[i]);
      }
    }

    intptr_t menu_id = Info.Menu(&MainGuid, &PluginMenu, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), nullptr, L"menu", nullptr, nullptr,
                                 Opt.rEnabled ? menuElements : menuElements + 12, Opt.rEnabled ? menu_size : 1);
    if (!Opt.rEnabled && menu_id == 0) {
      MenuId = 12;
    }
    else {
      MenuId = static_cast<int>(menu_id);
    }
  }
  if (MenuId >= 0) {
    try {
      FarEditor* editor = getCurrentEditor();
      if (!editor && (Opt.rEnabled || MenuId != 12)) {
        throw Exception(CString("Can't find current editor in array."));
      }

      switch (MenuId) {
        case 0:
          chooseType();
          break;
        case 1:
          editor->matchPair();
          break;
        case 2:
          editor->selectBlock();
          break;
        case 3:
          editor->selectPair();
          break;
        case 4:
          editor->listFunctions();
          break;
        case 5:
          editor->listErrors();
          break;
        case 6:
          editor->selectRegion();
          break;
        case 7:
          editor->getNameCurrentScheme();
          break;
        case 8:
          editor->locateFunction();
          break;
        case 10:
          editor->updateHighlighting();
          break;
        case 11:
          ReloadBase();
          break;
        case 12:
          menuConfigure();
          break;
        default:
          break;
      }
    } catch (Exception& e) {
      spdlog::error("{0}", e.what());
      SString msg("openMenu: ");
      msg.append(CString(e.what()));
      showExceptionMessage(msg.getWChars());
      disableColorer();
    }
  }
}

void FarEditorSet::viewFile(const String& path)
{
  try {
    if (!Opt.rEnabled) {
      throw Exception(CString("FarColorer is disabled"));
    }

    // Creates store of text lines
    TextLinesStore textLinesStore;
    textLinesStore.loadFile(&path, nullptr, true);
    // Base editor to make primary parse
    BaseEditor baseEditor(parserFactory.get(), &textLinesStore);
    RegionMapper* regionMap;
    try {
      regionMap = parserFactory->createStyledMapper(&DConsole, &CString(Opt.HrdName));
    } catch (ParserFactoryException& e) {
      spdlog::error("{0}", e.what());
      regionMap = parserFactory->createStyledMapper(&DConsole, nullptr);
    }
    baseEditor.setRegionMapper(regionMap);
    baseEditor.chooseFileType(&path);
    // initial event
    baseEditor.lineCountEvent((int) textLinesStore.getLineCount());
    // computing background color
    int background = 0x1F;
    const StyledRegion* rd = StyledRegion::cast(regionMap->getRegionDefine(CString("def:Text")));

    if (rd != nullptr && rd->bfore && rd->bback) {
      background = rd->fore + (rd->back << 4);
    }

    // File viewing in console window
    TextConsoleViewer viewer(&baseEditor, &textLinesStore, background, -1);
    viewer.view();
    delete regionMap;
  } catch (Exception& e) {
    showExceptionMessage(CString(e.what()).getWChars());
  }
}

size_t FarEditorSet::getCountFileTypeAndGroup() const
{
  size_t num = 0;
  const String* group = nullptr;
  FileType* type = nullptr;

  for (int idx = 0;; idx++, num++) {
    type = hrcParser->enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (group != nullptr && !group->equals(type->getGroup())) {
      num++;
    }

    group = type->getGroup();
  }
  return num;
}

FileTypeImpl* FarEditorSet::getFileTypeByIndex(int idx) const
{
  FileType* type = nullptr;
  const String* group = nullptr;

  for (int i = 0; idx >= 0; idx--, i++) {
    type = hrcParser->enumerateFileTypes(i);

    if (!type) {
      break;
    }

    if (group != nullptr && !group->equals(type->getGroup())) {
      idx--;
    }
    group = type->getGroup();
  }

  return dynamic_cast<FileTypeImpl*>(type);
}

void FarEditorSet::FillTypeMenu(ChooseTypeMenu* Menu, FileType* CurFileType) const
{
  const String* group = &DAutodetect;
  FileType* type = nullptr;

  for (int idx = 0;; idx++) {
    type = hrcParser->enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (group != nullptr && !group->equals(type->getGroup())) {
      Menu->AddGroup(type->getGroup()->getWChars());
      group = type->getGroup();
    }

    size_t i;
    const String* v;
    v = dynamic_cast<FileTypeImpl*>(type)->getParamValue(DFavorite);
    if (v && v->equals(&DTrue)) {
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

inline wchar_t* __cdecl Upper(wchar_t* Ch)
{
  CharUpperBuffW(Ch, 1);
  return Ch;
}

INT_PTR WINAPI KeyDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  wchar wkey[2];

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

  ChooseTypeMenu menu(GetMsg(mAutoDetect), GetMsg(mFavorites));
  FillTypeMenu(&menu, fe->getFileType());

  wchar_t bottom[20];
  _snwprintf(bottom, 20, GetMsg(mTotalTypes), hrcParser->getFileTypesCount());
  struct FarKey BreakKeys[3] = {VK_INSERT, 0, VK_DELETE, 0, VK_F4, 0};
  intptr_t BreakCode;
  while (true) {
    intptr_t i = Info.Menu(&MainGuid, &FileChooseMenu, -1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT, GetMsg(mSelectSyntax), bottom,
                           L"filetypechoose", BreakKeys, &BreakCode, menu.getItems(), menu.getItemsCount());

    if (i >= 0) {
      if (BreakCode == 0) {
        if (i != 0 && !menu.IsFavorite(i)) {
          menu.MoveToFavorites(i);
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

        const String* v;
        v = dynamic_cast<FileTypeImpl*>(menu.GetFileType(i))->getParamValue(DHotkey);
        if (v && v->length()) {
          KeyAssignDlgData[2].Data = v->getWChars();
        }

        HANDLE hDlg = Info.DialogInit(&MainGuid, &AssignKeyDlg, -1, -1, 34, 6, L"keyassign", KeyAssignDlgData, std::size(KeyAssignDlgData), 0, 0,
                                      KeyDialogProc, nullptr);
        intptr_t res = Info.DialogRun(hDlg);

        if (res != -1) {
          KeyAssignDlgData[2].Data =
              static_cast<const wchar_t*>(trim(reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, 2, nullptr))));
          if (menu.GetFileType(i)->getParamValue(DHotkey) == nullptr) {
            dynamic_cast<FileTypeImpl*>(menu.GetFileType(i))->addParam(&DHotkey);
          }
          CString hotkey = CString(KeyAssignDlgData[2].Data);
          menu.GetFileType(i)->setParamValue(DHotkey, &hotkey);
          menu.RefreshItemCaption(i);
        }
        menu.SetSelected(i);
        Info.DialogFree(hDlg);
      }
      else {
        if (i == 0) {
          String* s = getCurrentFileName();
          fe->chooseFileType(s);
          delete s;
          break;
        }
        fe->setFileType(menu.GetFileType(i));
        break;
      }
    }
    else {
      break;
    }
  }

  FarHrcSettings p(parserFactory.get());
  p.writeUserProfile();
  return true;
}

const String* FarEditorSet::getHRDescription(const String& name, const CString& _hrdClass) const
{
  const String* descr = nullptr;
  if (parserFactory != nullptr) {
    descr = &parserFactory->getHRDNode(_hrdClass, name)->hrd_description;
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

    return !fes->TestLoadBase(temp, userhrd, userhrc, false, FarEditorSet::HRCM_BOTH);
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool FarEditorSet::configure()
{
  try {
    PluginDialogBuilder Builder(Info, MainGuid, PluginConfig, mSetup, L"config", SettingDialogProc, this);
    Builder.AddCheckbox(mTurnOff, &Opt.rEnabled);
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

    std::vector<const HRDNode*> hrd_con_instances;
    std::vector<const wchar_t*> console_style;
    unsigned long flag_disable = 0;
    int current_style;
    if (Opt.rEnabled) {
      hrd_con_instances = parserFactory->enumHRDInstances(DConsole);
      current_style = getHrdArrayWithCurrent(Opt.HrdName, &hrd_con_instances, &console_style);
    }
    else {
      console_style.push_back(Opt.HrdName);
      current_style = 0;
      flag_disable = DIF_DISABLE;
    }
    Builder.AddText(mHRDName);
    Builder.AddComboBox(&current_style, nullptr, 30, console_style.data(), console_style.size(), DIF_LISTWRAPMODE | DIF_DROPDOWNLIST | flag_disable);

    Builder.AddCheckbox(mTrueMod, &Opt.TrueModOn);

    std::vector<const HRDNode*> hrd_rgb_instances;
    std::vector<const wchar_t*> rgb_style;
    flag_disable = 0;
    int current_rstyle;
    if (Opt.rEnabled) {
      hrd_rgb_instances = parserFactory->enumHRDInstances(DRgb);
      current_rstyle = getHrdArrayWithCurrent(Opt.HrdNameTm, &hrd_rgb_instances, &rgb_style);
    }
    else {
      rgb_style.push_back(Opt.HrdNameTm);
      current_rstyle = 0;
      flag_disable = DIF_DISABLE;
    }
    Builder.AddText(mHRDNameTrueMod);
    Builder.AddComboBox(&current_rstyle, nullptr, 30, rgb_style.data(), rgb_style.size(), DIF_LISTWRAPMODE | DIF_DROPDOWNLIST | flag_disable);

    Builder.ColumnBreak();

    Builder.AddCheckbox(mPairs, &Opt.drawPairs);
    Builder.AddCheckbox(mSyntax, &Opt.drawSyntax);
    Builder.AddCheckbox(mOldOutline, &Opt.oldOutline);
    Builder.AddCheckbox(mChangeBackgroundEditor, &Opt.ChangeBgEditor);

    Builder.AddCheckbox(mCross, &Opt.drawCross, 0, true);
    Builder.AddText(mCrossText);
    const wchar_t* cross_style[] = {GetMsg(mCrossBoth), GetMsg(mCrossVert), GetMsg(mCrossHoriz)};
    int cross_style_id = Opt.CrossStyle - 1;
    Builder.AddComboBox(&cross_style_id, nullptr, 25, cross_style, std::size(cross_style), DIF_LISTWRAPMODE | DIF_DROPDOWNLIST);

    Builder.EndColumns();
    Builder.AddOKCancel(mOk, mCancel);
    settingWindow.okButtonConfig = Builder.GetLastID() - 1;

    if (Builder.ShowDialog()) {
      if (flag_disable == 0) {
        wcsncpy(Opt.HrdName, hrd_con_instances.at(current_style)->hrd_name.getWChars(), std::size(Opt.HrdName));
        wcsncpy(Opt.HrdNameTm, hrd_rgb_instances.at(current_rstyle)->hrd_name.getWChars(), std::size(Opt.HrdNameTm));
      }
      if (cross_style_id != Opt.CrossStyle)
        Opt.CrossStyle = cross_style_id + 1;
      SaveSettings();
      if (Opt.rEnabled) {
        ReloadBase();
      }
      else {
        disableColorer();
      }
    }

    Info.EditorControl(CurrentEditor, ECTL_REDRAW, 0, nullptr);
    return true;
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());

    SString msg("configure: ");
    msg.append(CString(e.what()));
    showExceptionMessage(msg.getWChars());
    disableColorer();
    return false;
  }
}

int FarEditorSet::editorInput(const INPUT_RECORD& Rec)
{
  if (Opt.rEnabled) {
    FarEditor* editor = getCurrentEditor();
    if (editor) {
      return editor->editorInput(Rec);
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
        if (editor) {
          return editor->editorEvent(pInfo->Event, pInfo->Param);
        }
        return 0;
      } break;
      case EE_CHANGE: {
        //запрещено вызывать EditorControl (getCurrentEditor)
        auto it_editor = farEditorInstances.find(pInfo->EditorID);
        if (it_editor != farEditorInstances.end()) {
          return it_editor->second->editorEvent(pInfo->Event, pInfo->Param);
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
        }
        return 0;
      } break;
      default:
        break;
    }
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());

    SString msg("editorEvent: ");
    msg.append(CString(e.what()));
    showExceptionMessage(msg.getWChars());
    disableColorer();
  }

  return 0;
}

bool FarEditorSet::TestLoadBase(const wchar_t* catalogPath, const wchar_t* userHrdPath, const wchar_t* userHrcPath, const int full,
                                const HRC_MODE hrc_mode)
{
  bool res = true;
  const wchar_t* marr[2] = {GetMsg(mName), GetMsg(mReloading)};
  Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], 2, 0);

  std::unique_ptr<ParserFactory> parserFactoryLocal = nullptr;
  std::unique_ptr<RegionMapper> regionMapperLocal = nullptr;

  std::unique_ptr<SString> catalogPathS(PathToFullS(catalogPath, false));
  std::unique_ptr<SString> userHrdPathS(PathToFullS(userHrdPath, false));
  std::unique_ptr<SString> userHrcPathS(PathToFullS(userHrcPath, false));

  std::unique_ptr<SString> tpath;
  if (!catalogPathS || !catalogPathS->length()) {
    auto* path = new SString(*pluginPath);
    path->append(CString(FarCatalogXml));
    tpath.reset(path);
  }
  else {
    tpath = std::move(catalogPathS);
  }

  try {
    parserFactoryLocal = std::make_unique<ParserFactory>();
    parserFactoryLocal->loadCatalog(tpath.get());
    HRCParser* hrcParserLocal = parserFactoryLocal->getHRCParser();
    LoadUserHrd(userHrdPathS.get(), parserFactoryLocal.get());
    LoadUserHrc(userHrcPathS.get(), parserFactoryLocal.get());
    FarHrcSettings p(parserFactoryLocal.get());
    p.readProfile(pluginPath.get());
    p.readUserProfile();

    if (hrc_mode == HRCM_CONSOLE || hrc_mode == HRCM_BOTH) {
      try {
        regionMapperLocal.reset(parserFactoryLocal->createStyledMapper(&DConsole, sTempHrdName.get()));
      } catch (ParserFactoryException& e) {
        spdlog::error("{0}", e.what());
        regionMapperLocal.reset(parserFactoryLocal->createStyledMapper(&DConsole, nullptr));
      }
    }

    if (hrc_mode == HRCM_RGB || hrc_mode == HRCM_BOTH) {
      try {
        regionMapperLocal.reset(parserFactoryLocal->createStyledMapper(&DRgb, sTempHrdNameTm.get()));
      } catch (ParserFactoryException& e) {
        spdlog::error("{0}", e.what());
        regionMapperLocal.reset(parserFactoryLocal->createStyledMapper(&DRgb, nullptr));
      }
    }

    if (full) {
      for (int idx = 0;; idx++) {
        FileType* type = hrcParserLocal->enumerateFileTypes(idx);

        if (type == nullptr) {
          break;
        }

        SString tname;

        if (type->getGroup() != nullptr) {
          tname.append(type->getGroup());
          tname.append(CString(": "));
        }

        tname.append(type->getDescription());
        marr[1] = tname.getWChars();
        Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], 2, 0);
        if (idx % 5 == 0)
          Info.EditorControl(-1, ECTL_REDRAW, 0, nullptr);
        type->getBaseScheme();
      }
    }
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());
    showExceptionMessage(CString(e.what()).getWChars());
    res = false;
  }

  return res;
}

void FarEditorSet::ReloadBase()
{
  try {
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

    CString hrdClass;
    CString hrdName;

    if (Opt.TrueModOn) {
      hrdClass = DRgb;
      hrdName = CString(Opt.HrdNameTm);
    }
    else {
      hrdClass = DConsole;
      hrdName = CString(Opt.HrdName);
    }

    parserFactory = std::make_unique<ParserFactory>();
    parserFactory->loadCatalog(sCatalogPathExp.get());
    hrcParser = parserFactory->getHRCParser();
    LoadUserHrd(sUserHrdPathExp.get(), parserFactory.get());
    LoadUserHrc(sUserHrcPathExp.get(), parserFactory.get());
    FarHrcSettings p(parserFactory.get());
    p.readProfile(pluginPath.get());
    p.readUserProfile();
    defaultType = dynamic_cast<FileTypeImpl*>(hrcParser->getFileType(&DDefaultScheme));

    try {
      regionMapper.reset(parserFactory->createStyledMapper(&hrdClass, &hrdName));
    } catch (ParserFactoryException& e) {
      spdlog::error("{0}", e.what());
      regionMapper.reset(parserFactory->createStyledMapper(&hrdClass, nullptr));
    }
    //устанавливаем фон редактора при каждой перезагрузке схем.
    SetBgEditor();
  } catch (SettingsControlException& e) {
    spdlog::error("{0}", e.what());
    showExceptionMessage(CString(e.what()).getWChars());
    err_status = ERR_FARSETTINGS_ERROR;
    disableColorer();
  } catch (Exception& e) {
    spdlog::error("{0}", e.what());
    showExceptionMessage(CString(e.what()).getWChars());
    err_status = ERR_BASE_LOAD;
    disableColorer();
  }
}

size_t FarEditorSet::getEditorCount() const
{
  return farEditorInstances.size();
}

FarEditor* FarEditorSet::addCurrentEditor()
{
  EditorInfo ei {};
  ei.StructSize = sizeof(EditorInfo);
  if (!Info.EditorControl(CurrentEditor, ECTL_GETINFO, 0, &ei)) {
    return nullptr;
  }

  auto* editor = new FarEditor(&Info, parserFactory.get());
  std::pair<intptr_t, FarEditor*> pair_editor(ei.EditorID, editor);
  farEditorInstances.emplace(pair_editor);
  String* s = getCurrentFileName();
  editor->chooseFileType(s);
  delete s;
  editor->setTrueMod(Opt.TrueModOn);
  editor->setRegionMapper(regionMapper.get());
  editor->setDrawPairs(Opt.drawPairs);
  editor->setDrawSyntax(Opt.drawSyntax);
  editor->setOutlineStyle(Opt.oldOutline);
  editor->setCrossState(Opt.drawCross, Opt.CrossStyle);

  return editor;
}

String* FarEditorSet::getCurrentFileName()
{
  LPWSTR FileName = nullptr;
  size_t FileNameSize = Info.EditorControl(CurrentEditor, ECTL_GETFILENAME, 0, nullptr);

  if (FileNameSize) {
    FileName = new wchar_t[FileNameSize];
    Info.EditorControl(CurrentEditor, ECTL_GETFILENAME, FileNameSize, FileName);
  }

  CString fnpath(FileName);
  size_t slash_idx = fnpath.lastIndexOf('\\');

  if (slash_idx == -1) {
    slash_idx = fnpath.lastIndexOf('/');
  }
  auto* s = new SString(fnpath, slash_idx + 1);
  delete[] FileName;
  return s;
}

FarEditor* FarEditorSet::getCurrentEditor()
{
  EditorInfo ei {};
  ei.StructSize = sizeof(EditorInfo);
  Info.EditorControl(CurrentEditor, ECTL_GETINFO, 0, &ei);
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

void FarEditorSet::ApplySettingsToEditors()
{
  for (auto& farEditorInstance : farEditorInstances) {
    farEditorInstance.second->setTrueMod(Opt.TrueModOn);
    farEditorInstance.second->setDrawPairs(Opt.drawPairs);
    farEditorInstance.second->setDrawSyntax(Opt.drawSyntax);
    farEditorInstance.second->setOutlineStyle(Opt.oldOutline);
    farEditorInstance.second->setCrossState(Opt.drawCross, Opt.CrossStyle);
  }
}

void FarEditorSet::dropCurrentEditor(bool clean)
{
  EditorInfo ei {};
  ei.StructSize = sizeof(EditorInfo);
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
    auto* path = new SString(*pluginPath);
    path->append(CString(FarCatalogXml));
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
}

void FarEditorSet::applyLogSetting()
{
  if (Opt.LogEnabled) {
    auto level = spdlog::level::from_str(CString(Opt.logLevel).getChars());
    if (level != spdlog::level::off) {
      try {
        std::string file_name = "farcolorer.log";
        if (Opt.LogPath[0] != '\0') {
          SString sLogPathExp(PathToFullS(Opt.LogPath, false));
          file_name = std::string(sLogPathExp.getChars()).append("\\").append(file_name);
        }
        spdlog::drop_all();
        log = spdlog::basic_logger_mt("main", file_name);
        spdlog::set_default_logger(log);
        log->set_level(level);
      } catch (std::exception& e) {
        setEmptyLogger();
        showExceptionMessage(CString(e.what()).getWChars());
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
    const StyledRegion* def_text = StyledRegion::cast(regionMapper->getRegionDefine(CString("def:Text")));

    FarSetColors fsc {};
    FarColor fc {};
    fsc.StructSize = sizeof(FarSetColors);
    fsc.Flags = FSETCLR_REDRAW;
    fsc.ColorsCount = 1;
    fsc.StartIndex = COL_EDITORTEXT;
    if (Opt.TrueModOn) {
      fc.Flags = 0;
      fc.BackgroundColor = revertRGB(def_text->back);
      fc.ForegroundColor = revertRGB(def_text->fore);
    }
    else {
      fc.Flags = FCF_4BITMASK;
      fc.BackgroundColor = def_text->back;
      fc.ForegroundColor = def_text->fore;
    }
    fc.Reserved = nullptr;
    fsc.Colors = &fc;
    return Info.AdvControl(&MainGuid, ACTL_SETARRAYCOLOR, 0, &fsc) != 0;
  }
  return false;
}

void FarEditorSet::LoadUserHrd(const String* filename, ParserFactory* pf)
{
  if (filename && filename->length()) {
    xercesc::XercesDOMParser xml_parser;
    XmlParserErrorHandler err_handler;
    xml_parser.setErrorHandler(&err_handler);
    xml_parser.setLoadExternalDTD(false);
    xml_parser.setSkipDTDValidation(true);
    uXmlInputSource config = XmlInputSource::newInstance(filename->getWChars(), static_cast<const XMLCh*>(nullptr));
    xml_parser.parse(*config->getInputSource());
    if (err_handler.getSawErrors()) {
      throw ParserFactoryException(SString("Error reading ") + CString(filename));
    }
    xercesc::DOMDocument* catalog = xml_parser.getDocument();
    xercesc::DOMElement* elem = catalog->getDocumentElement();
    const XMLCh* tagHrdSets = L"hrd-sets";
    const XMLCh* tagHrd = L"hrd";
    if (elem == nullptr || !xercesc::XMLString::equals(elem->getNodeName(), tagHrdSets)) {
      throw Exception(CString("main '<hrd-sets>' block not found"));
    }
    for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
      if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
        auto* subelem = dynamic_cast<xercesc::DOMElement*>(node);
        if (xercesc::XMLString::equals(subelem->getNodeName(), tagHrd)) {
          auto hrd = CatalogParser::parseHRDSetsChild(subelem);
          if (hrd)
            pf->addHrd(std::move(hrd));
        }
      }
    }
  }
}

void FarEditorSet::LoadUserHrc(const String* filename, ParserFactory* pf)
{
  if (filename && filename->length()) {
    HRCParser* hr = pf->getHRCParser();
    uXmlInputSource dfis = XmlInputSource::newInstance(filename->getWChars(), static_cast<const XMLCh*>(nullptr));
    try {
      hr->loadSource(dfis.get());
    } catch (Exception& e) {
      throw Exception(e);
    }
  }
}

const String* FarEditorSet::getParamDefValue(FileTypeImpl* type, SString param) const
{
  const String* value;
  value = type->getParamDefaultValue(param);
  if (value == nullptr) {
    value = defaultType->getParamValue(param);
  }
  auto* p = new SString("<default-");
  p->append(CString(value));
  p->append(CString(">"));
  return p;
}

FarList* FarEditorSet::buildHrcList() const
{
  size_t num = getCountFileTypeAndGroup();
  const String* group = nullptr;
  FileType* type = nullptr;

  auto* hrcList = new FarListItem[num];
  memset(hrcList, 0, sizeof(FarListItem) * (num));

  for (int idx = 0, i = 0;; idx++, i++) {
    type = hrcParser->enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (group != nullptr && !group->equals(type->getGroup())) {
      hrcList[i].Flags = LIF_SEPARATOR;
      i++;
    }

    group = type->getGroup();

    const wchar_t* groupChars = nullptr;

    if (group != nullptr) {
      groupChars = group->getWChars();
    }
    else {
      groupChars = L"<no group>";
    }

    hrcList[i].Text = new wchar_t[255];
    _snwprintf(const_cast<wchar_t*>(hrcList[i].Text), 255, L"%s: %s", groupChars, type->getDescription()->getWChars());
  }

  hrcList[0].Flags = LIF_SELECTED;
  auto* ListItems = new FarList;
  ListItems->Items = hrcList;
  ListItems->ItemsNumber = num;
  ListItems->StructSize = sizeof(FarList);
  return ListItems;
}

FarList* FarEditorSet::buildParamsList(FileTypeImpl* type) const
{
  // max count params
  size_t size = type->getParamCount() + defaultType->getParamCount();
  auto* fparam = new FarListItem[size];
  memset(fparam, 0, sizeof(FarListItem) * (size));

  size_t count = 0;
  std::vector<SString> default_params = defaultType->enumParams();
  for (auto& default_param : default_params) {
    fparam[count++].Text = wcsdup(default_param.getWChars());
  }
  std::vector<SString> type_params = type->enumParams();
  for (auto& type_param : type_params) {
    if (defaultType->getParamValue(type_param) == nullptr) {
      fparam[count++].Text = wcsdup(type_param.getWChars());
    }
  }

  fparam[0].Flags = LIF_SELECTED;
  auto* lparam = new FarList;
  lparam->Items = fparam;
  lparam->ItemsNumber = count;
  lparam->StructSize = sizeof(FarList);
  return lparam;
}

void FarEditorSet::ChangeParamValueListType(HANDLE hDlg, bool dropdownlist)
{
  size_t s = Info.SendDlgMessage(hDlg, DM_GETDLGITEM, IDX_CH_PARAM_VALUE_LIST, nullptr);
  auto* DialogItem = static_cast<FarDialogItem*>(calloc(1, s));
  FarGetDialogItem fgdi {};
  fgdi.Item = DialogItem;
  fgdi.StructSize = sizeof(FarGetDialogItem);
  fgdi.Size = s;
  Info.SendDlgMessage(hDlg, DM_GETDLGITEM, IDX_CH_PARAM_VALUE_LIST, &fgdi);
  DialogItem->Flags = DIF_LISTWRAPMODE;
  if (dropdownlist) {
    DialogItem->Flags |= DIF_DROPDOWNLIST;
  }
  Info.SendDlgMessage(hDlg, DM_SETDLGITEM, IDX_CH_PARAM_VALUE_LIST, DialogItem);

  free(DialogItem);
}

void FarEditorSet::setCrossValueListToCombobox(FileTypeImpl* type, HANDLE hDlg)
{
  const String* value = type->getParamUserValue(DShowCross);
  const String* def_value = getParamDefValue(type, DShowCross);

  size_t count = 5;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DNone.getWChars();
  fcross[1].Text = DVertical.getWChars();
  fcross[2].Text = DHorizontal.getWChars();
  fcross[3].Text = DBoth.getWChars();
  fcross[4].Text = def_value->getWChars();
  auto* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 4;
  }
  else {
    if (value->equals(&DNone)) {
      ret = 0;
    }
    else if (value->equals(&DVertical)) {
      ret = 1;
    }
    else if (value->equals(&DHorizontal)) {
      ret = 2;
    }
    else if (value->equals(&DBoth)) {
      ret = 3;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setCrossPosValueListToCombobox(FileTypeImpl* type, HANDLE hDlg)
{
  const String* value = type->getParamUserValue(DCrossZorder);
  const String* def_value = getParamDefValue(type, DCrossZorder);

  size_t count = 3;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DBottom.getWChars();
  fcross[1].Text = DTop.getWChars();
  fcross[2].Text = def_value->getWChars();
  auto* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->equals(&DBottom)) {
      ret = 0;
    }
    else if (value->equals(&DTop)) {
      ret = 1;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setYNListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param)
{
  const String* value = type->getParamUserValue(param);
  const String* def_value = getParamDefValue(type, param);

  size_t count = 3;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DNo.getWChars();
  fcross[1].Text = DYes.getWChars();
  fcross[2].Text = def_value->getWChars();
  auto* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->equals(&DNo)) {
      ret = 0;
    }
    else if (value->equals(&DYes)) {
      ret = 1;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setTFListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param)
{
  const String* value = type->getParamUserValue(param);
  const String* def_value = getParamDefValue(type, param);

  size_t count = 3;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DFalse.getWChars();
  fcross[1].Text = DTrue.getWChars();
  fcross[2].Text = def_value->getWChars();
  auto* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->equals(&DFalse)) {
      ret = 0;
    }
    else if (value->equals(&DTrue)) {
      ret = 1;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, true);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  delete def_value;
  delete[] fcross;
  delete lcross;
}

void FarEditorSet::setCustomListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param)
{
  const String* value = type->getParamUserValue(param);
  const String* def_value = getParamDefValue(type, param);

  size_t count = 1;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = def_value->getWChars();
  auto* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  fcross[0].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, false);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);

  if (value != nullptr) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_CH_PARAM_VALUE_LIST, (void*) value->getWChars());
  }
  delete def_value;
  delete[] fcross;
  delete lcross;
}

FileTypeImpl* FarEditorSet::getCurrentTypeInDialog(HANDLE hDlg) const
{
  auto k = static_cast<int>(Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, IDX_CH_SCHEMAS, nullptr));
  return getFileTypeByIndex(k);
}

void FarEditorSet::OnChangeHrc(HANDLE hDlg)
{
  if (menuid != -1) {
    SaveChangedValueParam(hDlg);
  }
  FileTypeImpl* type = getCurrentTypeInDialog(hDlg);
  FarList* List = buildParamsList(type);

  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_LIST, List);
  delete[] List->Items;
  delete List;
  OnChangeParam(hDlg, 0);
}

void FarEditorSet::SaveChangedValueParam(HANDLE hDlg)
{
  FarListGetItem List = {0};
  List.StructSize = sizeof(FarListGetItem);
  List.ItemIndex = menuid;
  bool res = Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);

  if (!res)
    return;

  // param name
  CString p = CString(List.Item.Text);
  // param value
  CString v = CString(trim(reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CH_PARAM_VALUE_LIST, nullptr))));
  FileTypeImpl* type = getCurrentTypeInDialog(hDlg);
  const String* value = type->getParamUserValue(p);
  const String* def_value = getParamDefValue(type, p);
  if (value == nullptr || !value->length()) {  ////было default значение
    //если его изменили
    if (!v.equals(def_value)) {
      if (type->getParamValue(p) == nullptr) {
        type->addParam(&p);
      }
      type->setParamValue(p, &v);
    }
  }
  else {                     //было пользовательское значение
    if (!v.equals(value)) {  // changed
      type->setParamValue(p, &v);
    }
  }

  delete def_value;
}

void FarEditorSet::OnChangeParam(HANDLE hDlg, intptr_t idx)
{
  if (menuid != idx && menuid != -1) {
    SaveChangedValueParam(hDlg);
  }
  FileTypeImpl* type = getCurrentTypeInDialog(hDlg);
  FarListGetItem List = {0};
  List.StructSize = sizeof(FarListGetItem);
  List.ItemIndex = idx;
  bool res = Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);
  if (!res)
    return;

  menuid = idx;
  CString p = CString(List.Item.Text);

  const String* value;
  value = type->getParamDescription(p);
  if (value == nullptr) {
    value = defaultType->getParamDescription(p);
  }
  if (value != nullptr) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_CH_DESCRIPTION, (void*) value->getWChars());
  }

  COORD c;
  c.X = 0;
  Info.SendDlgMessage(hDlg, DM_SETCURSORPOS, IDX_CH_DESCRIPTION, &c);
  if (p.equals(&DShowCross)) {
    setCrossValueListToCombobox(type, hDlg);
  }
  else {
    if (p.equals(&DCrossZorder)) {
      setCrossPosValueListToCombobox(type, hDlg);
    }
    else if (p.equals(&DMaxLen) || p.equals(&DBackparse) || p.equals(&DDefFore) || p.equals(&DDefBack) || p.equals(&CString("firstlines")) ||
             p.equals(&CString("firstlinebytes")) || p.equals(&DHotkey)) {
      setCustomListValueToCombobox(type, hDlg, CString(List.Item.Text));
    }
    else if (p.equals(&DFullback)) {
      setYNListValueToCombobox(type, hDlg, CString(List.Item.Text));
    }
    else {
      setTFListValueToCombobox(type, hDlg, CString(List.Item.Text));
    }
  }
}

void FarEditorSet::OnSaveHrcParams(HANDLE hDlg)
{
  SaveChangedValueParam(hDlg);
  FarHrcSettings p(parserFactory.get());
  p.writeUserProfile();
}

INT_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  auto* fes = reinterpret_cast<FarEditorSet*>(Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, nullptr));

  switch (Msg) {
    case DN_GOTFOCUS: {
      if (fes->dialogFirstFocus) {
        fes->menuid = -1;
        fes->OnChangeHrc(hDlg);
        fes->dialogFirstFocus = false;
      }
      return false;
    } break;
    case DN_BTNCLICK:
      switch (Param1) {
        case IDX_CH_OK:
          fes->OnSaveHrcParams(hDlg);
          return false;
          break;
        default:
          break;
      }
      break;
    case DN_EDITCHANGE:
      switch (Param1) {
        case IDX_CH_SCHEMAS:
          fes->menuid = -1;
          fes->OnChangeHrc(hDlg);
          return true;
          break;
        default:
          break;
      }
      break;
    case DN_LISTCHANGE:
      switch (Param1) {
        case IDX_CH_PARAM_LIST:
          fes->OnChangeParam(hDlg, reinterpret_cast<intptr_t>(Param2));
          return true;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool FarEditorSet::configureHrc()
{
  if (!Opt.rEnabled) {
    return false;
  }

  FarDialogItem fdi[] = {
      // type, x1, y1, x2, y2, param, history, mask, flags, userdata, ptrdata, maxlen
      {DI_DOUBLEBOX, 2, 1, 56, 21, 0, nullptr, nullptr, 0, nullptr, 0, 0},               // IDX_CH_BOX,
      {DI_TEXT, 3, 3, 0, 3, 0, nullptr, nullptr, 0, nullptr, 0, 0},                      // IDX_CH_CAPTIONLIST,
      {DI_COMBOBOX, 10, 3, 54, 2, 0, nullptr, nullptr, 0, nullptr, 0, 0},                // IDX_CH_SCHEMAS,
      {DI_LISTBOX, 3, 4, 30, 17, 0, nullptr, nullptr, 0, nullptr, 0, 0},                 // IDX_CH_PARAM_LIST,
      {DI_TEXT, 32, 5, 0, 5, 0, nullptr, nullptr, 0, nullptr, 0, 0},                     // IDX_CH_PARAM_VALUE_CAPTION
      {DI_COMBOBOX, 32, 6, 54, 6, 0, nullptr, nullptr, 0, nullptr, 0, 0},                // IDX_CH_PARAM_VALUE_LIST
      {DI_EDIT, 4, 18, 54, 18, 0, nullptr, nullptr, 0, nullptr, 0, 0},                   // IDX_CH_DESCRIPTION,
      {DI_BUTTON, 37, 20, 0, 0, 0, nullptr, nullptr, DIF_DEFAULTBUTTON, nullptr, 0, 0},  // IDX_OK,
      {DI_BUTTON, 45, 20, 0, 0, 0, nullptr, nullptr, 0, nullptr, 0, 0},                  // IDX_CANCEL,
  };

  fdi[IDX_CH_BOX].Data = GetMsg(mUserHrcSettingDialog);
  fdi[IDX_CH_CAPTIONLIST].Data = GetMsg(mListSyntax);
  FarList* l = buildHrcList();
  fdi[IDX_CH_SCHEMAS].ListItems = l;
  fdi[IDX_CH_SCHEMAS].Flags = DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;
  fdi[IDX_CH_OK].Data = GetMsg(mOk);
  fdi[IDX_CH_CANCEL].Data = GetMsg(mCancel);
  fdi[IDX_CH_PARAM_LIST].Data = GetMsg(mParamList);
  fdi[IDX_CH_PARAM_VALUE_CAPTION].Data = GetMsg(mParamValue);
  fdi[IDX_CH_DESCRIPTION].Flags = DIF_READONLY;

  fdi[IDX_CH_PARAM_LIST].Flags = DIF_LISTWRAPMODE | DIF_LISTNOCLOSE;
  fdi[IDX_CH_PARAM_VALUE_LIST].Flags = DIF_LISTWRAPMODE;

  dialogFirstFocus = true;
  HANDLE hDlg = Info.DialogInit(&MainGuid, &HrcPluginConfig, -1, -1, 59, 23, L"confighrc", fdi, std::size(fdi), 0, 0, SettingHrcDialogProc, this);
  Info.DialogRun(hDlg);

  for (size_t idx = 0; idx < l->ItemsNumber; idx++) {
    delete[] l->Items[idx].Text;
  }
  delete[] l->Items;
  delete l;

  Info.DialogFree(hDlg);
  return true;
}

void FarEditorSet::showExceptionMessage(const wchar_t* message)
{
  const wchar_t* exceptionMessage[3] = {GetMsg(mName), message, GetMsg(mDie)};
  Info.Message(&MainGuid, &ErrorMessage, FMSG_WARNING, L"exception", &exceptionMessage[0], std::size(exceptionMessage), 1);
}

bool FarEditorSet::configureLogging()
{
  const wchar_t* levelList[] = {L"error", L"warning", L"info", L"debug"};
  const auto level_count = std::size(levelList);

  int log_level = 0;

  for (size_t i = 0; i < level_count; ++i) {
    if (SString(levelList[i]) == SString(Opt.logLevel)) {
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

HANDLE FarEditorSet::openFromMacro(const struct OpenInfo* oInfo)
{
  auto area = (FARMACROAREA) Info.MacroControl(&MainGuid, MCTL_GETAREA, 0, nullptr);
  auto* mi = (OpenMacroInfo*) oInfo->Data;
  if (mi->Count == 1)
    return oldMacro(area, mi);
  else
    return execMacro(area, mi);
}

HANDLE FarEditorSet::openFromCommandLine(const struct OpenInfo* oInfo)
{
  auto* ocli = (OpenCommandLineInfo*) oInfo->Data;
  // file name, which we received
  const wchar_t* file = ocli->CommandLine;

  wchar_t* nfile = PathToFull(file, true);
  if (nfile) {
    viewFile(CString(nfile));
  }

  delete[] nfile;
  return nullptr;
}

void FarEditorSet::setEmptyLogger()
{
  spdlog::drop_all();
  log = spdlog::null_logger_mt("main");
  spdlog::set_default_logger(log);
}

int FarEditorSet::getHrdArrayWithCurrent(const wchar_t* current, std::vector<const HRDNode*>* hrd_instances, std::vector<const wchar_t*>* out_array)
{
  size_t hrd_count = hrd_instances->size();
  auto current_style = 0;

  for (size_t i = 0; i < hrd_count; i++) {
    const HRDNode* hrd_node = hrd_instances->at(i);

    if (hrd_node->hrd_description.length() != 0) {
      out_array->push_back(hrd_node->hrd_description.getWChars());
    }
    else {
      out_array->push_back(hrd_node->hrd_name.getWChars());
    }

    if (SString(current).equals(&hrd_node->hrd_name)) {
      current_style = (int) i;
    }
  }
  return current_style;
}

#pragma region macro_functions

// TODO remove in 01/01/2021
void* FarEditorSet::oldMacro(FARMACROAREA area, OpenMacroInfo* params)
{
  int MenuCode = -1;
  std::unique_ptr<SString> command = nullptr;
  if (params->Count) {
    switch (params->Values[0].Type) {
      case FMVT_INTEGER:
        MenuCode = (int) params->Values[0].Integer;
        break;
      case FMVT_DOUBLE:
        MenuCode = (int) params->Values[0].Double;
        break;
      case FMVT_STRING:
        command = std::make_unique<SString>(CString(params->Values[0].String));
        break;
      default:
        MenuCode = -1;
    }
  }

  if (MenuCode >= 0 && area == MACROAREA_EDITOR) {
    openMenu(MenuCode - 1);
    return INVALID_HANDLE_VALUE;
  }
  else if (command) {
    if (CString("status").equals(command.get())) {
      if (params->Count == 1) {
        return isEnable() ? INVALID_HANDLE_VALUE : nullptr;
      }
      else {
        bool new_status = false;
        switch (params->Values[1].Type) {
          case FMVT_BOOLEAN:
            new_status = static_cast<bool>(params->Values[1].Boolean);
            break;
          case FMVT_INTEGER:
            new_status = static_cast<bool>(params->Values[1].Integer);
            break;
          default:
            new_status = true;
        }

        if (new_status) {
          enableColorer();
          return isEnable() ? INVALID_HANDLE_VALUE : nullptr;
        }
        else {
          disableColorer();
          return !isEnable() ? INVALID_HANDLE_VALUE : nullptr;
        }
      }
    }
  }
  return nullptr;
}

void* FarEditorSet::macroSettings(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR)
    return nullptr;
  SString command = SString(CString(params->Values[1].String));

  if (CString("Main").equalsIgnoreCase(&command)) {
    return configure() ? INVALID_HANDLE_VALUE : nullptr;
  }
  if (CString("Log").equalsIgnoreCase(&command)) {
    return configureLogging() ? INVALID_HANDLE_VALUE : nullptr;
  }
  if (CString("Hrc").equalsIgnoreCase(&command)) {
    return configureHrc() ? INVALID_HANDLE_VALUE : nullptr;
  }

  return nullptr;
}

void* FarEditorSet::macroTypes(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled)
    return nullptr;
  SString command = SString(CString(params->Values[1].String));

  if (CString("Menu").equalsIgnoreCase(&command)) {
    return chooseType() ? INVALID_HANDLE_VALUE : nullptr;
  }

  return nullptr;
}

void* FarEditorSet::macroBrackets(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor)
    return nullptr;

  SString command = SString(CString(params->Values[1].String));
  if (CString("Match").equalsIgnoreCase(&command)) {
    editor->matchPair();
    return INVALID_HANDLE_VALUE;
  }
  if (CString("SelectAll").equalsIgnoreCase(&command)) {
    editor->selectBlock();
    return INVALID_HANDLE_VALUE;
  }
  if (CString("SelectIn").equalsIgnoreCase(&command)) {
    editor->selectPair();
    return INVALID_HANDLE_VALUE;
  }

  return nullptr;
}

void* FarEditorSet::macroRegion(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor)
    return nullptr;

  SString command = SString(CString(params->Values[1].String));
  if (CString("Select").equalsIgnoreCase(&command)) {
    editor->selectRegion();
    return INVALID_HANDLE_VALUE;
  }
  if (CString("Show").equalsIgnoreCase(&command)) {
    editor->getNameCurrentScheme();
    return INVALID_HANDLE_VALUE;
  }

  return nullptr;
}

void* FarEditorSet::macroFunctions(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor)
    return nullptr;

  SString command = SString(CString(params->Values[1].String));
  if (CString("Show").equalsIgnoreCase(&command)) {
    editor->listFunctions();
    return INVALID_HANDLE_VALUE;
  }
  if (CString("Find").equalsIgnoreCase(&command)) {
    editor->locateFunction();
    return INVALID_HANDLE_VALUE;
  }

  return nullptr;
}

void* FarEditorSet::macroErrors(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor)
    return nullptr;

  SString command = SString(CString(params->Values[1].String));
  if (CString("Show").equalsIgnoreCase(&command)) {
    editor->listErrors();
    return INVALID_HANDLE_VALUE;
  }

  return nullptr;
}

void* FarEditorSet::macroEditor(FARMACROAREA area, OpenMacroInfo* params)
{
  if (area != MACROAREA_EDITOR || !Opt.rEnabled)
    return nullptr;

  FarEditor* editor = getCurrentEditor();
  if (!editor)
    return nullptr;
  SString command = SString(CString(params->Values[1].String));
  if (CString("CrossVisible").equalsIgnoreCase(&command)) {
    return macroReturnInt(editor->getVisibleCrossState());
  }
  if (CString("CrossStatus").equalsIgnoreCase(&command)) {
    // current status
    auto cur_status = editor->getCrossStatus();
    if (params->Count > 2) {
      // change status
      int val = static_cast<int>(macroGetValue(params->Values + 2));
      editor->setCrossStatus(val);
    }

    return macroReturnInt(cur_status);
  }
  if (CString("CrossType").equalsIgnoreCase(&command)) {
    // current style
    auto cur_style = editor->getCrossStyle();
    if (params->Count > 2) {
      // change style
      int val = static_cast<int>(macroGetValue(params->Values + 2));
      editor->setCrossStyle(val);
    }

    return macroReturnInt(cur_style);
  }
  return nullptr;
}

void* FarEditorSet::execMacro(FARMACROAREA area, OpenMacroInfo* params)
{
  if (params->Count < 2 || params->Values[0].Type != FMVT_STRING)
    return nullptr;

  SString command_type = SString(CString(params->Values[0].String));
  if (CString("Settings").equalsIgnoreCase(&command_type)) {
    return macroSettings(area, params);
  }

  if (CString("Types").equalsIgnoreCase(&command_type)) {
    return macroTypes(area, params);
  }

  if (CString("Brackets").equalsIgnoreCase(&command_type)) {
    return macroBrackets(area, params);
  }

  if (CString("Region").equalsIgnoreCase(&command_type)) {
    return macroRegion(area, params);
  }

  if (CString("Functions").equalsIgnoreCase(&command_type)) {
    return macroFunctions(area, params);
  }

  if (CString("Errors").equalsIgnoreCase(&command_type)) {
    return macroErrors(area, params);
  }

  if (CString("Editor").equalsIgnoreCase(&command_type)) {
    return macroEditor(area, params);
  }

  return nullptr;
}

#pragma endregion macro_functions