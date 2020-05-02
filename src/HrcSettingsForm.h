#ifndef _HRCSETTINGSFORM_H_
#define _HRCSETTINGSFORM_H_

#include "FarEditorSet.h"

class HrcSettingsForm
{
 public:
  explicit HrcSettingsForm(FarEditorSet* _farEditorSet);
  ~HrcSettingsForm()= default;

  bool Show();

  bool showForm();
  FarList* buildHrcList() const;
  void OnSaveHrcParams();
  void OnChangeParam(intptr_t idx);
  void SaveChangedValueParam() const;
  void getCurrentTypeInDialog();
  void OnChangeHrc();
  void setCustomListValueToCombobox(CString param) const;
  void setTFListValueToCombobox(CString param) const;
  void setYNListValueToCombobox(CString param) const;
  void setCrossPosValueListToCombobox() const;
  void setCrossValueListToCombobox() const;
  void ChangeParamValueListType(bool dropdownlist) const;
  FarList* buildParamsList(FileTypeImpl* type) const;
  const String* getParamDefValue(FileTypeImpl* type, SString param) const;
  size_t getCountFileTypeAndGroup() const;

  static FarList* buildFarList(FarListItem* list, size_t count);
  static void removeFarList(FarList* list);

  FarEditorSet* farEditorSet;
  intptr_t menuid;
  FileTypeImpl* current_filetype;
  HANDLE hDlg;
};

#endif  // _HRCSETTINGSFORM_H_
