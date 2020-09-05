#include "TurboScript.hpp"

#include "TurboFix.h"
#include "Compatibility.h"
#include "Constants.hpp"
#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "Util/Game.hpp"
#include "Util/String.hpp"
#include "Memory/Patches.h"

#include <inc/enums.h>
#include <inc/natives.h>
#include <filesystem>
#include <algorithm>

#include "Util/UI.hpp"

using VExt = VehicleExtensions;

CTurboScript::CTurboScript(const std::string& settingsFile)
    : mSettings(settingsFile)
    , mDefaultConfig{}
    , mVehicle(0)
    , mActiveConfig(nullptr) {
    mDefaultConfig.Name = "Default";
    mSoundEngine = irrklang::createIrrKlangDevice();
    mSoundEngine->setDefault3DSoundMinDistance(7.0f);
    mSoundEngine->setSoundVolume(0.5f);

    mSoundNames = {
        // "TurboFix\\Sounds\\BACK_FIRE_POP_1.wav",
        // "TurboFix\\Sounds\\BACK_FIRE_POP_2.wav",
        // "TurboFix\\Sounds\\BACK_FIRE_POP_3.wav",
        // "TurboFix\\Sounds\\EX_POP_5.wav",
        // "TurboFix\\Sounds\\EX_POP_6.wav",
        // "TurboFix\\Sounds\\EX_POP_7.wav",
        // "TurboFix\\Sounds\\EX_POP_8.wav",
        // "TurboFix\\Sounds\\EX_POP_SUB.wav",
        // "TurboFix\\Sounds\\POP.wav",
        // "TurboFix\\Sounds\\POP_00.wav",
        // "TurboFix\\Sounds\\POP_01.wav",
        // "TurboFix\\Sounds\\POP_02.wav",
        // "TurboFix\\Sounds\\POP_03.wav",
        // "TurboFix\\Sounds\\POP_04.wav",
        // "TurboFix\\Sounds\\POP_05.wav",
        // // "TurboFix\\Sounds\\POP_06.wav",
        // // "TurboFix\\Sounds\\POP_07.wav",
        // // "TurboFix\\Sounds\\POP_08.wav",
        // // "TurboFix\\Sounds\\POP_09.wav",
        // // "TurboFix\\Sounds\\POP_10.wav",
        // // "TurboFix\\Sounds\\POP_11.wav",
        // "TurboFix\\Sounds\\POP_12.wav",
        "TurboFix\\Sounds\\CRACKLE_0.wav",
        "TurboFix\\Sounds\\GUNSHOT_0.wav",
        "TurboFix\\Sounds\\GUNSHOT_1.wav",
        "TurboFix\\Sounds\\GUNSHOT_2.wav",
        "TurboFix\\Sounds\\GUNSHOT_3.mp3",
        "TurboFix\\Sounds\\GUNSHOT_4.mp3",
    };

    for (const auto& soundName : mSoundNames) {
        mSounds.push_back(mSoundEngine->play3D(soundName.c_str(), { 0,0,0 }, false, true));
    }
}

