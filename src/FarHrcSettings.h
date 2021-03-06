#ifndef _FARHRCSETTINGS_H_
#define _FARHRCSETTINGS_H_

#include <colorer/HRCParser.h>
#include <colorer/parsers/FileTypeImpl.h>
#include <colorer/parsers/ParserFactory.h>

const wchar_t FarCatalogXml[] = L"\\base\\catalog.xml";
const wchar_t FarProfileXml[] = L"\\bin\\hrcsettings.xml";
const wchar_t HrcSettings[] = L"HrcSettings";

class FarHrcSettingsException : public Exception
{
 public:
  FarHrcSettingsException() noexcept : Exception("[FarHrcSettingsException] ") {};
  FarHrcSettingsException(const String& msg) noexcept : FarHrcSettingsException()
  {
    what_str.append(msg);
  };
};

class FarHrcSettings
{
  friend class FileTypeImpl;

 public:
  FarHrcSettings(ParserFactory* _parserFactory)
  {
    parserFactory = _parserFactory;
  }
  void readXML(String* file, bool userValue);
  void readProfile(SString* plugin_path);
  void readUserProfile();
  void writeUserProfile();

 private:
  void UpdatePrototype(xercesc::DOMElement* elem, bool userValue);
  void readProfileFromRegistry();
  void writeProfileToRegistry();

  ParserFactory* parserFactory;
};

#endif