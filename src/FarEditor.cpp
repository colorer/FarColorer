#include "FarEditor.h"

FarEditor::FarEditor(PluginStartupInfo* info, ParserFactory* pf, const UnicodeString* file_name) : info(info), parserFactory(pf)
{
  auto eh = getEditorInfo();
  editor_id = eh.EditorID;
  chooseFileType(file_name);
}

FarEditor::~FarEditor()
{
  destroy();
}

void FarEditor::init()
{
  baseEditor = std::make_unique<BaseEditor>(parserFactory, this);

  UnicodeString def_out = UnicodeString(region_DefOutlined);
  UnicodeString def_err = UnicodeString(region_DefError);
  const Region* def_Outlined = parserFactory->getHrcLibrary().getRegion(&def_out);
  const Region* def_Error = parserFactory->getHrcLibrary().getRegion(&def_err);
  structOutliner = std::make_unique<Outliner>(baseEditor.get(), def_Outlined);
  errorOutliner = std::make_unique<Outliner>(baseEditor.get(), def_Error);

  // subscribe for event change text
  EditorSubscribeChangeEvent esce = {sizeof(EditorSubscribeChangeEvent), MainGuid};
  info->EditorControl(editor_id, ECTL_SUBSCRIBECHANGEEVENT, 0, &esce);
}

void FarEditor::destroy()
{
  structOutliner.reset();
  errorOutliner.reset();
  baseEditor.reset();
  // destroy subscribe
  EditorSubscribeChangeEvent esce = {sizeof(EditorSubscribeChangeEvent), MainGuid};
  info->EditorControl(editor_id, ECTL_UNSUBSCRIBECHANGEEVENT, 0, &esce);
}

void FarEditor::endJob(size_t lno)
{
  (void) lno;
  ret_str.reset();
}

UnicodeString* FarEditor::getLine(size_t lno)
{
  EditorGetString es {sizeof(EditorGetString), (intptr_t) lno};

  intptr_t len = 0;
  if (info->EditorControl(editor_id, ECTL_GETSTRING, 0, &es)) {
    len = es.StringLength;
    if (len > maxLineLength && maxLineLength > 0) {
      len = maxLineLength;
    }
    ret_str = std::make_unique<UnicodeString>(es.StringText, 0, (int) len);
    return ret_str.get();
  }
  else {
    return nullptr;
  }
}

void FarEditor::chooseFileType(const UnicodeString* fname)
{
  auto base_editor = std::make_unique<BaseEditor>(parserFactory, this);
  FileType* ftype = base_editor->chooseFileType(fname);
  if (UnicodeString(name_DefaultScheme).compare(ftype->getName()) != 0) {
    // это не default тип
    auto value_disabled = ftype->getParamValue(UnicodeString("disabled"));
    if (value_disabled && value_disabled->compare(UnicodeString(value_True)) == 0) {
      // тип заблокирован
      FileType* def = parserFactory->getHrcLibrary().getFileType(UnicodeString(name_DefaultScheme));
      if (!def) {
        throw Exception("No 'default' file type found");
      }
      auto value_used = ftype->getParamValue(UnicodeString("use-default"));
      auto def_value_used = def->getParamValue(UnicodeString("use-default"));
      if ((value_used && value_used->compare(UnicodeString(value_True)) == 0) ||
          (!value_used && def_value_used && def_value_used->compare(UnicodeString(value_True)) == 0)) {
        // используем default
        ftype = def;
      }
      else {
        // отключаем
        ftype = nullptr;
      }
    }
  }

  setFileType(ftype);
}

void FarEditor::setFileType(FileType* ftype)
{
  if (ftype) {
    if (!baseEditor) {
      init();
    }

    baseEditor->setFileType(ftype);
    // clear Outliner
    structOutliner->modifyEvent(0);
    errorOutliner->modifyEvent(0);

    reloadTypeSettings();
  }
  else {
    if (baseEditor) {
      cleanEditor();
      destroy();
    }
  }
}

void FarEditor::reloadTypeSettings()
{
  FileType* ftype = baseEditor->getFileType();
  auto& hrcLibrary = parserFactory->getHrcLibrary();
  FileType* def = hrcLibrary.getFileType(UnicodeString(name_DefaultScheme));

  if (def == nullptr) {
    throw Exception("No 'default' file type found");
  }

  int backparse = def->getParamValueInt(UnicodeString(param_Backparse), 2000);
  backparse = ftype->getParamValueInt(UnicodeString(param_Backparse), backparse);
  baseEditor->setBackParse(backparse);

  maxLineLength = def->getParamValueInt(UnicodeString(param_MaxLen), 0);
  maxLineLength = ftype->getParamValueInt(UnicodeString(param_MaxLen), maxLineLength);

  newfore = def->getParamValueInt(UnicodeString(param_DefFore), -1);
  newfore = ftype->getParamValueInt(UnicodeString(param_DefFore), newfore);

  newback = def->getParamValueInt(UnicodeString(param_DefBack), -1);
  newback = ftype->getParamValueInt(UnicodeString(param_DefBack), newback);

  const UnicodeString* value;
  value = ftype->getParamValue(UnicodeString(param_Fullback));
  if (!value) {
    value = def->getParamValue(UnicodeString(param_Fullback));
  }
  if (value != nullptr && value->compare(UnicodeString(value_No)) == 0) {
    fullBackground = false;
  }

  value = ftype->getParamValue(UnicodeString(param_CrossZorder));
  if (!value) {
    value = def->getParamValue(UnicodeString(param_CrossZorder));
  }
  if (value != nullptr && value->compare(UnicodeString(value_Top)) == 0) {
    crossZOrder = 1;
  }

  setCrossState(crossStatus, crossStyle);

  int maxBlockSize = def->getParamValueInt(UnicodeString(param_MaxBlockSize), 300);
  maxBlockSize = ftype->getParamValueInt(UnicodeString(param_MaxBlockSize), maxBlockSize);
  baseEditor->setMaxBlockSize(maxBlockSize);
}

