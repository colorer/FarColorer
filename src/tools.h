#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <colorer/Common.h>
#include "pcolorer.h"

wchar_t* rtrim(wchar_t* str);
wchar_t* ltrim(wchar_t* str);
wchar_t* trim(wchar_t* str);
wchar_t* Upper(wchar_t* Ch);

wchar_t* PathToFull(const wchar_t* path, bool unc);
UnicodeString* PathToFullS(const wchar_t* path, bool unc);

intptr_t macroGetValue(FarMacroValue* value, intptr_t def = 0);
FarMacroCall* macroReturnInt(long long int value);
FarMacroCall* macroReturnValues(FarMacroValue* values, int count);
void WINAPI MacroCallback(void* CallbackData, FarMacroValue* Values, size_t Count);
#endif
