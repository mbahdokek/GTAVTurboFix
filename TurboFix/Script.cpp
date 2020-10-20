#include "Script.hpp"

#include "TurboScript.hpp"
#include "TurboScriptNPC.hpp"
#include "ScriptMenu.hpp"
#include "Constants.hpp"
#include "Compatibility.h"

#include "Memory/Patches.h"
#include "Util/Logger.hpp"
#include "Util/Paths.hpp"
#include "Util/String.hpp"

#include <inc/natives.h>
#include <inc/main.h>
#include <fmt/format.h>
#include <memory>
#include <filesystem>

using namespace TurboFix;

namespace {
    std::shared_ptr<CScriptSettings> settings;
    std::shared_ptr<CTurboScript> playerScriptInst;
    std::vector<std::shared_ptr<CTurboScriptNPC>> npcScriptInsts;

    std::vector<CConfig> configs;
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

    settings = std::make_shared<CScriptSettings>(settingsGeneralPath);
    playerScriptInst = std::make_shared<CTurboScript>(*settings, configs);
    settings->Load();
    logger.Write(INFO, "Settings loaded");

    TurboFix::LoadConfigs();
    playerScriptInst->LoadSoundSets();

    if (!Patches::Test()) {
        logger.Write(ERROR, "[PATCH] Test failed");
        Patches::Error = true;
    }
    else {
        Patches::BoostLimiter(settings->Main.Enable);
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
        UpdatePatch();
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
            npcScriptInsts.push_back(std::make_shared<CTurboScriptNPC>(vehicle, *settings, configs));
            auto npcScriptInst = npcScriptInsts.back();

            // TODO: This can be more elegant (regarding logging)
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

void TurboFix::UpdatePatch() {
    if (settings->Main.Enable &&
        (playerScriptInst->ActiveConfig() != nullptr ||
        !npcScriptInsts.empty())) {
        Patches::BoostLimiter(true);
    }
    else {
        Patches::BoostLimiter(false);
    }
}

CScriptSettings& TurboFix::GetSettings() {
    return *settings;
}

CTurboScript* TurboFix::GetScript() {
    return playerScriptInst.get();
}

uint64_t TurboFix::GetNPCScriptCount() {
    return npcScriptInsts.size();
}

const std::vector<CConfig>& TurboFix::GetConfigs() {
    return configs;
}

uint32_t TurboFix::LoadConfigs() {
    namespace fs = std::filesystem;

    const std::string configsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Configs";

    logger.Write(DEBUG, "Clearing and reloading configs");

    configs.clear();

    if (!(fs::exists(fs::path(configsPath)) && fs::is_directory(fs::path(configsPath)))) {
        logger.Write(ERROR, "Directory [%s] not found!", configsPath.c_str());
        return 0;
    }

    for (const auto& file : fs::directory_iterator(configsPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".ini") {
            logger.Write(DEBUG, "Skipping [%s] - not .ini", file.path().c_str());
            continue;
        }

        CConfig config = CConfig::Read(fs::path(file).string());
        if (config.Name == "default") {
            configs.insert(configs.begin(), config);
            continue;
        }

        if (config.Models.empty() && config.Plates.empty()) {
            logger.Write(WARN,
                "Vehicle settings file [%s] contained no model names or plates, ignoring it",
                file.path().c_str());
            continue;
        }
        configs.push_back(config);
        logger.Write(DEBUG, "Loaded vehicle config [%s]", config.Name.c_str());
    }
    logger.Write(INFO, "Configs loaded: %d", configs.size());

    return static_cast<unsigned>(configs.size());
}
