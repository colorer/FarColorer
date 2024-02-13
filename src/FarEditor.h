#ifndef _FAREDITOR_H_
#define _FAREDITOR_H_

#include <colorer/editor/BaseEditor.h>
#include <colorer/editor/Outliner.h>
#include <colorer/handlers/StyledRegion.h>
#include "const_strings.h"
#include "pcolorer.h"

const intptr_t CurrentEditor = -1;

#define revertRGB(x) ((BYTE) ((x) >> 16 & 0xff) | ((BYTE) ((x) >> 8 & 0xff) << 8) | ((BYTE) ((x) &0xff) << 16))

/** FAR Editor internal plugin structures.
    Implements text parsing and different
    editor extended functions.
    @ingroup far_plugin
*/
class FarEditor : public LineSource
{
 public:
  enum class CROSS_STATUS { CROSS_OFF = 0, CROSS_ON = 1, CROSS_INSCHEME = 2 };
  enum class CROSS_STYLE { CSTYLE_NONE = 0, CSTYLE_VERT = 1, CSTYLE_HOR = 2, CSTYLE_BOTH = 3 };

  /** Creates FAR editor instance.
   */
  FarEditor(PluginStartupInfo* info, ParserFactory* pf, bool editorEnabled);
  /** Drops this editor */
  ~FarEditor() override;

  void endJob(size_t lno) override;
  /**
  Returns line number "lno" from FAR interface. Line is only valid until next call of this function,
  it also should not be disposed, this function takes care of this.
  */
  UnicodeString* getLine(size_t lno) override;

  /** Changes current assigned file type.
   */
  void setFileType(FileType* ftype);
  /** Returns currently selected file type.
   */
  [[nodiscard]] FileType* getFileType() const;

  /** Selects file type with it's extension and first lines
   */
  void chooseFileType(UnicodeString* fname);

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
  void setTrueMod(bool TrueMod_);

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
  /** Editor action: Lists functional region.
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
  void getCurrentRegionInfo(UnicodeString& region, UnicodeString& scheme);

  void changeCrossStyle(CROSS_STYLE newStyle);
  [[nodiscard]] CROSS_STYLE getVisibleCrossState() const;
  [[nodiscard]] int getCrossStatus() const;
  [[nodiscard]] int getCrossStyle() const;
  [[nodiscard]] bool isDrawPairs() const;
  [[nodiscard]] bool isDrawSyntax() const;

  Outliner* getFunctionOutliner();
  Outliner* getErrorOutliner();

  int getParseProgress();
  [[nodiscard]] bool isColorerEnable() const;
  boolean hasWork();

 private:
  PluginStartupInfo* info;

  ParserFactory* parserFactory;
  std::unique_ptr<BaseEditor> baseEditor;

  bool colorerEnable = true;
  int maxLineLength = 0;
  bool fullBackground = true;

  int crossStatus = 0;  // 0 - off,  1 - always, 2 - if included in the scheme
  int crossStyle = 0;
  // 3 - both; 1 - vertical; 2 - horizontal
  bool showVerticalCross = false;
  bool showHorizontalCross = false;
  int crossZOrder = 0;
  FarColor horzCrossColor{};
  FarColor vertCrossColor{};

  bool drawPairs = true;
  bool drawSyntax = true;
  bool oldOutline = false;
  bool TrueMod = true;

  bool inRedraw = false;
  int idleCount = 0;

  std::unique_ptr<UnicodeString> ret_str;

  int newfore = -1;
  int newback = -1;
  const StyledRegion* rdBackground = nullptr;
  std::unique_ptr<LineRegion> cursorRegion;

  size_t visibleLevel = 100;
  std::unique_ptr<Outliner> structOutliner;
  std::unique_ptr<Outliner> errorOutliner;
  intptr_t editor_id = -1;

  void reloadTypeSettings();
  EditorInfo enterHandler();
  FarColor convert(const StyledRegion* rd) const;
  void showOutliner(Outliner* outliner);
  void addFARColor(intptr_t lno, intptr_t s, intptr_t e, const FarColor& col, EDITORCOLORFLAGS TabMarkStyle = 0) const;
  void deleteFarColor(intptr_t lno, intptr_t s) const;
  [[nodiscard]] const wchar_t* GetMsg(int msg) const;
  static COLORREF getSuitableColor(COLORREF base_color, COLORREF blend_color);
  void drawCross(const EditorInfo& ei, intptr_t lno, const EditorConvertPos& ecp_cl) const;
};
#endif
