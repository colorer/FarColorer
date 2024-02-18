#ifndef FARCOLORER_HRCSETTINGSFORM_H
#define FARCOLORER_HRCSETTINGSFORM_H

#include "FarEditorSet.h"

class HrcSettingsForm
{
 public:
  explicit HrcSettingsForm(FarEditorSet* _farEditorSet, FileType* filetype);
  ~HrcSettingsForm() = default;

  bool Show();

  bool showForm();
  [[nodiscard]] FarList* buildHrcList() const;
  void OnSaveHrcParams() const;
  void OnChangeParam(intptr_t idx);
  void SaveChangedValueParam() const;
  void getCurrentTypeInDialog();
  void OnChangeHrc();
  void setCustomListValueToCombobox(const UnicodeString& param) const;
  void setTFListValueToCombobox(const UnicodeString& param) const;
  void setYNListValueToCombobox(const UnicodeString& param) const;
  void setCrossPosValueListToCombobox() const;
  void setCrossValueListToCombobox() const;
  void ChangeParamValueList(FarList* items, bool dropdownlist) const;
  FarList* buildParamsList(FileType* type) const;
  const UnicodeString* getParamDefValue(FileType* type, const UnicodeString& param) const;
  [[nodiscard]] size_t getCountFileTypeAndGroup() const;

  static FarList* buildFarList(FarListItem* list, size_t count);
  static void removeFarList(FarList* list);

  FarEditorSet* farEditorSet;
  intptr_t menuid;
  FileType* current_filetype;
  FileType* filetype_in_editor;
  HANDLE hDlg {};
};

#endif  // FARCOLORER_HRCSETTINGSFORM_H
