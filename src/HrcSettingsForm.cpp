#include "HrcSettingsForm.h"
#include <algorithm>
#include <array>
#include "FarHrcSettings.h"
#include "tools.h"

HrcSettingsForm::HrcSettingsForm(FarEditorSet* _farEditorSet, FileType* filetype)
    : farEditorSet(_farEditorSet), filetype_in_editor(filetype)
{
}

bool HrcSettingsForm::Show()
{
  return showForm();
}

INT_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  auto* fes = reinterpret_cast<HrcSettingsForm*>(Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, nullptr));

  switch (Msg) {
    case DN_INITDIALOG: {
      fes->menuid = -1;
      fes->OnChangeHrc();
      return false;
    }
    case DN_BTNCLICK:
      if (IDX_CH_OK == Param1) {
        fes->OnSaveHrcParams();
        return false;
      }
      break;
    case DN_EDITCHANGE:
      if (IDX_CH_SCHEMAS == Param1) {
        fes->menuid = -1;
        fes->OnChangeHrc();
        return true;
      }

      break;
    case DN_LISTCHANGE:
      if (IDX_CH_PARAM_LIST == Param1) {
        fes->OnChangeParam(reinterpret_cast<intptr_t>(Param2));
        return true;
      }
      break;
    default:
      break;
  }
  return Info.DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool HrcSettingsForm::showForm()
{
  if (!farEditorSet->Opt.rEnabled) {
    return false;
  }

  FarDialogItem fdi[] = {
      // type, x1, y1, x2, y2, param, history, mask, flags, userdata, ptrdata, maxlen
      {DI_DOUBLEBOX,  2,  1, 56, 21, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_BOX,
      {     DI_TEXT,  3,  3,  0,  3, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_CAPTIONLIST,
      { DI_COMBOBOX, 10,  3, 54,  2, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_SCHEMAS,
      {  DI_LISTBOX,  3,  4, 30, 17, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_PARAM_LIST,
      {     DI_TEXT, 32,  5,  0,  5, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_PARAM_VALUE_CAPTION
      { DI_COMBOBOX, 32,  6, 54,  6, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_PARAM_VALUE_LIST
      {     DI_EDIT,  4, 18, 54, 18, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_DESCRIPTION,
      {   DI_BUTTON, 37, 20,  0,  0, 0, nullptr, nullptr, DIF_DEFAULTBUTTON, nullptr, 0, 0}, // IDX_CH_OK,
      {   DI_BUTTON, 45, 20,  0,  0, 0, nullptr, nullptr,                 0, nullptr, 0, 0}, // IDX_CH_CANCEL,
  };

  fdi[IDX_CH_BOX].Data = FarEditorSet::GetMsg(mUserHrcSettingDialog);
  fdi[IDX_CH_CAPTIONLIST].Data = FarEditorSet::GetMsg(mListSyntax);
  FarList* l = buildHrcList();
  fdi[IDX_CH_SCHEMAS].ListItems = l;
  fdi[IDX_CH_SCHEMAS].Flags = DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;
  fdi[IDX_CH_OK].Data = FarEditorSet::GetMsg(mOk);
  fdi[IDX_CH_CANCEL].Data = FarEditorSet::GetMsg(mCancel);
  fdi[IDX_CH_PARAM_LIST].Data = FarEditorSet::GetMsg(mParamList);
  fdi[IDX_CH_PARAM_VALUE_CAPTION].Data = FarEditorSet::GetMsg(mParamValue);
  fdi[IDX_CH_DESCRIPTION].Flags = DIF_READONLY;

  fdi[IDX_CH_PARAM_LIST].Flags = DIF_LISTWRAPMODE | DIF_LISTNOCLOSE;

  hDlg = Info.DialogInit(&MainGuid, &HrcPluginConfig, -1, -1, 59, 23, L"confighrc", fdi, std::size(fdi), 0, 0,
                         SettingHrcDialogProc, this);
  if (IDX_CH_OK == Info.DialogRun(hDlg)) {
    SaveChangedValueParam();
  }

  removeFarList(l);

  Info.DialogFree(hDlg);
  return true;
}

size_t HrcSettingsForm::getCountFileTypeAndGroup() const
{
  size_t num = 0;
  UnicodeString group;

  for (int idx = 0;; idx++) {
    const FileType* type = farEditorSet->parserFactory->getHrcLibrary().enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    num++;
    if (group.compare(type->getGroup()) != 0) {
      num++;
      group = type->getGroup();
    }
  }
  return num;
}

FarList* HrcSettingsForm::buildHrcList() const
{
  const size_t num = getCountFileTypeAndGroup();
  UnicodeString group;

  auto* hrcList = new FarListItem[num] {};

  HrcLibrary& hrcLibrary = farEditorSet->parserFactory->getHrcLibrary();
  for (int idx = 0, i = 0;; idx++, i++) {
    FileType* type = hrcLibrary.enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (type == filetype_in_editor) {
      hrcList[i].Flags = LIF_SELECTED;
    }

    if (group.compare(type->getGroup()) != 0) {
      hrcList[i].Flags = LIF_SEPARATOR;
      i++;
    }

    group = type->getGroup();

    std::wstring groupChars;

    if (group != nullptr) {
      groupChars = UStr::to_stdwstr(group);
    }
    else {
      groupChars = std::wstring(L"<no group>");
    }

    hrcList[i].Text = new wchar_t[255];
    _snwprintf(const_cast<wchar_t*>(hrcList[i].Text), 255, L"%s: %s", groupChars.c_str(),
               UStr::to_stdwstr(type->getDescription()).c_str());
    hrcList[i].UserData = (intptr_t) type;
  }

  return buildFarList(hrcList, num);
}

void HrcSettingsForm::OnChangeParam(intptr_t idx)
{
  if (menuid != idx && menuid != -1) {
    SaveChangedValueParam();
  }
  FarListGetItem List {sizeof(FarListGetItem), idx};
  bool res = Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);
  if (!res)
    return;

  menuid = idx;
  const auto p = UnicodeString(List.Item.Text);

  const UnicodeString* value = current_filetype->getParamDescription(p);
  if (value == nullptr) {
    value = farEditorSet->defaultType->getParamDescription(p);
  }
  if (value != nullptr) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_CH_DESCRIPTION, (void*) UStr::to_stdwstr(value).c_str());
  }

  // set visible begin of text
  COORD c {0, 0};
  Info.SendDlgMessage(hDlg, DM_SETCURSORPOS, IDX_CH_DESCRIPTION, &c);

  if (UnicodeString(param_ShowCross).compare(p) == 0) {
    setCrossValueListToCombobox();
  }
  else {
    if (UnicodeString(param_CrossZorder).compare(p) == 0) {
      setCrossPosValueListToCombobox();
    }
    else if (UnicodeString(param_MaxLen).compare(p) == 0 || UnicodeString(param_Backparse).compare(p) == 0 ||
             UnicodeString(param_DefFore).compare(p) == 0 || UnicodeString(param_DefBack).compare(p) == 0 ||
             UnicodeString(param_Firstlines).compare(p) == 0 || UnicodeString(param_Firstlinebytes).compare(p) == 0 ||
             UnicodeString(param_HotKey).compare(p) == 0 || UnicodeString(param_MaxBlockSize).compare(p) == 0)
    {
      setCustomListValueToCombobox(UnicodeString(List.Item.Text));
    }
    else if (UnicodeString(param_Fullback).compare(p) == 0) {
      setYNListValueToCombobox(UnicodeString(List.Item.Text));
    }
    else {
      setTFListValueToCombobox(UnicodeString(List.Item.Text));
    }
  }
}

void HrcSettingsForm::OnSaveHrcParams() const
{
  SaveChangedValueParam();
  FarHrcSettings p(farEditorSet, farEditorSet->parserFactory.get());
  p.writeUserProfile();
}

void HrcSettingsForm::SaveChangedValueParam() const
{
  FarListGetItem List = {sizeof(FarListGetItem), menuid};
  bool res = Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);

  if (!res)
    return;

  // param name
  const auto p = UnicodeString(List.Item.Text);
  // param value
  const auto v = UnicodeString(trim(
      reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CH_PARAM_VALUE_LIST, nullptr))));

  const UnicodeString* value = current_filetype->getParamUserValue(p);
  const UnicodeString def_value = getParamDefValue(current_filetype, p);
  if (v.compare(def_value) == 0) {
    if (value != nullptr)
      current_filetype->setParamValue(p, nullptr);
  }
  else if (value == nullptr || v.compare(*value) != 0) {  // changed
    farEditorSet->addParamAndValue(current_filetype, p, v);
  }
}

void HrcSettingsForm::getCurrentTypeInDialog()
{
  auto k = static_cast<int>(Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, IDX_CH_SCHEMAS, nullptr));
  FarListGetItem f {sizeof(FarListGetItem), k};
  bool res = Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_SCHEMAS, (void*) &f);
  if (res)
    current_filetype = (FileType*) f.Item.UserData;
}

