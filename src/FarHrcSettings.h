#ifndef _FARHRCSETTINGS_H_
#define _FARHRCSETTINGS_H_

#include <colorer/parsers/helpers/FileTypeImpl.h>
#include <colorer/HRCParser.h>
#include <colorer/ParserFactory.h>

#include"pcolorer.h"
#include "SettingsControl.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 50 // in msdn 16383 , but we have enough 50

const wchar_t FarCatalogXml[] = L"\\base\\catalog.xml";
const wchar_t FarProfileXml[] = L"\\bin\\hrcsettings.xml";
const wchar_t HrcSettings[] = L"HrcSettings";


class FarHrcSettingsException : public Exception
{
public:
  FarHrcSettingsException() {};
  FarHrcSettingsException(const String &msg)
  {
    message->append(DString("FarHrcSettingsException: ")).append(msg);
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
  void readProfile();
  void readUserProfile();
  void writeUserProfile();

private:
  void UpdatePrototype(xercesc::DOMElement* elem, bool userValue);
  void readProfileFromRegistry();
  void writeProfileToRegistry();

  ParserFactory* parserFactory;

};


#endif