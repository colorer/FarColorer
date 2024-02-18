#include "pcolorer.h"
#include "FarEditorSet.h"
#include "version.h"

FarEditorSet* editorSet = nullptr;
bool inEventProcess = false;
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
  gInfo->Version = MAKEFARVERSION(PLUGIN_VER_MAJOR, PLUGIN_VER_MINOR, PLUGIN_VER_PATCH, 0, VS_RELEASE);
  gInfo->Guid = MainGuid;
  gInfo->Title = PLUGIN_NAME;
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
void WINAPI ExitFARW(const struct ExitInfo* /*eInfo*/)
{
  delete editorSet;
}

/**
  Open plugin configuration of actions dialog.
*/
HANDLE WINAPI OpenW(const struct OpenInfo* oInfo)
{
  HANDLE result = nullptr;
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
      result = editorSet->openFromMacro(oInfo);
    case OPEN_LEFTDISKMENU:
    case OPEN_PLUGINSMENU:
    case OPEN_FINDLIST:
    case OPEN_SHORTCUT:
    case OPEN_VIEWER:
    case OPEN_FILEPANEL:
    case OPEN_DIALOG:
    case OPEN_ANALYSE:
    case OPEN_RIGHTDISKMENU:
    case OPEN_LUAMACRO:
      break;
  }

  spdlog::default_logger()->flush();
  return result;
}

/**
  Configures plugin.
*/
intptr_t WINAPI ConfigureW(const struct ConfigureInfo* /*cInfo*/)
{
  if (!editorSet) {
    editorSet = new FarEditorSet();
  }
  editorSet->menuConfigure();
  spdlog::default_logger()->flush();
  return 1;
}

/**
  Processes FAR editor events and
  makes text colorizing here.
*/
intptr_t WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo* pInfo)
{
  if (inEventProcess) {
    return 0;
  }

  inEventProcess = true;

  if (!editorSet) {
    editorSet = new FarEditorSet();
  }

  int result = editorSet->editorEvent(pInfo);

  inEventProcess = false;
  spdlog::default_logger()->flush();
  return result;
}

intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo* pInfo)
{
  if (inEventProcess) {
    return 0;
  }

  inEventProcess = true;
  if (!editorSet) {
    editorSet = new FarEditorSet();
  }

  int result = editorSet->editorInput(pInfo->Rec);

  inEventProcess = false;
  spdlog::default_logger()->flush();
  return result;
}

intptr_t WINAPI ProcessSynchroEventW(const ProcessSynchroEventInfo* pInfo)
{
  if (pInfo->Event != SE_COMMONSYNCHRO) {
    return 0;
  }
  try {
    if (editorSet && editorSet->getEditorCount() > 0) {
      INPUT_RECORD ir;
      ir.EventType = KEY_EVENT;
      ir.Event.KeyEvent.wVirtualKeyCode = 0;
      int result = editorSet->editorInput(ir);
      spdlog::default_logger()->flush();
      return result;
    }
  } catch (...) {
  }

  return 0;
}
