#include "TurboScript.hpp"

#include "Constants.hpp"
#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "Util/Game.hpp"
#include "Util/String.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <filesystem>
#include <algorithm>

CTurboScript::CTurboScript(const std::string& settingsFile)
    : mSettings(settingsFile)
    , mDefaultConfig{}
    , mVehicle(0)
    , mActiveConfig(nullptr) {
    mExt.initOffsets();
    mDefaultConfig.Name = "Default";
}

void CTurboScript::UpdateActiveConfig() {
    if (!Util::VehicleAvailable(mVehicle, PLAYER::PLAYER_PED_ID())) {
        mActiveConfig = nullptr;
        return;
    }

    Hash model = ENTITY::GET_ENTITY_MODEL(mVehicle);
    std::string plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(mVehicle);

    // First pass - match model and plate
    auto foundConfig = std::find_if(mConfigs.begin(), mConfigs.end(), [&](const auto& config) {
        bool modelMatch = std::find(config.Models.begin(), config.Models.end(), model) != config.Models.end();
        bool plateMatch = std::find(config.Plates.begin(), config.Plates.end(), plate) != config.Plates.end();
        return modelMatch && plateMatch;
    });

    // second pass - match model with any plate
    if (foundConfig == mConfigs.end()) {
        foundConfig = std::find_if(mConfigs.begin(), mConfigs.end(), [&](const auto& config) {
            bool modelMatch = std::find(config.Models.begin(), config.Models.end(), model) != config.Models.end();
            bool plateMatch = config.Plates.empty();
            return modelMatch && plateMatch;
        });
    }

    // third pass - use default
    if (foundConfig == mConfigs.end()) {
        mActiveConfig = &mDefaultConfig;
    }
    else {
        mActiveConfig = &*foundConfig;
    }
}

void CTurboScript::Tick() {
    Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);

    // Update active vehicle and config
    if (playerVehicle != mVehicle) {
        mVehicle = playerVehicle;

        UpdateActiveConfig();
    }

    if (mActiveConfig && mSettings.Main.Enable) {
        updateTurbo();
    }
}

unsigned CTurboScript::LoadConfigs() {
    namespace fs = std::filesystem;

    const std::string configsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Configs";

    logger.Write(DEBUG, "Clearing and reloading configs");

    mConfigs.clear();

    if (!(fs::exists(fs::path(configsPath)) && fs::is_directory(fs::path(configsPath)))) {
        logger.Write(ERROR, "Directory [%s] not found!", configsPath.c_str());
        return 0;
    }

    for (auto& file : fs::directory_iterator(configsPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".ini") {
            logger.Write(DEBUG, "Skipping [%d] - not .ini", file.path().c_str());
            continue;
        }

        CConfig config = CConfig::Read(fs::path(file).string());
        if (config.Name == "default") {
            mDefaultConfig = config;
            continue;
        }

        if (config.Models.empty() && config.Plates.empty()) {
            logger.Write(WARN,
                "Vehicle settings file [%s] contained no model names or plates, ignoring it",
                file.path().c_str());
            continue;
        }
        mConfigs.push_back(config);
        logger.Write(DEBUG, "Loaded vehicle config [%s]", config.Name.c_str());
    }
    logger.Write(INFO, "Configs loaded: %d", mConfigs.size());

    return static_cast<unsigned>(mConfigs.size());
}

void CTurboScript::updateTurbo() {
    if (!VEHICLE::IS_TOGGLE_MOD_ON(mVehicle, VehicleToggleModTurbo))
        return;

    float currentBoost = mExt.GetTurbo(mVehicle);

    // closed throttle: vacuum
    // open throttle: boost ~ RPM * throttle

    float rpm = mExt.GetCurrentRPM(mVehicle);
    rpm = map(rpm, 
        mActiveConfig->RPMSpoolStart, 
        mActiveConfig->RPMSpoolEnd,
        0.0f, 
        1.0f);
    rpm = std::clamp(rpm, 0.0f, 1.0f);

    float throttle = abs(mExt.GetThrottle(mVehicle));

    float now = throttle * rpm;
    now = map(now, 0.0f, 1.0f, 
        mActiveConfig->MinBoost, 
        mActiveConfig->MaxBoost);

    float lerpRate;
    if (now > currentBoost)
        lerpRate = mActiveConfig->SpoolRate;
    else
        lerpRate = mActiveConfig->UnspoolRate;

    float newBoost = lerp(currentBoost, now, 1.0f - pow(1.0f - lerpRate, MISC::GET_FRAME_TIME()));
    mExt.SetTurbo(mVehicle, newBoost);
}
