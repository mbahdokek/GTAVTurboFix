#include "ScriptMenu.hpp"
#include "Constants.hpp"
#include "Util/UI.hpp"

#include <fmt/format.h>
#include <inc/natives.h>

CScriptMenu::CScriptMenu(std::string settingsFile,
                         std::function<void()> onInit,
                         std::function<void()> onExit )
    : mSettingsFile(std::move(settingsFile)) {
    mMenu.RegisterOnMain(std::move(onInit));
    mMenu.RegisterOnExit(std::move(onExit));
    mMenu.SetFiles(mSettingsFile);
    mMenu.Initialize();
    mMenu.ReadSettings();
}

// This is ugly, but it'll have to do for now.

void update_mainmenu(NativeMenu::Menu& menu, CTurboScript& context) {
    menu.Title("Turbo Fix");
    menu.Subtitle(std::string("~b~") + Constants::DisplayVersion);

    menu.BoolOption("Enable", context.Settings().Main.Enable);

    menu.MenuOption("Configs", "configsmenu");

    CConfig* activeConfig = context.ActiveConfig();
    menu.MenuOption(fmt::format("Active config: {}", activeConfig ? activeConfig->Name : "None"), "editconfigmenu");
}

void update_configsmenu(NativeMenu::Menu& menu, CTurboScript& context) {
    menu.Title("Configs");
    menu.Subtitle("Overview");

    if (menu.Option("Reload configs")) {
        context.LoadConfigs();
        context.UpdateActiveConfig();
    }

    for (const auto& config : context.Configs()) {
        bool selected;
        menu.OptionPlus(config.Name, {}, &selected);

        if (selected) {
            std::vector<std::string> extras{
                fmt::format("Models: {}", fmt::join(config.ModelNames, ", ")),
                fmt::format("Plates: {}", fmt::join(config.Plates, ", ")),

                fmt::format("RPM Spool Start: {:.2f}", config.RPMSpoolStart),
                fmt::format("RPM Spool End: {:.2f}", config.RPMSpoolEnd),
                fmt::format("Min boost: {:.2f}", config.MinBoost),
                fmt::format("Max boost: {:.2f}", config.MaxBoost),
                fmt::format("Spool rate: {:.5f}", config.SpoolRate),
                fmt::format("Unspool rate: {:.5f}", config.UnspoolRate),

            };

            menu.OptionPlusPlus(extras);
        }
    }
}

void update_editconfigmenu(NativeMenu::Menu& menu, CTurboScript& context) {
    menu.Title("Config edit");
    CConfig* config = context.ActiveConfig();
    menu.Subtitle(config ? config->Name : "None");

    if (config == nullptr) {
        menu.Option("No active configuration");
        return;
    }

    menu.FloatOption("RPM Spool Start", config->RPMSpoolStart, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("RPM Spool End", config->RPMSpoolEnd, 0.0f, 1.0f, 0.01f);
    menu.FloatOption("Min boost", config->MinBoost, -1.0f, 1.0f, 0.01f);
    menu.FloatOption("Max boost", config->MaxBoost, -1.0f, 1.0f, 0.01f);
    menu.FloatOption("Spool rate", config->SpoolRate, 0.0f, 1.0f, 0.00005f);
    menu.FloatOption("Unspool rate", config->UnspoolRate, 0.0f, 1.0f, 0.00005f);

    if (menu.Option("Save changes")) {
        config->Write();
        UI::Notify("Saved changes", true);
        context.LoadConfigs();
        context.UpdateActiveConfig();
    }

    if (menu.Option("Save as...")) {
        UI::Notify("Enter new config name." ,true);
        std::string newName = UI::GetKeyboardResult();

        UI::Notify("Enter model(s).", true);
        std::string newModel = UI::GetKeyboardResult();

        if (config->Write(newName, newModel))
            UI::Notify("Saved as new configuration", true);
        else
            UI::Notify("Failed to save as new configuration", true);
        context.LoadConfigs();
        context.UpdateActiveConfig();
    }
}

void CScriptMenu::Tick(CTurboScript& turboScript) {
    mMenu.CheckKeys();

    /* mainmenu */
    if (mMenu.CurrentMenu("mainmenu")) { update_mainmenu(mMenu, turboScript); }

    /* mainmenu -> configsmenu */
    if (mMenu.CurrentMenu("configsmenu")) { update_configsmenu(mMenu, turboScript); }

    /* mainmenu -> editconfigmenu */
    if (mMenu.CurrentMenu("editconfigmenu")) { update_editconfigmenu(mMenu, turboScript); }


    mMenu.EndMenu();
}
