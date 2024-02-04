#ifndef _FARHRCSETTINGS_H_
#define _FARHRCSETTINGS_H_

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
  void readXML(UnicodeString* file, bool userValue);
  void readProfile(UnicodeString* plugin_path);
  void readUserProfile();
  void writeUserProfile();

 private:
  void UpdatePrototype(xercesc::DOMElement* elem, bool userValue);
  void readProfileFromRegistry();
  void writeProfileToRegistry();

  ParserFactory* parserFactory;
  FarEditorSet* farEditorSet;
};

#endif