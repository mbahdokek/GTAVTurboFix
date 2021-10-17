#include "ScriptMenu.hpp"
#include "Script.hpp"
#include "TurboScript.hpp"
#include "Constants.hpp"

#include "Memory/Patches.h"
#include "ScriptMenuUtils.h"

#include "Util/UI.hpp"
#include "Util/Math.hpp"

#include <fmt/format.h>

namespace TurboFix {
    std::vector<std::string> FormatTurboConfig(CTurboScript& context, const CConfig& config);
    bool PromptSave(CTurboScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType);
}

std::vector<CScriptMenu<CTurboScript>::CSubmenu> TurboFix::BuildMenu() {
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> submenus;
    /* mainmenu */
    submenus.emplace_back("mainmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("TurboFix");
        mbCtx.Subtitle(std::string("~b~") + Constants::DisplayVersion);

        Ped playerPed = PLAYER::PLAYER_PED_ID();
        Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

        if (!playerVehicle || !ENTITY::DOES_ENTITY_EXIST(playerVehicle)) {
            mbCtx.Option("No vehicle", { "Get in a vehicle to change its gear stats." });
            mbCtx.MenuOption("Developer options", "developermenu");
            return;
        }

        // activeConfig can always be assumed if in any vehicle.
        CConfig* activeConfig = context.ActiveConfig();
        float currentBoost = context.GetCurrentBoost();
        float currentBoostPercent =
            map(currentBoost,
                activeConfig->Turbo.MinBoost, activeConfig->Turbo.MaxBoost,
                -100.0f, 100.0f);

        std::string turboExtraTitle;
        std::vector<std::string> extra;
        if (!context.GetHasTurbo()) {
            turboExtraTitle = "N/A";
            extra = { "No turbo installed." };
        }
        else {
            turboExtraTitle = activeConfig->Name;
            extra = FormatTurboConfig(context, *activeConfig);
            extra.push_back(fmt::format("Current boost: {:.2f} ({:.0f}%)", currentBoost, currentBoostPercent));
        }

        mbCtx.OptionPlus("Turbo info", extra, nullptr, nullptr, nullptr, turboExtraTitle);

        mbCtx.MenuOption("Edit configuration", "editconfigmenu",
            { "Enter to edit the current configuration." });

        if (mbCtx.MenuOption("Load configuration", "loadmenu", 
            { "Load another configuration into the current config." })) {
            TurboFix::LoadConfigs();
        }

        mbCtx.MenuOption("Save configuration", "savemenu",
            { "Save the current configuration to disk." });

        mbCtx.MenuOption("Developer options", "developermenu");
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

        mbCtx.FloatOptionCb("Falloff RPM", config->Turbo.FalloffRPM, 0.2f, 1.0, 0.01f, MenuUtils::GetKbFloat,
            { "RPM where boost/added power starts falling off.",
              "Only active if higher than 'RPM Spool End', otherwise no falloff happens." });

        mbCtx.FloatOptionCb("Falloff boost", config->Turbo.FalloffBoost, 0.0f, 1000000.0f, 0.01f, MenuUtils::GetKbFloat,
            { "Boost at redline, if falloff is active." });

        mbCtx.MenuOption("Anti-lag settings", "antilagsettingsmenu",
            { "Anti-lag keeps the turbo spinning when off-throttle at higher RPMs." });

        mbCtx.MenuOption("Boost by gear", "boostbygearmenu",
            { "Set boost levels for each gear." });

        mbCtx.MenuOption("Dial settings", "dialsettingsmenu", 
            { "Remap the turbo dial on vehicle dashboards.",
              "DashHook and a vehicle with working boost gauge are required for this feature." });
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

    /* mainmenu -> editconfigmenu -> boostbygearmenu */
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

    /* mainmenu -> loadmenu */
    submenus.emplace_back("loadmenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Load configurations");

        CConfig* config = context.ActiveConfig();
        mbCtx.Subtitle(fmt::format("Current: ",config ? config->Name : "None"));

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        if (TurboFix::GetConfigs().empty()) {
            mbCtx.Option("No saved ratios");
        }

        for (const auto& config : TurboFix::GetConfigs()) {
            bool selected;
            bool triggered = mbCtx.OptionPlus(config.Name, {}, &selected);

            if (selected) {
                mbCtx.OptionPlusPlus(FormatTurboConfig(context, config));
            }

            if (triggered) {
                context.ApplyConfig(config);
                UI::Notify(fmt::format("Applied config {}.", config.Name), true);
            }
        }
    });

    /* mainmenu -> savemenu */
    submenus.emplace_back("savemenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Save configuration");
        mbCtx.Subtitle("");
        auto* config = context.ActiveConfig();

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        Hash model = ENTITY::GET_ENTITY_MODEL(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false));
        const char* plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false));

        if (mbCtx.Option("Save",
            { "Save the current configuration to the current active configuration.",
              fmt::format("Current active configuration: {}.", context.ActiveConfig()->Name) })) {
            config->Write(CConfig::ESaveType::GenericNone); // Don't write model/plate
            TurboFix::LoadConfigs();
            UI::Notify("Saved changes", true);
        }

        if (mbCtx.Option("Save as specific vehicle",
            { "Save current turbo configuration for the current vehicle model and license plate.",
               "Automatically loads for vehicles of this model with this license plate." })) {
            if (PromptSave(context, *config, model, plate, CConfig::ESaveType::Specific))
                TurboFix::LoadConfigs();
        }

        if (mbCtx.Option("Save as generic vehicle",
            { "Save current turbo configuration for the current vehicle model."
                "Automatically loads for any vehicle of this model.",
                "Overridden by license plate config, if present." })) {
            if (PromptSave(context, *config, model, std::string(), CConfig::ESaveType::GenericModel))
                TurboFix::LoadConfigs();
        }

        if (mbCtx.Option("Save as generic",
            { "Save current turbo configuration, but don't make it automatically load for any vehicle." })) {
            if (PromptSave(context, *config, 0, std::string(), CConfig::ESaveType::GenericNone))
                TurboFix::LoadConfigs();
        }
    });

    /* mainmenu -> developermenu */
    submenus.emplace_back("developermenu", [](NativeMenu::Menu& mbCtx, CTurboScript& context) {
        mbCtx.Title("Developer options");
        mbCtx.Subtitle("");

        mbCtx.Option(fmt::format("NPC instances: {}", TurboFix::GetNPCScriptCount()),
            { "TurboFix works for all NPC vehicles with the turbo upgrade installed.",
              "This is the number of vehicles the script is working for." });
        mbCtx.BoolOption("NPC Details", TurboFix::GetSettings().Debug.NPCDetails);
    });

    return submenus;
}

