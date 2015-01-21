#include"FarHrcSettings.h"
#include <xml/XmlParserErrorHandler.h>

void FarHrcSettings::readProfile()
{
  StringBuffer *path=new StringBuffer(PluginPath);
  path->append(DString(FarProfileXml));
  readXML(path, false);

  delete path;
}

void FarHrcSettings::readXML(String *file, bool userValue)
{
  xercesc::XercesDOMParser xml_parser;
  XmlParserErrorHandler error_handler(null);
  xml_parser.setErrorHandler(&error_handler);
  xml_parser.setLoadExternalDTD(false);
  xml_parser.setSkipDTDValidation(true);
  XmlInputSource* config = XmlInputSource::newInstance(file->getWChars(), (XMLCh*)nullptr);
  xml_parser.parse(*config->getInputSource());
  if (error_handler.getSawErrors()) {
    delete config;
    throw ParserFactoryException(DString("Error reading hrcsettings.xml."));
  }
  xercesc::DOMDocument *catalog = xml_parser.getDocument();
  xercesc::DOMElement *elem = catalog->getDocumentElement();

  const XMLCh *tagPrototype = L"prototype";
  const XMLCh *tagHrcSettings = L"hrc-settings";

  if (elem == null || !xercesc::XMLString::equals(elem->getNodeName(), tagHrcSettings)) {
    delete config;
    throw FarHrcSettingsException(DString("main '<hrc-settings>' block not found"));
  }
  for (xercesc::DOMNode *node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      xercesc::DOMElement *subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), tagPrototype)) {
        UpdatePrototype(subelem, userValue);
      }
    };
  }
  delete config;
}

void FarHrcSettings::UpdatePrototype(xercesc::DOMElement *elem, bool userValue)
{
  const XMLCh *tagProtoAttrParamName = L"name";
  const XMLCh *tagParam = L"param";
  const XMLCh *tagParamAttrParamName = L"name";
  const XMLCh *tagParamAttrParamValue = L"value";
  const XMLCh *tagParamAttrParamDescription = L"description";
  const XMLCh *typeName = elem->getAttribute(tagProtoAttrParamName);
  if (typeName == null){
    return;
  }
  HRCParser *hrcParser = parserFactory->getHRCParser();
  FileTypeImpl *type = static_cast<FileTypeImpl *>(hrcParser->getFileType(&DString(typeName)));
  if (type== null){
    return;
  };

  for (xercesc::DOMNode *node = elem->getFirstChild(); node != nullptr; node = node->getNextSibling()) {
    if (node->getNodeType() == xercesc::DOMNode::ELEMENT_NODE) {
      xercesc::DOMElement *subelem = static_cast<xercesc::DOMElement*>(node);
      if (xercesc::XMLString::equals(subelem->getNodeName(), tagParam)) {
        const XMLCh *name = subelem->getAttribute(tagParamAttrParamName);
        const XMLCh *value = subelem->getAttribute(tagParamAttrParamValue);
        const XMLCh *descr = subelem->getAttribute(tagParamAttrParamDescription);

        if (*name == '\0' || *value == '\0'){
          continue;
        }

        if (type->getParamValue(DString(name))==null){
          type->addParam(new SString(DString(name)));
        }
        if (descr != null){
          type->setParamDescription(DString(name), new SString(DString(descr)));
        }
        if (userValue){
          type->setParamValue(DString(name), new SString(DString(value)));
        }
        else{
          delete type->getParamDefaultValue(DString(name));
          type->setParamDefaultValue(DString(name), new SString(DString(value)));
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

    if (type->getParamCount() && type->getParamUserValueCount()){// params>0 and user values >0
      size_t type_subkey = ColorerSettings.rGetSubKey(hrc_subkey,type->getName()->getWChars());
      // enum all params
      std::vector<SString> type_params = type->enumParams();
      for (auto paramname=type_params.begin(); paramname!=type_params.end(); ++paramname){      
        const String *v = type->getParamUserValue(*paramname);
        if (v != NULL){
          ColorerSettings.Set(type_subkey, paramname->getWChars(), v->getWChars());
        }
      }
    }
  }

}
