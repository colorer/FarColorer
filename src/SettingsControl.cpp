#include "SettingsControl.h"
#include <colorer/unicode/CString.h>

SettingsControl::SettingsControl()
{
  FarSettingsCreate fsc;
  fsc.Guid = MainGuid;
  fsc.StructSize = sizeof(FarSettingsCreate);
  if (Info.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, PSL_ROAMING, &fsc)) {
    farSettingHandle = fsc.Handle;
  } else {
    farSettingHandle = INVALID_HANDLE_VALUE;
    throw SettingsControlException(CString("Access error to the FarSettings."));
  }
}

SettingsControl::~SettingsControl()
{
  Info.SettingsControl(farSettingHandle, SCTL_FREE, 0, nullptr);
}

const wchar_t* SettingsControl::Get(size_t Root, const wchar_t* Name, const wchar_t* Default)
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_STRING};
  if (Info.SettingsControl(farSettingHandle, SCTL_GET, 0, &item)) {
    return item.String;
  }
  return Default;
}

void SettingsControl::Get(size_t Root, const wchar_t* Name, wchar_t* Value, size_t Size, const wchar_t* Default)
{
  lstrcpynW(Value, Get(Root, Name, Default), (int) Size);
}

unsigned __int64 SettingsControl::Get(size_t Root, const wchar_t* Name, unsigned __int64 Default)
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_QWORD};
  if (Info.SettingsControl(farSettingHandle, SCTL_GET, 0, &item)) {
    return item.Number;
  }
  return Default;
}

bool SettingsControl::Set(size_t Root, const wchar_t* Name, const wchar_t* Value)
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_STRING};
  item.String = Value;
  return Info.SettingsControl(farSettingHandle, SCTL_SET, 0, &item) != FALSE;
}

bool SettingsControl::Set(size_t Root, const wchar_t* Name, unsigned __int64 Value)
{
  FarSettingsItem item = {sizeof(FarSettingsItem), Root, Name, FST_QWORD};
  item.Number = Value;
  return Info.SettingsControl(farSettingHandle, SCTL_SET, 0, &item) != FALSE;
}

size_t SettingsControl::rGetSubKey(size_t Root, const wchar_t* Name)
{
  FarSettingsValue fsv = {sizeof(FarSettingsValue), Root, Name};
  return (size_t) Info.SettingsControl(farSettingHandle, SCTL_CREATESUBKEY, 0, &fsv);
}

bool SettingsControl::rEnum(size_t Root, FarSettingsEnum* fse)
{
  fse->Root = Root;
  return !!Info.SettingsControl(farSettingHandle, SCTL_ENUM, 0, fse);
}

bool SettingsControl::rDeleteSubKey(size_t Root, const wchar_t* Name)
{
  FarSettingsValue fsv = {sizeof(FarSettingsValue), Root, Name};
  return !!Info.SettingsControl(farSettingHandle, SCTL_DELETE, 0, &fsv);
}

__int64 SettingsControl::Get(size_t Root, const wchar_t* Name, __int64 Default)
{
  return (__int64) Get(Root, Name, (unsigned __int64) Default);
}

int SettingsControl::Get(size_t Root, const wchar_t* Name, int Default)
{
  return (int) Get(Root, Name, (unsigned __int64) Default);
}

unsigned int SettingsControl::Get(size_t Root, const wchar_t* Name, unsigned int Default)
{
  return (unsigned int) Get(Root, Name, (unsigned __int64) Default);
}

DWORD SettingsControl::Get(size_t Root, const wchar_t* Name, DWORD Default)
{
  return (DWORD) Get(Root, Name, (unsigned __int64) Default);
}

bool SettingsControl::Get(size_t Root, const wchar_t* Name, bool Default)
{
  return Get(Root, Name, Default ? 1ull : 0ull) ? true : false;
}

bool SettingsControl::Set(size_t Root, const wchar_t* Name, __int64 Value)
{
  return Set(Root, Name, (unsigned __int64) Value);
}

bool SettingsControl::Set(size_t Root, const wchar_t* Name, int Value)
{
  return Set(Root, Name, (unsigned __int64) Value);
}

bool SettingsControl::Set(size_t Root, const wchar_t* Name, unsigned int Value)
{
  return Set(Root, Name, (unsigned __int64) Value);
}

bool SettingsControl::Set(size_t Root, const wchar_t* Name, DWORD Value)
{
  return Set(Root, Name, (unsigned __int64) Value);
}

bool SettingsControl::Set(size_t Root, const wchar_t* Name, bool Value)
{
  return Set(Root, Name, Value ? 1ull : 0ull);
}