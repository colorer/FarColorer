#include "HrcSettingsForm.h"
#include <colorer/common/UStr.h>
#include "tools.h"

HrcSettingsForm::HrcSettingsForm(FarEditorSet* _farEditorSet, FileType* filetype)
{
  menuid = 0;
  farEditorSet = _farEditorSet;
  current_filetype = nullptr;
  filetype_in_editor = filetype;
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
      return false;  //-V601
    }
    case DN_BTNCLICK:
      if (IDX_CH_OK == Param1) {
        fes->OnSaveHrcParams();
        return false;  //-V601
      }
      break;
    case DN_EDITCHANGE:
      if (IDX_CH_SCHEMAS == Param1) {
        fes->menuid = -1;
        fes->OnChangeHrc();
        return true;  //-V601
      }

      break;
    case DN_LISTCHANGE:
      if (IDX_CH_PARAM_LIST == Param1) {
        fes->OnChangeParam(reinterpret_cast<intptr_t>(Param2));
        return true;  //-V601
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
      {DI_DOUBLEBOX, 2, 1, 56, 21, 0, nullptr, nullptr, 0, nullptr, 0, 0},               // IDX_CH_BOX,
      {DI_TEXT, 3, 3, 0, 3, 0, nullptr, nullptr, 0, nullptr, 0, 0},                      // IDX_CH_CAPTIONLIST,
      {DI_COMBOBOX, 10, 3, 54, 2, 0, nullptr, nullptr, 0, nullptr, 0, 0},                // IDX_CH_SCHEMAS,
      {DI_LISTBOX, 3, 4, 30, 17, 0, nullptr, nullptr, 0, nullptr, 0, 0},                 // IDX_CH_PARAM_LIST,
      {DI_TEXT, 32, 5, 0, 5, 0, nullptr, nullptr, 0, nullptr, 0, 0},                     // IDX_CH_PARAM_VALUE_CAPTION
      {DI_COMBOBOX, 32, 6, 54, 6, 0, nullptr, nullptr, 0, nullptr, 0, 0},                // IDX_CH_PARAM_VALUE_LIST
      {DI_EDIT, 4, 18, 54, 18, 0, nullptr, nullptr, 0, nullptr, 0, 0},                   // IDX_CH_DESCRIPTION,
      {DI_BUTTON, 37, 20, 0, 0, 0, nullptr, nullptr, DIF_DEFAULTBUTTON, nullptr, 0, 0},  // IDX_CH_OK,
      {DI_BUTTON, 45, 20, 0, 0, 0, nullptr, nullptr, 0, nullptr, 0, 0},                  // IDX_CH_CANCEL,
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
  fdi[IDX_CH_PARAM_VALUE_LIST].Flags = DIF_LISTWRAPMODE;

  hDlg = Info.DialogInit(&MainGuid, &HrcPluginConfig, -1, -1, 59, 23, L"confighrc", fdi, std::size(fdi), 0, 0, SettingHrcDialogProc, this);
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
  const UnicodeString* group = nullptr;
  FileType* type;

  for (int idx = 0;; idx++) {
    type = farEditorSet->hrcLibrary->enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    num++;
    if (group != nullptr && group->compare(*type->getGroup()) != 0) {
      num++;
    }

    group = type->getGroup();
  }
  return num;
}

FarList* HrcSettingsForm::buildHrcList() const
{
  size_t num = getCountFileTypeAndGroup();
  const UnicodeString* group = nullptr;
  FileType* type;

  auto* hrcList = new FarListItem[num];
  memset(hrcList, 0, sizeof(FarListItem) * (num));

  for (int idx = 0, i = 0;; idx++, i++) {
    type = farEditorSet->hrcLibrary->enumerateFileTypes(idx);

    if (type == nullptr) {
      break;
    }

    if (type == filetype_in_editor) {
      hrcList[i].Flags = LIF_SELECTED;
    }

    if (group != nullptr && group->compare(*type->getGroup()) != 0) {
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
    _snwprintf(const_cast<wchar_t*>(hrcList[i].Text), 255, L"%s: %s", groupChars.c_str(), UStr::to_stdwstr(type->getDescription()).c_str());
    hrcList[i].UserData = (intptr_t) type;
  }

  return buildFarList(hrcList, num);
}

void HrcSettingsForm::OnChangeParam(intptr_t idx)
{
  if (menuid != idx && menuid != -1) {
    SaveChangedValueParam();
  }
  FarListGetItem List {};
  List.StructSize = sizeof(FarListGetItem);
  List.ItemIndex = idx;
  bool res = Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);
  if (!res)
    return;

  menuid = idx;
  UnicodeString p = UnicodeString(List.Item.Text);

  const UnicodeString* value;
  value = current_filetype->getParamDescription(p);
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
             UnicodeString(param_HotKey).compare(p) == 0 || UnicodeString(param_MaxBlockSize).compare(p) == 0) {
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
  FarHrcSettings p(farEditorSet->parserFactory.get());
  p.writeUserProfile();
}

