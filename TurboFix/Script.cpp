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
    std::vector<std::string> soundSets;
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
    settings->Load();
    logger.Write(INFO, "Settings loaded");

    TurboFix::LoadConfigs();
    TurboFix::LoadSoundSets();

    playerScriptInst = std::make_shared<CTurboScript>(*settings, configs, soundSets);

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
            settings->Load();
            TurboFix::LoadConfigs();
            TurboFix::LoadSoundSets();
        },
        []() {
            // OnExit
            settings->Save();
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
            npcScriptInsts.push_back(std::make_shared<CTurboScriptNPC>(vehicle, *settings, configs, soundSets));
            auto npcScriptInst = npcScriptInsts.back();

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

void TurboFix::UpdateActiveConfigs() {
    if (playerScriptInst)
        playerScriptInst->UpdateActiveConfig(true);

    for (const auto& inst : npcScriptInsts) {
        inst->UpdateActiveConfig(false);
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

const std::vector<std::string>& TurboFix::GetSoundSets() {
    return soundSets;
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
        TurboFix::UpdateActiveConfigs();
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

    if (configs.empty() ||
        !configs.empty() && configs[0].Name != "default") {
        logger.Write(WARN, "No default config found, generating a default one and saving it...");
        CConfig defaultConfig;
        defaultConfig.Name = "default";
        configs.insert(configs.begin(), defaultConfig);
        defaultConfig.Write();
    }

    logger.Write(INFO, "Configs loaded: %d", configs.size());

    TurboFix::UpdateActiveConfigs();
    return static_cast<unsigned>(configs.size());
}

uint32_t TurboFix::LoadSoundSets() {
    namespace fs = std::filesystem;

    const std::string soundSetsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Sounds";

    logger.Write(DEBUG, "Clearing and reloading sound sets");

    soundSets.clear();

    if (!(fs::exists(fs::path(soundSetsPath)) && fs::is_directory(fs::path(soundSetsPath)))) {
        logger.Write(ERROR, "Directory [%s] not found!", soundSetsPath.c_str());
        return 0;
    }

    for (const auto& dirEntry : fs::directory_iterator(soundSetsPath)) {
        auto path = fs::path(dirEntry);
        if (!fs::is_directory(path)) {
            logger.Write(DEBUG, "Skipping [%s] - not a directory", path.stem().string().c_str());
            continue;
        }

        if (!std::filesystem::exists(path / "EX_POP_0.wav") ||
            !std::filesystem::exists(path / "EX_POP_1.wav") ||
            !std::filesystem::exists(path / "EX_POP_2.wav") ||
            !std::filesystem::exists(path / "EX_POP_SUB.wav")) {
            logger.Write(WARN, "Skipping [%s] - missing a sound file.", path.stem().string().c_str());
            continue;
        }

        soundSets.push_back(fs::path(dirEntry).stem().string());
        logger.Write(DEBUG, "Added sound set [%s]", path.stem().string().c_str());
    }

    logger.Write(DEBUG, "Added sound set [NoSound]");
    soundSets.emplace_back("NoSound");

    logger.Write(INFO, "Sound sets loaded: %d", soundSets.size());

    return static_cast<unsigned>(soundSets.size());
}
