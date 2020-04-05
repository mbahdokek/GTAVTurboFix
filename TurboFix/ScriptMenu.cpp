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

    menu.BoolOption("Enable", context.Settings().Main.Enable,
        { "Enable or disable the entire script." });

    menu.MenuOption("Configs", "configsmenu",
        { "An overview of configurations available." });

    CConfig* activeConfig = context.ActiveConfig();
    menu.MenuOption(fmt::format("Active config: {}", activeConfig ? activeConfig->Name : "None"),
        "editconfigmenu", 
        { "Enter to edit the current configuration." });
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

    menu.FloatOption("RPM Spool Start", config->RPMSpoolStart, 0.0f, 1.0f, 0.01f,
        { "At what RPM the turbo starts spooling up.",
          "0.2 RPM is idle."});

    menu.FloatOption("RPM Spool End", config->RPMSpoolEnd, 0.0f, 1.0f, 0.01f,
        { "At what RPM the turbo spooling rate is maximal.",
          "1.0 RPM is rev limit."});

    menu.FloatOption("Min boost", config->MinBoost, -1.0f, 1.0f, 0.01f, 
        { "What the idle boost/vacuum is." });

    menu.FloatOption("Max boost", config->MaxBoost, -1.0f, 1.0f, 0.01f,
        { "What full boost is." });

    menu.FloatOption("Spool rate", config->SpoolRate, 0.0f, 1.0f, 0.00005f,
        { "How fast the turbo spools up, in part per 1 second.",
          "So 0.5 is it spools up to half its max after 1 second.",
          "0.999 is almost instant. Keep under 1.0." });

    menu.FloatOption("Unspool rate", config->UnspoolRate, 0.0f, 1.0f, 0.00005f,
        { "How fast the turbo slows down. Calculation is same as above." });

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

        if (newName.empty() || newModel.empty()) {
            UI::Notify("No config name or model name entered. Not saving anything.", true);
            return;
        }

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
