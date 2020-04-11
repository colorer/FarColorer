#include <colorer/parsers/FileTypeImpl.h>
#include "FarEditor.h"
#include "ChooseTypeMenu.h"

ChooseTypeMenu::ChooseTypeMenu(const wchar_t* AutoDetect, const wchar_t* Favorites)
{
  ItemCount = 0;
  Item = nullptr;
  ItemSelected = 0;

  SString s;
  s.append(CString("&A ")).append(CString(AutoDetect));
  AddItem(s.getWChars(), 0, nullptr, 0);
  AddItem(Favorites, MIF_SEPARATOR, nullptr, 1);
}

ChooseTypeMenu::~ChooseTypeMenu()
{
  for (size_t idx = 0; idx < ItemCount; idx++) {
    if (Item[idx].Text) {
      free((void*) Item[idx].Text);
    }
  }
  free(Item);
}

void ChooseTypeMenu::DeleteItem(size_t index)
{
  if (Item[index].Text) {
    free((void*) Item[index].Text);
  }
  memmove(Item + index, Item + index + 1, sizeof(FarMenuItem) * (ItemCount - (index + 1)));
  ItemCount--;
  if (ItemSelected >= index) {
    ItemSelected--;
  }
}

FarMenuItem* ChooseTypeMenu::getItems() const
{
  return Item;
}

size_t ChooseTypeMenu::AddItem(const wchar_t* Text, const MENUITEMFLAGS Flags, const FileType* UserData, size_t PosAdd)
{
  if (PosAdd > ItemCount) {
    PosAdd = ItemCount;
  }

  if (!(ItemCount & 255)) {
    FarMenuItem* NewPtr = static_cast<FarMenuItem*>(realloc(Item, sizeof(FarMenuItem) * (ItemCount + 256 + 1)));
    if (!NewPtr) {
      throw Exception(CString("ChooseTypeMenu: not enough available memory."));
    }

    Item = NewPtr;
  }

  if (PosAdd < ItemCount) {
    memmove(Item + PosAdd + 1, Item + PosAdd, sizeof(FarMenuItem) * (ItemCount - PosAdd));
  }

  ItemCount++;

  Item[PosAdd].Flags = Flags;
  Item[PosAdd].Text = _wcsdup(Text);
  Item[PosAdd].UserData = (DWORD_PTR) UserData;
  ZeroMemory(Item[PosAdd].Reserved, sizeof(Item[PosAdd].Reserved));
  Item[PosAdd].AccelKey.ControlKeyState = 0;
  Item[PosAdd].AccelKey.VirtualKeyCode = 0;

  return PosAdd;
}

size_t ChooseTypeMenu::AddGroup(const wchar_t* Text)
{
  return AddItem(Text, MIF_SEPARATOR, nullptr);
}

size_t ChooseTypeMenu::AddItem(const FileType* fType, size_t PosAdd)
{
  SString* s = GenerateName(fType);
  size_t k = AddItem(s->getWChars(), 0, fType, PosAdd);
  delete s;
  return k;
}

void ChooseTypeMenu::SetSelected(size_t index)
{
  if (index < ItemCount) {
    Item[ItemSelected].Flags &= ~MIF_SELECTED;
    Item[index].Flags |= MIF_SELECTED;
    ItemSelected = index;
  }
}

size_t ChooseTypeMenu::GetNext(size_t index) const
{
  size_t p;
  for (p = index + 1; p < ItemCount; p++) {
    if (!(Item[p].Flags & MIF_SEPARATOR)) {
      break;
    }
  }
  if (p < ItemCount) {
    return p;
  } else {
    for (p = favorite_idx; p < ItemCount && !(Item[p].Flags & MIF_SEPARATOR); p++);
    return p + 1;
  }
}

FileType* ChooseTypeMenu::GetFileType(size_t index) const
{
  return (FileType*)Item[index].UserData;
}

void ChooseTypeMenu::MoveToFavorites(size_t index)
{
  FileTypeImpl* f = (FileTypeImpl*)Item[index].UserData;
  DeleteItem(index);
  size_t k = AddFavorite(f);
  SetSelected(k);
  HideEmptyGroup();
  if (f->getParamValue(DFavorite) == nullptr) {
    f->addParam(&DFavorite);
  }
  f->setParamValue(DFavorite, &DTrue);
}

size_t ChooseTypeMenu::AddFavorite(const FileType* fType)
{
  size_t i;
  for (i = favorite_idx; i < ItemCount && !(Item[i].Flags & MIF_SEPARATOR); i++);
  size_t p = AddItem(fType, i = ItemCount ? i : i + 1);
  if (ItemSelected >= p) {
    ItemSelected++;
  }
  return p;
}

void ChooseTypeMenu::HideEmptyGroup() const
{
  for (size_t i = favorite_idx; i < ItemCount - 1; i++) {
    if ((Item[i].Flags & MIF_SEPARATOR) && (Item[i + 1].Flags & MIF_SEPARATOR)) {
      Item[i].Flags |= MIF_HIDDEN;
    }
  }
}

void ChooseTypeMenu::DelFromFavorites(size_t index)
{
  FileTypeImpl* f = (FileTypeImpl*)Item[index].UserData;
  DeleteItem(index);
  AddItemInGroup(f);
  if (Item[index].Flags & MIF_SEPARATOR) {
    SetSelected(GetNext(index));
  } else {
    SetSelected(index);
  }
  f->setParamValue(DFavorite, &DFalse);
}

size_t ChooseTypeMenu::AddItemInGroup(FileType* fType)
{
  size_t i;
  const String* group = fType->getGroup();
  for (i = favorite_idx; i < ItemCount && !((Item[i].Flags & MIF_SEPARATOR) && (group->compareTo(CString(Item[i].Text)) == 0)); i++);
  if (Item[i].Flags & MIF_HIDDEN) {
    Item[i].Flags &= ~MIF_HIDDEN;
  }
  size_t k = AddItem(fType, i + 1);
  if (ItemSelected >= k) {
    ItemSelected++;
  }
  return k;
}

bool ChooseTypeMenu::IsFavorite(size_t index) const
{
  size_t i;
  for (i = favorite_idx; i < ItemCount && !(Item[i].Flags & MIF_SEPARATOR); i++);
  i = ItemCount ? i : i + 1;
  if (i > index) {
    return true;
  }
  return false;
}

void ChooseTypeMenu::RefreshItemCaption(size_t index)
{
  if (Item[index].Text) {
    free((void*) Item[index].Text);
  }

  SString* s = GenerateName(GetFileType(index));
  Item[index].Text = _wcsdup(s->getWChars());
  delete s;
}

SString* ChooseTypeMenu::GenerateName(const FileType* fType)
{
  const String* v;
  v = ((FileTypeImpl*)fType)->getParamValue(DHotkey);
  SString* s = new SString;
  if (v != nullptr && v->length()) {
    s->append(CString("&")).append(v);
  } else {
    s->append(CString(" "));
  }
  s->append(CString(" ")).append(((FileType*)fType)->getDescription());

  return s;
}