void HrcSettingsForm::SaveChangedValueParam() const
{
  FarListGetItem List = {0};
  List.StructSize = sizeof(FarListGetItem);
  List.ItemIndex = menuid;
  bool res = Info.SendDlgMessage(hDlg, DM_LISTGETITEM, IDX_CH_PARAM_LIST, &List);

  if (!res)
    return;

  // param name
  UnicodeString p = UnicodeString(List.Item.Text);
  // param value
  UnicodeString v = UnicodeString(trim(reinterpret_cast<wchar_t*>(Info.SendDlgMessage(hDlg, DM_GETCONSTTEXTPTR, IDX_CH_PARAM_VALUE_LIST, nullptr))));

  const UnicodeString* value = current_filetype->getParamUserValue(p);
  const UnicodeString* def_value = getParamDefValue(current_filetype, p);
  if (v.compare(*def_value) == 0) {
    if (value != nullptr)
      current_filetype->setParamValue(p, nullptr);
  }
  else if (value == nullptr || v.compare(*value) != 0) {  // changed
    if (current_filetype->getParamValue(p) == nullptr) {
      current_filetype->addParam(&p);
    }
    current_filetype->setParamValue(p, &v);
  }
  delete def_value;
}

void HrcSettingsForm::getCurrentTypeInDialog()
{
  auto k = static_cast<int>(Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, IDX_CH_SCHEMAS, nullptr));
  FarListGetItem f {};
  f.StructSize = sizeof(FarListGetItem);
  f.ItemIndex = k;
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
  const UnicodeString* def_value = getParamDefValue(current_filetype, param);

  size_t count = 3;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = _wcsdup(value_No);
  fcross[1].Text = _wcsdup(value_Yes);
  fcross[2].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());
  delete def_value;

  size_t ret;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->compare(UnicodeString(value_No)) == 0) {
      ret = 0;
    }
    else if (value->compare(UnicodeString(value_Yes)) == 0) {
      ret = 1;
    }
    else {
      ret = 2;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(true);
  auto* lcross = buildFarList(fcross, count);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  removeFarList(lcross);
}

void HrcSettingsForm::setTFListValueToCombobox(const UnicodeString& param) const
{
  const UnicodeString* value = current_filetype->getParamUserValue(param);
  const UnicodeString* def_value = getParamDefValue(current_filetype, param);

  size_t count = 3;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = _wcsdup(value_False);
  fcross[1].Text = _wcsdup(value_True);
  fcross[2].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());
  delete def_value;

  size_t ret;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->compare(UnicodeString(value_False)) == 0) {
      ret = 0;
    }
    else if (value->compare(UnicodeString(value_True)) == 0) {
      ret = 1;
    }
    else {
      ret = 2;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(true);
  auto* lcross = buildFarList(fcross, count);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  removeFarList(lcross);
}

void HrcSettingsForm::setCustomListValueToCombobox(const UnicodeString& param) const
{
  const UnicodeString* value = current_filetype->getParamUserValue(param);
  const UnicodeString* def_value = getParamDefValue(current_filetype, param);

  size_t count = 1;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());
  delete def_value;

  fcross[0].Flags = LIF_SELECTED;
  ChangeParamValueListType(false);
  auto* lcross = buildFarList(fcross, count);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);

  if (value != nullptr) {
    Info.SendDlgMessage(hDlg, DM_SETTEXTPTR, IDX_CH_PARAM_VALUE_LIST, (void*) UStr::to_stdwstr(value).c_str());
  }
  removeFarList(lcross);
}

