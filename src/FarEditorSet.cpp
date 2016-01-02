#include "FarEditorSet.h"
#include <xml/XmlParserErrorHandler.h>
#include <colorer/handlers/FileErrorHandler.h>
#include <colorer/ParserFactoryException.h>

FarEditorSet::FarEditorSet()
{
  in_construct = true;
  err_status = ERR_NO_ERROR;
  parserFactory = nullptr;
  regionMapper = nullptr;
  hrcParser = nullptr;
  sHrdName = nullptr;
  sHrdNameTm = nullptr;
  sCatalogPath = nullptr;
  sCatalogPathExp = nullptr;
  sTempHrdName = nullptr;
  sTempHrdNameTm = nullptr;
  dialogFirstFocus = false;
  menuid = 0;
  sUserHrdPath = nullptr;
  sUserHrdPathExp = nullptr;
  sUserHrcPath = nullptr;
  sUserHrcPathExp = nullptr;
  error_handler = nullptr;
  sLogPath = nullptr;
  sLogPathExp = nullptr;
  xercesc::XMLPlatformUtils::Initialize();
  ReloadBase();
  viewFirst = 0;
  CurrentMenuItem = 0;
  in_construct = false;
}

FarEditorSet::~FarEditorSet()
{
  dropAllEditors(false);
  delete sHrdName;
  delete sHrdNameTm;
  delete sCatalogPath;
  delete sCatalogPathExp;
  delete sUserHrdPath;
  delete sUserHrdPathExp;
  delete sUserHrcPath;
  delete sUserHrcPathExp;
  delete sLogPath;
  delete sLogPathExp;
  delete regionMapper;
  delete parserFactory;
  delete error_handler;
  xercesc::XMLPlatformUtils::Terminate();
}

void FarEditorSet::openMenu(int MenuId)
{
  if (MenuId <= 0) {
    const size_t menu_size = 13;
    int iMenuItems[menu_size] = {
      mListTypes, mMatchPair, mSelectBlock, mSelectPair,
      mListFunctions, mFindErrors, mSelectRegion, mCurrentRegionName, mLocateFunction, -1,
      mUpdateHighlight, mReloadBase, mConfigure
    };
    FarMenuItem menuElements[menu_size];
    memset(menuElements, 0, sizeof(menuElements));
    if (rEnabled) {
      menuElements[0].Flags = MIF_SELECTED;
    }
    for (int i = menu_size - 1; i >= 0; i--) {
      if (iMenuItems[i] == -1) {
        menuElements[i].Flags |= MIF_SEPARATOR;;
      } else {
        menuElements[i].Text = GetMsg(iMenuItems[i]);
      }
    }

    intptr_t menu_id = Info.Menu(&MainGuid, &PluginMenu, -1, -1, 0, FMENU_WRAPMODE, GetMsg(mName), 0, L"menu", nullptr, nullptr,
                                 rEnabled ? menuElements : menuElements + 12, rEnabled ? menu_size : 1);
    if (!rEnabled && menu_id == 0) {
      MenuId = 12;
    } else {
      MenuId = menu_id;
    }

  }
  if (MenuId >= 0) {
    try {
      FarEditor* editor = getCurrentEditor();
      if (!editor && (rEnabled || MenuId != 12)) {
        throw Exception(DString("Can't find current editor in array."));
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
          configure(true);
          break;
      }
    } catch (Exception &e) {
      if (getErrorHandler()) {
        getErrorHandler()->error(*e.getMessage());
      }

      StringBuffer msg("openMenu: ");
      msg.append(e.getMessage());
      showExceptionMessage(msg.getWChars());
      disableColorer();
    }
  }
}


void FarEditorSet::viewFile(const String &path)
{
  if (viewFirst == 0) {
    viewFirst = 1;
  }
  try {
    if (!rEnabled) {
      throw Exception(DString("FarColorer is disabled"));
    }

    // Creates store of text lines
    TextLinesStore textLinesStore;
    textLinesStore.loadFile(&path, nullptr, true);
    // Base editor to make primary parse
    BaseEditor baseEditor(parserFactory, &textLinesStore);
    RegionMapper* regionMap;
    try {
      regionMap = parserFactory->createStyledMapper(&DConsole, sHrdName);
    } catch (ParserFactoryException &e) {
      if (getErrorHandler() != nullptr) {
        getErrorHandler()->error(*e.getMessage());
      }
      regionMap = parserFactory->createStyledMapper(&DConsole, nullptr);
    }
    baseEditor.setRegionMapper(regionMap);
    baseEditor.chooseFileType(&path);
    // initial event
    baseEditor.lineCountEvent(textLinesStore.getLineCount());
    // computing background color
    int background = 0x1F;
    const StyledRegion* rd = StyledRegion::cast(regionMap->getRegionDefine(DString("def:Text")));

    if (rd != nullptr && rd->bfore && rd->bback) {
      background = rd->fore + (rd->back << 4);
    }

    // File viewing in console window
    TextConsoleViewer viewer(&baseEditor, &textLinesStore, background, -1);
    viewer.view();
    delete regionMap;
  } catch (Exception &e) {
    showExceptionMessage(e.getMessage()->getWChars());
  }
}

size_t FarEditorSet::getCountFileTypeAndGroup()
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

FileTypeImpl* FarEditorSet::getFileTypeByIndex(int idx)
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

  return (FileTypeImpl*)type;
}

void FarEditorSet::FillTypeMenu(ChooseTypeMenu* Menu, FileType* CurFileType)
{
  const String* group = nullptr;
  FileType* type = nullptr;

  group = &DAutodetect;

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
    v = ((FileTypeImpl*)type)->getParamValue(DFavorite);
    if (v && v->equals(&DTrue)) {
      i = Menu->AddFavorite(type);
    } else {
      i = Menu->AddItem(type);
    }
    if (type == CurFileType) {
      Menu->SetSelected(i);
    }
  }

}

inline wchar_t* __cdecl Upper(wchar_t* Ch)
{
  CharUpperBuff(Ch, 1);
  return Ch;
}

