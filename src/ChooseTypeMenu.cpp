#include "ChooseTypeMenu.h"
#include <colorer/parsers/FileTypeImpl.h>
#include "FarEditor.h"

ChooseTypeMenu::ChooseTypeMenu(const wchar_t* AutoDetect, const wchar_t* Favorites)
{
  ItemSelected = 0;
  favorite_end_idx = favorite_start_idx;

  SString s;
  s.append(CString("&A ")).append(CString(AutoDetect));
  AddItem(s.getWChars(), 0, nullptr, 0);
  AddItem(Favorites, MIF_SEPARATOR, nullptr, 1);
}

ChooseTypeMenu::~ChooseTypeMenu()
{
  for (auto& idx : menu_item) {
    free((void*) idx.Text);
  }
  menu_item.clear();
}

void ChooseTypeMenu::DeleteItem(size_t index)
{
  free((void*) menu_item[index].Text);
  menu_item.erase(menu_item.begin() + index);
  if (ItemSelected >= index) {
    ItemSelected--;
  }
}

FarMenuItem const* ChooseTypeMenu::getItems() const
{
  return menu_item.data();
}

size_t ChooseTypeMenu::AddItem(const wchar_t* Text, MENUITEMFLAGS Flags, const FileType* UserData, size_t PosAdd)
{
  FarMenuItem new_elem {};
  new_elem.Flags = Flags;
  new_elem.Text = _wcsdup(Text);
  new_elem.UserData = (DWORD_PTR) UserData;
  new_elem.AccelKey.ControlKeyState = 0;
  new_elem.AccelKey.VirtualKeyCode = 0;

  size_t pos;
  if (PosAdd > menu_item.size()) {
    menu_item.push_back(new_elem);
    pos = menu_item.size();
  } else {
    menu_item.insert(menu_item.begin() + PosAdd, new_elem);
    pos = PosAdd;
  }

  if (ItemSelected >= pos) {
    ItemSelected++;
  }
  return pos;
}

size_t ChooseTypeMenu::AddGroup(const wchar_t* Text)
{
  return AddItem(Text, MIF_SEPARATOR, nullptr);
}

size_t ChooseTypeMenu::AddItem(const FileType* fType, size_t PosAdd)
{
  USString s = GenerateName(fType);
  size_t k = AddItem(s->getWChars(), 0, fType, PosAdd);
  return k;
}

void ChooseTypeMenu::SetSelected(size_t index)
{
  if (index < menu_item.size()) {
    menu_item[ItemSelected].Flags &= ~MIF_SELECTED;
    menu_item[index].Flags |= MIF_SELECTED;
    ItemSelected = index;
  }
}

size_t ChooseTypeMenu::GetNext(size_t index) const
{
  size_t p;
  for (p = index + 1; p < menu_item.size(); p++) {
    if (!(menu_item[p].Flags & MIF_SEPARATOR)) {
      break;
    }
  }
  if (p < menu_item.size()) {
    return p;
  } else {
    return favorite_end_idx + 2;
  }
}

FileType* ChooseTypeMenu::GetFileType(size_t index) const
{
  return (FileType*) menu_item[index].UserData;
}

void ChooseTypeMenu::MoveToFavorites(size_t index)
{
  auto* f = (FileTypeImpl*) menu_item[index].UserData;
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
  size_t p = AddItem(fType, favorite_end_idx + 1);
  favorite_end_idx = p;
  return p;
}

void ChooseTypeMenu::HideEmptyGroup()
{
  for (size_t i = favorite_end_idx + 1; i < menu_item.size() - 1; i++) {
    if ((menu_item[i].Flags & MIF_SEPARATOR) && (menu_item[i + 1].Flags & MIF_SEPARATOR)) {
      menu_item[i].Flags |= MIF_HIDDEN;
    }
  }
}

void ChooseTypeMenu::DelFromFavorites(size_t index)
{
  auto* f = (FileTypeImpl*) menu_item[index].UserData;
  DeleteItem(index);
  favorite_end_idx--;
  AddItemInGroup(f);
  if (menu_item[index].Flags & MIF_SEPARATOR) {
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
  for (i = favorite_end_idx + 1;
       i < menu_item.size() && !((menu_item[i].Flags & MIF_SEPARATOR) && (group->compareTo(CString(menu_item[i].Text)) == 0)); i++)
    ;
  if (menu_item[i].Flags & MIF_HIDDEN) {
    menu_item[i].Flags &= ~MIF_HIDDEN;
  }
  size_t k = AddItem(fType, i + 1);
  return k;
}

bool ChooseTypeMenu::IsFavorite(size_t index) const
{
  return (index >= favorite_start_idx) && (index <= favorite_end_idx);
}

void ChooseTypeMenu::RefreshItemCaption(size_t index)
{
  free((void*) menu_item[index].Text);

  USString s = GenerateName(GetFileType(index));
  menu_item[index].Text = _wcsdup(s->getWChars());
}

USString ChooseTypeMenu::GenerateName(const FileType* fType)
{
  const String* v;
  v = ((FileTypeImpl*) fType)->getParamValue(DHotkey);
  auto s = std::make_unique<SString>();
  if (v != nullptr && v->length()) {
    s->append(CString("&")).append(v);
  } else {
    s->append(CString(" "));
  }
  s->append(CString(" ")).append(((FileType*) fType)->getDescription());

  return s;
}