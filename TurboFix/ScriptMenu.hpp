#pragma once
#include "TurboScript.hpp"

#include <menu.h>
#include <string>

class CScriptMenu
{
public:
    CScriptMenu(std::string settingsFile,
        std::function<void()> onInit,
        std::function<void()> onExit);

    void Tick(CTurboScript& turboScript);

private:
    std::string mSettingsFile;
    NativeMenu::Menu mMenu;
};

