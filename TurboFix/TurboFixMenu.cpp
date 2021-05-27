#include "ScriptMenu.hpp"
#include "Script.hpp"
#include "TurboScript.hpp"
#include "Constants.hpp"

#include "Memory/Patches.h"
#include "ScriptMenuUtils.h"

#include "Util/UI.hpp"

#include <fmt/format.h>

std::vector<CScriptMenu<CTurboScript>::CSubmenu> TurboFix::BuildMenu() {
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> submenus;
    /* mainmenu */
    submenus.emplace_back("mainmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Turbo Fix");
        mbCtx.Subtitle(std::string("~b~") + Constants::DisplayVersion);

        if (mbCtx.BoolOption("Enable", TurboFix::GetSettings().Main.Enable,
            { "Enable or disable the entire script." })) {
            Patches::BoostLimiter(TurboFix::GetSettings().Main.Enable);
        }

        CConfig* activeConfig = context.ActiveConfig();
        mbCtx.MenuOption(fmt::format("Active config: {}", activeConfig ? activeConfig->Name : "None"),
            "editconfigmenu",
            { "Enter to edit the current configuration." });

        mbCtx.MenuOption("Configs", "configsmenu",
            { "An overview of configurations available." });

        mbCtx.MenuOption("Developer options", "developermenu");
        });

    /* mainmenu -> configsmenu */
    submenus.emplace_back("configsmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Configs");
        mbCtx.Subtitle("Overview");

        if (mbCtx.Option("Reload configs")) {
            TurboFix::LoadConfigs();
        }

        for (const auto& config : TurboFix::GetConfigs()) {
            bool selected;
            mbCtx.OptionPlus(config.Name, {}, &selected);

            if (selected) {
                std::vector<std::string> extras{
                    fmt::format("Models: {}", fmt::join(config.ModelNames, ", ")),
                    fmt::format("Plates: {}", fmt::join(config.Plates, ", ")),

                    fmt::format("RPM Spool Start: {:.2f}", config.Turbo.RPMSpoolStart),
                    fmt::format("RPM Spool End: {:.2f}", config.Turbo.RPMSpoolEnd),
                    fmt::format("Max boost: {:.2f}", config.Turbo.MaxBoost),
                    fmt::format("Spool rate: {:.5f}", config.Turbo.SpoolRate),
                    fmt::format("Anti-lag: {}",
                        config.AntiLag.Enable ?
                            fmt::format("Yes ({} effects)", config.AntiLag.Effects ? "with" : "without") :
                            "No"
                    )
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

        if (mbCtx.BoolOption("Install turbo", config->Turbo.ForceTurbo,
            { "Automatically install the turbo upgrade on the vehicle, if it doesn't have one already." })) {
            VEHICLE::TOGGLE_VEHICLE_MOD(context.GetVehicle(), VehicleToggleModTurbo, config->Turbo.ForceTurbo);
        }

        mbCtx.FloatOptionCb("RPM Spool Start", config->Turbo.RPMSpoolStart, 0.0f, 1.0f, 0.01f, MenuUtils::GetKbFloat,
            { "At what RPM the turbo starts building boost.",
              "0.2 RPM is idle." });

        mbCtx.FloatOptionCb("RPM Spool End", config->Turbo.RPMSpoolEnd, 0.0f, 1.0f, 0.01f, MenuUtils::GetKbFloat,
            { "At what RPM the turbo boost is maximal.",
              "1.0 RPM is rev limit." });

        mbCtx.FloatOptionCb("Min boost", config->Turbo.MinBoost, -1000000.0f, 0.0f, 0.01f, MenuUtils::GetKbFloat,
            { "What the max vacuum is, e.g. when closing the throttle at high RPM.",
              "Keep this at a similar amplitude to max boost."});

        mbCtx.FloatOptionCb("Max boost", config->Turbo.MaxBoost, 0.0f, 1000000.0f, 0.01f, MenuUtils::GetKbFloat,
            { "What full boost is. A value of 1.0 adds 10% of the current engine power." });

        mbCtx.FloatOptionCb("Spool rate", config->Turbo.SpoolRate, 0.01f, 0.999999f, 0.00005f, MenuUtils::GetKbFloat,
            { "How fast the turbo spools up, in part per 1 second.",
              "So 0.5 is it spools up to half its max after 1 second.",
              "0.999 is almost instant. Keep under 1.0." });

        mbCtx.FloatOptionCb("Unspool rate", config->Turbo.UnspoolRate, 0.01f, 0.999999f, 0.00005f, MenuUtils::GetKbFloat,
            { "How fast the turbo slows down. Calculation is same as above." });

        mbCtx.MenuOption("Anti-lag settings", "antilagsettingsmenu",
            { "Anti-lag keeps the turbo spinning when off-throttle at higher RPMs." });

        mbCtx.MenuOption("Boost by gear", "boostbygearmenu",
            { "Set boost levels for each gear." });

        mbCtx.MenuOption("Dial settings", "dialsettingsmenu", 
            { "Remap the turbo dial on vehicle dashboards.",
              "DashHook and a vehicle with working boost gauge are required for this feature." });

        if (mbCtx.Option("Save changes")) {
            config->Write();
            UI::Notify("Saved changes", true);
            TurboFix::LoadConfigs();
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
            TurboFix::LoadConfigs();
        }
        });

    /* mainmenu -> editconfigmenu -> dialsettingsmenu */
    submenus.emplace_back("dialsettingsmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Dial adjustment");
        CConfig* config = context.ActiveConfig();
        mbCtx.Subtitle(config ? config->Name : "None");

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        mbCtx.FloatOptionCb("Dial offset (boost)", config->Dial.BoostOffset, -10.0f, 10.0f, 0.05f, MenuUtils::GetKbFloat,
            { "Starting offset of the boost dial. Press Enter to manually enter a number." });

        mbCtx.FloatOptionCb("Dial scale (boost)", config->Dial.BoostScale, -10.0f, 10.0f, 0.05f, MenuUtils::GetKbFloat,
            { "Scaling of the boost dial. Press Enter to manually enter a number." });

        mbCtx.FloatOptionCb("Dial offset (vacuum)", config->Dial.VacuumOffset, -10.0f, 10.0f, 0.05f, MenuUtils::GetKbFloat,
            { "Starting offset of the vacuum dial. Press Enter to manually enter a number." });

        mbCtx.FloatOptionCb("Dial scale (vacuum)", config->Dial.VacuumScale, -10.0f, 10.0f, 0.05f, MenuUtils::GetKbFloat,
            { "Scaling of the vacuum dial. Press Enter to manually enter a number." });

        mbCtx.BoolOption("Dial boost includes vacuum", config->Dial.BoostIncludesVacuum,
            { "Remap vacuum data to the boost dial, for combined vacuum and boost dials. Vacuum offset is ignored." });
        });

    /* mainmenu -> editconfigmenu -> antilagsettingsmenu */
    submenus.emplace_back("antilagsettingsmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Anti-lag");
        CConfig* config = context.ActiveConfig();
        mbCtx.Subtitle(config ? config->Name : "None");

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        mbCtx.BoolOption("Enable", config->AntiLag.Enable,
            { "Keeps the turbo spooled up off-throttle." });
        mbCtx.FloatOption("Min RPM", config->AntiLag.MinRPM, 0.2f, 1.0f, 0.05f,
            { "Minimum RPM where anti-lag is active." });

        mbCtx.BoolOption("Effects", config->AntiLag.Effects,
            { "Exhaust pops, bangs and fire." });

        mbCtx.IntOption("Period", config->AntiLag.PeriodMs, 1, 1000, 5,
            { "The minimum time between the effects playing, in milliseconds." });
        mbCtx.IntOption("Randomness", config->AntiLag.RandomMs, 1, 1000, 5,
            { "The random time range between the effects playing, in milliseconds." });

        mbCtx.BoolOption("Off-throttle loud", config->AntiLag.LoudOffThrottle,
            { "Continue the loud pops and bangs after initial throttle lift." });
        mbCtx.IntOption("Off-throttle loud interval", config->AntiLag.LoudOffThrottleIntervalMs, 1, 1000, 5,
            { "The minimum time between the off-throttle loud pops and bangs." });

        std::vector<std::string> soundSetsStr;
        for (const auto& soundset : TurboFix::GetSoundSets()) {
            soundSetsStr.push_back(soundset.Name);
        }
        if (mbCtx.StringArray("Sound set", soundSetsStr, context.SoundSetIndex())) {
            config->AntiLag.SoundSet = TurboFix::GetSoundSets()[context.SoundSetIndex()].Name;
        }
        mbCtx.FloatOptionCb("Volume", config->AntiLag.Volume, 0.0f, 2.0f, 0.05f, MenuUtils::GetKbFloat);
        });

    submenus.emplace_back("boostbygearmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Boost by gear");
        CConfig* config = context.ActiveConfig();
        mbCtx.Subtitle(config ? config->Name : "None");

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        mbCtx.BoolOption("Enable", config->BoostByGear.Enable,
            { "Enables boost by gear, which limits the boost level for each gear." });

        int numGears = config->BoostByGear.Gear.rbegin()->first;
        int oldNum = numGears;
        if (mbCtx.IntOption("Number of gears", numGears, 1, 10, 1,
            { "Number of gears for your map." })) {
            // added so just increase the container size with the new gear and use the highest boost
            if (numGears > oldNum) {
                config->BoostByGear.Gear[numGears] = config->BoostByGear.Gear[oldNum];
            }

            // deleted so pop back?
            if (numGears < oldNum) {
                config->BoostByGear.Gear.erase(config->BoostByGear.Gear.find(oldNum));
            }
        }

        for (int i = 1; i <= numGears; ++i) {
            mbCtx.FloatOptionCb(
                fmt::format("Gear {} boost", i),
                config->BoostByGear.Gear[i],
                0.0f, config->Turbo.MaxBoost,
                0.05f,
                MenuUtils::GetKbFloat);
        }
        });

    /* mainmenu -> developermenu */
    submenus.emplace_back("developermenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Developer options");
        mbCtx.Subtitle("");

        mbCtx.Option(fmt::format("NPC instances: {}", TurboFix::GetNPCScriptCount()));
        mbCtx.BoolOption("NPC Details", TurboFix::GetSettings().Debug.NPCDetails);
        });

    return submenus;
}
