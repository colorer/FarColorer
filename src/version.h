#ifndef FARCOLORER_VERSION_H_
#define FARCOLORER_VERSION_H_

#ifdef _WIN64
#define PLATFORM " x64"
#elif defined _M_ARM64
#define PLATFORM " ARM64"
#elif defined _WIN32
#define PLATFORM " x86"
#else
#define PLATFORM ""
#endif

#define PLUGIN_VER_MAJOR 1
#define PLUGIN_VER_MINOR 4
#define PLUGIN_VER_PATCH 25
#define PLUGIN_DESC L"FarColorer - Syntax Highlighting for Far Manager 3" PLATFORM
#define PLUGIN_NAME L"FarColorer"
#define PLUGIN_FILENAME L"colorer.dll"
#define PLUGIN_COPYRIGHT L"(c) 1999-2009 Igor Russkih, (c) Aleksey Dobrunov 2009-2024"

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define PLUGIN_VERSION STRINGIZE(PLUGIN_VER_MAJOR) "." STRINGIZE(PLUGIN_VER_MINOR) "." STRINGIZE(PLUGIN_VER_PATCH)

#endif  // FARCOLORER_VERSION_H_