std::vector<std::string> TurboFix::FormatTurboConfig(CTurboScript& context, const CConfig& config) {
    // https://itstillruns.com/horsepower-vs-boost-pressure-10009983.html
    // a turbo will increase horsepower by about 7 percent per pound of boost over a naturally aspirated configuration
    // 1.0 boost = +10% torque, so 1.0 boost = 1 psi?
    //  (approx {:.1f} bar/{:.1f} psi)
    // , config.Turbo.MaxBoost / 2.0f, (config.Turbo.MaxBoost / 2.0f) * 14.5038f

    std::string antilagExtra;
    if (config.AntiLag.Enable) {
        if (config.AntiLag.Effects) {
            antilagExtra = fmt::format("Yes (Soundset: {} @ {:.2f})",
                config.AntiLag.SoundSet, config.AntiLag.Volume);
        }
        else {
            antilagExtra = "Yes (no effects)";
        }
    }
    else {
        antilagExtra = "No";
    }

    std::vector<std::string> extras{
        fmt::format("Name: {}", config.Name),
        fmt::format("Model: {}", config.ModelName.empty() ? "None (Generic)" : config.ModelName),
        fmt::format("Plate: {}", config.Plate.empty() ? "None" : fmt::format("[{}]", config.Plate)),
        "",
        fmt::format("RPM Spool Start: {:.2f}", config.Turbo.RPMSpoolStart),
        fmt::format("RPM Spool End: {:.2f}", config.Turbo.RPMSpoolEnd),
        fmt::format("Max boost: {:.2f}", config.Turbo.MaxBoost),
        fmt::format("Spool rate: {:.5f}", config.Turbo.SpoolRate),
        fmt::format("Anti-lag: {}", antilagExtra),
        fmt::format("Boost by gear: {}", config.BoostByGear.Enable ? "Yes" : "No")
    };

    return extras;
}

bool TurboFix::PromptSave(CTurboScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType) {
    UI::Notify("Enter new config name.", true);
    std::string newName = UI::GetKeyboardResult();

    if (newName.empty()) {
        UI::Notify("No config name entered. Not saving anything.", true);
        return false;
    }

    if (config.Write(newName, model, plate, saveType))
        UI::Notify("Saved as new configuration", true);
    else
        UI::Notify("Failed to save as new configuration", true);
    TurboFix::LoadConfigs();

    return true;
}
