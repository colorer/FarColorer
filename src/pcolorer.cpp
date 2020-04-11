#include "pcolorer.h"
#include "FarEditorSet.h"
#include "tools.h"
#include "version.h"

FarEditorSet* editorSet = nullptr;
bool inCreateEditorSet = false;
PluginStartupInfo Info;
FarStandardFunctions FSF;

/**
  Returns message from FAR current language.
*/
const wchar_t* GetMsg(int msg)
{
  return Info.GetMsg(&MainGuid, msg);
}

/**
  Global information about the plugin
*/
void WINAPI GetGlobalInfoW(struct GlobalInfo* gInfo)
{
  gInfo->StructSize = sizeof(GlobalInfo);
  gInfo->MinFarVersion = MAKEFARVERSION(3, 0, 0, 3371, VS_RELEASE);
  gInfo->Version = MAKEFARVERSION(VER_FILEVERSION, VS_RELEASE);
  gInfo->Guid = MainGuid;
  gInfo->Title = L"FarColorer";
  gInfo->Description = L"Syntax highlighting in Far editor";
  gInfo->Author = L"Igor Ruskih, Dobrunov Aleksey, Eugene Efremov";
}

/**
  Plugin initialization and creation of editor set support class.
*/
void WINAPI SetStartupInfoW(const struct PluginStartupInfo* fei)
{
  Info = *fei;
  FSF = *fei->FSF;
  Info.FSF = &FSF;
}

/**
  Plugin strings in FAR interface.
*/
void WINAPI GetPluginInfoW(struct PluginInfo* pInfo)
{
  static wchar_t* PluginMenuStrings;
  memset(pInfo, 0, sizeof(*pInfo));
  pInfo->Flags = PF_EDITOR | PF_DISABLEPANELS;
  pInfo->StructSize = sizeof(*pInfo);
  pInfo->PluginConfig.Count = 1;
  pInfo->PluginMenu.Count = 1;
  PluginMenuStrings = (wchar_t*) GetMsg(mName);
  pInfo->PluginConfig.Strings = &PluginMenuStrings;
  pInfo->PluginMenu.Strings = &PluginMenuStrings;
  pInfo->PluginConfig.Guids = &PluginConfig;
  pInfo->PluginMenu.Guids = &PluginMenu;
  pInfo->CommandPrefix = L"clr";
}

/**
  On FAR exit. Destroys all internal structures.
*/
void WINAPI ExitFARW(const struct ExitInfo* eInfo)
{
  delete editorSet;
}

/**
  Open plugin configuration of actions dialog.
*/
HANDLE WINAPI OpenW(const struct OpenInfo* oInfo)
{
  if (!editorSet) {
    editorSet = new FarEditorSet();
  }

  switch (oInfo->OpenFrom) {
    case OPEN_EDITOR:
      editorSet->openMenu();
      break;
    case OPEN_COMMANDLINE:
      editorSet->openFromCommandLine(oInfo);
      break;
    case OPEN_FROMMACRO:
      editorSet->openFromMacro(oInfo);
      break;
    case OPEN_LEFTDISKMENU:
      break;
    case OPEN_PLUGINSMENU:
      break;
    case OPEN_FINDLIST:
      break;
    case OPEN_SHORTCUT:
      break;
    case OPEN_VIEWER:
      break;
    case OPEN_FILEPANEL:
      break;
    case OPEN_DIALOG:
      break;
    case OPEN_ANALYSE:
      break;
    case OPEN_RIGHTDISKMENU:
      break;
    case OPEN_LUAMACRO:
      break;
  }
  return editorSet;
}

/**
  Configures plugin.
*/
intptr_t WINAPI ConfigureW(const struct ConfigureInfo* cInfo)
{
  if (!editorSet) {
    editorSet = new FarEditorSet();
  }
  editorSet->menuConfigure();
  return 1;
}

/**
  Processes FAR editor events and
  makes text colorizing here.
*/
intptr_t WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo* pInfo)
{
  if (!inCreateEditorSet) {
    if (!editorSet) {
      inCreateEditorSet = true;
      editorSet = new FarEditorSet();
      inCreateEditorSet = false;  //-V519

      // при создании FarEditorSet мы теряем сообщение EE_REDRAW, из-за SetBgEditor. компенсируем это
      ProcessEditorEventInfo pInfo2 {};
      pInfo2.EditorID = pInfo->EditorID;
      pInfo2.Event = EE_REDRAW;
      pInfo2.StructSize = sizeof(ProcessEditorEventInfo);
      pInfo2.Param = pInfo->Param;
      return editorSet->editorEvent(&pInfo2);
    } else {
      return editorSet->editorEvent(pInfo);
    }
  } else {
    return 0;
  }
}

intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo* pInfo)
{
  return editorSet->editorInput(pInfo->Rec);
}

intptr_t WINAPI ProcessSynchroEventW(const ProcessSynchroEventInfo* pInfo)
{
  try {
    if (editorSet && editorSet->getEditorCount() > 0) {
      INPUT_RECORD ir;
      ir.EventType = KEY_EVENT;
      ir.Event.KeyEvent.wVirtualKeyCode = 0;
      return editorSet->editorInput(ir);
    }
  } catch (...) {
  }
  return 0;
}
