#include "TurboScript.hpp"

#include "TurboFix.h"
#include "Compatibility.h"
#include "Constants.hpp"
#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "Util/Game.hpp"
#include "Util/String.hpp"
#include "Util/UI.hpp"
#include "Memory/Patches.h"

#include <inc/enums.h>
#include <inc/natives.h>
#include <filesystem>
#include <algorithm>

using VExt = VehicleExtensions;

CTurboScript::CTurboScript(const std::string& settingsFile)
    : mSettings(settingsFile)
    , mDefaultConfig{}
    , mVehicle(0)
    , mActiveConfig(nullptr)
    , mLastAntilagDelay(0) {
    mDefaultConfig.Name = "Default";
    mSoundEngine = irrklang::createIrrKlangDevice(irrklang::ESOD_DIRECT_SOUND_8);
    mSoundEngine->setDefault3DSoundMinDistance(7.5f);
    mSoundEngine->setSoundVolume(0.20f);

    for (uint32_t i = 0; i < 10; ++i) {
        
    }

    mSoundNames = {
        "TurboFix\\Sounds\\GUNSHOT_0.wav",
        "TurboFix\\Sounds\\GUNSHOT_1.wav",
        "TurboFix\\Sounds\\GUNSHOT_2.wav",
    };

    mExhaustBones = {
        "exhaust",
        "exhaust_2",
        "exhaust_3",
        "exhaust_4",
        "exhaust_5",
        "exhaust_6",
        "exhaust_7",
        "exhaust_8",
        "exhaust_9",
        "exhaust_10",
        "exhaust_11",
        "exhaust_12",
        "exhaust_13",
        "exhaust_14",
        "exhaust_15",
        "exhaust_16",
    };
}

CTurboScript::~CTurboScript() = default;

void CTurboScript::UpdateActiveConfig() {
    if (!Util::VehicleAvailable(mVehicle, PLAYER::PLAYER_PED_ID(), false)) {
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
        Patches::BoostLimiter(mActiveConfig && Settings().Main.Enable);
    }

    if (mActiveConfig && Util::VehicleAvailable(mVehicle, PLAYER::PLAYER_PED_ID(), false) && mSettings.Main.Enable) {
        updateTurbo();
    }
}