FileType* FarEditor::getFileType() const
{
  if (baseEditor) {
    return baseEditor->getFileType();
  }
  else {
    return nullptr;
  }
}

void FarEditor::setCrossState(int status, int style)
{
  crossStatus = status;
  crossStyle = style;

  switch (static_cast<CROSS_STATUS>(crossStatus)) {
    case CROSS_STATUS::CROSS_OFF:
      changeCrossStyle(CROSS_STYLE::CSTYLE_NONE);
      break;
    case CROSS_STATUS::CROSS_ON:
      changeCrossStyle((CROSS_STYLE) crossStyle);
      break;
    case CROSS_STATUS::CROSS_INSCHEME:
      FileType* ftype = baseEditor->getFileType();
      auto& hrcLibrary = parserFactory->getHrcLibrary();
      FileType* def = hrcLibrary.getFileType(UnicodeString(name_DefaultScheme));
      const UnicodeString* value;
      value = ftype->getParamValue(UnicodeString(param_ShowCross));
      if (!value) {
        value = def->getParamValue(UnicodeString(param_ShowCross));
      }

      if (value) {
        if (value->compare(UnicodeString(value_None)) == 0) {
          changeCrossStyle(CROSS_STYLE::CSTYLE_NONE);
        }
        else if (value->compare(UnicodeString(value_Vertical)) == 0) {
          changeCrossStyle(CROSS_STYLE::CSTYLE_VERT);
        }
        else if (value->compare(UnicodeString(value_Horizontal)) == 0) {
          changeCrossStyle(CROSS_STYLE::CSTYLE_HOR);
        }
        else if (value->compare(UnicodeString(value_Both)) == 0) {
          changeCrossStyle(CROSS_STYLE::CSTYLE_BOTH);
        }
      }
      break;
  }
}

void FarEditor::setDrawPairs(bool _drawPairs)
{
  drawPairs = _drawPairs;
}

void FarEditor::setDrawSyntax(bool _drawSyntax)
{
  drawSyntax = _drawSyntax;
}

void FarEditor::setOutlineStyle(bool _oldStyle)
{
  oldOutline = _oldStyle;
}

void FarEditor::setTrueMod(bool TrueMod_)
{
  this->TrueMod = TrueMod_;
}

void FarEditor::setRegionMapper(RegionMapper* rs)
{
  baseEditor->setRegionMapper(rs);
  rdBackground = StyledRegion::cast(baseEditor->rd_def_Text);

  // if horizontal cross color don`t set
  if (!baseEditor->rd_def_HorzCross) {
    // set default color
    horzCrossColor = {};
    if (TrueMod) {
      horzCrossColor.BackgroundRGBA = {128, 128, 128, 0xFF};
    }
    else {
      horzCrossColor.BackgroundColor = 0x7;
      horzCrossColor.Flags |= FCF_4BITMASK;
    }
  }
  else {
    // else use it
    horzCrossColor = convert(StyledRegion::cast(baseEditor->rd_def_HorzCross));
  }

  // if vertical cross color don`t set
  if (!baseEditor->rd_def_VertCross) {
    // set default color
    vertCrossColor = {};
    if (TrueMod) {
      vertCrossColor.BackgroundRGBA = {128, 128, 128, 0xFF};
    }
    else {
      vertCrossColor.BackgroundColor = 0x7;
      vertCrossColor.Flags |= FCF_4BITMASK;
    }
  }
  else {
    // else use it
    vertCrossColor = convert(StyledRegion::cast(baseEditor->rd_def_VertCross));
  }
}

void FarEditor::matchPair()
{
  EditorInfo ei = getEditorInfo();
  PairMatch* pm = baseEditor->searchGlobalPair((int) ei.CurLine, (int) ei.CurPos);

  if ((pm == nullptr) || (pm->eline == -1)) {
    baseEditor->releasePairMatch(pm);
    return;
  }

  EditorSetPosition esp {sizeof(EditorSetPosition), pm->eline, 0, -1, -1, -1, -1};

  if (!pm->topPosition) {
    esp.CurPos = pm->end->start;
  }
  else {
    esp.CurPos = pm->end->end - 1;
  }

  if (esp.CurLine < ei.TopScreenLine || esp.CurLine > ei.TopScreenLine + ei.WindowSizeY) {
    esp.TopScreenLine = pm->eline - ei.WindowSizeY / 2;

    if (esp.TopScreenLine < 0) {
      esp.TopScreenLine = 0;
    }
  }

  info->EditorControl(editor_id, ECTL_SETPOSITION, 0, &esp);
  baseEditor->releasePairMatch(pm);
}

void FarEditor::selectPair()
{
  EditorInfo ei = getEditorInfo();
  PairMatch* pm = baseEditor->searchGlobalPair((int) ei.CurLine, (int) ei.CurPos);

  if ((pm == nullptr) || (pm->eline == -1)) {
    baseEditor->releasePairMatch(pm);
    return;
  }

  int X1, X2, Y1, Y2;
  if (pm->topPosition) {
    X1 = pm->start->end;
    X2 = pm->end->start - 1;
    Y1 = pm->sline;
    Y2 = pm->eline;
  }
  else {
    X2 = pm->start->start - 1;
    X1 = pm->end->end;
    Y2 = pm->sline;
    Y1 = pm->eline;
  }

  EditorSelect es {sizeof(EditorSelect), BTYPE_STREAM, Y1, X1, X2 - X1 + 1, Y2 - Y1 + 1};
  info->EditorControl(editor_id, ECTL_SELECT, 0, &es);

  baseEditor->releasePairMatch(pm);
}