INT_PTR WINAPI KeyDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  INPUT_RECORD* record = nullptr;
  wchar wkey[2];

  record = (INPUT_RECORD*)Param2;
  if (Msg == DN_CONTROLINPUT && record->EventType == KEY_EVENT) {
    int key = record->Event.KeyEvent.wVirtualKeyCode;
    if (key == VK_ESCAPE  || key == VK_RETURN) {
      return FALSE;
    }
    if (key > 31  && key != VK_F1) {
      FSF.FarInputRecordToName((const INPUT_RECORD*)Param2, wkey, 2);
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

void FarEditorSet::chooseType()
{
  FarEditor* fe = getCurrentEditor();
  if (!fe) {
    return;
  }

  ChooseTypeMenu menu(GetMsg(mAutoDetect), GetMsg(mFavorites));
  FillTypeMenu(&menu, fe->getFileType());

  wchar_t bottom[20];
  _snwprintf(bottom, 20, GetMsg(mTotalTypes), hrcParser->getFileTypesCount());
  struct FarKey BreakKeys[3] = {VK_INSERT, 0, VK_DELETE, 0, VK_F4, 0};
  intptr_t BreakCode;
  while (1) {
    intptr_t i = Info.Menu(&MainGuid, &FileChooseMenu, -1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT,
                           GetMsg(mSelectSyntax), bottom, L"filetypechoose", BreakKeys, &BreakCode, menu.getItems(), menu.getItemsCount());

    if (i >= 0) {
      if (BreakCode == 0) {
        if (i != 0 && !menu.IsFavorite(i)) {
          menu.MoveToFavorites(i);
        } else {
          menu.SetSelected(i);
        }
      } else if (BreakCode == 1) {
        if (i != 0 && menu.IsFavorite(i)) {
          menu.DelFromFavorites(i);
        } else {
          menu.SetSelected(i);
        }
      } else if (BreakCode == 2) {
        if (i == 0)  {
          menu.SetSelected(i);
          continue;
        }

        FarDialogItem KeyAssignDlgData[] = {
          {DI_DOUBLEBOX, 3, 1, 30, 4, 0, nullptr, nullptr, 0, GetMsg(mKeyAssignDialogTitle)},
          {DI_TEXT, -1, 2, 0, 2, 0, nullptr, nullptr, 0, GetMsg(mKeyAssignTextTitle)},
          {DI_EDIT, 5, 3, 28, 3, 0, nullptr, nullptr, DIF_FOCUS | DIF_DEFAULTBUTTON, L""},
        };

        const String* v;
        v = ((FileTypeImpl*)menu.GetFileType(i))->getParamValue(DHotkey);
        if (v && v->length()) {
          KeyAssignDlgData[2].Data = v->getWChars();
        }

        HANDLE hDlg = Info.DialogInit(&MainGuid, &AssignKeyDlg, -1, -1, 34, 6, L"keyassign", KeyAssignDlgData, ARRAY_SIZE(KeyAssignDlgData), 0, 0, KeyDialogProc, nullptr);
        intptr_t res = Info.DialogRun(hDlg);

        if (res != -1) {
          KeyAssignDlgData[2].Data = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, 2, 0));
          if (menu.GetFileType(i)->getParamValue(DHotkey) == nullptr) {
            ((FileTypeImpl*)menu.GetFileType(i))->addParam(&DHotkey);
          }
          menu.GetFileType(i)->setParamValue(DHotkey, &DString(KeyAssignDlgData[2].Data));
          menu.RefreshItemCaption(i);
        }
        menu.SetSelected(i);
        Info.DialogFree(hDlg);
      } else {
        if (i == 0) {
          String* s = getCurrentFileName();
          fe->chooseFileType(s);
          delete s;
          break;
        }
        fe->setFileType(menu.GetFileType(i));
        break;
      }
    } else {
      break;
    }
  }

  FarHrcSettings p(parserFactory);
  p.writeUserProfile();
}

const String* FarEditorSet::getHRDescription(const String &name, DString _hrdClass)
{
  const String* descr = nullptr;
  if (parserFactory != nullptr) {
    descr = parserFactory->getHRDescription(_hrdClass, name);
  }

  if (descr == nullptr) {
    descr = &name;
  }

  return descr;
}

