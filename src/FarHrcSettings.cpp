#include "FarHrcSettings.h"
#include "SettingsControl.h"
#include "colorer/parsers/CatalogParser.h"

void FarHrcSettings::readPluginHrcSettings(const UnicodeString* plugin_path)
{
  auto path = UnicodeString(*plugin_path);
  path.append(UnicodeString(FarProfileXml));
  parserFactory->loadHrcSettings(&path,false);
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
