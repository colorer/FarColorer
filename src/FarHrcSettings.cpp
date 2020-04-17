#include "FarHrcSettings.h"
#include <colorer/parsers/ParserFactoryException.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "SettingsControl.h"

void FarHrcSettings::readProfile(SString* plugin_path)
{
  auto* path = new SString(plugin_path);
  path->append(CString(FarProfileXml));
  readXML(path, false);

  delete path;
}

void FarHrcSettings::readXML(String* file, bool userValue)
{
  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  uXmlInputSource config = XmlInputSource::newInstance(file->getWChars(), static_cast<XMLCh*>(nullptr));
  xml_parser.parse(*config->getInputSource());
  if (error_handler.getSawErrors()) {
    throw ParserFactoryException(CString("Error reading hrcsettings.xml."));
  }
  xercesc::DOMDocument* catalog = xml_parser.getDocument();
  xercesc::DOMElement* elem = catalog->getDocumentElement();

  const XMLCh* tagPrototype = L"prototype";
  const XMLCh* tagHrcSettings = L"hrc-settings";

  if (elem == nullptr || !xercesc::XMLString::equals(elem->getNodeName(), tagHrcSettings)) {
    throw FarHrcSettingsException(CString("main '<hrc-settings>' block not found"));
  }
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      xercesc::DOMElement* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), tagPrototype)) {
        UpdatePrototype(subelem, userValue);
      }
    }
  }
}

void FarHrcSettings::UpdatePrototype(xercesc::DOMElement* elem, bool userValue)
{
  const XMLCh* tagProtoAttrParamName = L"name";
  const XMLCh* tagParam = L"param";
  const XMLCh* tagParamAttrParamName = L"name";
  const XMLCh* tagParamAttrParamValue = L"value";
  const XMLCh* tagParamAttrParamDescription = L"description";
  const XMLCh* typeName = elem->getAttribute(tagProtoAttrParamName);
  if (typeName == nullptr) {
    return;
  }
  HRCParser* hrcParser = parserFactory->getHRCParser();
  CString typenamed = CString(typeName);
  FileTypeImpl* type = static_cast<FileTypeImpl*>(hrcParser->getFileType(&typenamed));
  if (type == nullptr) {
    return;
  }

  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      xercesc::DOMElement* subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), tagParam)) {
        const XMLCh* name = subelem->getAttribute(tagParamAttrParamName);
        const XMLCh* value = subelem->getAttribute(tagParamAttrParamValue);
        const XMLCh* descr = subelem->getAttribute(tagParamAttrParamDescription);

        if (*name == '\0') {
          continue;
        }

        if (type->getParamValue(CString(name)) == nullptr) {
          type->addParam(&CString(name));
        }
        if (descr != nullptr) {
          type->setParamDescription(CString(name), &CString(descr));
        }
        if (userValue) {
          type->setParamValue(CString(name), &CString(value));
        } else {
          delete type->getParamDefaultValue(CString(name));
          type->setParamDefaultValue(CString(name), &CString(value));
        }
      }
    }
  }
}

void FarHrcSettings::readUserProfile()
{
  readProfileFromRegistry();
}

void FarHrcSettings::readProfileFromRegistry()
{
  HRCParser* hrcParser = parserFactory->getHRCParser();

  SettingsControl ColorerSettings;
  size_t hrc_subkey;
  hrc_subkey = ColorerSettings.rGetSubKey(0, HrcSettings);
  FarSettingsEnum fse{};
  fse.StructSize = sizeof(FarSettingsEnum);
  // enum all the sections in HrcSettings
  if (ColorerSettings.rEnum(hrc_subkey, &fse)) {
    for (size_t i = 0; i < fse.Count; i++) {
      if (fse.Items[i].Type == FST_SUBKEY) {
        // check whether we have such a scheme
        CString named = CString(fse.Items[i].Name);
        FileTypeImpl* type = static_cast<FileTypeImpl*>(hrcParser->getFileType(&named));
        if (type) {
          // enum all params in the section
          size_t type_subkey;
          type_subkey = ColorerSettings.rGetSubKey(hrc_subkey, fse.Items[i].Name);
          FarSettingsEnum type_fse{};
          type_fse.StructSize = sizeof(FarSettingsEnum);
          if (ColorerSettings.rEnum(type_subkey, &type_fse)) {
            for (size_t j = 0; j < type_fse.Count; j++) {
              if (type_fse.Items[j].Type == FST_STRING) {
                CString name_fse = CString(type_fse.Items[j].Name);
                if (type->getParamValue(name_fse) == nullptr) {
                  type->addParam(&name_fse);
                }
                const wchar_t* p = ColorerSettings.Get(type_subkey, type_fse.Items[j].Name, static_cast<wchar_t*>(nullptr));
                if (p) {
                  CString dp = CString(p);
                  type->setParamValue(CString(type_fse.Items[j].Name), &dp);
                }
              }
            }
          }
        }
      }
    }
  }
}

void FarHrcSettings::writeUserProfile()
{
  writeProfileToRegistry();
}

void FarHrcSettings::writeProfileToRegistry()
{
  HRCParser* hrcParser = parserFactory->getHRCParser();
  FileTypeImpl* type = nullptr;

  SettingsControl ColorerSettings;
  ColorerSettings.rDeleteSubKey(0, HrcSettings);
  size_t hrc_subkey;
  hrc_subkey = ColorerSettings.rGetSubKey(0, HrcSettings);

  // enum all FileTypes
  for (int idx = 0;; idx++) {
    type = static_cast<FileTypeImpl*>(hrcParser->enumerateFileTypes(idx));

    if (!type) {
      break;
    }

    if (type->getParamCount() && type->getParamUserValueCount()) {  // params>0 and user values >0
      size_t type_subkey = ColorerSettings.rGetSubKey(hrc_subkey, type->getName()->getWChars());
      // enum all params
      std::vector<SString> type_params = type->enumParams();
      for (auto paramname = type_params.begin(); paramname != type_params.end(); ++paramname) {
        const String* v = type->getParamUserValue(*paramname);
        if (v != nullptr) {
          ColorerSettings.Set(type_subkey, paramname->getWChars(), v->getWChars());
        }
      }
    }
  }
}