CTurboScript::~CTurboScript() {
    //mSoundEngine->drop();
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
        Patches::BoostLimiter(mActiveConfig && Settings().Main.Enable);
    }

    if (mActiveConfig && mSettings.Main.Enable) {
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

long long lastAntilagDelay;
long long lastThrottleLift;

void CTurboScript::DoExplodyThing(Vehicle c_veh, float explSz, bool loud) {
    std::vector<std::string> p_flame_location {
        "exhaust",
        "exhaust_2",
        "exhaust_3",
        "exhaust_4"
    };

    for (const auto& bone : p_flame_location) {
        int boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(c_veh, bone.c_str());
        if (boneIdx == -1)
            continue;

        Vector3 bonePos = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(c_veh, boneIdx);
        //Vector3 vehiclePos = ENTITY::GET_ENTITY_COORDS(c_veh, true);
        // 61 sounds best, no echo or boomy, but still too loud
        //FIRE::ADD_EXPLOSION(bonePos.x, bonePos.y, bonePos.z, 61, 0.0, true, true, 0.0, true);

        //if (loud) {
        //    // OK! But LOUD. And echo-y. At least peds don't run away.
        //    AUDIO::PLAY_SOUND_FROM_COORD(0, "Crate_Land", bonePos.x, bonePos.y, bonePos.z, "FBI_05_SOUNDS", false, 0, 0);
        //}
        //else {
        //
        //    int val = rand() % 3;
        //
        //    if (val == 0) {
        //        // OK as a soft in-between sound. Not when concurrent.
        //        AUDIO::PLAY_SOUND_FROM_COORD(0, "Drop_Case", bonePos.x, bonePos.y, bonePos.z, "JWL_PREP_2A_SOUNDS", false, 0, 0);
        //    }
        //    else if (val == 1) {
        //        // Maybe OK as in-between sound, but too metal and compressed otherwise.
        //        //AUDIO::PLAY_SOUND_FROM_COORD(0, "Grate_Release", bonePos.x, bonePos.y, bonePos.z, "FBI_05_SOUNDS", false, 0, 0);
        //    }
        //    else if (val == 2) {
        //        // Eh, needs more bass
        //        AUDIO::PLAY_SOUND_FROM_COORD(0, "Release_Crate", bonePos.x, bonePos.y, bonePos.z, "FBI_05_SOUNDS", false, 0, 0);
        //    }
        //}

        Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
        Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(0);
        camRot.x = deg2rad(camRot.x);
        camRot.y = deg2rad(camRot.y);
        camRot.z = deg2rad(camRot.z);
        //Vector3 camFwd = CAM::GET_GAME
        //
        //GetOffsetInWorldCoords(camPos, camRot, )

        //bool randEnable = (rand() % 3) == 0;

        auto randIndex = rand() % mSounds.size();

        UI::ShowText(0.5,0.5,1.0,std::to_string(randIndex));
        mSoundEngine->setListenerPosition({ camPos.x, camPos.y, camPos.z }, { camRot.x, camRot.y, camRot.z }, { 0,0,0 }, { 0,0,1 });
        //if (randEnable)

        if (mSounds[randIndex] != nullptr) {
            if (mSounds[randIndex]->isFinished()) {
                mSounds[randIndex] = mSoundEngine->play3D(mSoundNames[randIndex].c_str(), { bonePos.x,bonePos.y,bonePos.z });
            }
            else if (mSounds[randIndex]->getIsPaused()) {
                mSounds[randIndex]->setPosition({ bonePos.x,bonePos.y,bonePos.z });
                mSounds[randIndex]->setIsPaused(false);
            }
        }
        else {
            mSounds[randIndex] = mSoundEngine->play3D(mSoundNames[randIndex].c_str(), { bonePos.x,bonePos.y,bonePos.z });
        }

        //mSoundEngine->play3D(sounds[randIndex].c_str(), { bonePos.x,bonePos.y,bonePos.z });
        //mSoundEngine->play3D("TurboFix\\Sounds\\EX_POP_SUB.wav", { bonePos.x,bonePos.y,bonePos.z });
        
        GRAPHICS::USE_PARTICLE_FX_ASSET("core");
        auto createdPart = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY_BONE("veh_backfire", c_veh, 0.0, 0.0, 0.0, 0.0,
            0.0,
            0.0, boneIdx, explSz, false, false, false);

        GRAPHICS::STOP_PARTICLE_FX_LOOPED(createdPart, 1);
    }
}

int firstBoomCount = 0;

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

    // TODO: First loud/big
    // Following: Less loud/big

    if (VExt::GetThrottleP(mVehicle) < 0.1f && VExt::GetCurrentRPM(mVehicle) > 0.6f)
    {
        // 4800 RPM = 80Hz
        //   -> 20 combustion strokes per cylinder per second
        //   -> 50ms between combusions per cylinder
        if (MISC::GET_GAME_TIMER() > lastAntilagDelay + rand() % 50 + 50)
        {
            float explSz;
            if (firstBoomCount < 12) {
                explSz = 2.4f;
            }
            else {
                explSz = 0.9f;
            }
            DoExplodyThing(mVehicle, explSz, firstBoomCount < 12);

            float boostAdd = mActiveConfig->MaxBoost - currentBoost;
            boostAdd = boostAdd * (static_cast<float>(rand() % 7 + 4)* 0.1f);
            float alBoost = currentBoost + boostAdd;

            newBoost = alBoost;
            newBoost = std::clamp(newBoost,
                mActiveConfig->MinBoost,
                mActiveConfig->MaxBoost);

            lastAntilagDelay = MISC::GET_GAME_TIMER();
            firstBoomCount++;
        }
    }
    else {
        firstBoomCount = 0;
    }

    if (DashHook::Available()) {
        VehicleDashboardData dashData{};
        DashHook::GetData(&dashData);
        float boostNorm = TF_GetNormalizedBoost();
        dashData.boost = std::clamp(boostNorm, 0.0f, 1.0f);
        dashData.vacuum = map(boostNorm, -1.0f, 0.0f, 0.0f, 1.0f);
        dashData.vacuum = std::clamp(dashData.vacuum, 0.0f, 1.0f);
        DashHook::SetData(dashData);
    }

    VExt::SetTurbo(mVehicle, newBoost);
}