void HrcSettingsForm::OnChangeHrc()
{
  if (menuid != -1) {
    SaveChangedValueParam();
  }
  getCurrentTypeInDialog();
  FarList* List = buildParamsList(current_filetype);

  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_LIST, List);
  removeFarList(List);
  OnChangeParam(0);
}

void HrcSettingsForm::setYNListValueToCombobox(const UnicodeString& param) const
{
  const UnicodeString* value = current_filetype->getParamUserValue(param);
  const UnicodeString def_value = getParamDefValue(current_filetype, param);

  std::array<FarListItem, 3> fcross {};
  fcross[0].Text = _wcsdup(value_No);
  fcross[1].Text = _wcsdup(value_Yes);
  fcross[2].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());

  size_t ret;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->compare(value_No) == 0) {
      ret = 0;
    }
    else if (value->compare(value_Yes) == 0) {
      ret = 1;
    }
    else {
      ret = 2;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  FarList lcross {sizeof(FarList), fcross.size(), fcross.data()};
  ChangeParamValueList(&lcross, true);
}

void HrcSettingsForm::setTFListValueToCombobox(const UnicodeString& param) const
{
  const UnicodeString* value = current_filetype->getParamUserValue(param);
  const UnicodeString def_value = getParamDefValue(current_filetype, param);

  std::array<FarListItem, 3> fcross {};
  fcross[0].Text = _wcsdup(value_False);
  fcross[1].Text = _wcsdup(value_True);
  fcross[2].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());

  size_t ret;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->compare(value_False) == 0) {
      ret = 0;
    }
    else if (value->compare(value_True) == 0) {
      ret = 1;
    }
    else {
      ret = 2;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  FarList lcross {sizeof(FarList), fcross.size(), fcross.data()};
  ChangeParamValueList(&lcross, true);
}

