#include "TurboScript.hpp"

#include "Compatibility.h"
#include "Constants.hpp"
#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Game.hpp"
#include "Util/UI.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>
#include <filesystem>
#include <algorithm>

using VExt = VehicleExtensions;

CTurboScript::CTurboScript(
    CScriptSettings& settings,
    std::vector<CConfig>& configs,
    std::vector<std::string>& soundSets)
    : mSettings(settings)
    , mConfigs(configs)
    , mDefaultConfig(configs[0])
    , mVehicle(0)
    , mActiveConfig(nullptr)
    , mLastAntilagDelay(0)
    , mPopCount(0)
    , mLastThrottle(0)
    , mSoundSets(soundSets)
    , mSoundSetIndex(0)
    , mIsNPC(false) {

    mSoundEngine = irrklang::createIrrKlangDevice(irrklang::ESOD_DIRECT_SOUND_8);
    mSoundEngine->setDefault3DSoundMinDistance(7.5f);
    mSoundEngine->setSoundVolume(0.20f);

    mSoundNames = {
        "EX_POP_0.wav",
        "EX_POP_1.wav",
        "EX_POP_2.wav",
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

void CTurboScript::UpdateActiveConfig(bool playerCheck) {
    if (playerCheck) {
        if (!Util::VehicleAvailable(mVehicle, PLAYER::PLAYER_PED_ID(), false)) {
            mActiveConfig = nullptr;
            return;
        }
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

        UpdateActiveConfig(true);
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

float CTurboScript::updateAntiLag(float currentBoost, float newBoost) {
    float currentThrottle = VExt::GetThrottleP(mVehicle);
    if (VExt::GetThrottleP(mVehicle) < 0.1f && VExt::GetCurrentRPM(mVehicle) > 0.6f) {
        // 4800 RPM = 80Hz
        //   -> 20 combustion strokes per cylinder per second
        //   -> 50ms between combusions per cylinder -> 10ms average?
        if (MISC::GET_GAME_TIMER() > static_cast<int>(mLastAntilagDelay) + rand() % 50 + 50) {
            if (mActiveConfig->AntiLagEffects)
                runEffects(mVehicle, mPopCount, currentThrottle);

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

        if (mActiveConfig->DialBoostIncludesVacuum) {
            float boost;
            
            if (newBoost >= 0.0f) {
                boost = map(newBoost, 0.0f, mActiveConfig->MaxBoost, 0.0f, 1.0f);
                boost *= mActiveConfig->DialBoostScale;
            }
            else {
                boost = map(newBoost, mActiveConfig->MinBoost, 0.0f, -1.0f, 0.0f);
                boost *= mActiveConfig->DialVacuumScale;
            }

            // Attempt at smoothing boost/vacuum transition, but too much headache to get right.
            // float boostScale = map(newBoost,
            //     mActiveConfig->MinBoost, -mActiveConfig->MinBoost,
            //     mActiveConfig->DialVacuumScale, mActiveConfig->DialBoostScale);
            // float minScale = std::min(mActiveConfig->DialVacuumScale, mActiveConfig->DialBoostScale);
            // float maxScale = std::max(mActiveConfig->DialVacuumScale, mActiveConfig->DialBoostScale);
            // boostScale = std::clamp(boostScale, minScale, maxScale);
            // boost *= boostScale;

            boost += mActiveConfig->DialBoostOffset;
            dashData.boost = boost;
        }
        else {
            float boost = std::clamp(map(newBoost, 0.0f, mActiveConfig->MaxBoost, 0.0f, 1.0f), 0.0f, 1.0f);
            boost *= mActiveConfig->DialBoostScale; // scale (0.0, 1.0)
            boost += mActiveConfig->DialBoostOffset; // Add offset
            dashData.boost = boost;

            float vacuum = std::clamp(map(newBoost, mActiveConfig->MinBoost, 0.0f, 0.0f, 1.0f), 0.0f, 1.0f);
            vacuum *= mActiveConfig->DialVacuumScale; // scale (0.0, 1.0)
            vacuum += mActiveConfig->DialVacuumOffset; // Add offset
            dashData.vacuum = vacuum;
        }

        DashHook::SetData(dashData);
    }
}

void CTurboScript::runEffects(Vehicle vehicle, uint32_t popCount, float currentThrottle) {
    uint32_t maxPopCount = mActiveConfig->AntiLagSoundTicks;

    Vector3 camPos = CAM::GET_FINAL_RENDERED_CAM_COORD();
    Vector3 camRot = CAM::GET_FINAL_RENDERED_CAM_ROT(0);
    Vector3 camDir = RotationToDirection(camRot);

    // UI::DrawSphere(camPos + camDir * 0.25f, 0.0625f, 255, 0, 0, 255);
    mSoundEngine->setSoundVolume(mActiveConfig->AntiLagSoundVolume);
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
        const std::string soundNameBass = "EX_POP_SUB.wav";
        // UI::DrawSphere(bonePos, 0.125f, 0, 255, 0, 255);

        if (loud) {
            explSz = 2.9f;
            auto randIndex = rand() % mSoundNames.size();
            soundName = mSoundNames[randIndex];
        }
        else if (popCount < maxPopCount + rand() % maxPopCount && maxPopCount > 0) {
            explSz = 1.4f;
            soundName = soundNameBass;
        }
        else {
            explSz = 0.9f;
            soundName = std::string();
        }

        if (mActiveConfig->AntiLagSoundSet != "NoSound") {
                std::string soundFinalName =
                fmt::format(R"({}\Sounds\{}\{})", Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
                    Constants::ModDir, mActiveConfig->AntiLagSoundSet, soundName);
            std::string soundBassFinalName =
                fmt::format(R"({}\Sounds\{}\{})", Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
                    Constants::ModDir, mActiveConfig->AntiLagSoundSet, soundNameBass);

            if (!soundName.empty())
                mSoundEngine->play3D(soundFinalName.c_str(), { bonePos.x, bonePos.y, bonePos.z });

            mSoundEngine->play3D(soundBassFinalName.c_str(), { bonePos.x, bonePos.y, bonePos.z });
        }

        GRAPHICS::USE_PARTICLE_FX_ASSET("core");

        Vector3 vehRot = ENTITY::GET_ENTITY_ROTATION(vehicle, 0);
        auto createdPart = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD("veh_backfire",
            bonePos.x, bonePos.y, bonePos.z,
            vehRot.x, vehRot.y, vehRot.z,
            explSz, false, false, false, false);

        GRAPHICS::STOP_PARTICLE_FX_LOOPED(createdPart, 1);
    }
}

void CTurboScript::updateTurbo() {
    if (!VEHICLE::IS_TOGGLE_MOD_ON(mVehicle, VehicleToggleModTurbo) ||
        !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(mVehicle)) {
        float currentBoost = VExt::GetTurbo(mVehicle);
        float newBoost = lerp(currentBoost, 0.0f, 1.0f - pow(1.0f - mActiveConfig->UnspoolRate, MISC::GET_FRAME_TIME()));
        if (!mIsNPC)
            updateDial(newBoost);
        VExt::SetTurbo(mVehicle, newBoost);
        return;
    }

    float currentBoost = VExt::GetTurbo(mVehicle);
    currentBoost = std::clamp(currentBoost,
        mActiveConfig->MinBoost,
        mActiveConfig->MaxBoost);

    // No throttle:
    //   0.2 RPM -> NA
    //   1.0 RPM -> MinBoost
    //
    // Full throttle:
    //   0.2 RPM to RPMSpoolStart -> NA
    //   RPMSpoolEnd to 1.0 RPM -> MaxBoost

    float boostClosed = map(VExt::GetCurrentRPM(mVehicle), 
        0.2f, 1.0f, 
        0.0f, mActiveConfig->MinBoost);
    boostClosed = std::clamp(boostClosed, mActiveConfig->MinBoost, 0.0f);

    float boostWOT = map(VExt::GetCurrentRPM(mVehicle), 
        mActiveConfig->RPMSpoolStart, mActiveConfig->RPMSpoolEnd,
        0.0f, mActiveConfig->MaxBoost);
    boostWOT = std::clamp(boostWOT, 0.0f, mActiveConfig->MaxBoost);

    float now = map(abs(VExt::GetThrottle(mVehicle)), 
        0.0f, 1.0f, 
        boostClosed, boostWOT);

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

    if (!mIsNPC)
        updateDial(newBoost);

    if (mSettings.Debug.NPCDetails) {
        Vector3 loc = ENTITY::GET_ENTITY_COORDS(mVehicle, true);
        loc.z += 1.0f;
        UI::ShowText3D(loc, {
            { fmt::format("Cfg: {}", mActiveConfig->Name) },
            { fmt::format("Boost: {}", newBoost) },
        });
    }

    VExt::SetTurbo(mVehicle, newBoost);
}