void FarEditor::selectBlock()
{
  EditorInfo ei = getEditorInfo();
  PairMatch* pm = baseEditor->searchGlobalPair((int) ei.CurLine, (int) ei.CurPos);

  if ((pm == nullptr) || (pm->eline == -1)) {
    baseEditor->releasePairMatch(pm);
    return;
  }

  int X1, X2, Y1, Y2;
  if (pm->topPosition) {
    X1 = pm->start->start;
    X2 = pm->end->end - 1;
    Y1 = pm->sline;
    Y2 = pm->eline;
  }
  else {
    X2 = pm->start->end - 1;
    X1 = pm->end->start;
    Y2 = pm->sline;
    Y1 = pm->eline;
  }

  EditorSelect es {sizeof(EditorSelect), BTYPE_STREAM, Y1, X1, X2 - X1 + 1, Y2 - Y1 + 1};
  info->EditorControl(editor_id, ECTL_SELECT, 0, &es);

  baseEditor->releasePairMatch(pm);
}

void FarEditor::selectRegion()
{
  EditorInfo ei = getEditorInfo();
  EditorGetString egs {sizeof(EditorGetString), ei.CurLine};
  info->EditorControl(editor_id, ECTL_GETSTRING, 0, &egs);

  EditorSelect es {sizeof(EditorSelect)};
  if (cursorRegion != nullptr) {
    intptr_t end = cursorRegion->end;

    if (end == -1) {
      end = egs.StringLength;
    }

    if (end - cursorRegion->start > 0) {
      es.BlockType = BTYPE_STREAM;
      es.BlockStartLine = ei.CurLine;
      es.BlockStartPos = cursorRegion->start;
      es.BlockHeight = 1;
      es.BlockWidth = end - cursorRegion->start;
      info->EditorControl(editor_id, ECTL_SELECT, 0, &es);
    }
  }
}

void FarEditor::getNameCurrentScheme()
{
  if (cursorRegion) {
    UnicodeString region, scheme;
    region.append("Region: ");
    scheme.append("Scheme: ");
    if (cursorRegion->region != nullptr) {
      const Region* r = cursorRegion->region;
      region.append(r->getName());
    }
    if (cursorRegion->scheme != nullptr) {
      scheme.append(*cursorRegion->scheme->getName());
    }
    auto region_str = UStr::to_stdwstr(&region);
    auto scheme_str = UStr::to_stdwstr(&scheme);
    const wchar_t* exceptionMessage[3] = {GetMsg(mRegionName), region_str.c_str(), scheme_str.c_str()};
    info->Message(&MainGuid, &RegionName, FMSG_MB_OK | FMSG_LEFTALIGN, nullptr, &exceptionMessage[0], std::size(exceptionMessage), 1);
  }
}

void FarEditor::getCurrentRegionInfo(UnicodeString& region, UnicodeString& scheme)
{
  if (cursorRegion) {
    if (cursorRegion->region != nullptr) {
      const Region* r = cursorRegion->region;
      region.append(r->getName());
    }
    if (cursorRegion->scheme != nullptr) {
      scheme.append(*cursorRegion->scheme->getName());
    }
  }
}

void FarEditor::listFunctions()
{
  baseEditor->validate(-1, false);
  showOutliner(structOutliner.get());
}

void FarEditor::listErrors()
{
  baseEditor->validate(-1, false);
  showOutliner(errorOutliner.get());
}

void FarEditor::locateFunction()
{
  // extract word
  EditorInfo ei = getEditorInfo();
  UnicodeString& curLine = *getLine(ei.CurLine);
  int cpos = (int) ei.CurPos;
  int sword = cpos;
  int eword = cpos;

  while (cpos < curLine.length() && (Character::isLetterOrDigit(curLine[cpos]) || curLine[cpos] != '_')) {
    while (Character::isLetterOrDigit(curLine[eword]) || curLine[eword] == '_') {
      if (eword == curLine.length() - 1) {
        break;
      }

      eword++;
    }

    while (Character::isLetterOrDigit(curLine[sword]) || curLine[sword] == '_') {
      if (sword == 0) {
        break;
      }

      sword--;
    }

    UnicodeString funcname(curLine, sword + 1, eword - sword - 1);
    COLORER_LOG_DEBUG("FC] Letter %", funcname);
    baseEditor->validate(-1, false);
    OutlineItem* item_found = nullptr;
    OutlineItem* item_last = nullptr;
    size_t items_num = structOutliner->itemCount();

    if (items_num == 0) {
      break;
    }

    // search through the outliner
    for (size_t idx = 0; idx < items_num; idx++) {
      OutlineItem* item = structOutliner->getItem(idx);

      if (UStr::indexOfIgnoreCase(*item->token,funcname) != -1) {
        if (item->lno == (size_t) ei.CurLine) {
          item_last = item;
        }
        else {
          item_found = item;
        }
      }
    }

    if (!item_found) {
      item_found = item_last;
    }

    if (!item_found) {
      break;
    }

    EditorSetPosition esp {sizeof(EditorSetPosition)};
    esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
    esp.CurLine = (intptr_t) item_found->lno;
    esp.CurPos = item_found->pos;
    esp.TopScreenLine = esp.CurLine - ei.WindowSizeY / 2;

    if (esp.TopScreenLine < 0) {
      esp.TopScreenLine = 0;
    }

    info->EditorControl(editor_id, ECTL_SETPOSITION, 0, &esp);
    info->EditorControl(editor_id, ECTL_REDRAW, 0, nullptr);
    return;  //-V612
  }

  const wchar_t* msg[2] = {GetMsg(mNothingFound), GetMsg(mGotcha)};
  info->Message(&MainGuid, &NothingFoundMesage, 0, nullptr, msg, 2, 1);
}

