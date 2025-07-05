#ifndef FARCOLORER_PCOLORER_H
#define FARCOLORER_PCOLORER_H

#include <initguid.h>
#include <plugin.hpp>
// Dialog Guid
// {D2F36B62-A470-418d-83A3-ED7A3710E5B5}
DEFINE_GUID(MainGuid, 0xd2f36b62, 0xa470, 0x418d, 0x83, 0xa3, 0xed, 0x7a, 0x37, 0x10, 0xe5, 0xb5);
// {87C92249-430D-4334-AC33-05E7423286E9}
DEFINE_GUID(PluginConfig, 0x87c92249, 0x430d, 0x4334, 0xac, 0x33, 0x5, 0xe7, 0x42, 0x32, 0x86, 0xe9);
// {0497F43A-A8B9-4af1-A3A4-FA568F455707}
DEFINE_GUID(HrcPluginConfig, 0x497f43a, 0xa8b9, 0x4af1, 0xa3, 0xa4, 0xfa, 0x56, 0x8f, 0x45, 0x57, 0x7);
// {C6BE56D8-A80A-4f7d-A331-A711435F2665}
DEFINE_GUID(AssignKeyDlg, 0xc6be56d8, 0xa80a, 0x4f7d, 0xa3, 0x31, 0xa7, 0x11, 0x43, 0x5f, 0x26, 0x65);
// {3D1031EA-B67A-451C-9FC6-081320D3A139}
DEFINE_GUID(LoggingConfig, 0x3d1031ea, 0xb67a, 0x451c, 0x9f, 0xc6, 0x8, 0x13, 0x20, 0xd3, 0xa1, 0x39);

// Menu Guid
// {45453CAC-499D-4b37-82B8-0A77F7BD087C}
DEFINE_GUID(PluginMenu, 0x45453cac, 0x499d, 0x4b37, 0x82, 0xb8, 0xa, 0x77, 0xf7, 0xbd, 0x8, 0x7c);
// {46921647-DB52-44CA-8D8B-F34EA8B02E5D}
DEFINE_GUID(FileChooseMenu, 0x46921647, 0xdb52, 0x44ca, 0x8d, 0x8b, 0xf3, 0x4e, 0xa8, 0xb0, 0x2e, 0x5d);
// {18A6F7DF-375D-4d3d-8137-DC50AC52B71E}
DEFINE_GUID(HrdMenu, 0x18a6f7df, 0x375d, 0x4d3d, 0x81, 0x37, 0xdc, 0x50, 0xac, 0x52, 0xb7, 0x1e);
// {A8A298BA-AD5A-4094-8E24-F65BF38E6C1F}
DEFINE_GUID(OutlinerMenu, 0xa8a298ba, 0xad5a, 0x4094, 0x8e, 0x24, 0xf6, 0x5b, 0xf3, 0x8e, 0x6c, 0x1f);
// {63E396BA-8E7F-4E38-A7A8-CBB7E9AC1E6D}
DEFINE_GUID(ConfigMenu, 0x63e396ba, 0x8e7f, 0x4e38, 0xa7, 0xa8, 0xcb, 0xb7, 0xe9, 0xac, 0x1e, 0x6d);

// Message Guid
// {0C954AC8-2B69-4c74-94C8-7AB10324A005}
DEFINE_GUID(ErrorMessage, 0xc954ac8, 0x2b69, 0x4c74, 0x94, 0xc8, 0x7a, 0xb1, 0x3, 0x24, 0xa0, 0x5);
// {DEE3B49D-4A55-48a8-9DC8-D11DA04CBF37}
DEFINE_GUID(ReloadBaseMessage, 0xdee3b49d, 0x4a55, 0x48a8, 0x9d, 0xc8, 0xd1, 0x1d, 0xa0, 0x4c, 0xbf, 0x37);
// {AB214DCE-450B-4389-9E3B-533C7A6D786C}
DEFINE_GUID(NothingFoundMesage, 0xab214dce, 0x450b, 0x4389, 0x9e, 0x3b, 0x53, 0x3c, 0x7a, 0x6d, 0x78, 0x6c);
// {70656884-B7BD-4440-A8FF-6CE781C7DC6A}
DEFINE_GUID(RegionName, 0x70656884, 0xb7bd, 0x4440, 0xa8, 0xff, 0x6c, 0xe7, 0x81, 0xc7, 0xdc, 0x6a);

extern PluginStartupInfo Info;
extern FarStandardFunctions FSF;

/** FAR .lng file identifiers. */
enum {
  mName,
  mSetup,
  mTurnOff,
  mTrueMod,
  mPairs,
  mSyntax,
  mOldOutline,
  mOk,
  mReloadAll,
  mCancel,
  mCatalogFile,
  mHRDName,
  mHRDNameTrueMod,
  mListTypes,
  mMatchPair,
  mSelectBlock,
  mSelectPair,
  mListFunctions,
  mFindErrors,
  mSelectRegion,
  mCurrentRegionName,
  mLocateFunction,
  mUpdateHighlight,
  mReloadBase,
  mConfigure,
  mTotalTypes,
  mSelectSyntax,
  mOutliner,
  mNothingFound,
  mGotcha,
  mChoose,
  mReloading,
  mDie,
  mChangeBackgroundEditor,
  mUserHrdFile,
  mUserHrcFile,
  mUserHrcSettings,
  mUserHrcSetting,
  mUserHrcSettingDialog,
  mListSyntax,
  mParamList,
  mParamValue,
  mAutoDetect,
  mFavorites,
  mDisable,
  mKeyAssignDialogTitle,
  mKeyAssignTextTitle,
  mRegionName,
  mLog,
  mLogging,
  mLogTurnOff,
  mLogLevel,
  mLogPath,
  mMainSettings,
  mSettings,
  mStyleConf,
  mCrossText,
  mCrossBoth,
  mCrossVert,
  mCrossHoriz,
  mCross,
  mPerfomance,
  mBuildPeriod
};

#endif // FARCOLORER_PCOLORER_H
