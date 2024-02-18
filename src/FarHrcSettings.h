#ifndef FARCOLORER_FARHRCSETTINGS_H
#define FARCOLORER_FARHRCSETTINGS_H

#include <colorer/FileType.h>
#include <colorer/HrcLibrary.h>
#include <colorer/ParserFactory.h>
#include <xercesc/dom/DOM.hpp>
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
  void readXML(UnicodeString* file);
  void readPluginHrcSettings(UnicodeString* plugin_path);
  void readUserProfile();
  void writeUserProfile();

 private:
  void UpdatePrototype(xercesc::DOMElement* elem);

  ParserFactory* parserFactory;
  FarEditorSet* farEditorSet;
};

#endif // FARCOLORER_FARHRCSETTINGS_H