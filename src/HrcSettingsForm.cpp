#include "HrcSettingsForm.h"
#include "tools.h"

HrcSettingsForm::HrcSettingsForm(FarEditorSet* _farEditorSet) : dialogFirstFocus(false), menuid(0)
{
  farEditorSet = _farEditorSet;
}

HrcSettingsForm::~HrcSettingsForm() {}

bool HrcSettingsForm::Show()
{
  return showForm();
}

INT_PTR WINAPI SettingHrcDialogProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
{
  auto* fes = reinterpret_cast<HrcSettingsForm*>(Info.SendDlgMessage(hDlg, DM_GETDLGDATA, 0, nullptr));

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
      {DI_BUTTON, 37, 20, 0, 0, 0, nullptr, nullptr, DIF_DEFAULTBUTTON, nullptr, 0, 0},  // IDX_OK,
      {DI_BUTTON, 45, 20, 0, 0, 0, nullptr, nullptr, 0, nullptr, 0, 0},                  // IDX_CANCEL,
  };

  fdi[IDX_CH_BOX].Data = farEditorSet->GetMsg(mUserHrcSettingDialog);
  fdi[IDX_CH_CAPTIONLIST].Data = farEditorSet->GetMsg(mListSyntax);
  FarList* l = buildHrcList();
  fdi[IDX_CH_SCHEMAS].ListItems = l;
  fdi[IDX_CH_SCHEMAS].Flags = DIF_LISTWRAPMODE | DIF_DROPDOWNLIST;
  fdi[IDX_CH_OK].Data = farEditorSet->GetMsg(mOk);
  fdi[IDX_CH_CANCEL].Data = farEditorSet->GetMsg(mCancel);
  fdi[IDX_CH_PARAM_LIST].Data = farEditorSet->GetMsg(mParamList);
  fdi[IDX_CH_PARAM_VALUE_CAPTION].Data = farEditorSet->GetMsg(mParamValue);
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

size_t HrcSettingsForm::getCountFileTypeAndGroup() const
{
  size_t num = 0;
  const String* group = nullptr;
  FileType* type = nullptr;

  for (int idx = 0;; idx++, num++) {
    type = farEditorSet->hrcParser->enumerateFileTypes(idx);

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

FileTypeImpl* HrcSettingsForm::getFileTypeByIndex(int idx) const
{
  FileType* type = nullptr;
  const String* group = nullptr;

  for (int i = 0; idx >= 0; idx--, i++) {
    type = farEditorSet->hrcParser->enumerateFileTypes(i);

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

FarList* HrcSettingsForm::buildHrcList() const
{
  size_t num = getCountFileTypeAndGroup();
  const String* group = nullptr;
  FileType* type = nullptr;

  auto* hrcList = new FarListItem[num];
  memset(hrcList, 0, sizeof(FarListItem) * (num));

  for (int idx = 0, i = 0;; idx++, i++) {
    type = farEditorSet->hrcParser->enumerateFileTypes(idx);

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

void HrcSettingsForm::OnChangeParam(HANDLE hDlg, intptr_t idx)
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
    value = farEditorSet->defaultType->getParamDescription(p);
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
    else if (p.equals(&DMaxLen) || p.equals(&DBackparse) || p.equals(&DDefFore) || p.equals(&DDefBack) || CString("firstlines").equals(&p) ||
             CString("firstlinebytes").equals(&p) || p.equals(&DHotkey)) {
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

void HrcSettingsForm::OnSaveHrcParams(HANDLE hDlg)
{
  SaveChangedValueParam(hDlg);
  FarHrcSettings p(farEditorSet->parserFactory.get());
  p.writeUserProfile();
}

void HrcSettingsForm::SaveChangedValueParam(HANDLE hDlg)
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

FileTypeImpl* HrcSettingsForm::getCurrentTypeInDialog(HANDLE hDlg) const
{
  auto k = static_cast<int>(Info.SendDlgMessage(hDlg, DM_LISTGETCURPOS, IDX_CH_SCHEMAS, nullptr));
  return getFileTypeByIndex(k);
}

void HrcSettingsForm::OnChangeHrc(HANDLE hDlg)
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

void HrcSettingsForm::setYNListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param)
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

void HrcSettingsForm::setTFListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param)
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

void HrcSettingsForm::setCustomListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param)
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

void HrcSettingsForm::setCrossValueListToCombobox(FileTypeImpl* type, HANDLE hDlg)
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

void HrcSettingsForm::setCrossPosValueListToCombobox(FileTypeImpl* type, HANDLE hDlg)
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

const String* HrcSettingsForm::getParamDefValue(FileTypeImpl* type, SString param) const
{
  const String* value;
  value = type->getParamDefaultValue(param);
  if (value == nullptr) {
    value = farEditorSet->defaultType->getParamValue(param);
  }
  auto* p = new SString("<default-");
  p->append(CString(value));
  p->append(CString(">"));
  return p;
}

FarList* HrcSettingsForm::buildParamsList(FileTypeImpl* type) const
{
  // max count params
  size_t size = type->getParamCount() + farEditorSet->defaultType->getParamCount();
  auto* fparam = new FarListItem[size];
  memset(fparam, 0, sizeof(FarListItem) * (size));

  size_t count = 0;
  std::vector<SString> default_params = farEditorSet->defaultType->enumParams();
  for (auto& default_param : default_params) {
    fparam[count++].Text = wcsdup(default_param.getWChars());
  }
  std::vector<SString> type_params = type->enumParams();
  for (auto& type_param : type_params) {
    if (farEditorSet->defaultType->getParamValue(type_param) == nullptr) {
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

void HrcSettingsForm::ChangeParamValueListType(HANDLE hDlg, bool dropdownlist)
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