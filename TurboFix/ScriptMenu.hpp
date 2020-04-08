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
    class CSubmenu {
    public:
        CSubmenu(NativeMenu::Menu& menuCtx, std::string name, 
            std::function<void(NativeMenu::Menu&, CTurboScript&)> menuBody)
            : mMenuCtx(menuCtx)
            , mName(std::move(name))
            , mBody(std::move(menuBody)) { }

        void Update(CTurboScript& context) {
            if (mMenuCtx.CurrentMenu(mName))
                mBody(mMenuCtx, context);
        }
    private:
        NativeMenu::Menu& mMenuCtx;
        std::string mName;
        std::string mTitle;
        std::string mSubtitle;
        std::function<void(NativeMenu::Menu&, CTurboScript&)> mBody;
        
    };

    std::string mSettingsFile;
    NativeMenu::Menu mMenuBase;
    std::vector<CSubmenu> mSubmenus;
};

