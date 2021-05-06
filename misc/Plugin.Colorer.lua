local ColorerGUID = "D2F36B62-A470-418D-83A3-ED7A3710E5B5";

Macro {
  area = "Editor";
  description = "Colorer: Find Errors";
  key = "/.Alt'/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Errors", "Show")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: List Functions";
  key = "/.Alt;/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Functions", "Show")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: List Functions";
  key = "F5";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Functions", "Show")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: Select Region";
  key = "/.AltK/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Region", "Select")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: List Types";
  key = "/.AltL/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Types", "Menu")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: Find Function";
  key = "/.AltO/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Functions", "Find")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: Select Pair";
  key = "/.AltP/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Brackets", "SelectAll")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: Reload Colorer";
  key = "/.AltR/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Settings", "Reload")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: Match Pair";
  key = "/.Alt\\[/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Brackets", "SelectAll")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: Select Block";
  key = "/.Alt]/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    Plugin.Call(ColorerGUID, "Brackets", "SelectIn")
  end;
}

Macro {
  area = "Editor";
  description = "Colorer: Cross in current editor on/off";
  key = "/.CtrlShiftC/";
  condition = function()
    return Plugin.Exist(ColorerGUID)
  end;
  action = function()
    visible, style = Plugin.Call(ColorerGUID, "Editor", "CrossVisible")
    if (visible == 1) then
      visible = 0
    else
      visible = 1
    end
    Plugin.Call(ColorerGUID, "Editor", "CrossVisible", visible)
  end;
}