INT_PTR WINAPI SettingDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  FarEditorSet* fes = (FarEditorSet*)Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);;

  switch (Msg) {
    case DN_BTNCLICK:
      switch (Param1) {
        case IDX_HRD_SELECT: {
          SString* tempSS = new SString(fes->chooseHRDName(fes->sTempHrdName, DConsole));
          delete fes->sTempHrdName;
          fes->sTempHrdName = tempSS;
          const String* descr = fes->getHRDescription(*fes->sTempHrdName, DConsole);
          Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_HRD_SELECT, (void*)descr->getWChars());
          Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
          return true;
        }
        break;
        case IDX_HRD_SELECT_TM: {
          SString* tempSS = new SString(fes->chooseHRDName(fes->sTempHrdNameTm, DRgb));
          delete fes->sTempHrdNameTm;
          fes->sTempHrdNameTm = tempSS;
          const String* descr = fes->getHRDescription(*fes->sTempHrdNameTm, DRgb);
          Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_HRD_SELECT_TM, (void*)descr->getWChars());
          Info.SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
          return true;
        }
        break;
        case IDX_RELOAD_ALL: {
          Info.SendDlgMessage(hDlg, DM_SHOWDIALOG , false, 0);
          wchar_t* catalog = trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CATALOG_EDIT, 0));
          wchar_t* userhrd = trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRD_EDIT, 0));
          wchar_t* userhrc = trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRC_EDIT, 0));
          bool trumod = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0);
          fes->TestLoadBase(catalog, userhrd, userhrc, true, trumod ? FarEditorSet::HRCM_BOTH : FarEditorSet::HRCM_CONSOLE);
          Info.SendDlgMessage(hDlg, DM_SHOWDIALOG , true, 0);
          return true;
        }
        break;
        case IDX_HRC_SETTING: {
          fes->configureHrc();
          return true;
        }
        break;
        case IDX_OK:
          const wchar_t* temp = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CATALOG_EDIT, 0));
          const wchar_t* userhrd = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRD_EDIT, 0));
          const wchar_t* userhrc = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRC_EDIT, 0));
          bool trumod = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0);
          int k = (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_ENABLED, 0);

          if (fes->GetCatalogPath()->compareTo(DString(temp)) || fes->GetUserHrdPath()->compareTo(DString(userhrd))
              || (!fes->GetPluginStatus() && k) || (trumod == true)) {
            if (fes->TestLoadBase(temp, userhrd, userhrc, false, trumod ? FarEditorSet::HRCM_BOTH : FarEditorSet::HRCM_CONSOLE)) {
              return false;
            } else {
              return true;
            }
          }

          return false;
          break;
      }
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void FarEditorSet::configure(bool fromEditor)
{
  try {
    FarDialogItem fdi[] = {
      // type, x1, y1, x2, y2, param, history, mask, flags,  data, maxlen,userdata
      { DI_DOUBLEBOX, 3, 1, 55, 25, 0, 0, 0, 0, 0, 0, 0},     //IDX_BOX,
      { DI_CHECKBOX, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_DISABLED,
      { DI_CHECKBOX, 5, 3, 0, 0, 0, 0, 0, DIF_3STATE, 0, 0, 0}, //IDX_CROSS,
      { DI_TEXT, 7, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0},            //IDX_CROSS_TEXT,
      { DI_COMBOBOX, 20, 4, 40, 4, 0, 0, 0, 0, 0, 0, 0},      //IDX_CROSS_STYLE,
      { DI_CHECKBOX, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_PAIRS,
      { DI_CHECKBOX, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_SYNTAX,
      { DI_CHECKBOX, 5, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_OLDOUTLINE,
      { DI_CHECKBOX, 5, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_CHANGE_BG,
      { DI_TEXT, 5, 9, 0, 9, 0, 0, 0, 0, 0, 0, 0},            //IDX_HRD,
      { DI_BUTTON, 20, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0},         //IDX_HRD_SELECT,
      { DI_TEXT, 5, 10, 0, 10, 0, 0, 0, 0, 0, 0, 0},            //IDX_CATALOG,
      { DI_EDIT, 6, 11, 52, 11, 0, L"catalog", 0, DIF_HISTORY, 0, 0, 0}, //IDX_CATALOG_EDIT
      { DI_TEXT, 5, 12, 0, 12, 0, 0, 0, 0, 0, 0, 0},          //IDX_USERHRC,
      { DI_EDIT, 6, 13, 52, 13, 0, L"userhrc", 0, DIF_HISTORY, 0, 0, 0}, //IDX_USERHRC_EDIT
      { DI_TEXT, 5, 14, 0, 14, 0, 0, 0, 0, 0, 0, 0},          //IDX_USERHRD,
      { DI_EDIT, 6, 15, 52, 15, 0, L"userhrd", 0, DIF_HISTORY, 0, 0, 0}, //IDX_USERHRD_EDIT
      { DI_TEXT, 5, 16, 0, 16, 0, 0, 0, 0, 0, 0, 0},          //IDX_LOG,
      { DI_EDIT, 6, 17, 52, 17, 0, L"log", 0, DIF_HISTORY, 0, 0, 0}, //IDX_LOG_EDIT
      { DI_SINGLEBOX, 4, 18, 54, 18, 0, 0, 0, 0, 0, 0, 0},    //IDX_TM_BOX,
      { DI_CHECKBOX, 5, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0},       //IDX_TRUEMOD,
      { DI_TEXT, 5, 20, 0, 20, 0, 0, 0, 0, 0, 0, 0},          //IDX_HRD_TM,
      { DI_BUTTON, 20, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_HRD_SELECT_TM,
      { DI_SINGLEBOX, 4, 21, 54, 21, 0, 0, 0, 0, 0, 0, 0},    //IDX_TM_BOX_OFF,
      { DI_BUTTON, 5, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0},         //IDX_RELOAD_ALL,
      { DI_BUTTON, 30, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_HRC_SETTING,
      { DI_BUTTON, 35, 23, 0, 0, 0, 0, 0, DIF_DEFAULTBUTTON, 0, 0, 0}, //IDX_OK,
      { DI_BUTTON, 45, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0},        //IDX_CANCEL,
    };//type, x1, y1, x2, y2, param, history, mask, flags,  data, maxlen,userdata

    fdi[IDX_BOX].Data = GetMsg(mSetup);
    fdi[IDX_ENABLED].Data = GetMsg(mTurnOff);
    fdi[IDX_ENABLED].Selected = rEnabled;
    fdi[IDX_TRUEMOD].Data = GetMsg(mTrueMod);
    fdi[IDX_TRUEMOD].Selected = TrueModOn;
    fdi[IDX_CROSS].Data = GetMsg(mCross);
    fdi[IDX_CROSS].Selected = drawCross;
    fdi[IDX_CROSS_TEXT].Data = GetMsg(mCrossText);

    FarList fl;
    FarListItem* style_list = new FarListItem[3];
    memset(style_list, 0, sizeof(FarListItem) * 3);
    style_list[0].Text = GetMsg(mCrossBoth);
    style_list[1].Text = GetMsg(mCrossVert);
    style_list[2].Text = GetMsg(mCrossHoriz);
    style_list[CrossStyle].Flags = LIF_SELECTED;
    fl.StructSize = sizeof(FarList);
    fl.ItemsNumber = 3;
    fl.Items = style_list;
    fdi[IDX_CROSS_STYLE].ListItems = &fl;
    fdi[IDX_CROSS_STYLE].Flags = DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;

    fdi[IDX_PAIRS].Data = GetMsg(mPairs);
    fdi[IDX_PAIRS].Selected = drawPairs;
    fdi[IDX_SYNTAX].Data = GetMsg(mSyntax);
    fdi[IDX_SYNTAX].Selected = drawSyntax;
    fdi[IDX_OLDOUTLINE].Data = GetMsg(mOldOutline);
    fdi[IDX_OLDOUTLINE].Selected = oldOutline;
    fdi[IDX_CATALOG].Data = GetMsg(mCatalogFile);
    fdi[IDX_CATALOG_EDIT].Data = sCatalogPath->getWChars();
    fdi[IDX_USERHRC].Data = GetMsg(mUserHrcFile);
    fdi[IDX_USERHRC_EDIT].Data = sUserHrcPath->getWChars();
    fdi[IDX_USERHRD].Data = GetMsg(mUserHrdFile);
    fdi[IDX_USERHRD_EDIT].Data = sUserHrdPath->getWChars();
    fdi[IDX_HRD].Data = GetMsg(mHRDName);

    const String* descr = nullptr;
    sTempHrdName = new SString(sHrdName);
    descr = getHRDescription(*sTempHrdName, DConsole);

    fdi[IDX_HRD_SELECT].Data = descr->getWChars();
    const String* descr2 = nullptr;
    sTempHrdNameTm = new SString(sHrdNameTm);
    descr2 = getHRDescription(*sTempHrdNameTm, DRgb);

    fdi[IDX_HRD_TM].Data = GetMsg(mHRDNameTrueMod);
    fdi[IDX_HRD_SELECT_TM].Data = descr2->getWChars();
    fdi[IDX_CHANGE_BG].Data = GetMsg(mChangeBackgroundEditor);
    fdi[IDX_CHANGE_BG].Selected = ChangeBgEditor;
    fdi[IDX_RELOAD_ALL].Data = GetMsg(mReloadAll);
    fdi[IDX_HRC_SETTING].Data = GetMsg(mUserHrcSetting);
    fdi[IDX_OK].Data = GetMsg(mOk);
    fdi[IDX_CANCEL].Data = GetMsg(mCancel);
    fdi[IDX_TM_BOX].Data = GetMsg(mTrueModSetting);

    fdi[IDX_LOG].Data = GetMsg(mLog);
    fdi[IDX_LOG_EDIT].Data = sLogPath->getWChars();

    /*
    * Dialog activation
    */
    HANDLE hDlg = Info.DialogInit(&MainGuid, &PluginConfig, -1, -1, 58, 25, L"config", fdi, ARRAY_SIZE(fdi), 0, 0, SettingDialogProc, this);
    intptr_t i = Info.DialogRun(hDlg);

    if (i == IDX_OK) {
      fdi[IDX_CATALOG_EDIT].Data = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CATALOG_EDIT, 0));
      fdi[IDX_USERHRD_EDIT].Data = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRD_EDIT, 0));
      fdi[IDX_USERHRC_EDIT].Data = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_USERHRC_EDIT, 0));
      fdi[IDX_LOG_EDIT].Data = (const wchar_t*)trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_LOG_EDIT, 0));
      //check whether or not to reload the base
      int k = false;

      if (sCatalogPath->compareTo(DString(fdi[IDX_CATALOG_EDIT].Data)) ||
          sUserHrdPath->compareTo(DString(fdi[IDX_USERHRD_EDIT].Data)) ||
          sUserHrcPath->compareTo(DString(fdi[IDX_USERHRC_EDIT].Data)) ||
          sHrdName->compareTo(*sTempHrdName) ||
          sHrdNameTm->compareTo(*sTempHrdNameTm)) {
        k = true;
      }

      fdi[IDX_ENABLED].Selected = (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_ENABLED, 0);
      drawCross = (int)Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_CROSS, 0);
      CrossStyle = (int)Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, IDX_CROSS_STYLE, 0);
      drawPairs = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_PAIRS, 0);
      drawSyntax = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_SYNTAX, 0);
      oldOutline = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_OLDOUTLINE, 0);
      ChangeBgEditor = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_CHANGE_BG, 0);
      fdi[IDX_TRUEMOD].Selected = !!Info.SendDlgMessage(hDlg, DM_GETCHECK, IDX_TRUEMOD, 0);
      delete sHrdName;
      delete sHrdNameTm;
      delete sCatalogPath;
      delete sUserHrdPath;
      delete sUserHrcPath;
      sHrdName = sTempHrdName;
      sHrdNameTm = sTempHrdNameTm;
      sCatalogPath = new SString(DString(fdi[IDX_CATALOG_EDIT].Data));
      sUserHrdPath = new SString(DString(fdi[IDX_USERHRD_EDIT].Data));
      sUserHrcPath = new SString(DString(fdi[IDX_USERHRC_EDIT].Data));
      setLogPath(fdi[IDX_LOG_EDIT].Data);

      // if the plugin has been enable, and we will disable
      if (rEnabled && !fdi[IDX_ENABLED].Selected) {
        rEnabled = false;
        TrueModOn = !!(fdi[IDX_TRUEMOD].Selected);
        SaveSettings();
        disableColorer();
      } else {
        if ((!rEnabled && fdi[IDX_ENABLED].Selected) || k) {
          rEnabled = true;
          TrueModOn = !!(fdi[IDX_TRUEMOD].Selected);
          SaveSettings();
          ReloadBase();
        } else {
          if (TrueModOn != !!fdi[IDX_TRUEMOD].Selected) {
            TrueModOn = !!(fdi[IDX_TRUEMOD].Selected);
            SaveSettings();
            ReloadBase();
          } else {
            SaveSettings();
            ApplySettingsToEditors();
            SetBgEditor();
          }
        }
      }
    }

    Info.DialogFree(hDlg);
    delete[] style_list;

  } catch (Exception &e) {
    if (getErrorHandler() != nullptr) {
      getErrorHandler()->error(*e.getMessage());
    }

    StringBuffer msg("configure: ");
    msg.append(e.getMessage());
    showExceptionMessage(msg.getWChars());
    disableColorer();
  }
}