void HrcSettingsForm::setCrossValueListToCombobox() const
{
  const UnicodeString* value = current_filetype->getParamUserValue(UnicodeString(param_ShowCross));
  const UnicodeString* def_value = getParamDefValue(current_filetype, UnicodeString(param_ShowCross));

  size_t count = 5;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = _wcsdup(value_None);
  fcross[1].Text = _wcsdup(value_Vertical);
  fcross[2].Text = _wcsdup(value_Horizontal);
  fcross[3].Text = _wcsdup(value_Both);
  fcross[4].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());
  delete def_value;

  size_t ret = 0;
  if (value == nullptr || !value->length()) {
    ret = 4;
  }
  else {
    if (value->compare(UnicodeString(value_None)) == 0) {
      ret = 0;
    }
    else if (value->compare(UnicodeString(value_Vertical)) == 0) {
      ret = 1;
    }
    else if (value->compare(UnicodeString(value_Horizontal)) == 0) {
      ret = 2;
    }
    else if (value->compare(UnicodeString(value_Both)) == 0) {
      ret = 3;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(true);
  auto* lcross = buildFarList(fcross, count);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  removeFarList(lcross);
}

void HrcSettingsForm::setCrossPosValueListToCombobox() const
{
  const UnicodeString* value = current_filetype->getParamUserValue(UnicodeString(param_CrossZorder));
  const UnicodeString* def_value = getParamDefValue(current_filetype, UnicodeString(param_CrossZorder));

  size_t count = 3;
  auto* fcross = new FarListItem[count];
  memset(fcross, 0, sizeof(FarListItem) * (count));
  fcross[0].Text = _wcsdup(value_Bottom);
  fcross[1].Text = _wcsdup(value_Top);
  fcross[2].Text = _wcsdup(UStr::to_stdwstr(def_value).c_str());
  delete def_value;

  size_t ret;
  if (value == nullptr || !value->length()) {
    ret = 2;
  }
  else {
    if (value->compare(UnicodeString(value_Bottom)) == 0) {
      ret = 0;
    }
    else if (value->compare(UnicodeString(value_Top)) == 0) {
      ret = 1;
    }
    else {
      ret = 2;
    }
  }
  fcross[ret].Flags = LIF_SELECTED;
  ChangeParamValueListType(true);
  auto* lcross = buildFarList(fcross, count);
  Info.SendDlgMessage(hDlg, DM_LISTSET, IDX_CH_PARAM_VALUE_LIST, lcross);
  removeFarList(lcross);
}

const UnicodeString* HrcSettingsForm::getParamDefValue(FileType* type, const UnicodeString& param) const
{
  const UnicodeString* value;
  value = type->getParamDefaultValue(param);
  if (value == nullptr) {
    value = farEditorSet->defaultType->getParamValue(param);
  }
  if (value == nullptr) {
    return new UnicodeString("<default->");
  }
  else {
    auto* p = new UnicodeString("<default-");
    p->append(UnicodeString(*value));
    p->append(UnicodeString(">"));
    return p;
  }
}

FarList* HrcSettingsForm::buildParamsList(FileType* type) const
{
  // max count params
  size_t size = type->getParamCount() + farEditorSet->defaultType->getParamCount();
  auto* fparam = new FarListItem[size];
  memset(fparam, 0, sizeof(FarListItem) * (size));

  size_t count = 0;
  std::vector<UnicodeString> type_params = type->enumParams();
  for (auto& type_param : type_params) {
    if (farEditorSet->defaultType->getParamValue(type_param) == nullptr) {
      fparam[count++].Text = _wcsdup(UStr::to_stdwstr(&type_param).c_str());
    }
  }
  std::vector<UnicodeString> default_params = farEditorSet->defaultType->enumParams();
  for (auto& default_param : default_params) {
    fparam[count++].Text = _wcsdup(UStr::to_stdwstr(&default_param).c_str());
  }

  fparam[0].Flags = LIF_SELECTED;
  return buildFarList(fparam, count);
}

void HrcSettingsForm::ChangeParamValueListType(bool dropdownlist) const
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