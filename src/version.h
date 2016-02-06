#define VER_FILEVERSION             1,2,4,0
#define VER_FILEVERSION_STR         "1.2.4.0"

#define VER_PRODUCTVERSION          VER_FILEVERSION
#define VER_PRODUCTVERSION_STR      VER_FILEVERSION_STR
#ifdef WIN64
  #define CONF " (x64)"
#else
  #define CONF ""
#endif
#define FILE_DESCRIPTION "FarColorer - Syntax Highlighting for Far Manager 3.0" CONF