const SString FarEditorSet::chooseHRDName(const String* current, DString _hrdClass)
{
  if (parserFactory == nullptr) {
    return current;
  }

  std::vector<SString> hrd_instances = parserFactory->enumHRDInstances(_hrdClass);
  int count = hrd_instances.size();
  FarMenuItem* menuElements = new FarMenuItem[count];
  memset(menuElements, 0, sizeof(FarMenuItem)*count);

  for (int i = 0; i < count; i++) {
    const SString name = hrd_instances.at(i);
    const String* descr = parserFactory->getHRDescription(_hrdClass, name);

    if (descr == nullptr) {
      descr = &name;
    }

    menuElements[i].Text = descr->getWChars();

    if (current->equals(&name)) {
      menuElements[i].Flags = MIF_SELECTED;
    }
  }

  intptr_t result = Info.Menu(&MainGuid, &HrdMenu, -1, -1, 0, FMENU_WRAPMODE | FMENU_AUTOHIGHLIGHT,
                              GetMsg(mSelectHRD), 0, L"hrd", nullptr, nullptr, menuElements, count);
  delete[] menuElements;

  if (result == -1) {
    return current;
  }

  return hrd_instances.at(result);
}

int FarEditorSet::editorInput(const INPUT_RECORD &Rec)
{
  if (rEnabled) {
    FarEditor* editor = getCurrentEditor();
    if (editor) {
      return editor->editorInput(Rec);
    }
  }
  return 0;
}

int FarEditorSet::editorEvent(const struct ProcessEditorEventInfo* pInfo)
{
  // check whether all the editors cleaned
  if (!rEnabled && farEditorInstances.size() && pInfo->Event == EE_GOTFOCUS) {
    dropCurrentEditor(true);
    return 0;
  }

  if (!rEnabled) {
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
      }
      break;
      case EE_CHANGE: {
        //запрещено вызывать EditorControl (getCurrentEditor)
        auto it_editor = farEditorInstances.find(&SString(pInfo->EditorID));
        if (it_editor != farEditorInstances.end()) {
          return it_editor->second->editorEvent(pInfo->Event, pInfo->Param);
        } else {
          return 0;
        }
      }
      break;
      case EE_READ: {
        addCurrentEditor();
        return 0;
      }
      break;
      case EE_CLOSE: {
        auto it_editor = farEditorInstances.find(&SString(pInfo->EditorID));
        delete it_editor->second;
        farEditorInstances.erase(&SString(pInfo->EditorID));
        return 0;
      }
      break;
    }
  } catch (Exception &e) {
    if (getErrorHandler()) {
      getErrorHandler()->error(*e.getMessage());
    }

    StringBuffer msg("editorEvent: ");
    msg.append(e.getMessage());
    showExceptionMessage(msg.getWChars());
    disableColorer();
  }

  return 0;
}

