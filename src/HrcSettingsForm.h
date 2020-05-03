#ifndef _HRCSETTINGSFORM_H_
#define _HRCSETTINGSFORM_H_

#include "FarEditorSet.h"

class HrcSettingsForm
{
 public:
  explicit HrcSettingsForm(FarEditorSet* _farEditorSet, FileType* filetype);
  ~HrcSettingsForm() = default;

  bool Show();

  bool showForm();
  FarList* buildHrcList() const;
  void OnSaveHrcParams() const;
  void OnChangeParam(intptr_t idx);
  void SaveChangedValueParam() const;
  void getCurrentTypeInDialog();
  void OnChangeHrc();
  void setCustomListValueToCombobox(const CString& param) const;
  void setTFListValueToCombobox(const CString& param) const;
  void setYNListValueToCombobox(const CString& param) const;
  void setCrossPosValueListToCombobox() const;
  void setCrossValueListToCombobox() const;
  void ChangeParamValueListType(bool dropdownlist) const;
  FarList* buildParamsList(FileTypeImpl* type) const;
  const String* getParamDefValue(FileTypeImpl* type, const SString& param) const;
  size_t getCountFileTypeAndGroup() const;

  static FarList* buildFarList(FarListItem* list, size_t count);
  static void removeFarList(FarList* list);

  FarEditorSet* farEditorSet;
  intptr_t menuid;
  FileTypeImpl* current_filetype;
  FileType* filetype_in_editor;
  HANDLE hDlg {};
};

#endif  // _HRCSETTINGSFORM_H_
