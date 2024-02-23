#include "FarHrcSettings.h"
#include <colorer/common/UStr.h>
#include <colorer/base/XmlTagDefs.h>
#include <colorer/xml/XmlParserErrorHandler.h>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "SettingsControl.h"
#include "colorer/parsers/CatalogParser.h"

void FarHrcSettings::loadUserHrc(const UnicodeString* filename)
{
  if (filename && !filename->isEmpty()) {
    parserFactory->loadHrcPath(*filename);
  }
}

void FarHrcSettings::loadUserHrd(const UnicodeString* filename)
{
  if (!filename || filename->isEmpty()) {
    return;
  }

  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler err_handler;
  xml_parser.setErrorHandler(&err_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  uXmlInputSource config = XmlInputSource::newInstance(filename);
  xml_parser.parse(*config->getInputSource());
  if (err_handler.getSawErrors()) {
    throw ParserFactoryException(UnicodeString("Error reading ").append(*filename));
  }
  xercesc::DOMDocument* catalog = xml_parser.getDocument();
  xercesc::DOMElement* elem = catalog->getDocumentElement();
  const XMLCh* tagHrdSets = catTagHrdSets;
  const XMLCh* tagHrd = catTagHrd;
  if (elem == nullptr || !xercesc::XMLString::equals(elem->getNodeName(), tagHrdSets)) {
    throw Exception("main '<hrd-sets>' block not found");
  }
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = dynamic_cast<xercesc::DOMElement*>(node);
      if (subelem && xercesc::XMLString::equals(subelem->getNodeName(), tagHrd)) {
        auto hrd = CatalogParser::parseHRDSetsChild(subelem);
        if (hrd)
          parserFactory->addHrd(std::move(hrd));
      }
    }
  }
}

void FarHrcSettings::readPluginHrcSettings(const UnicodeString* plugin_path)
{
  auto path = UnicodeString(*plugin_path);
  path.append(UnicodeString(FarProfileXml));
  readXML(&path);
}

void FarHrcSettings::readXML(const UnicodeString* file)
{
  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler;
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  xml_parser.setDisableDefaultEntityResolution(true);
  uXmlInputSource config = XmlInputSource::newInstance(file);
  xml_parser.parse(*config->getInputSource());
  if (error_handler.getSawErrors()) {
    throw ParserFactoryException("Error reading hrcsettings.xml.");
  }
  xercesc::DOMDocument* catalog = xml_parser.getDocument();
  xercesc::DOMElement* elem = catalog->getDocumentElement();

  const XMLCh* tagPrototype = hrcTagPrototype;
  const char16_t tagHrcSettings[] = u"hrc-settings\0";

  if (elem == nullptr || !xercesc::XMLString::equals(elem->getNodeName(), tagHrcSettings)) {
    throw FarHrcSettingsException("main '<hrc-settings>' block not found");
  }
  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = dynamic_cast<xercesc::DOMElement*>(node);
      if (subelem && xercesc::XMLString::equals(subelem->getNodeName(), tagPrototype)) {
        UpdatePrototype(subelem);
      }
    }
  }
}

void FarHrcSettings::UpdatePrototype(xercesc::DOMElement* elem)
{
  auto typeName = elem->getAttribute(hrcPrototypeAttrName);
  if (typeName == nullptr) {
    return;
  }
  auto& hrcLibrary = parserFactory->getHrcLibrary();
  UnicodeString typenamed = UnicodeString(typeName);
  auto* type = hrcLibrary.getFileType(&typenamed);
  if (type == nullptr) {
    return;
  }

  for (xercesc::DOMNode* node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      auto* subelem = dynamic_cast<xercesc::DOMElement*>(node);
      if (subelem && xercesc::XMLString::equals(subelem->getNodeName(), hrcTagParam)) {
        auto name = subelem->getAttribute(hrcParamAttrName);
        auto value = subelem->getAttribute(hrcParamAttrValue);
        auto descr = subelem->getAttribute(hrcParamAttrDescription);

        if (UStr::isEmpty(name)) {
          continue;
        }

        UnicodeString cname = UnicodeString(name);
        UnicodeString cvalue = UnicodeString(value);
        UnicodeString cdescr = UnicodeString(descr);
        if (type->getParamValue(cname) == nullptr) {
          type->addParam(cname, cvalue);
        }
        else {
          type->setParamDefaultValue(cname, &cvalue);
        }
        if (descr != nullptr) {
          type->setParamDescription(cname, &cdescr);
        }
      }
    }
  }
}

void FarHrcSettings::readUserProfile()
{
  auto& hrcLibrary = parserFactory->getHrcLibrary();

  SettingsControl ColorerSettings;
  auto hrc_subkey = ColorerSettings.rGetSubKey(0, HrcSettings);
  FarSettingsEnum fse {sizeof(FarSettingsEnum)};

  // enum all the sections in HrcSettings
  if (ColorerSettings.rEnum(hrc_subkey, &fse)) {
    for (size_t i = 0; i < fse.Count; i++) {
      if (fse.Items[i].Type == FST_SUBKEY) {
        // check whether we have such a scheme
        UnicodeString named = UnicodeString(fse.Items[i].Name);
        auto* type = hrcLibrary.getFileType(&named);
        if (type) {
          // enum all params in the section
          auto type_subkey = ColorerSettings.rGetSubKey(hrc_subkey, fse.Items[i].Name);
          FarSettingsEnum type_fse {sizeof(FarSettingsEnum)};
          if (ColorerSettings.rEnum(type_subkey, &type_fse)) {
            for (size_t j = 0; j < type_fse.Count; j++) {
              if (type_fse.Items[j].Type == FST_STRING) {
                const wchar_t* p = ColorerSettings.Get(type_subkey, type_fse.Items[j].Name, static_cast<wchar_t*>(nullptr));
                if (p) {
                  UnicodeString name_fse = UnicodeString(type_fse.Items[j].Name);
                  UnicodeString dp = UnicodeString(p);
                  farEditorSet->addParamAndValue(type, name_fse, dp);
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
  auto& hrcLibrary = parserFactory->getHrcLibrary();

  SettingsControl ColorerSettings;
  auto hrc_subkey = ColorerSettings.rGetSubKey(0, HrcSettings);

  // enum all FileTypes
  for (int idx = 0;; idx++) {
    auto type = hrcLibrary.enumerateFileTypes(idx);

    if (!type) {
      break;
    }

    if (type->getParamCount()) {  // params>0
      auto type_subkey = ColorerSettings.rGetSubKey(hrc_subkey, UStr::to_stdwstr(type->getName()).c_str());
      // enum all params
      std::vector<UnicodeString> type_params = type->enumParams();
      for (auto& type_param : type_params) {
        const UnicodeString* v = type->getParamUserValue(type_param);
        if (v != nullptr) {
          ColorerSettings.Set(type_subkey, UStr::to_stdwstr(&type_param).c_str(), UStr::to_stdwstr(v).c_str());
        }
        else {
          ColorerSettings.rDeleteSubKey(type_subkey, UStr::to_stdwstr(&type_param).c_str());
        }
      }
    }
  }
}