void FarEditor::updateHighlighting()
{
  EditorInfo ei = getEditorInfo();
  baseEditor->validate((int) ei.TopScreenLine, true);
}

int FarEditor::editorInput(const INPUT_RECORD& Rec)
{
  if (Rec.EventType == KEY_EVENT && Rec.Event.KeyEvent.wVirtualKeyCode == 0) {
    if (baseEditor->haveInvalidLine()) {
      auto invalid_line1 = baseEditor->getInvalidLine();
      idleCount++;
      if (idleCount > 10) {
        idleCount = 10;
      }
      baseEditor->idleJob(idleCount * 10);
      auto invalid_line2 = baseEditor->getInvalidLine();
      EditorInfo ei = getEditorInfo();

      if ((invalid_line1 < ei.TopScreenLine && invalid_line2 >= ei.TopScreenLine) ||
          (invalid_line1 < ei.TopScreenLine + ei.WindowSizeY && invalid_line2 >= ei.TopScreenLine + ei.WindowSizeY)) {
        info->EditorControl(editor_id, ECTL_REDRAW, 0, nullptr);
      }
    }
  }
  else if (Rec.EventType == KEY_EVENT) {
    idleCount = 0;
  }

  return 0;
}

COLORREF FarEditor::getSuitableColor(const COLORREF base_color, const COLORREF blend_color)
{
  /*0 - black
    1 - blue
    2 - green
    3 - cyan
    4 - bordo
    5 - purple
    6 - brown
    7 - light gray
    8 - gray
    9 - light blue
    A - light green
    B - light cyan
    C - red
    D - light purple
    E - yellow
    F - white*/
  if (base_color == blend_color) {
    switch (blend_color) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        return blend_color + 8;
      case 8:
        return 7;
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        return blend_color - 8;
      default:
        return blend_color;
    }
  }
  else {
    return blend_color;
  }
}

