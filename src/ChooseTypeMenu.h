#ifndef _CHOOSE_TYPE_MENU_H_
#define _CHOOSE_TYPE_MENU_H_

#include <vector>
#include "pcolorer.h"

class ChooseTypeMenu
{
public:
  ChooseTypeMenu(const wchar_t* AutoDetect, const wchar_t* Favorites);
  ~ChooseTypeMenu();
  FarMenuItem const* getItems() const;

  size_t getItemsCount() const
  {
    return menu_item.size();
  }

  size_t AddItem(const FileType* fType, size_t PosAdd = 0x7FFFFFFF);
  size_t AddItemInGroup(FileType* fType);
  size_t AddGroup(const wchar_t* Text);
  void SetSelected(size_t index);
  size_t GetNext(size_t index) const;
  FileType* GetFileType(size_t index) const;
  void MoveToFavorites(size_t index);
  size_t AddFavorite(const FileType* fType);
  void DeleteItem(size_t index);

  void HideEmptyGroup();
  void DelFromFavorites(size_t index);
  bool IsFavorite(size_t index) const;
  void RefreshItemCaption(size_t index);
  USString GenerateName(const FileType* fType);

private:
  std::vector<FarMenuItem> menu_item;
  size_t ItemSelected; // Index of selected item

  size_t AddItem(const wchar_t* Text, MENUITEMFLAGS Flags, const FileType* UserData = nullptr, size_t PosAdd = 0x7FFFFFFF);

  static const size_t favorite_start_idx = 1;
  size_t favorite_end_idx;
};


#endif //_CHOOSE_TYPE_MENU_H_