bool FarEditorSet::TestLoadBase(const wchar_t* catalogPath, const wchar_t* userHrdPath, const wchar_t* userHrcPath, const int full, const HRC_MODE hrc_mode)
{
  bool res = true;
  const wchar_t* marr[2] = { GetMsg(mName), GetMsg(mReloading) };
  HANDLE scr = Info.SaveScreen(0, 0, -1, -1);
  Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], 2, 0);

  ParserFactory* parserFactoryLocal = nullptr;
  RegionMapper* regionMapperLocal = nullptr;
  HRCParser* hrcParserLocal = nullptr;

  SString* catalogPathS = PathToFullS(catalogPath, false);
  SString* userHrdPathS = PathToFullS(userHrdPath, false);
  SString* userHrcPathS = PathToFullS(userHrcPath, false);

  SString* tpath;
  if (!catalogPathS || !catalogPathS->length()) {
    StringBuffer* path = new StringBuffer(PluginPath);
    path->append(DString(FarCatalogXml));
    tpath = path;
  } else {
    tpath = catalogPathS;
  }

  try {
    parserFactoryLocal = new ParserFactory(error_handler);
    parserFactoryLocal->loadCatalog(tpath);
    delete tpath;
    hrcParserLocal = parserFactoryLocal->getHRCParser();
    LoadUserHrd(userHrdPathS, parserFactoryLocal);
    LoadUserHrc(userHrcPathS, parserFactoryLocal);
    FarHrcSettings p(parserFactoryLocal);
    p.readProfile();
    p.readUserProfile();

    if (hrc_mode == HRCM_CONSOLE || hrc_mode == HRCM_BOTH) {
      try {
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, sTempHrdName);
      } catch (ParserFactoryException &e) {
        if ((parserFactoryLocal != nullptr) && (parserFactoryLocal->getErrorHandler() != nullptr)) {
          parserFactoryLocal->getErrorHandler()->error(*e.getMessage());
        }
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DConsole, nullptr);
      }
      delete regionMapperLocal;
      regionMapperLocal = nullptr;
    }

    if (hrc_mode == HRCM_RGB || hrc_mode == HRCM_BOTH) {
      try {
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, sTempHrdNameTm);
      } catch (ParserFactoryException &e) {
        if ((parserFactoryLocal != nullptr) && (parserFactoryLocal->getErrorHandler() != nullptr)) {
          parserFactoryLocal->getErrorHandler()->error(*e.getMessage());
        }
        regionMapperLocal = parserFactoryLocal->createStyledMapper(&DRgb, nullptr);
      }
    }

    Info.RestoreScreen(scr);
    if (full) {
      for (int idx = 0;; idx++) {
        FileType* type = hrcParserLocal->enumerateFileTypes(idx);

        if (type == nullptr) {
          break;
        }

        StringBuffer tname;

        if (type->getGroup() != nullptr) {
          tname.append(type->getGroup());
          tname.append(DString(": "));
        }

        tname.append(type->getDescription());
        marr[1] = tname.getWChars();
        scr = Info.SaveScreen(0, 0, -1, -1);
        Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], 2, 0);
        type->getBaseScheme();
        Info.RestoreScreen(scr);
      }
    }
  } catch (Exception &e) {

    if ((parserFactoryLocal != nullptr) && (parserFactoryLocal->getErrorHandler() != nullptr)) {
      parserFactoryLocal->getErrorHandler()->error(*e.getMessage());
    }

    showExceptionMessage(e.getMessage()->getWChars());
    Info.RestoreScreen(scr);
    res = false;
  }

  delete regionMapperLocal;
  delete parserFactoryLocal;

  return res;
}

void FarEditorSet::ReloadBase()
{
  HANDLE scr = Info.SaveScreen(0, 0, -1, -1);

  try {
    ReadSettings();
    if (!rEnabled) {
      Info.RestoreScreen(scr);
      return;
    }

    const wchar_t* marr[2] = { GetMsg(mName), GetMsg(mReloading) };
    Info.Message(&MainGuid, &ReloadBaseMessage, 0, nullptr, &marr[0], 2, 0);
    dropAllEditors(true);
    delete regionMapper;
    delete parserFactory;
    parserFactory = nullptr;
    regionMapper = nullptr;

    if (TrueModOn) {
      hrdClass = DRgb;
      hrdName = sHrdNameTm;
    } else {
      hrdClass = DConsole;
      hrdName = sHrdName;
    }

    parserFactory = new ParserFactory(error_handler);
    parserFactory->loadCatalog(sCatalogPathExp);
    hrcParser = parserFactory->getHRCParser();
    LoadUserHrd(sUserHrdPathExp, parserFactory);
    LoadUserHrc(sUserHrcPathExp, parserFactory);
    FarHrcSettings p(parserFactory);
    p.readProfile();
    p.readUserProfile();
    defaultType = (FileTypeImpl*)hrcParser->getFileType(&DDefaultScheme);

    try {
      regionMapper = parserFactory->createStyledMapper(&hrdClass, &hrdName);
    } catch (ParserFactoryException &e) {
      if (getErrorHandler() != nullptr) {
        getErrorHandler()->error(*e.getMessage());
      }
      regionMapper = parserFactory->createStyledMapper(&hrdClass, nullptr);
    }
    //устанавливаем фон редактора при каждой перезагрузке схем.
    SetBgEditor();
    if (!in_construct) {
      //в случае изменения настроек в диалоге, надо перерисовать текущий редактор
      FarEditor* editor = addCurrentEditor();
      if (editor) {
        editor->editorEvent(EE_REDRAW, EEREDRAW_ALL);
      }
    }
  } catch (SettingsControlException &e) {

    if (getErrorHandler() != nullptr) {
      getErrorHandler()->error(*e.getMessage());
    }
    showExceptionMessage(e.getMessage()->getWChars());
    err_status = ERR_FARSETTINGS_ERROR;
    disableColorer();
  } catch (Exception &e) {

    if (getErrorHandler() != nullptr) {
      getErrorHandler()->error(*e.getMessage());
    }
    showExceptionMessage(e.getMessage()->getWChars());
    err_status = ERR_BASE_LOAD;
    disableColorer();
  }

  Info.RestoreScreen(scr);
}

colorer::ErrorHandler* FarEditorSet::getErrorHandler()
{
  if (parserFactory == nullptr) {
    return nullptr;
  }

  return parserFactory->getErrorHandler();
}

FarEditor* FarEditorSet::addCurrentEditor()
{
  if (viewFirst == 1) {
    viewFirst = 2;
    ReloadBase();
  }

  EditorInfo ei;
  ei.StructSize = sizeof(EditorInfo);
  if (!Info.EditorControl(CurrentEditor, ECTL_GETINFO, NULL, &ei)) {
    return nullptr;
  }

  FarEditor* editor = new FarEditor(&Info, parserFactory);
  std::pair<SString, FarEditor*> pair_editor(&SString(ei.EditorID), editor);
  farEditorInstances.emplace(pair_editor);
  String* s = getCurrentFileName();
  editor->chooseFileType(s);
  delete s;
  editor->setTrueMod(TrueModOn);
  editor->setRegionMapper(regionMapper);
  editor->setDrawCross(drawCross, CrossStyle);
  editor->setDrawPairs(drawPairs);
  editor->setDrawSyntax(drawSyntax);
  editor->setOutlineStyle(oldOutline);

  return editor;
}

String* FarEditorSet::getCurrentFileName()
{
  LPWSTR FileName = nullptr;
  size_t FileNameSize = Info.EditorControl(CurrentEditor, ECTL_GETFILENAME, NULL, nullptr);

  if (FileNameSize) {
    FileName = new wchar_t[FileNameSize];

    if (FileName) {
      Info.EditorControl(CurrentEditor, ECTL_GETFILENAME, FileNameSize, FileName);
    }
  }

  DString fnpath(FileName);
  int slash_idx = fnpath.lastIndexOf('\\');

  if (slash_idx == -1) {
    slash_idx = fnpath.lastIndexOf('/');
  }
  SString* s = new SString(fnpath, slash_idx + 1);
  delete[] FileName;
  return s;
}

FarEditor* FarEditorSet::getCurrentEditor()
{
  EditorInfo ei;
  ei.StructSize = sizeof(EditorInfo);
  Info.EditorControl(CurrentEditor, ECTL_GETINFO, NULL, &ei);
  auto if_editor = farEditorInstances.find(&SString(ei.EditorID));
  if (if_editor != farEditorInstances.end()) {
    return if_editor->second;
  } else {
    return nullptr;
  }
}

