#ifndef _FAREDITOR_H_
#define _FAREDITOR_H_

#include <colorer/editor/BaseEditor.h>
#include <colorer/editor/Outliner.h>
#include <colorer/handlers/StyledRegion.h>
#include "pcolorer.h"

const intptr_t CurrentEditor = -1;
extern const CString DDefaultScheme;
extern const CString DShowCross;
extern const CString DNone;
extern const CString DVertical;
extern const CString DHorizontal;
extern const CString DBoth;
extern const CString DCrossZorder;
extern const CString DBottom;
extern const CString DTop;
extern const CString DYes;
extern const CString DNo;
extern const CString DTrue;
extern const CString DFalse;
extern const CString DBackparse;
extern const CString DMaxLen;
extern const CString DDefFore;
extern const CString DDefBack;
extern const CString DFullback;
extern const CString DHotkey;
extern const CString DFavorite;
extern const CString DMaxblocksize;

#define revertRGB(x) (BYTE)(x >> 16 & 0xff) | ((BYTE)(x >> 8 & 0xff) << 8) | ((BYTE)(x & 0xff) << 16)

/** FAR Editor internal plugin structures.
    Implements text parsing and different
    editor extended functions.
    @ingroup far_plugin
*/
class FarEditor : public LineSource
{
 public:
  enum CROSS_STATUS { CROSS_OFF = 0, CROSS_ON = 1, CROSS_INSCHEME = 2 };
  enum CROSS_STYLE { CSTYLE_NONE = 0, CSTYLE_VERT = 1, CSTYLE_HOR = 2, CSTYLE_BOTH = 3 };

  /** Creates FAR editor instance.
   */
  FarEditor(PluginStartupInfo* info, ParserFactory* pf);
  /** Drops this editor */
  ~FarEditor() override;

  void endJob(int lno);
  /**
  Returns line number "lno" from FAR interface. Line is only valid until next call of this function,
  it also should not be disposed, this function takes care of this.
  */
  SString* getLine(size_t lno) override;

  /** Changes current assigned file type.
   */
  void setFileType(FileType* ftype);
  /** Returns currently selected file type.
   */
  FileType* getFileType() const;

  /** Selects file type with it's extension and first lines
   */
  void chooseFileType(String* fname);

  /** Installs specified RegionMapper implementation.
  This class serves to request mapping of regions into
  real colors.
  */
  void setRegionMapper(RegionMapper* rs);

  /**
   * Change editor properties. These overwrites default HRC settings
   */
  void setCrossState(int status, int style);
  void setCrossStatus(int status);
  void setCrossStyle(int style);
  void setDrawPairs(bool drawPairs);
  void setDrawSyntax(bool drawSyntax);
  void setOutlineStyle(bool oldStyle);
  void setTrueMod(bool _TrueMod);

  /** Editor action: pair matching.
   */
  void matchPair();
  /** Editor action: pair selection.
   */
  void selectPair();
  /** Editor action: pair selection with current block.
   */
  void selectBlock();
  /** Editor action: Selection of current region under cursor.
   */
  void selectRegion();
  /** Editor action: Lists fuctional region.
   */
  void listFunctions();
  /** Editor action: Lists syntax errors in text.
   */
  void listErrors();
  /**
   * Locates a function under cursor and tries to jump to it using outliner information
   */
  void locateFunction();

  /** Invalidates current syntax highlighting
   */
  void updateHighlighting();

  /** Handle passed FAR editor event */
  int editorEvent(intptr_t event, void* param);
  /** Dispatch editor input event */
  int editorInput(const INPUT_RECORD& Rec);

  void cleanEditor();

  void getNameCurrentScheme();
  void getCurrentRegionInfo(SString& region, SString& scheme);

  void changeCrossStyle(CROSS_STYLE newStyle);
  int getVisibleCrossState() const;
  int getCrossStatus() const;
  int getCrossStyle() const;
  bool isDrawPairs() const;
  bool isDrawSyntax() const;

  Outliner* getFunctionOutliner();
  Outliner* getErrorOutliner();

  int getParseProgress();

 private:
  PluginStartupInfo* info;

  ParserFactory* parserFactory;
  std::unique_ptr<BaseEditor> baseEditor;

  int maxLineLength = 0;
  bool fullBackground = true;

  int crossStatus = 0;  // 0 - off,  1 - always, 2 - if included in the scheme
  int crossStyle = 0;
  // 3 - both; 1 - vertical; 2 - horizontal
  bool showVerticalCross = false;
  bool showHorizontalCross = false;
  int crossZOrder = 0;
  FarColor horzCrossColor;
  FarColor vertCrossColor;

  bool drawPairs = true;
  bool drawSyntax = true;
  bool oldOutline = false;
  bool TrueMod = true;

  int WindowSizeX = 0;
  int WindowSizeY = 0;
  bool inRedraw = false;
  int idleCount = 0;

  int prevLinePosition = 0;
  int blockTopPosition = -1;

  std::unique_ptr<SString> ret_str;

  int newfore = -1;
  int newback = -1;
  const StyledRegion* rdBackground = nullptr;
  std::unique_ptr<LineRegion> cursorRegion;

  int visibleLevel = 100;
  std::unique_ptr<Outliner> structOutliner;
  std::unique_ptr<Outliner> errorOutliner;
  intptr_t editor_id = -1;

  void reloadTypeSettings();
  EditorInfo enterHandler();
  FarColor convert(const StyledRegion* rd) const;
  bool foreDefault(const FarColor& col) const;
  bool backDefault(const FarColor& col) const;
  void showOutliner(Outliner* outliner);
  void addFARColor(intptr_t lno, intptr_t s, intptr_t e, const FarColor& col, EDITORCOLORFLAGS TabMarkStyle = 0) const;
  void deleteFarColor(intptr_t lno, intptr_t s) const;
  const wchar_t* GetMsg(int msg) const;
  static COLORREF getSuitableColor(const COLORREF base_color, const COLORREF blend_color);
};
#endif