int FarEditor::editorEvent(intptr_t event, void* param)
{
  if (event == EE_CHANGE) {
    auto* editor_change = static_cast<EditorChange*>(param);
    baseEditor->modifyEvent((int) editor_change->StringNumber);
    return 0;
  }
  // ignore event
  if (event != EE_REDRAW || (param == EEREDRAW_ALL && inRedraw)) {
    return 0;
  }

  if (rdBackground == nullptr) {
    throw Exception("HRD Background region 'def:Text' not found");
  }

  EditorInfo cur_editor = getEditorInfo();

  baseEditor->visibleTextEvent((int) cur_editor.TopScreenLine, (int) cur_editor.WindowSizeY);
  baseEditor->lineCountEvent((int) cur_editor.TotalLines);

  cursorRegion.reset();

  // Position the cursor on the screen
  EditorConvertPos cursor_position {sizeof(EditorConvertPos), -1, cur_editor.CurPos};
  info->EditorControl(editor_id, ECTL_REALTOTAB, 0, &cursor_position);

  bool show_whitespase = (cur_editor.Options & EOPT_SHOWWHITESPACE) != 0;
  bool show_eol = (cur_editor.Options & EOPT_SHOWLINEBREAK) != 0;

  for (auto lno = cur_editor.TopScreenLine; lno < cur_editor.TopScreenLine + cur_editor.WindowSizeY && lno < cur_editor.TotalLines; lno++) {
    // clean line in far editor
    deleteFarColor(lno);

    EditorGetString current_string {sizeof(EditorGetString), lno};
    info->EditorControl(editor_id, ECTL_GETSTRING, 0, &current_string);
    // position previously found a column in the current row
    EditorConvertPos ecp_cl {sizeof(EditorConvertPos), lno, cursor_position.DestPos};
    info->EditorControl(editor_id, ECTL_TABTOREAL, 0, &ecp_cl);
    auto cur_line_length = current_string.StringLength;
    auto cur_line_position = ecp_cl.DestPos;

    if (drawSyntax) {
      LineRegion* cur_region = baseEditor->getLineRegions((int) lno);
      if (cur_region) {
        for (; cur_region; cur_region = cur_region->next) {
          if (cur_region->special || cur_region->start == cur_region->end || (cur_region->end != -1 && cur_region->end < cur_editor.LeftPos)) {
            continue;
          }
          if (cur_region->start > cur_editor.LeftPos + cur_editor.WindowSizeX) {
            break;
          }

          intptr_t lend = cur_region->end;
          if (lend == -1) {
            if (fullBackground){
              lend = cur_editor.LeftPos + cur_editor.WindowSizeX * 2;
            }else{
              lend = cur_line_length;
            }
          }
          if (lno == cur_editor.CurLine && (cur_region->start <= cur_editor.CurPos) && (cur_editor.CurPos <= lend)) {
            cursorRegion = std::make_unique<LineRegion>(*cur_region);
          }

          FarColor col = convert(cur_region->styled());
          // remove the front in color whitespaces to display correctly in the far hidden characters (tab, space)
          intptr_t j = cur_region->start;
          bool whitespace = false;
          while (j < lend) {
            FarColor col1 = col;
            intptr_t start = j;
            intptr_t end = lend;
            if (show_whitespase) {
              // поиск кусков обрабатываемого региона которые только whitespace или только не whitespace
              if (current_string.StringText[j] == L' ' || current_string.StringText[j] == L'\t') {
                while ((j <= cur_line_length) && (j < lend) && (current_string.StringText[j] == L' ' || current_string.StringText[j] == L'\t')) {
                  j++;
                }
                end = j >= cur_line_length ? lend : j;
                whitespace = true;
                // цвет символов whitespace равны цвету по умолчанию
                col1.ForegroundColor = rdBackground->fore;
              }
              else {
                while ((j <= cur_line_length) && (j < lend) && (current_string.StringText[j] != L' ' && current_string.StringText[j] != L'\t')) {
                  j++;
                }
                end = j >= cur_line_length ? lend : j;
                whitespace = false;
              }
            }

            if (lno == cur_editor.CurLine && showHorizontalCross) {
              // color for horizontal cross
              if (crossZOrder != 0 && !whitespace) {
                col1.ForegroundColor = horzCrossColor.ForegroundColor;
              }
              col1.BackgroundColor = getSuitableColor(col1.ForegroundColor, horzCrossColor.BackgroundColor);
            }
            // рисуем основной кусок региона, горизонтальную часть
            addFARColor(lno, start, end, col1);

            if (end > cur_line_length && show_eol) {
              // исправляем цвет символов конца строки
              FarColor col2 = col;
              col2.ForegroundColor = rdBackground->fore;
              if (lno == cur_editor.CurLine && showHorizontalCross) {
                col2.BackgroundColor = horzCrossColor.BackgroundColor;
              }
              auto eol_len = wcslen(current_string.StringEOL);
              eol_len = eol_len > 0 ? eol_len : 1; // eol = 0 for end of file
              addFARColor(lno, cur_line_length, cur_line_length + eol_len, col2);
            }

            if (showVerticalCross && start <= cur_line_position && cur_line_position < end) {
              // вертикальный крест
              FarColor col3 = col;
              if ((cur_line_position == cur_line_length || cur_line_position == cur_line_length + 1) && show_eol) {
                col3.ForegroundColor = rdBackground->fore;
              }
              else {
                if (crossZOrder != 0 && !whitespace) {
                  col3.ForegroundColor = vertCrossColor.ForegroundColor;
                }
              }
              col3.BackgroundColor = getSuitableColor(col3.ForegroundColor, vertCrossColor.BackgroundColor);
              addFARColor(lno, cur_line_position, cur_line_position + 1, col3, ECF_TABMARKCURRENT);
            }
            j = end;
          }
        }
      }
      else {
        // always draw cross
        drawCross(cur_editor, lno, cur_line_position);
      }
    }
    else {
      drawCross(cur_editor, lno, cur_line_position);
    }
  }

  // pair brackets
  if (drawPairs) {
    PairMatch* pm = baseEditor->searchLocalPair((int) cur_editor.CurLine, (int) cur_editor.CurPos);
    if (pm != nullptr) {
      // start bracket
      FarColor col = convert(pm->start->styled());

      // horizontal cross
      if (showHorizontalCross) {
        col.BackgroundColor = horzCrossColor.BackgroundColor;
      }
      addFARColor(cur_editor.CurLine, pm->start->start, pm->start->end, col);

      // vertical cross
      if (showVerticalCross && !showHorizontalCross && pm->start->start <= cur_editor.CurPos && cur_editor.CurPos < pm->start->end) {
        col.BackgroundColor = vertCrossColor.BackgroundColor;
        addFARColor(cur_editor.CurLine, cur_editor.CurPos, cur_editor.CurPos + 1, col);
      }

      // end bracket
      if (pm->eline != -1) {
        col = convert(pm->end->styled());

        // horizontal cross
        if (showHorizontalCross && pm->eline == cur_editor.CurLine) {
          col.BackgroundColor = horzCrossColor.BackgroundColor;
        }
        addFARColor(pm->eline, pm->end->start, pm->end->end, col);

        EditorConvertPos ecp2 {sizeof(EditorConvertPos), pm->eline, cursor_position.DestPos};
        info->EditorControl(editor_id, ECTL_TABTOREAL, 0, &ecp2);

        // vertical cross
        if (showVerticalCross && pm->end->start <= ecp2.DestPos && ecp2.DestPos < pm->end->end) {
          col.BackgroundColor = vertCrossColor.BackgroundColor;
          addFARColor(pm->eline, ecp2.DestPos, ecp2.DestPos + 1, col);
        }
      }

      baseEditor->releasePairMatch(pm);
    }
  }

  if (param != EEREDRAW_ALL) {
    inRedraw = true;
    info->EditorControl(editor_id, ECTL_REDRAW, 0, nullptr);
    inRedraw = false;
  }

  return 1;
}

void FarEditor::drawCross(const EditorInfo& ei, intptr_t lno, intptr_t ecp_cl) const
{
  if (lno == ei.CurLine && showHorizontalCross) {
    addFARColor(lno, 0, ei.LeftPos + ei.WindowSizeX, horzCrossColor);
  }
  if (showVerticalCross) {
    addFARColor(lno, ecp_cl, ecp_cl + 1, vertCrossColor);
  }
}