const wchar_t* FarEditorSet::GetMsg(int msg)
{
  return (Info.GetMsg(&MainGuid, msg));
}

void FarEditorSet::disableColorer()
{
  rEnabled = false;
  if (!(err_status & ERR_FARSETTINGS_ERROR)) {
    SettingsControl ColorerSettings;
    ColorerSettings.Set(0, cRegEnabled, rEnabled);
  }

  dropCurrentEditor(true);

  delete regionMapper;
  delete parserFactory;
  parserFactory = nullptr;
  regionMapper = nullptr;
}

void FarEditorSet::ApplySettingsToEditors()
{
  for (auto fe = farEditorInstances.begin(); fe != farEditorInstances.end(); ++fe) {
    fe->second->setTrueMod(TrueModOn);
    fe->second->setDrawCross(drawCross, CrossStyle);
    fe->second->setDrawPairs(drawPairs);
    fe->second->setDrawSyntax(drawSyntax);
    fe->second->setOutlineStyle(oldOutline);
  }
}

void FarEditorSet::dropCurrentEditor(bool clean)
{
  EditorInfo ei;
  ei.StructSize = sizeof(EditorInfo);
  Info.EditorControl(CurrentEditor, ECTL_GETINFO, NULL, &ei);
  auto it_editor = farEditorInstances.find(&SString(ei.EditorID));
  if (it_editor != farEditorInstances.end()) {
    if (clean) {
      it_editor->second->cleanEditor();
    }
    delete it_editor->second;
    farEditorInstances.erase(&SString(ei.EditorID));
    Info.EditorControl(CurrentEditor, ECTL_REDRAW, NULL, nullptr);
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
  const wchar_t* hrdName = ColorerSettings.Get(0, cRegHrdName, cHrdNameDefault);
  const wchar_t* hrdNameTm = ColorerSettings.Get(0, cRegHrdNameTm, cHrdNameTmDefault);
  const wchar_t* catalogPath = ColorerSettings.Get(0, cRegCatalog, cCatalogDefault);
  const wchar_t* userHrdPath = ColorerSettings.Get(0, cRegUserHrdPath, cUserHrdPathDefault);
  const wchar_t* userHrcPath = ColorerSettings.Get(0, cRegUserHrcPath, cUserHrcPathDefault);
  const wchar_t* LogPath = ColorerSettings.Get(0, cRegLogPath, cLogPathDefault);

  delete sHrdName;
  delete sHrdNameTm;
  delete sCatalogPath;
  delete sCatalogPathExp;
  delete sUserHrdPath;
  delete sUserHrdPathExp;
  delete sUserHrcPath;
  delete sUserHrcPathExp;
  sHrdName = nullptr;
  sCatalogPath = nullptr;
  sCatalogPathExp = nullptr;
  sUserHrdPath = nullptr;
  sUserHrdPathExp = nullptr;
  sUserHrcPath = nullptr;
  sUserHrcPathExp = nullptr;

  sHrdName = new SString(DString(hrdName));
  sHrdNameTm = new SString(DString(hrdNameTm));
  sCatalogPath = new SString(DString(catalogPath));
  sCatalogPathExp = PathToFullS(catalogPath, false);
  if (!sCatalogPathExp || !sCatalogPathExp->length()) {
    delete sCatalogPathExp;
    StringBuffer* path = new StringBuffer(PluginPath);
    path->append(DString(FarCatalogXml));
    sCatalogPathExp = path;
  }
  sUserHrdPath = new SString(DString(userHrdPath));
  sUserHrdPathExp = PathToFullS(userHrdPath, false);
  sUserHrcPath = new SString(DString(userHrcPath));
  sUserHrcPathExp = PathToFullS(userHrcPath, false);
  setLogPath(LogPath);

  rEnabled = ColorerSettings.Get(0, cRegEnabled, cEnabledDefault);
  drawCross = ColorerSettings.Get(0, cRegCrossDraw, cCrossDrawDefault);
  CrossStyle = ColorerSettings.Get(0, cRegCrossStyle, cCrossStyleDefault);
  drawPairs = ColorerSettings.Get(0, cRegPairsDraw, cPairsDrawDefault);
  drawSyntax = ColorerSettings.Get(0, cRegSyntaxDraw, cSyntaxDrawDefault);
  oldOutline = ColorerSettings.Get(0, cRegOldOutLine, cOldOutLineDefault);
  TrueModOn = ColorerSettings.Get(0, cRegTrueMod, cTrueMod);
  ChangeBgEditor = ColorerSettings.Get(0, cRegChangeBgEditor, cChangeBgEditor);
}

void FarEditorSet::setLogPath(const wchar_t* log_path)
{
  if (sLogPath && sLogPath->compareToIgnoreCase(DString(log_path)) != 0) {
    delete error_handler;
    error_handler = nullptr;
  }
  delete sLogPath;
  delete sLogPathExp;
  sLogPath = new SString(DString(log_path));
  sLogPathExp = PathToFullS(log_path, false);
  if (error_handler == nullptr && sLogPathExp != nullptr) {
    try {
      error_handler = new FileErrorHandler(sLogPathExp, Encodings::ENC_UTF8, false);
    } catch (Exception &e) {
      error_handler = nullptr;
      showExceptionMessage(e.getMessage()->getWChars());
    }
  }

}

void FarEditorSet::SaveSettings()
{
  SettingsControl ColorerSettings;
  ColorerSettings.Set(0, cRegEnabled, rEnabled);
  ColorerSettings.Set(0, cRegHrdName, sHrdName->getWChars());
  ColorerSettings.Set(0, cRegHrdNameTm, sHrdNameTm->getWChars());
  ColorerSettings.Set(0, cRegCatalog,  sCatalogPath->getWChars());
  ColorerSettings.Set(0, cRegCrossDraw, drawCross);
  ColorerSettings.Set(0, cRegCrossStyle, CrossStyle);
  ColorerSettings.Set(0, cRegPairsDraw, drawPairs);
  ColorerSettings.Set(0, cRegSyntaxDraw, drawSyntax);
  ColorerSettings.Set(0, cRegOldOutLine, oldOutline);
  ColorerSettings.Set(0, cRegTrueMod, TrueModOn);
  ColorerSettings.Set(0, cRegChangeBgEditor, ChangeBgEditor);
  ColorerSettings.Set(0, cRegUserHrdPath, sUserHrdPath->getWChars());
  ColorerSettings.Set(0, cRegUserHrcPath, sUserHrcPath->getWChars());
  ColorerSettings.Set(0, cRegLogPath, sLogPath->getWChars());
}

bool FarEditorSet::SetBgEditor()
{
  if (rEnabled && ChangeBgEditor) {

    const StyledRegion* def_text = StyledRegion::cast(regionMapper->getRegionDefine(DString("def:Text")));

    FarSetColors fsc;
    FarColor fc;
    fsc.StructSize = sizeof(FarSetColors);
    fsc.Flags = FSETCLR_REDRAW;
    fsc.ColorsCount = 1;
    fsc.StartIndex = COL_EDITORTEXT;
    if (TrueModOn) {
      fc.Flags = 0;
      fc.BackgroundColor = revertRGB(def_text->back);
      fc.ForegroundColor = revertRGB(def_text->fore);
    } else {
      fc.Flags = FCF_4BITMASK;
      fc.BackgroundColor = def_text->back;
      fc.ForegroundColor = def_text->fore;
    }
    fc.Reserved = nullptr;
    fsc.Colors = &fc;
    return !!Info.AdvControl(&MainGuid, ACTL_SETARRAYCOLOR, 0, &fsc);
  }
  return false;
}

void FarEditorSet::LoadUserHrd(const String* filename, ParserFactory* pf)
{
  if (filename && filename->length()) {
    xercesc::XercesDOMParser xml_parser;
    XmlParserErrorHandler err_handler(error_handler);
    xml_parser.setErrorHandler(&err_handler);
    xml_parser.setLoadExternalDTD(false);
    xml_parser.setSkipDTDValidation(true);
    XmlInputSource* config = XmlInputSource::newInstance(filename->getWChars(), (XMLCh*)nullptr);
    xml_parser.parse(*config->getInputSource());
    if (err_handler.getSawErrors()) {
      throw ParserFactoryException(StringBuffer("Error reading ") + DString(filename));
    }
    xercesc::DOMDocument* catalog = xml_parser.getDocument();
    xercesc::DOMElement* elem = catalog->getDocumentElement();
    const XMLCh* tagHrdSets = L"hrd-sets";
    const XMLCh* tagHrd = L"hrd";
    if (elem == nullptr || !xercesc::XMLString::equals(elem->getNodeName(), tagHrdSets)) {
      throw Exception(DString("main '<hrd-sets>' block not found"));
    }
    for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
      if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
        xercesc::DOMElement* subelem = static_cast<xercesc::DOMElement*>(node);
        if (xercesc::XMLString::equals(subelem->getNodeName(), tagHrd)) {
          pf->parseHRDSetsChild(subelem);
        }
      }
    }
    delete config;
  }
}

