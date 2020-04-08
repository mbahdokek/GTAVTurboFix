#pragma once

#include <menu.h>
#include <string>

template <typename T>
class CScriptMenu
{
public:
    class CSubmenu {
    public:
        CSubmenu(std::string name,
            std::function<void(NativeMenu::Menu&, T&)> menuBody)
            : mName(std::move(name))
            , mBody(std::move(menuBody)) { }

        void Update(NativeMenu::Menu& mbCtx, T& scriptContext) {
            if (mbCtx.CurrentMenu(mName))
                mBody(mbCtx, scriptContext);
        }
    private:
        std::string mName;
        std::string mTitle;
        std::string mSubtitle;
        std::function<void(NativeMenu::Menu&, T&)> mBody;
    };

    CScriptMenu(std::string settingsFile,
        std::function<void()> onInit,
        std::function<void()> onExit,
        std::vector<CSubmenu> submenus)
        : mSettingsFile(std::move(settingsFile))
        , mSubmenus(std::move(submenus)) {
        mMenuBase.RegisterOnMain(std::move(onInit));
        mMenuBase.RegisterOnExit(std::move(onExit));
        mMenuBase.SetFiles(mSettingsFile);
        mMenuBase.Initialize();
        mMenuBase.ReadSettings();
    }

    void Tick(T& scriptContext) {
        mMenuBase.CheckKeys();

        for (auto& submenu : mSubmenus) {
            submenu.Update(mMenuBase, scriptContext);
        }

        mMenuBase.EndMenu();
    }

private:
    std::string mSettingsFile;
    NativeMenu::Menu mMenuBase;
    std::vector<CSubmenu> mSubmenus;
};