void FarEditor::showOutliner(Outliner* outliner)
{
  FarMenuItem* menu;
  EditorSetPosition esp {sizeof(EditorSetPosition)};
  bool moved = false;
  intptr_t code = 0;
  const int FILTER_SIZE = 40;
  FarKey breakKeys[] = {{VK_BACK, 0},
                        {VK_RETURN, 0},
                        {VK_OEM_1, 0},
                        {VK_OEM_MINUS, 0},
                        {VK_TAB, 0},
                        {VK_UP, LEFT_CTRL_PRESSED},
                        {VK_DOWN, LEFT_CTRL_PRESSED},
                        {VK_LEFT, LEFT_CTRL_PRESSED},
                        {VK_RIGHT, LEFT_CTRL_PRESSED},
                        {VK_RETURN, LEFT_CTRL_PRESSED},
                        {VK_OEM_1, LEFT_CTRL_PRESSED},
                        {VK_OEM_MINUS, SHIFT_PRESSED},
                        {VK_OEM_3, SHIFT_PRESSED},
                        {'0', 0},
                        {'1', 0},
                        {'2', 0},
                        {'3', 0},
                        {'4', 0},
                        {'5', 0},
                        {'6', 0},
                        {'7', 0},
                        {'8', 0},
                        {'9', 0},
                        {'A', 0},
                        {'B', 0},
                        {'C', 0},
                        {'D', 0},
                        {'E', 0},
                        {'F', 0},
                        {'G', 0},
                        {'H', 0},
                        {'I', 0},
                        {'J', 0},
                        {'K', 0},
                        {'L', 0},
                        {'M', 0},
                        {'N', 0},
                        {'O', 0},
                        {'P', 0},
                        {'Q', 0},
                        {'R', 0},
                        {'S', 0},
                        {'T', 0},
                        {'U', 0},
                        {'V', 0},
                        {'W', 0},
                        {'X', 0},
                        {'Y', 0},
                        {'Z', 0},
                        {VK_SPACE, 0}};
  int keys_size = sizeof(breakKeys) / sizeof(FarKey);

  wchar_t prefix[FILTER_SIZE + 1];
  wchar_t autofilter[FILTER_SIZE + 1];
  wchar_t filter[FILTER_SIZE + 1];
  int flen = 0;
  *filter = 0;
  size_t maxLevel = 0;
  bool stopMenu = false;
  size_t items_num = outliner->itemCount();

  if (items_num == 0) {
    stopMenu = true;
  }

  menu = new FarMenuItem[items_num];
  EditorInfo ei_curr = getEditorInfo();

  while (!stopMenu) {
    size_t i;
    memset(menu, 0, sizeof(FarMenuItem) * items_num);
    // items in FAR's menu;
    int menu_size = 0;
    int selectedItem = 0;
    std::vector<int> treeStack;

    EditorInfo ei = getEditorInfo();
    for (i = 0; i < items_num; i++) {
      OutlineItem* item = outliner->getItem(i);

      if (filter[0] == '\0' || UStr::indexOfIgnoreCase(*item->token, filter) != -1) {
        auto treeLevel = Outliner::manageTree(treeStack, item->level);

        if (maxLevel < treeLevel) {
          maxLevel = (int) treeLevel;
        }

        if (treeLevel > visibleLevel) {
          continue;
        }

        auto* menuItem = new wchar_t[255];

        if (!oldOutline) {
          int si = _snwprintf(menuItem, 255, L"%4zd ", item->lno + 1);

          for (size_t lIdx = 0; lIdx < treeLevel; lIdx++) {
            menuItem[si++] = ' ';
            menuItem[si++] = ' ';
          }

          auto region = item->region->getName();

          wchar_t cls = Character::toLowerCase((region)[region.indexOf(':') + 1]);

          si += _snwprintf(menuItem + si, 255 - si, L"%c ", cls);

          size_t labelLength = item->token->length();

          if (labelLength + si > 110) {
            labelLength = 110;
          }

          wcsncpy(menuItem + si, UStr::to_stdwstr(item->token.get()).c_str(), labelLength);
          menuItem[si + labelLength] = 0;
        }
        else {
          UnicodeString* line = getLine(item->lno);
          size_t labelLength = line->length();

          if (labelLength > 110) {
            labelLength = 110;
          }

          wcsncpy(menuItem, UStr::to_stdwstr(line).c_str(), labelLength);
          menuItem[labelLength] = 0;
        }

        // set position on nearest top function
        menu[menu_size].Text = menuItem;
        menu[menu_size].UserData = reinterpret_cast<intptr_t>(item);

        if (ei.CurLine >= (int) item->lno) {
          selectedItem = menu_size;
        }

        menu_size++;
      }
    }

    if (selectedItem > 0) {
      menu[selectedItem].Flags = MIF_SELECTED;
    }

    if (menu_size == 0 && flen > 0) {
      flen--;
      filter[flen] = 0;
      continue;
    }

    int aflen = flen;
    // Find same function prefix
    bool same = true;
    int plen = 0;
    wcscpy(autofilter, filter);

    while (code != 0 && menu_size > 1 && same && plen < FILTER_SIZE) {
      plen = aflen + 1;
      int auto_ptr = UStr::indexOfIgnoreCase(menu[0].Text, autofilter);

      if (int(wcslen(menu[0].Text) - auto_ptr) < plen) {
        break;
      }

      wcsncpy(prefix, menu[0].Text + auto_ptr, plen);
      prefix[plen] = 0;

      for (int j = 1; j < menu_size; j++) {
        if (UStr::indexOfIgnoreCase(menu[j].Text,prefix) == -1) {
          same = false;
          break;
        }
      }

      if (same) {
        aflen++;
        wcscpy(autofilter, prefix);
      }
    }

    wchar_t top[128];
    const wchar_t* topline = GetMsg(mOutliner);
    wchar_t captionfilter[FILTER_SIZE + 1];
    wcsncpy(captionfilter, filter, flen);
    captionfilter[flen] = 0;

    if (aflen > flen) {
      wcscat(captionfilter, L"?");
      wcsncat(captionfilter, autofilter + flen, aflen - flen);
      captionfilter[aflen + 1] = 0;
    }

    _snwprintf(top, 128, topline, captionfilter);

    intptr_t sel = info->Menu(&MainGuid, &OutlinerMenu, -1, -1, 0, FMENU_SHOWAMPERSAND | FMENU_WRAPMODE, top, GetMsg(mChoose), L"add", breakKeys,
                              &code, menu, menu_size);

    // handle mouse selection
    if (sel != -1 && code == -1) {
      code = 1;
    }

    switch (code) {
      case -1:
        stopMenu = true;
        break;
      case 0:  // VK_BACK

        if (flen > 0) {
          flen--;
        }

        filter[flen] = 0;
        break;
      case 1: {  // VK_RETURN
        if (menu_size == 0) {
          break;
        }

        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        auto* item = reinterpret_cast<OutlineItem*>(menu[sel].UserData);
        esp.CurLine = (intptr_t) item->lno;
        esp.CurPos = item->pos;
        esp.TopScreenLine = esp.CurLine - ei.WindowSizeY / 2;

        if (esp.TopScreenLine < 0) {
          esp.TopScreenLine = 0;
        }

        info->EditorControl(editor_id, ECTL_SETPOSITION, 0, &esp);
        stopMenu = true;
        moved = true;
        break;
      }
      case 2:  // ;

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = ';';
        filter[++flen] = 0;
        break;
      case 3:  // -

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = '-';
        filter[++flen] = 0;
        break;
      case 4:  // VK_TAB
        wcscpy(filter, autofilter);
        flen = aflen;
        break;
      case 5: {  // ctrl-up
        if (menu_size == 0) {
          break;
        }

        if (sel == 0) {
          sel = menu_size - 1;
        }
        else {
          sel--;
        }

        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        auto* item = reinterpret_cast<OutlineItem*>(menu[sel].UserData);
        esp.CurLine = (intptr_t) item->lno;
        esp.CurPos = item->pos;
        esp.TopScreenLine = esp.CurLine - ei.WindowSizeY / 2;

        if (esp.TopScreenLine < 0) {
          esp.TopScreenLine = 0;
        }

        info->EditorControl(editor_id, ECTL_SETPOSITION, 0, &esp);
        info->EditorControl(editor_id, ECTL_REDRAW, 0, nullptr);
        info->EditorControl(editor_id, ECTL_GETINFO, 0, &ei);
        break;
      }
      case 6: {  // ctrl-down
        if (menu_size == 0) {
          break;
        }

        if (sel == menu_size - 1) {
          sel = 0;
        }
        else {
          sel++;
        }

        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        auto* item = reinterpret_cast<OutlineItem*>(menu[sel].UserData);
        esp.CurLine = (intptr_t) item->lno;
        esp.CurPos = item->pos;
        esp.TopScreenLine = esp.CurLine - ei.WindowSizeY / 2;

        if (esp.TopScreenLine < 0) {
          esp.TopScreenLine = 0;
        }

        info->EditorControl(editor_id, ECTL_SETPOSITION, 0, &esp);
        info->EditorControl(editor_id, ECTL_REDRAW, 0, nullptr);
        info->EditorControl(editor_id, ECTL_GETINFO, 0, &ei);
        break;
      }
      case 7: {  // ctrl-left
        if (visibleLevel > maxLevel) {
          visibleLevel = maxLevel - 1;
        }
        else {
          if (visibleLevel > 0) {
            visibleLevel--;
          }
        }

        if (visibleLevel < 0) {
          visibleLevel = 0;
        }

        break;
      }
      case 8: {  // ctrl-right
        visibleLevel++;
        break;
      }
      case 9: {  // ctrl-return
        // read current position
        info->EditorControl(editor_id, ECTL_GETINFO, 0, &ei);
        // insert text
        auto* item = reinterpret_cast<OutlineItem*>(menu[sel].UserData);
        UnicodeString str = UnicodeString(*item->token);
        //!! warning , after call next line  object 'item' changes
        info->EditorControl(editor_id, ECTL_INSERTTEXT, 0, (void*) UStr::to_stdwstr(&str).c_str());

        // move the cursor to the end of the inserted string
        esp.CurTabPos = esp.LeftPos = esp.Overtype = esp.TopScreenLine = -1;
        esp.CurLine = -1;
        esp.CurPos = ei.CurPos + str.length();
        info->EditorControl(editor_id, ECTL_SETPOSITION, 0, &esp);

        stopMenu = true;
        moved = true;
        break;
      }
      case 10:  // :

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = ':';
        filter[++flen] = 0;
        break;
      case 11:  // _

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = '_';
        filter[++flen] = 0;
        break;
      case 12:  // ~

        if (flen == FILTER_SIZE) {
          break;
        }

        filter[flen] = '~';
        filter[++flen] = 0;
        break;
      default:

        if (flen == FILTER_SIZE || code > keys_size) {
          break;
        }
        filter[flen] = static_cast<wchar_t>(Character::toLowerCase(breakKeys[code].VirtualKeyCode));
        filter[++flen] = 0;
        break;
    }
  }

  for (size_t i = 0; i < items_num; i++) {
    delete[] menu[i].Text;
  }

  delete[] menu;

  if (!moved) {
    // restoring position
    esp.CurLine = ei_curr.CurLine;
    esp.CurPos = ei_curr.CurPos;
    esp.CurTabPos = ei_curr.CurTabPos;
    esp.TopScreenLine = ei_curr.TopScreenLine;
    esp.LeftPos = ei_curr.LeftPos;
    esp.Overtype = ei_curr.Overtype;
    info->EditorControl(editor_id, ECTL_SETPOSITION, 0, &esp);
  }

  if (items_num == 0) {
    const wchar_t* msg[2] = {GetMsg(mNothingFound), GetMsg(mGotcha)};
    info->Message(&MainGuid, &NothingFoundMesage, 0, nullptr, msg, 2, 1);
  }
}

EditorInfo FarEditor::getEditorInfo()
{
  EditorInfo ei {sizeof(EditorInfo)};
  info->EditorControl(editor_id, ECTL_GETINFO, 0, &ei);

  return ei;
}

FarColor FarEditor::convert(const StyledRegion* rd) const
{
  FarColor col = FarColor();

  if (rdBackground == nullptr) {
    return col;
  }

  int fore = (newfore != -1) ? newfore : rdBackground->fore;
  int back = (newback != -1) ? newback : rdBackground->back;

  col.ForegroundColor = fore;
  col.BackgroundColor = back;

  if (rd != nullptr) {
    if (rd->isForeSet) {
      col.ForegroundColor = rd->fore;
    }
    if (rd->isBackSet) {
      col.BackgroundColor = rd->back;
    }

    if (rd->style & StyledRegion::RD_BOLD) {
      col.Flags |= FCF_FG_BOLD;
    }
    if (rd->style & StyledRegion::RD_ITALIC) {
      col.Flags |= FCF_FG_ITALIC;
    }
    if (rd->style & StyledRegion::RD_UNDERLINE) {
      col.Flags |= FCF_FG_U_DATA0;
    }
    if (rd->style & StyledRegion::RD_STRIKEOUT) {
      col.Flags |= FCF_FG_STRIKEOUT;
    }
  }

  if (!TrueMod) {
    col.Flags |= FCF_4BITMASK;
  }
  else {
    col.ForegroundColor = revertRGB(col.ForegroundColor);
    col.BackgroundColor = revertRGB(col.BackgroundColor);
  }

  return col;
}

