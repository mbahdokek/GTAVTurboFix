#include "Script.hpp"

#include "TurboScript.hpp"
#include "TurboScriptNPC.hpp"
#include "ScriptMenu.hpp"
#include "Constants.hpp"
#include "Compatibility.h"

#include "Memory/Patches.h"
#include "Util/Logger.hpp"
#include "Util/Paths.hpp"

#include <inc/natives.h>
#include <inc/main.h>
#include <fmt/format.h>
#include <memory>

using namespace TurboFix;

namespace {
    std::shared_ptr<CTurboScript> playerScriptInst;
    std::vector<std::shared_ptr<CTurboScriptNPC>> npcScriptInsts;
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
        UpdateNPC();
        WAIT(0);
    }
}

void TurboFix::UpdateNPC() {
    std::vector<std::shared_ptr<CTurboScriptNPC>> instsToDelete;

    std::vector<Vehicle> allVehicles(1024);
    int actualSize = worldGetAllVehicles(allVehicles.data(), 1024);
    allVehicles.resize(actualSize);

    for(const auto& vehicle : allVehicles) {
        if (ENTITY::IS_ENTITY_DEAD(vehicle, 0) ||
            vehicle == playerScriptInst->GetVehicle() ||
            !VEHICLE::IS_TOGGLE_MOD_ON(vehicle, VehicleToggleModTurbo))
            continue;

        auto it = std::find_if(npcScriptInsts.begin(), npcScriptInsts.end(), [vehicle](const auto& inst) {
            return inst->GetVehicle() == vehicle;
        });

        if (it == npcScriptInsts.end()) {
            // TODO: Resolve somewhere else more sensible.
            const std::string settingsGeneralPath =
                Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
                Constants::ModDir +
                "\\settings_general.ini";

            npcScriptInsts.push_back(std::make_shared<CTurboScriptNPC>(vehicle, settingsGeneralPath));
            auto npcScriptInst = npcScriptInsts.back();

            // TODO: This can be more elegant (regarding logging)
            npcScriptInst->Settings().Load();
            npcScriptInst->LoadConfigs();
            npcScriptInst->LoadSoundSets();
            npcScriptInst->UpdateActiveConfig(false);
        }
    }

    for (const auto& inst : npcScriptInsts) {
        if (!ENTITY::DOES_ENTITY_EXIST(inst->GetVehicle()) || 
            ENTITY::IS_ENTITY_DEAD(inst->GetVehicle(), 0) ||
            inst->GetVehicle() == playerScriptInst->GetVehicle() ||
            !VEHICLE::IS_TOGGLE_MOD_ON(inst->GetVehicle(), VehicleToggleModTurbo)) {
            instsToDelete.push_back(inst);
        }
        else {
            inst->Tick();
        }
    }

    for (const auto& inst : instsToDelete) {
        npcScriptInsts.erase(std::remove(npcScriptInsts.begin(), npcScriptInsts.end(), inst), npcScriptInsts.end());
    }
}

CTurboScript* TurboFix::GetScript() {
    return playerScriptInst.get();
}

uint64_t TurboFix::GetNPCScriptCount() {
    return npcScriptInsts.size();
}
