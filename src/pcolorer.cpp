#include"pcolorer.h"
#include"tools.h"
#include"FarEditorSet.h"
#include "version.h"

FarEditorSet *editorSet = NULL;
PluginStartupInfo Info;
FarStandardFunctions FSF;
StringBuffer *PluginPath;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )  
{
  switch( fdwReason ) 
  { 
  case DLL_PROCESS_ATTACH:
    {
      // obtain the path to the folder plugin, without being attached to the file name
      wchar_t path[MAX_PATH];
      if (!GetModuleFileName(hinstDLL, path, MAX_PATH)){
        return false;
      }
      DString module(path, 0);
      int pos = module.lastIndexOf('\\');
      pos = module.lastIndexOf('\\',pos);
      PluginPath = new StringBuffer(DString(module, 0, pos));
    }
    break;

  case DLL_PROCESS_DETACH:
    delete PluginPath;
    break;
  }

  return true;  
}
/**
  Returns message from FAR current language.
*/
const wchar_t *GetMsg(int msg)
{
  return(Info.GetMsg(&MainGuid, msg));
};

/**
  Global information about the plugin
*/
void WINAPI GetGlobalInfoW(struct GlobalInfo *gInfo)
{
  gInfo->StructSize = sizeof(GlobalInfo);
  gInfo->MinFarVersion = MAKEFARVERSION(3,0,0,3371,VS_RELEASE);
  gInfo->Version = MAKEFARVERSION(VER_FILEVERSION,VS_RELEASE);
  gInfo->Guid = MainGuid;
  gInfo->Title = L"FarColorer";
  gInfo->Description =L"Syntax highlighting in Far editor";
  gInfo->Author = L"Igor Ruskih, Dobrunov Aleksey, Eugene Efremov";
}

/**
  Plugin initialization and creation of editor set support class.
*/
void WINAPI SetStartupInfoW(const struct PluginStartupInfo *fei)
{
  Info = *fei;
  FSF = *fei->FSF;
  Info.FSF = &FSF;
};

/**
  Plugin strings in FAR interface.
*/
void WINAPI GetPluginInfoW(struct PluginInfo *pInfo)
{
  static wchar_t* PluginMenuStrings;
  memset(pInfo, 0, sizeof(*pInfo));
  pInfo->Flags = PF_EDITOR | PF_DISABLEPANELS;
  pInfo->StructSize = sizeof(*pInfo);
  pInfo->PluginConfig.Count = 1;
  pInfo->PluginMenu.Count = 1;
  PluginMenuStrings = (wchar_t*)GetMsg(mName);
  pInfo->PluginConfig.Strings = &PluginMenuStrings;
  pInfo->PluginMenu.Strings = &PluginMenuStrings;
  pInfo->PluginConfig.Guids = &PluginConfig;
  pInfo->PluginMenu.Guids = &PluginMenu;
  pInfo->CommandPrefix = L"clr";
};

/**
  On FAR exit. Destroys all internal structures.
*/
void WINAPI ExitFARW(const struct ExitInfo *eInfo)
{
  if (editorSet){
    delete editorSet;
  }
};

/**
  Open plugin configuration of actions dialog.
*/
HANDLE WINAPI OpenW(const struct OpenInfo *oInfo)
{
  switch (oInfo->OpenFrom) {
    case OPEN_EDITOR:
      editorSet->openMenu();
      break;
    case OPEN_COMMANDLINE:
      {
		OpenCommandLineInfo *ocli =(OpenCommandLineInfo*)oInfo->Data;
        //file name, which we received
        const wchar_t *file = ocli->CommandLine;

        wchar_t *nfile = PathToFull(file,true);
        if (nfile){
          if (!editorSet){
            editorSet = new FarEditorSet();
          }
          editorSet->viewFile(DString(nfile));
        }

        delete[] nfile;
      }
      break;
    case OPEN_FROMMACRO:
      {
        intptr_t i= Info.MacroControl(&MainGuid, MCTL_GETAREA, 0, NULL);
        OpenMacroInfo* mi=(OpenMacroInfo*)oInfo->Data;
        int MenuCode=-1;
        if (mi->Count)
        {
          switch (mi->Values[0].Type) {
          case FMVT_INTEGER: MenuCode=(int)mi->Values[0].Integer; break;
          case FMVT_DOUBLE: MenuCode=(int)mi->Values[0].Double; break;
          default: MenuCode = -1;
          }
        }

        if (MenuCode < 0)
          return INVALID_HANDLE_VALUE;
        editorSet->openMenu(MenuCode);
      }
      break;
  }
  return INVALID_HANDLE_VALUE;
};

/**
  Configures plugin.
*/
intptr_t WINAPI ConfigureW(const struct ConfigureInfo *cInfo)
{
  if (!editorSet){
    editorSet = new FarEditorSet();
  }
  editorSet->configure(false);
  return 1;
};

/**
  Processes FAR editor events and
  makes text colorizing here.
*/
intptr_t WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo *pInfo)
{
  if (!editorSet){
    editorSet = new FarEditorSet();
  }
  return editorSet->editorEvent(pInfo);
};

intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *pInfo)
{
  return editorSet->editorInput(pInfo->Rec);
}

// in order to not fall when it starts in far2 
int WINAPI GetMinFarVersionW(void)
{
  #define MAKEFARVERSION_OLD(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))
  
  return MAKEFARVERSION_OLD(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD);
}


/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */