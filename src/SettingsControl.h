#ifndef _SETTINGSCONTROL_H_
#define _SETTINGSCONTROL_H_

#include "pcolorer.h"
#include <colorer/Exception.h>

class SettingsControl
{
public:
  SettingsControl();
  ~SettingsControl();

  const wchar_t*   Get(size_t Root, const wchar_t *Name, const wchar_t *Default);
  unsigned __int64 Get(size_t Root, const wchar_t *Name, unsigned __int64 Default);
  __int64          Get(size_t Root, const wchar_t *Name, __int64 Default) { return (__int64)Get(Root,Name,(unsigned __int64)Default); }
  int              Get(size_t Root, const wchar_t *Name, int Default)  { return (int)Get(Root,Name,(unsigned __int64)Default); }
  unsigned int     Get(size_t Root, const wchar_t *Name, unsigned int Default) { return (unsigned int)Get(Root,Name,(unsigned __int64)Default); }
  DWORD            Get(size_t Root, const wchar_t *Name, DWORD Default) { return (DWORD)Get(Root,Name,(unsigned __int64)Default); }
  bool             Get(size_t Root, const wchar_t *Name, bool Default) { return Get(Root,Name,Default?1ull:0ull)?true:false; }

  bool Set(size_t Root, const wchar_t *Name, unsigned __int64 Value);
  bool Set(size_t Root, const wchar_t *Name, const wchar_t *Value);
  bool Set(size_t Root, const wchar_t *Name, __int64 Value) { return Set(Root,Name,(unsigned __int64)Value); }
  bool Set(size_t Root, const wchar_t *Name, int Value) { return Set(Root,Name,(unsigned __int64)Value); }
  bool Set(size_t Root, const wchar_t *Name, unsigned int Value) { return Set(Root,Name,(unsigned __int64)Value); }
  bool Set(size_t Root, const wchar_t *Name, DWORD Value) { return Set(Root,Name,(unsigned __int64)Value); }
  bool Set(size_t Root, const wchar_t *Name, bool Value) { return Set(Root,Name,Value?1ull:0ull); }

  size_t rGetSubKey(size_t Root, const wchar_t *Name);
  bool rEnum(size_t Root, FarSettingsEnum *fse);
  bool rDeleteSubKey(size_t Root,const wchar_t *Name);

private:
  HANDLE farSettingHandle;
};

class SettingsControlException : public Exception{
public:
  SettingsControlException() noexcept : Exception("[SettingsControl] ") {};
  SettingsControlException(const String &msg) noexcept : SettingsControlException(){
    what_str.append(msg);
  };
};

#endif 