void FarEditorSet::LoadUserHrc(const String* filename, ParserFactory* pf)
{
  if (filename && filename->length()) {
    HRCParser* hr = pf->getHRCParser();
    XmlInputSource* dfis = XmlInputSource::newInstance(filename->getWChars(), (XMLCh*)nullptr);
    try {
      hr->loadSource(dfis);
      delete dfis;
    } catch (Exception &e) {
      delete dfis;
      throw Exception(e);
    }
  }
}

const String* FarEditorSet::getParamDefValue(FileTypeImpl* type, SString param)
{
  const String* value;
  value = type->getParamDefaultValue(param);
  if (value == nullptr) {
    value = defaultType->getParamValue(param);
  }
  StringBuffer* p = new StringBuffer("<default-");
  p->append(DString(value));
  p->append(DString(">"));
  return p;
}

FarList* FarEditorSet::buildHrcList()
{
  size_t num = getCountFileTypeAndGroup();;
  const String* group = nullptr;
  FileType* type = nullptr;

  FarListItem* hrcList = new FarListItem[num];
  memset(hrcList, 0, sizeof(FarListItem) * (num));
  group = nullptr;

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
    } else {
      groupChars = L"<no group>";
    }

    hrcList[i].Text = new wchar_t[255];
    _snwprintf((wchar_t*)hrcList[i].Text, 255, L"%s: %s", groupChars, type->getDescription()->getWChars());
  }

  hrcList[0].Flags = LIF_SELECTED;
  FarList* ListItems = new FarList;
  ListItems->Items = hrcList;
  ListItems->ItemsNumber = num;
  ListItems->StructSize = sizeof(FarList);
  return ListItems;
}

FarList* FarEditorSet::buildParamsList(FileTypeImpl* type)
{
  //max count params
  size_t size = type->getParamCount() + defaultType->getParamCount();
  FarListItem* fparam = new FarListItem[size];
  memset(fparam, 0, sizeof(FarListItem) * (size));

  size_t count = 0;
  std::vector<SString> default_params = defaultType->enumParams();
  for (auto paramname = default_params.begin(); paramname != default_params.end(); ++paramname) {
    fparam[count++].Text = wcsdup(paramname->getWChars());
  }
  std::vector<SString> type_params = type->enumParams();
  for (auto paramname = type_params.begin(); paramname != type_params.end(); ++paramname) {
    if (defaultType->getParamValue(*paramname) == nullptr) {
      fparam[count++].Text = wcsdup(paramname->getWChars());
    }
  }

  fparam[0].Flags = LIF_SELECTED;
  FarList* lparam = new FarList;
  lparam->Items = fparam;
  lparam->ItemsNumber = count;
  lparam->StructSize = sizeof(FarList);
  return lparam;

}

void FarEditorSet::ChangeParamValueListType(HANDLE hDlg, bool dropdownlist)
{
  size_t s = Info.SendDlgMessage(hDlg, DM_GETDLGITEM, IDX_CH_PARAM_VALUE_LIST, nullptr);
  struct FarDialogItem* DialogItem = (FarDialogItem*) calloc(1, s);
  FarGetDialogItem fgdi;
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
  const String* value = ((FileTypeImpl*)type)->getParamUserValue(DShowCross);
  const String* def_value = getParamDefValue(type, DShowCross);

  size_t count = 5;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DNone.getWChars();
  fcross[1].Text = DVertical.getWChars();
  fcross[2].Text = DHorizontal.getWChars();
  fcross[3].Text = DBoth.getWChars();
  fcross[4].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 4;
  } else {
    if (value->equals(&DNone)) {
      ret = 0;
    } else if (value->equals(&DVertical)) {
      ret = 1;
    } else if (value->equals(&DHorizontal)) {
      ret = 2;
    } else if (value->equals(&DBoth)) {
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
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DBottom.getWChars();
  fcross[1].Text = DTop.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 2;
  } else {
    if (value->equals(&DBottom)) {
      ret = 0;
    } else if (value->equals(&DTop)) {
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

void FarEditorSet::setYNListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, DString param)
{
  const String* value = type->getParamUserValue(param);
  const String* def_value = getParamDefValue(type, param);

  size_t count = 3;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DNo.getWChars();
  fcross[1].Text = DYes.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 2;
  } else {
    if (value->equals(&DNo)) {
      ret = 0;
    } else if (value->equals(&DYes)) {
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

void FarEditorSet::setTFListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, DString param)
{
  const String* value = type->getParamUserValue(param);
  const String* def_value = getParamDefValue(type, param);

  size_t count = 3;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = DFalse.getWChars();
  fcross[1].Text = DTrue.getWChars();
  fcross[2].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  size_t ret = 2;
  if (value == nullptr || !value->length()) {
    ret = 2;
  } else {
    if (value->equals(&DFalse)) {
      ret = 0;
    } else if (value->equals(&DTrue)) {
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

void FarEditorSet::setCustomListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, DString param)
{
  const String* value = type->getParamUserValue(param);
  const String* def_value = getParamDefValue(type, param);

  size_t count = 1;
  FarListItem* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = def_value->getWChars();
  FarList* lcross = new FarList;
  lcross->Items = fcross;
  lcross->ItemsNumber = count;
  lcross->StructSize = sizeof(FarList);

  fcross[0].Flags = LIF_SELECTED;
  ChangeParamValueListType(hDlg, false);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);

  if (value != nullptr) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR , IDX_CH_PARAM_VALUE_LIST, (void*)value->getWChars());
  }
  delete def_value;
  delete[] fcross;
  delete lcross;
}

FileTypeImpl* FarEditorSet::getCurrentTypeInDialog(HANDLE hDlg)
{
  int k = (int)Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, IDX_CH_SCHEMAS, 0);
  return getFileTypeByIndex(k);
}

void  FarEditorSet::OnChangeHrc(HANDLE hDlg)
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
  Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);

  //param name
  DString p = DString(List.Item.Text);
  //param value
  DString v = DString(trim((wchar_t*)Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CH_PARAM_VALUE_LIST, 0)));
  FileTypeImpl* type = getCurrentTypeInDialog(hDlg);
  const String* value = ((FileTypeImpl*)type)->getParamUserValue(p);
  const String* def_value = getParamDefValue(type, p);
  if (value == nullptr || !value->length()) { //было default значение
    //если его изменили
    if (!v.equals(def_value)) {
      if (type->getParamValue(p) == nullptr) {
        ((FileTypeImpl*)type)->addParam(&p);
      }
      type->setParamValue(p, &v);
    }
  } else { //было пользовательское значение
    if (!v.equals(value)) { //changed
      type->setParamValue(p, &v);
    }
  }

  delete def_value;
}

