#ifndef FARCOLORER_SETTINGSCONTROL_H
#define FARCOLORER_SETTINGSCONTROL_H

#include <colorer/Exception.h>
#include "pcolorer.h"

class SettingsControl
{
 public:
  SettingsControl();
  ~SettingsControl();

  const wchar_t* Get(size_t Root, const wchar_t* Name, const wchar_t* Default);
  void Get(size_t Root, const wchar_t* Name, wchar_t* Value, size_t Size, const wchar_t* Default);
  unsigned __int64 Get(size_t Root, const wchar_t* Name, unsigned __int64 Default);
  __int64 Get(size_t Root, const wchar_t* Name, __int64 Default);
  int Get(size_t Root, const wchar_t* Name, int Default);
  unsigned int Get(size_t Root, const wchar_t* Name, unsigned int Default);
  DWORD Get(size_t Root, const wchar_t* Name, DWORD Default);
  bool Get(size_t Root, const wchar_t* Name, bool Default);

  bool Set(size_t Root, const wchar_t* Name, unsigned __int64 Value);
  bool Set(size_t Root, const wchar_t* Name, const wchar_t* Value);
  bool Set(size_t Root, const wchar_t* Name, __int64 Value);
  bool Set(size_t Root, const wchar_t* Name, int Value);
  bool Set(size_t Root, const wchar_t* Name, unsigned int Value);
  bool Set(size_t Root, const wchar_t* Name, DWORD Value);
  bool Set(size_t Root, const wchar_t* Name, bool Value);

  size_t rGetSubKey(size_t Root, const wchar_t* Name);
  bool rEnum(size_t Root, FarSettingsEnum* fse);
  bool rDeleteSubKey(size_t Root, const wchar_t* Name);

 private:
  HANDLE farSettingHandle;
};

class SettingsControlException : public Exception
{
 public:
  explicit SettingsControlException(const UnicodeString& msg) noexcept : Exception("[SettingsControl] " + msg) {}
};

#endif // FARCOLORER_SETTINGSCONTROL_H