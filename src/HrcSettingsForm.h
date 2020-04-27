#ifndef _HRCSETTINGSFORM_H_
#define _HRCSETTINGSFORM_H_

#include "FarEditorSet.h"

class HrcSettingsForm
{
 public:
  explicit HrcSettingsForm(FarEditorSet* _farEditorSet);
  ~HrcSettingsForm();

  bool Show();

  bool showForm();
  FarList* buildHrcList() const;
  void OnSaveHrcParams(HANDLE hDlg);
  void OnChangeParam(HANDLE hDlg, intptr_t idx);
  void SaveChangedValueParam(HANDLE hDlg);
  FileTypeImpl* getCurrentTypeInDialog(HANDLE hDlg) const;
  void OnChangeHrc(HANDLE hDlg);
  void setCustomListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param);
  void setTFListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param);
  void setYNListValueToCombobox(FileTypeImpl* type, HANDLE hDlg, CString param);
  void setCrossPosValueListToCombobox(FileTypeImpl* type, HANDLE hDlg);
  void setCrossValueListToCombobox(FileTypeImpl* type, HANDLE hDlg);
  void ChangeParamValueListType(HANDLE hDlg, bool dropdownlist);
  FarList* buildParamsList(FileTypeImpl* type) const;
  const String* getParamDefValue(FileTypeImpl* type, SString param) const;
  size_t getCountFileTypeAndGroup() const;
  FileTypeImpl* getFileTypeByIndex(int idx) const;

  FarEditorSet* farEditorSet;
  bool dialogFirstFocus;
  intptr_t menuid;
};

#endif  // _HRCSETTINGSFORM_H_