void  FarEditorSet::OnChangeParam(HANDLE hDlg, int idx)
{
  if (menuid != idx && menuid != -1) {
    SaveChangedValueParam(hDlg);
  }
  FileTypeImpl* type = getCurrentTypeInDialog(hDlg);
  FarListGetItem List = {0};
  List.StructSize = sizeof(FarListGetItem);
  List.ItemIndex = idx;
  Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);

  menuid = idx;
  DString p = DString(List.Item.Text);

  const String* value;
  value = type->getParamDescription(p);
  if (value == nullptr) {
    value = defaultType->getParamDescription(p);
  }
  if (value != nullptr) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR , IDX_CH_DESCRIPTION, (void*)value->getWChars());
  }

  COORD c;
  c.X = 0;
  Info.SendDlgMessage(hDlg, DM_SETCURSORPOS , IDX_CH_DESCRIPTION, &c);
  if (p.equals(&DShowCross)) {
    setCrossValueListToCombobox(type, hDlg);
  } else {
    if (p.equals(&DCrossZorder)) {
      setCrossPosValueListToCombobox(type, hDlg);
    } else if (p.equals(&DMaxLen) || p.equals(&DBackparse) || p.equals(&DDefFore) || p.equals(&DDefBack)
               || p.equals("firstlines") || p.equals("firstlinebytes") || p.equals(&DHotkey)) {
      setCustomListValueToCombobox(type, hDlg, DString(List.Item.Text));
    } else if (p.equals(&DFullback)) {
      setYNListValueToCombobox(type, hDlg, DString(List.Item.Text));
    } else {
      setTFListValueToCombobox(type, hDlg, DString(List.Item.Text));
    }
  }

}

void FarEditorSet::OnSaveHrcParams(HANDLE hDlg)
{
  SaveChangedValueParam(hDlg);
  FarHrcSettings p(parserFactory);
  p.writeUserProfile();
}

INT_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  FarEditorSet* fes = (FarEditorSet*)Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);;

  switch (Msg) {
    case DN_GOTFOCUS: {
      if (fes->dialogFirstFocus) {
        fes->menuid = -1;
        fes->OnChangeHrc(hDlg);
        fes->dialogFirstFocus = false;
      }
      return false;
    }
    break;
    case DN_BTNCLICK:
      switch (Param1) {
        case IDX_CH_OK:
          fes->OnSaveHrcParams(hDlg);
          return false;
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
      }
      break;
    case DN_LISTCHANGE:
      switch (Param1) {
        case IDX_CH_PARAM_LIST:
          fes->OnChangeParam(hDlg, (int)Param2);
          return true;
          break;
      }
      break;
  }

  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

void FarEditorSet::configureHrc()
{
  if (!rEnabled) {
    return;
  }

  FarDialogItem fdi[] = {
    // type, x1, y1, x2, y2, param, history, mask, flags, userdata, ptrdata, maxlen
    { DI_DOUBLEBOX, 2, 1, 56, 21, 0, 0, 0, 0, 0, 0, 0},                 //IDX_CH_BOX,
    { DI_TEXT, 3, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0},                        //IDX_CH_CAPTIONLIST,
    { DI_COMBOBOX, 10, 3, 54, 2, 0, 0, 0, 0, 0, 0, 0},                  //IDX_CH_SCHEMAS,
    { DI_LISTBOX, 3, 4, 30, 17, 0, 0, 0, 0, 0, 0, 0},                   //IDX_CH_PARAM_LIST,
    { DI_TEXT, 32, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0},                       //IDX_CH_PARAM_VALUE_CAPTION
    { DI_COMBOBOX, 32, 6, 54, 6, 0, 0, 0, 0, 0, 0, 0},                  //IDX_CH_PARAM_VALUE_LIST
    { DI_EDIT, 4, 18, 54, 18, 0, 0, 0, 0, 0, 0, 0},                     //IDX_CH_DESCRIPTION,
    { DI_BUTTON, 37, 20, 0, 0, 0, 0, 0, DIF_DEFAULTBUTTON, 0, 0, 0},    //IDX_OK,
    { DI_BUTTON, 45, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0},                    //IDX_CANCEL,
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
  fdi[IDX_CH_PARAM_VALUE_LIST].Flags = DIF_LISTWRAPMODE ;

  dialogFirstFocus = true;
  HANDLE hDlg = Info.DialogInit(&MainGuid, &HrcPluginConfig, -1, -1, 59, 23, L"confighrc", fdi, ARRAY_SIZE(fdi), 0, 0, SettingHrcDialogProc, this);
  Info.DialogRun(hDlg);

  for (size_t idx = 0; idx < l->ItemsNumber; idx++) {
    if (l->Items[idx].Text) {
      delete[] l->Items[idx].Text;
    }
  }
  delete[] l->Items;
  delete l;

  Info.DialogFree(hDlg);
}

void FarEditorSet::showExceptionMessage(const wchar_t* message)
{
  const wchar_t* exceptionMessage[4] = {GetMsg(mName), GetMsg(mCantLoad), message, GetMsg(mDie)};
  Info.Message(&MainGuid, &ErrorMessage, FMSG_WARNING, L"exception", &exceptionMessage[0], sizeof(exceptionMessage) / sizeof(exceptionMessage[0]), 1);
}

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */