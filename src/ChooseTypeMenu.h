#ifndef _CHOOSE_TYPE_MENU_H_
#define _CHOOSE_TYPE_MENU_H_

#include "pcolorer.h"
#include"FarEditor.h"
#include<colorer/parsers/helpers/FileTypeImpl.h>

class ChooseTypeMenu
{
public:
  ChooseTypeMenu(const wchar_t *AutoDetect,const wchar_t *Favorites);
  ~ChooseTypeMenu();
  FarMenuItem *getItems();
  size_t getItemsCount(){return ItemCount;};

  size_t AddItem(const FileType* fType, size_t PosAdd=0x7FFFFFFF);
  size_t AddItemInGroup(FileType* fType);
  size_t AddGroup(const wchar_t *Text);
  void SetSelected(size_t index);
  size_t GetNext(size_t index);
  FileType* GetFileType(size_t index);
  void MoveToFavorites(size_t index);
  size_t AddFavorite(const FileType* fType);
  void DeleteItem(size_t index);

  void HideEmptyGroup();
  void DelFromFavorites(size_t index);
  bool IsFavorite(size_t index);
  void RefreshItemCaption(size_t index);
  StringBuffer* GenerateName(const FileType* fType);

private:
  size_t ItemCount;
  FarMenuItem *Item;

  size_t ItemSelected; // Index of selected item 

  size_t AddItem(const wchar_t *Text, const MENUITEMFLAGS Flags, const FileType* UserData = NULL, size_t PosAdd=0x7FFFFFFF);

  static const size_t favorite_idx=2;
};


#endif _CHOOSE_TYPE_MENU_H_