void HrcSettingsForm::setCustomListValueToCombobox(const UnicodeString& param) const
{
  const UnicodeString* value = current_filetype->getParamUserValue(param);
  const UnicodeString def_value = getParamDefValue(current_filetype, param);

  std::array<FarListItem, 1> fcross {};
  fcross[0].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());

  fcross[0].Flags = LIF_SELECTED;
  FarList lcross {sizeof(FarList), fcross.size(), fcross.data()};
  ChangeParamValueList(&lcross, false);

  if (value != nullptr) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_CH_PARAM_VALUE_LIST, (void*) UStr::to_stdwstr(value).c_str());
  }
}

void HrcSettingsForm::setCrossValueListToCombobox() const
{
  auto uparam = UnicodeString(param_ShowCross);
  const UnicodeString* value = current_filetype->getParamUserValue(uparam);
  const UnicodeString def_value = getParamDefValue(current_filetype, uparam);

  std::array<FarListItem, 5> fcross {};
  fcross[0].Text = _wcsdup(value_None);
  fcross[1].Text = _wcsdup(value_Vertical);
  fcross[2].Text = _wcsdup(value_Horizontal);
  fcross[3].Text = _wcsdup(value_Both);
  fcross[4].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());

  size_t ret = 0;
  if (value == nullptr || !value->length()) {
    ret = 4;
  }
  else {
    if (value->compare(value_None) == 0) {
      ret = 0;
    }
    else if (value->compare(value_Vertical) == 0) {
      ret = 1;
    }
    else if (value->compare(value_Horizontal) == 0) {
      ret = 2;
    }
    else if (value->compare(value_Both) == 0) {
      ret = 3;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  FarList lcross {sizeof(FarList), fcross.size(), fcross.data()};
  ChangeParamValueList(&lcross, true);
}

void HrcSettingsForm::setCrossPosValueListToCombobox() const
{
  auto uparam = UnicodeString(param_CrossZorder);
  const UnicodeString* value = current_filetype->getParamUserValue(uparam);
  const UnicodeString def_value = getParamDefValue(current_filetype, uparam);

  std::array<FarListItem, 3> fcross {};
  fcross[0].Text = _wcsdup(value_Bottom);
  fcross[1].Text = _wcsdup(value_Top);
  fcross[2].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());

  size_t ret;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->compare(value_Bottom) == 0) {
      ret = 0;
    }
    else if (value->compare(value_Top) == 0) {
      ret = 1;
    }
    else {
      ret = 2;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;

  FarList lcross {sizeof(FarList), fcross.size(), fcross.data()};
  ChangeParamValueList(&lcross, true);
}

