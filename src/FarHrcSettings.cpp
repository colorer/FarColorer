#include "FarHrcSettings.h"
#include <colorer/base/XmlTagDefs.h>
#include <colorer/xml/XmlReader.h>
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

  XmlInputSource config(*filename);
  XmlReader xml_parser(config);
  if (!xml_parser.parse()) {
    throw ParserFactoryException(UnicodeString("Error reading ").append(*filename));
  }
  std::list<XMLNode> nodes;
  xml_parser.getNodes(nodes);

  if (nodes.begin()->name !=  catTagHrdSets) {
    throw Exception("main '<hrd-sets>' block not found");
  }
  for (const auto& node : nodes.begin()->children) {
    if (node.name == catTagHrd) {
      auto hrd = CatalogParser::parseHRDSetsChild(node);
      if (hrd)
        parserFactory->addHrd(std::move(hrd));
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
  XmlInputSource config(*file);
  XmlReader xml_parser(config);
  if (!xml_parser.parse()) {
    throw ParserFactoryException("Error reading hrcsettings.xml.");
  }
  std::list<XMLNode> nodes;
  xml_parser.getNodes(nodes);

  if (nodes.begin()->name !=  u"hrc-settings") {
    throw FarHrcSettingsException("main '<hrc-settings>' block not found");
  }
  for (const auto& node : nodes.begin()->children) {
    if (node.name == hrcTagPrototype) {
      UpdatePrototype(node);
    }
  }
}

void FarHrcSettings::UpdatePrototype(const XMLNode& elem)
{
  const auto& typeName = elem.getAttrValue(hrcPrototypeAttrName);
  if (typeName.isEmpty()) {
    return;
  }
  auto& hrcLibrary = parserFactory->getHrcLibrary();
  auto* type = hrcLibrary.getFileType(typeName);
  if (type == nullptr) {
    return;
  }

  for (const auto& node : elem.children) {
    if (node.name == hrcTagParam) {
      const auto& name = node.getAttrValue(hrcParamAttrName);
      const auto& value = node.getAttrValue(hrcParamAttrValue);
      const auto& descr = node.getAttrValue(hrcParamAttrDescription);

      if (name.isEmpty()) {
        continue;
      }

      if (type->getParamValue(name) == nullptr) {
        type->addParam(name, value);
      }
      else {
        type->setParamDefaultValue(name, &value);
      }
      if (descr != nullptr) {
        type->setParamDescription(name, &descr);
      }
    }
  }
}

void FarHrcSettings::readUserProfile(const FileType* def_filetype)
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
                  farEditorSet->addParamAndValue(type, name_fse, dp, def_filetype);
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
