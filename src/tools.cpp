#include "tools.h"

wchar_t* rtrim(wchar_t* str)
{
  wchar_t* ptr = str;
  str += wcslen(str);

  while (iswspace(*(--str))) {
    *str = 0;
  }

  return ptr;
}

wchar_t* ltrim(wchar_t* str)
{
  while (iswspace(*(str++))) {
  }
  return str - 1;
}

wchar_t* trim(wchar_t* str)
{
  return ltrim(rtrim(str));
}

/**
  Function converts a path in the UNC path.
  Source path can be framed by quotes, be a relative, or contain environment variables
*/
wchar_t* PathToFull(const wchar_t* path, bool unc)
{
  size_t len = wcslen(path);
  if (!len) {
    return nullptr;
  }

  wchar_t* new_path = nullptr;
  // we remove quotes, if they are present, focusing on the first character
  // if he quote it away and the first and last character.
  // If the first character quote, but the latter does not - well, it's not our
  // problem, and so and so error
  if (*path == L'"') {
    len--;
    new_path = new wchar_t[len];
    wcsncpy(new_path, &path[1], len - 1);
    new_path[len - 1] = '\0';
  } else {
    len++;
    new_path = new wchar_t[len];
    wcscpy(new_path, path);
  }

  // replace the environment variables to their values
  size_t i = ExpandEnvironmentStringsW(new_path, nullptr, 0);
  if (i > len) {
    len = i;
  }
  wchar_t* temp = new wchar_t[len];
  ExpandEnvironmentStringsW(new_path, temp, static_cast<DWORD>(i));
  delete[] new_path;
  new_path = temp;

  CONVERTPATHMODES mode;
  if (unc) {
    mode = CPM_NATIVE;
  } else {
    mode = CPM_FULL;
  }

  // take the full path to the file, converting all kinds of ../ ./ etc
  size_t p = FSF.ConvertPath(mode, new_path, nullptr, 0);
  if (p > len) {
    len = p;
    wchar_t* temp = new wchar_t[len];
    wcscpy(temp, new_path);
    delete[] new_path;
    new_path = temp;
  }
  FSF.ConvertPath(mode, new_path, new_path, len);

  return trim(new_path);
}

SString* PathToFullS(const wchar_t* path, bool unc)
{
  SString* spath = nullptr;
  wchar_t* t = PathToFull(path, unc);
  if (t) {
    spath = new SString(t);
  }
  delete[] t;
  return spath;
}

intptr_t GetValue(FarMacroValue* value, intptr_t def)
{
  intptr_t result = def;
  if (FMVT_INTEGER == value->Type)
    result = value->Integer;
  else if (FMVT_DOUBLE == value->Type)
    result = static_cast<intptr_t>(value->Double);
  return result;
}