UnicodeString HrcSettingsForm::getParamDefValue(const FileType* type, const UnicodeString& param) const
{
  const UnicodeString* value = type->getParamDefaultValue(param);
  if (value == nullptr) {
    value = farEditorSet->defaultType->getParamValue(param);
  }

  UnicodeString p("<default-");

  if (value == nullptr) {
    return p+">";
  }

  p.append(*value);
  p.append(">");
  return p;
}

FarList* HrcSettingsForm::buildParamsList(const FileType* type) const
{
  auto type_params = type->enumParams();
  auto default_params = farEditorSet->defaultType->enumParams();
  type_params.insert(type_params.end(), default_params.begin(), default_params.end());
  std::sort(type_params.begin(), type_params.end());
  auto last = std::unique(type_params.begin(), type_params.end());
  type_params.erase(last, type_params.end());

  size_t count = 0;
  auto* fparam = new FarListItem[type_params.size()] {};
  for (const auto& param : type_params) {
    fparam[count++].Text = _wcsdup(UStr::to_stdwstr(&param).c_str());
  }
  fparam[0].Flags = LIF_SELECTED;
  return buildFarList(fparam, type_params.size());
}

void HrcSettingsForm::ChangeParamValueList(FarList* items, bool dropdownlist) const
{
  FARDIALOGITEMFLAGS flags = DIF_LISTWRAPMODE;
  if (dropdownlist) {
    flags |= DIF_DROPDOWNLIST;
  }
  FarDialogItem fdi {DI_COMBOBOX, 32, 6, 54, 6, 0, nullptr, nullptr, flags, nullptr, 0, 0};
  // так не работает
  // fdi.ListItems = items;

  Info.SendDlgMessage(hDlg, DM_SETDLGITEM, IDX_CH_PARAM_VALUE_LIST, &fdi);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, items);
}

FarList* HrcSettingsForm::buildFarList(FarListItem* list, size_t count)
{
  auto* lparam = new FarList;
  lparam->Items = list;
  lparam->ItemsNumber = count;
  lparam->StructSize = sizeof(FarList);
  return lparam;
}

void HrcSettingsForm::removeFarList(FarList* list)
{
  for (size_t idx = 0; idx < list->ItemsNumber; idx++) {
    delete[] list->Items[idx].Text;
  }
  delete[] list->Items;
  delete list;
}