float CTurboScript::GetCurrentBoost() {
    if (mActiveConfig)
        return VExt::GetTurbo(mVehicle);
    return 0.0f;
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

float CTurboScript::updateAntiLag(float currentBoost, float newBoost) {
    float currentThrottle = VExt::GetThrottleP(mVehicle);
    if (VExt::GetThrottleP(mVehicle) < 0.1f && VExt::GetCurrentRPM(mVehicle) > 0.6f) {
        // 4800 RPM = 80Hz
        //   -> 20 combustion strokes per cylinder per second
        //   -> 50ms between combusions per cylinder -> 10ms average?
        if (MISC::GET_GAME_TIMER() > static_cast<int>(mLastAntilagDelay) + rand() % 50 + 50) {
            runPtfxAudio(mVehicle, mPopCount, currentThrottle);

            float boostAdd = mActiveConfig->MaxBoost - currentBoost;
            boostAdd = boostAdd * (static_cast<float>(rand() % 7 + 4) * 0.1f);
            float alBoost = currentBoost + boostAdd;

            newBoost = alBoost;
            newBoost = std::clamp(newBoost,
                mActiveConfig->MinBoost,
                mActiveConfig->MaxBoost);

            mLastAntilagDelay = MISC::GET_GAME_TIMER();
            mPopCount++;
        }
    }
    else {
        mPopCount = 0;
    }

    mLastThrottle = currentThrottle;
    return newBoost;
}

void CTurboScript::updateDial(float newBoost) {
    if (DashHook::Available()) {
        VehicleDashboardData dashData{};
        DashHook::GetData(&dashData);
        float boostNorm = newBoost;

        if (mActiveConfig->DialBoostIncludesVacuum) {
            float boost = boostNorm;
            boost += mActiveConfig->DialBoostOffset; // Add offset

            if (boostNorm >= 0.0f) {
                boost *= mActiveConfig->DialBoostScale;
            }
            else {
                boost *= mActiveConfig->DialVacuumScale;
            }
            dashData.boost = boost;
        }
        else {
            float boost = std::clamp(boostNorm, 0.0f, 1.0f);
            boost *= mActiveConfig->DialBoostScale; // scale (0.0, 1.0)
            boost += mActiveConfig->DialBoostOffset; // Add offset
            dashData.boost = boost;
        }

        float vacuum = std::clamp(map(boostNorm, -1.0f, 0.0f, 0.0f, 1.0f), 0.0f, 1.0f);
        vacuum *= mActiveConfig->DialVacuumScale; // scale (0.0, 1.0)
        vacuum -= mActiveConfig->DialVacuumOffset; // Add offset
        dashData.vacuum = vacuum;

        DashHook::SetData(dashData);
    }
}

void CTurboScript::runPtfxAudio(Vehicle vehicle, uint32_t popCount, float currentThrottle) {
    uint32_t maxPopCount = mActiveConfig->BaseLoudCount;
    Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
    Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(0);
    Vector3 camDir = RotationToDirection(camRot);

    // UI::DrawSphere(camPos + camDir, 0.125f, 255, 0, 0, 255);

    mSoundEngine->setListenerPosition(
        irrklang::vec3df( camPos.x, camPos.y, camPos.z ),
        irrklang::vec3df( camDir.x, camDir.y, camDir.z ),
        irrklang::vec3df( 0, 0, 0 ),
        irrklang::vec3df( 0, 0, -1 )
    );

    bool loud = false;
    // if lifted entirely within 200ms
    if ((mLastThrottle - currentThrottle) / MISC::GET_FRAME_TIME() > 1000.0f / 200.0f) {
        loud = true;
    }

    for (const auto& bone : mExhaustBones) {
        int boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(vehicle, bone.c_str());
        if (boneIdx == -1)
            continue;

        Vector3 bonePos = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(vehicle, boneIdx);
        float explSz;
        std::string soundName;

        // UI::DrawSphere(bonePos, 0.125f, 0, 255, 0, 255);

        if (loud) {
            explSz = 2.9f;
            auto randIndex = rand() % mSoundNames.size();
            soundName = mSoundNames[randIndex];
        }
        else if (popCount < maxPopCount + rand() % maxPopCount && maxPopCount > 0) {
            explSz = 1.4f;
            soundName = "TurboFix\\Sounds\\EX_POP_SUB.wav";
        }
        else {
            explSz = 0.9f;
            soundName = std::string();
        }
        if (!soundName.empty())
            auto* soundBackfire = mSoundEngine->play3D(soundName.c_str(), { bonePos.x, bonePos.y, bonePos.z });
        auto* soundBass = mSoundEngine->play3D("TurboFix\\Sounds\\EX_POP_SUB.wav", { bonePos.x, bonePos.y, bonePos.z });

        GRAPHICS::USE_PARTICLE_FX_ASSET("core");
        auto createdPart = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY_BONE("veh_backfire", vehicle, 0.0, 0.0, 0.0, 0.0,
            0.0,
            0.0, boneIdx, explSz, false, false, false);

        GRAPHICS::STOP_PARTICLE_FX_LOOPED(createdPart, 1);
    }
}

void CTurboScript::updateTurbo() {
    if (!VEHICLE::IS_TOGGLE_MOD_ON(mVehicle, VehicleToggleModTurbo))
        return;

    float currentBoost = VExt::GetTurbo(mVehicle);
    currentBoost = std::clamp(currentBoost,
        mActiveConfig->MinBoost,
        mActiveConfig->MaxBoost);

    // closed throttle: vacuum
    // open throttle: boost ~ RPM * throttle

    float rpm = VExt::GetCurrentRPM(mVehicle);
    rpm = map(rpm, 
        mActiveConfig->RPMSpoolStart, 
        mActiveConfig->RPMSpoolEnd,
        0.0f, 
        1.0f);
    rpm = std::clamp(rpm, 0.0f, 1.0f);

    float throttle = abs(VExt::GetThrottle(mVehicle));

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
    newBoost = std::clamp(newBoost, 
        mActiveConfig->MinBoost,
        mActiveConfig->MaxBoost);

    if (mActiveConfig->AntiLag) {
        newBoost = updateAntiLag(currentBoost, newBoost);
    }

    updateDial(TF_GetNormalizedBoost());
    VExt::SetTurbo(mVehicle, newBoost);
}
