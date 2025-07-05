#ifndef FARCOLORER_FARHRCSETTINGS_H
#define FARCOLORER_FARHRCSETTINGS_H

#include <colorer/ParserFactory.h>
#include "FarEditorSet.h"

const wchar_t FarCatalogXml[] = L"\\base\\catalog.xml";
const wchar_t FarProfileXml[] = L"\\bin\\hrcsettings.xml";
const wchar_t HrcSettings[] = L"HrcSettings";

class FarHrcSettingsException : public Exception
{
 public:
  explicit FarHrcSettingsException(const UnicodeString& msg) noexcept : Exception("[FarHrcSettingsException] " + msg) {}
};

class FarHrcSettings
{
 public:
  explicit FarHrcSettings(FarEditorSet* _farEditorSet, ParserFactory* _parserFactory)
  {
    parserFactory = _parserFactory;
    farEditorSet = _farEditorSet;
  }
  void readPluginHrcSettings(const UnicodeString* plugin_path);
  void readUserProfile(const FileType* def_filetype = nullptr);
  void writeUserProfile();

 private:
  ParserFactory* parserFactory;
  FarEditorSet* farEditorSet;
};

#endif // FARCOLORER_FARHRCSETTINGS_H