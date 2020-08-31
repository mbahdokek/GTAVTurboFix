#include "ScriptMenu.hpp"
#include "TurboFix.hpp"
#include "TurboScript.hpp"
#include "Constants.hpp"

#include "Memory/Patches.h"

#include "Util/UI.hpp"

#include <fmt/format.h>

std::vector<CScriptMenu<CTurboScript>::CSubmenu> TurboFix::BuildMenu() {
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> submenus;
    /* mainmenu */
    submenus.emplace_back("mainmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Turbo Fix");
        mbCtx.Subtitle(std::string("~b~") + Constants::DisplayVersion);

        if (mbCtx.BoolOption("Enable", context.Settings().Main.Enable,
            { "Enable or disable the entire script." })) {
            Patches::BoostLimiter(context.Settings().Main.Enable);
        }

        mbCtx.MenuOption("Configs", "configsmenu",
            { "An overview of configurations available." });

        CConfig* activeConfig = context.ActiveConfig();
        mbCtx.MenuOption(fmt::format("Active config: {}", activeConfig ? activeConfig->Name : "None"),
            "editconfigmenu",
            { "Enter to edit the current configuration." });
        });

    /* mainmenu -> configsmenu */
    submenus.emplace_back("configsmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Configs");
        mbCtx.Subtitle("Overview");

        if (mbCtx.Option("Reload configs")) {
            context.LoadConfigs();
            context.UpdateActiveConfig();
        }

        for (const auto& config : context.Configs()) {
            bool selected;
            mbCtx.OptionPlus(config.Name, {}, &selected);

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

                mbCtx.OptionPlusPlus(extras);
            }
        }
        });

    /* mainmenu -> editconfigmenu */
    submenus.emplace_back("editconfigmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Config edit");
        CConfig* config = context.ActiveConfig();
        mbCtx.Subtitle(config ? config->Name : "None");

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        mbCtx.FloatOption("RPM Spool Start", config->RPMSpoolStart, 0.0f, 1.0f, 0.01f,
            { "At what RPM the turbo starts spooling up.",
              "0.2 RPM is idle." });

        mbCtx.FloatOption("RPM Spool End", config->RPMSpoolEnd, 0.0f, 1.0f, 0.01f,
            { "At what RPM the turbo spooling rate is maximal.",
              "1.0 RPM is rev limit." });

        mbCtx.FloatOption("Min boost", config->MinBoost, -1.0f, 1.0f, 0.01f,
            { "What the idle boost/vacuum is." });

        mbCtx.FloatOption("Max boost", config->MaxBoost, -1.0f, 1.0f, 0.01f,
            { "What full boost is." });

        mbCtx.FloatOption("Spool rate", config->SpoolRate, 0.01f, 0.99999f, 0.00005f,
            { "How fast the turbo spools up, in part per 1 second.",
              "So 0.5 is it spools up to half its max after 1 second.",
              "0.999 is almost instant. Keep under 1.0." });

        mbCtx.FloatOption("Unspool rate", config->UnspoolRate, 0.01f, 0.99999f, 0.00005f,
            { "How fast the turbo slows down. Calculation is same as above." });

        if (mbCtx.Option("Save changes")) {
            config->Write();
            UI::Notify("Saved changes", true);
            context.LoadConfigs();
            context.UpdateActiveConfig();
        }

        if (mbCtx.Option("Save as...")) {
            UI::Notify("Enter new config name.", true);
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
        });
    return submenus;
}
