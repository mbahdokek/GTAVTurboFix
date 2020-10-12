#include "Script.hpp"

#include "TurboScript.hpp"
#include "ScriptMenu.hpp"
#include "Constants.hpp"
#include "Compatibility.h"

#include "Memory/Patches.h"

#include "Util/Logger.hpp"
#include "Util/Paths.hpp"

#include <inc/main.h>
#include <memory>

using namespace TurboFix;

namespace {
    std::shared_ptr<CTurboScript> scriptInst;
}

void TurboFix::ScriptMain() {
    const std::string settingsGeneralPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\settings_general.ini";
    const std::string settingsMenuPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\settings_menu.ini";

    scriptInst = std::make_shared<CTurboScript>(settingsGeneralPath);
    CTurboScript& script = *scriptInst;
    script.Settings().Load();
    logger.Write(INFO, "Settings loaded");

    uint32_t configsLoaded = script.LoadConfigs();
    logger.Write(INFO, "%u Configs loaded", configsLoaded);

    uint32_t soundSetsLoaded = script.LoadSoundSets();
    logger.Write(INFO, "%u Sound sets loaded");

    if (!Patches::Test()) {
        logger.Write(ERROR, "[PATCH] Test failed");
        Patches::Error = true;
    }
    else {
        Patches::BoostLimiter(script.Settings().Main.Enable);
    }
    VehicleExtensions::Init();
    Compatibility::Setup();

    CScriptMenu menu(settingsMenuPath, 
        []() {
            // OnInit: Nope
        },
        []() {
            // OnExit: Nope
        },
        BuildMenu()
    );

    while(true) {
        script.Tick();
        menu.Tick(script);
        WAIT(0);
    }
}

CTurboScript* TurboFix::GetScript() {
    return scriptInst.get();
}
