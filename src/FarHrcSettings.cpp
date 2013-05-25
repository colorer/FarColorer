#include"FarHrcSettings.h"

void FarHrcSettings::readProfile()
{
  StringBuffer *path=new StringBuffer(PluginPath);
  path->append(DString(FarProfileXml));
  readXML(path, false);

  delete path;
}

void FarHrcSettings::readXML(String *file, bool userValue)
{
  DocumentBuilder docbuilder;
  colorer::InputSource *dfis = colorer::InputSource::newInstance(file);
  Document *xmlDocument = docbuilder.parse(dfis);
  Element *types = xmlDocument->getDocumentElement();

  if (*types->getNodeName() != "hrc-settings"){
    docbuilder.free(xmlDocument);
    throw FarHrcSettingsException(DString("main '<hrc-settings>' block not found"));
  }
  for (Node *elem = types->getFirstChild(); elem; elem = elem->getNextSibling()){
    if (*elem->getNodeName() == "prototype"){
      UpdatePrototype((Element*)elem, userValue);
      continue;
    }
  };
  docbuilder.free(xmlDocument);
  delete dfis;
}

void FarHrcSettings::UpdatePrototype(Element *elem, bool userValue)
{
  const String *typeName = elem->getAttribute(DString("name"));
  if (typeName == null){
    return;
  }
  HRCParser *hrcParser = parserFactory->getHRCParser();
  FileTypeImpl *type = static_cast<FileTypeImpl *>(hrcParser->getFileType(typeName));
  if (type== null){
    return;
  };
  for(Node *content = elem->getFirstChild(); content != null; content = content->getNextSibling()){
    if (*content->getNodeName() == "param"){
      const String *name = ((Element*)content)->getAttribute(DString("name"));
      const String *value = ((Element*)content)->getAttribute(DString("value"));
      const String *descr = ((Element*)content)->getAttribute(DString("description"));
      if (name == null || value == null){
        continue;
      };

      if (type->getParamValue(SString(name))==null){
        type->addParam(name);
      }
      if (descr != null){
        type->setParamDescription(SString(name), descr);
      }
      if (userValue){
        delete type->getParamNotDefaultValue(DString(name));
        type->setParamValue(SString(name), value);
      }
      else{
        delete type->getParamDefaultValue(DString(name));
        type->setParamDefaultValue(SString(name), value);
      }
    };
  };
}

void FarHrcSettings::readUserProfile()
{
  readProfileFromRegistry();
}

void FarHrcSettings::readProfileFromRegistry()
{
  HRCParser *hrcParser = parserFactory->getHRCParser();
  
  SettingsControl ColorerSettings;
  size_t hrc_subkey;
  hrc_subkey = ColorerSettings.rGetSubKey(0,HrcSettings);
  FarSettingsEnum fse;
  fse.StructSize = sizeof(FarSettingsEnum);
  // enum all the sections in HrcSettings
  if (ColorerSettings.rEnum(hrc_subkey,&fse)){
    for (size_t i=0; i<fse.Count; i++){
      if (fse.Items[i].Type == FST_SUBKEY){
        //check whether we have such a scheme
        FileTypeImpl *type = static_cast<FileTypeImpl *>(hrcParser->getFileType(&DString(fse.Items[i].Name)));
        if (type){
          // enum all params in the section
          size_t type_subkey;
          type_subkey = ColorerSettings.rGetSubKey(hrc_subkey,fse.Items[i].Name);
          FarSettingsEnum type_fse;
          type_fse.StructSize = sizeof(FarSettingsEnum);
          if (ColorerSettings.rEnum(type_subkey,&type_fse)){
            for (size_t j=0; j<type_fse.Count; j++){
              if (type_fse.Items[j].Type == FST_STRING){
                if (type->getParamValue(DString(type_fse.Items[j].Name))==null){
                  type->addParam(&DString(type_fse.Items[j].Name));
                }
				const wchar_t *p = ColorerSettings.Get(type_subkey,type_fse.Items[j].Name,(wchar_t*)NULL);
				if (p) {
					delete type->getParamNotDefaultValue(DString(type_fse.Items[j].Name));
					type->setParamValue(DString(type_fse.Items[j].Name), &DString(p));
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
  HRCParser *hrcParser = parserFactory->getHRCParser();
  FileTypeImpl *type = NULL;

  SettingsControl ColorerSettings;
  ColorerSettings.rDeleteSubKey(0,HrcSettings);
  size_t hrc_subkey;
  hrc_subkey = ColorerSettings.rGetSubKey(0,HrcSettings);

  // enum all FileTypes
  for (int idx = 0; ; idx++){
    type =static_cast<FileTypeImpl *>(hrcParser->enumerateFileTypes(idx));

    if (!type){
      break;
    }

    if (type->getParamCount() && type->getParamNotDefaultValueCount()){// params>0 and user values >0
      size_t type_subkey = ColorerSettings.rGetSubKey(hrc_subkey,type->getName()->getWChars());
      // enum all params
      for (int i=0;;i++){
        const String *p=type->enumerateParameters(i);
        if (!p){
          break;
        }
        const String *v=type->getParamNotDefaultValue(*p);
        if (v!=NULL){
          ColorerSettings.Set(type_subkey,p->getWChars(),v->getWChars());
        }
      }
    }
  }

}