void FarEditor::deleteFarColor(intptr_t lno, intptr_t s) const
{
  EditorDeleteColor edc {sizeof(EditorDeleteColor), MainGuid, lno, s};
  info->EditorControl(editor_id, ECTL_DELCOLOR, 0, &edc);
}

void FarEditor::addFARColor(intptr_t lno, intptr_t s, intptr_t e, const FarColor& col, EDITORCOLORFLAGS TabMarkStyle) const
{
  EditorColor ec {sizeof(EditorColor), lno, 0, s, e - 1, 0, TabMarkStyle, col, MainGuid};
  MAKE_OPAQUE(ec.Color.BackgroundColor);
  MAKE_OPAQUE(ec.Color.ForegroundColor);
  info->EditorControl(editor_id, ECTL_ADDCOLOR, 0, &ec);
  COLORER_LOG_TRACE("editor:%, line:%, start:%, end:%, color_bg:%, color_fg:%", editor_id, lno, s, e - 1, col.BackgroundColor,
                col.ForegroundColor);
}

const wchar_t* FarEditor::GetMsg(int msg) const
{
  return info->GetMsg(&MainGuid, msg);
}

void FarEditor::cleanEditor()
{
  EditorInfo ei = getEditorInfo();
  for (int i = 0; i < ei.TotalLines; i++) {
    deleteFarColor(i, -1);
  }
}

FarEditor::CROSS_STYLE FarEditor::getVisibleCrossState() const
{
  if (!showHorizontalCross && !showVerticalCross)
    return CROSS_STYLE::CSTYLE_NONE;
  if (showHorizontalCross && showVerticalCross)
    return CROSS_STYLE::CSTYLE_BOTH;
  if (!showHorizontalCross && showVerticalCross)
    return CROSS_STYLE::CSTYLE_VERT;
  if (showHorizontalCross && !showVerticalCross)
    return CROSS_STYLE::CSTYLE_HOR;

  return CROSS_STYLE::CSTYLE_NONE;
}

void FarEditor::changeCrossStyle(FarEditor::CROSS_STYLE newStyle)
{
  switch (newStyle) {
    case CROSS_STYLE::CSTYLE_NONE:
      showHorizontalCross = false;
      showVerticalCross = false;
      break;
    case CROSS_STYLE::CSTYLE_BOTH:
      showHorizontalCross = true;
      showVerticalCross = true;
      break;
    case CROSS_STYLE::CSTYLE_VERT:
      showHorizontalCross = false;
      showVerticalCross = true;
      break;
    case CROSS_STYLE::CSTYLE_HOR:
      showHorizontalCross = true;
      showVerticalCross = false;
      break;
  }
}

int FarEditor::getCrossStatus() const
{
  return crossStatus;
}

int FarEditor::getCrossStyle() const
{
  return crossStyle;
}

void FarEditor::setCrossStatus(int status)
{
  setCrossState(status, crossStyle);
}

void FarEditor::setCrossStyle(int style)
{
  setCrossState(crossStatus, style);
}

Outliner* FarEditor::getFunctionOutliner()
{
  baseEditor->validate(-1, false);
  return structOutliner.get();
}

Outliner* FarEditor::getErrorOutliner()
{
  baseEditor->validate(-1, false);
  return errorOutliner.get();
}

bool FarEditor::isDrawPairs() const
{
  return drawPairs;
}

bool FarEditor::isDrawSyntax() const
{
  return drawSyntax;
}

int FarEditor::getParseProgress()
{
  auto eh = getEditorInfo();
  auto invalid_line = baseEditor->getInvalidLine();

  return (int) (100 * invalid_line / eh.TotalLines);
}

bool FarEditor::isColorerEnable() const
{
  return baseEditor != nullptr;
}

boolean FarEditor::hasWork()
{
  auto invalid_line = baseEditor->getInvalidLine();
  EditorInfo ei = getEditorInfo();
  return ei.TotalLines - invalid_line > 0;
}
