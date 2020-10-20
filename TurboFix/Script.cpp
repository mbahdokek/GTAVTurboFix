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
    std::shared_ptr<CTurboScript> playerScriptInst;
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

    playerScriptInst = std::make_shared<CTurboScript>(settingsGeneralPath);
    playerScriptInst->Settings().Load();
    logger.Write(INFO, "Settings loaded");

    playerScriptInst->LoadConfigs();
    playerScriptInst->LoadSoundSets();

    if (!Patches::Test()) {
        logger.Write(ERROR, "[PATCH] Test failed");
        Patches::Error = true;
    }
    else {
        Patches::BoostLimiter(playerScriptInst->Settings().Main.Enable);
    }
    VehicleExtensions::Init();
    Compatibility::Setup();

    CScriptMenu menu(settingsMenuPath, 
        []() {
            // OnInit
            playerScriptInst->LoadSoundSets();
        },
        []() {
            // OnExit: Nope
        },
        BuildMenu()
    );

    while(true) {
        playerScriptInst->Tick();
        menu.Tick(*playerScriptInst);
        WAIT(0);
    }
}

CTurboScript* TurboFix::GetScript() {
    return playerScriptInst.get();
}
