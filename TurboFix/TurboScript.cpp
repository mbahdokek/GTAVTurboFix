#include "TurboScript.hpp"

#include "Compatibility.h"
#include "Constants.hpp"
#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Game.hpp"
#include "Util/UI.hpp"
#include "Util/String.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>
#include <filesystem>
#include <algorithm>

using VExt = VehicleExtensions;

CTurboScript::CTurboScript(
    CScriptSettings& settings,
    std::vector<CConfig>& configs,
    std::vector<SSoundSet>& soundSets)
    : mSettings(settings)
    , mConfigs(configs)
    , mDefaultConfig(configs[0])
    , mVehicle(0)
    , mActiveConfig(nullptr)
    , mLastFxTime(0)
    , mLastLoudTime(0)
    , mLastThrottle(0)
    , mSoundSets(soundSets)
    , mSoundSetIndex(0)
    , mIsNPC(false) {

    mSoundEngine = irrklang::createIrrKlangDevice(irrklang::ESOD_DIRECT_SOUND_8);
    mSoundEngine->setDefault3DSoundMinDistance(7.5f);
    mSoundEngine->setSoundVolume(0.20f);

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
    auto foundConfig = std::find_if(mConfigs.begin(), mConfigs.end(), [&](const CConfig& config) {
        bool modelMatch = config.ModelHash == model;
        bool plateMatch = Util::strcmpwi(config.Plate, plate);
        return modelMatch && plateMatch;
    });

    // second pass - match model with any plate
    if (foundConfig == mConfigs.end()) {
        foundConfig = std::find_if(mConfigs.begin(), mConfigs.end(), [&](const CConfig& config) {
            bool modelMatch = config.ModelHash == model;
            bool plateMatch = config.Plate.empty();
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

    if (mActiveConfig->Turbo.ForceTurbo && !VEHICLE::IS_TOGGLE_MOD_ON(mVehicle, VehicleToggleModTurbo)) {
        VEHICLE::TOGGLE_VEHICLE_MOD(mVehicle, VehicleToggleModTurbo, true);
    }

    updateSoundSetIndex(mActiveConfig->AntiLag.SoundSet);
}

void CTurboScript::ApplyConfig(const CConfig& config) {
    if (!mActiveConfig)
        return;

    mActiveConfig->Turbo.ForceTurbo = config.Turbo.ForceTurbo;
    mActiveConfig->Turbo.RPMSpoolStart = config.Turbo.RPMSpoolStart;
    mActiveConfig->Turbo.RPMSpoolEnd = config.Turbo.RPMSpoolEnd;
    mActiveConfig->Turbo.MinBoost = config.Turbo.MinBoost;
    mActiveConfig->Turbo.MaxBoost = config.Turbo.MaxBoost;
    mActiveConfig->Turbo.SpoolRate = config.Turbo.SpoolRate;
    mActiveConfig->Turbo.UnspoolRate = config.Turbo.UnspoolRate;

    mActiveConfig->BoostByGear.Enable = config.BoostByGear.Enable;
    mActiveConfig->BoostByGear.Gear = config.BoostByGear.Gear;

    mActiveConfig->AntiLag.Enable = config.AntiLag.Enable;
    mActiveConfig->AntiLag.MinRPM = config.AntiLag.MinRPM;
    mActiveConfig->AntiLag.Effects = config.AntiLag.Effects;
    mActiveConfig->AntiLag.PeriodMs = config.AntiLag.PeriodMs;
    mActiveConfig->AntiLag.RandomMs = config.AntiLag.RandomMs;

    mActiveConfig->AntiLag.LoudOffThrottle = config.AntiLag.LoudOffThrottle;
    mActiveConfig->AntiLag.LoudOffThrottleIntervalMs = config.AntiLag.LoudOffThrottleIntervalMs;
    mActiveConfig->AntiLag.SoundSet = config.AntiLag.SoundSet;
    mActiveConfig->AntiLag.Volume = config.AntiLag.Volume;

    mActiveConfig->Dial.BoostOffset = config.Dial.BoostOffset;
    mActiveConfig->Dial.BoostScale = config.Dial.BoostScale;
    mActiveConfig->Dial.VacuumOffset = config.Dial.VacuumOffset;
    mActiveConfig->Dial.VacuumScale = config.Dial.VacuumScale;
    mActiveConfig->Dial.BoostIncludesVacuum = config.Dial.BoostIncludesVacuum;

    updateSoundSetIndex(mActiveConfig->AntiLag.SoundSet);
}

void CTurboScript::Tick() {
    Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);

    // Update active vehicle and config
    if (playerVehicle != mVehicle) {
        mVehicle = playerVehicle;

        UpdateActiveConfig(true);
    }

    if (mActiveConfig && Util::VehicleAvailable(mVehicle, PLAYER::PLAYER_PED_ID(), false)) {
        updateTurbo();
    }
}

bool CTurboScript::GetHasTurbo() {
    return VEHICLE::IS_TOGGLE_MOD_ON(mVehicle, VehicleToggleModTurbo);
}

float CTurboScript::GetCurrentBoost() {
    if (mActiveConfig)
        return VExt::GetTurbo(mVehicle);
    return 0.0f;
}

float CTurboScript::updateAntiLag(float currentBoost, float newBoost, float limBoost) {
    float currentThrottle = VExt::GetThrottleP(mVehicle);
    if (abs(VExt::GetThrottleP(mVehicle)) < 0.1f && VExt::GetCurrentRPM(mVehicle) > mActiveConfig->AntiLag.MinRPM) {
        if (mActiveConfig->AntiLag.Effects) {
            int delayMs = mLastFxTime + rand() % mActiveConfig->AntiLag.RandomMs + mActiveConfig->AntiLag.PeriodMs;
            int gameTime = MISC::GET_GAME_TIMER();
            if (gameTime > delayMs) {
                bool loud = false;

                int loudDelayMs = mLastLoudTime + rand() % mActiveConfig->AntiLag.RandomMs +
                    mActiveConfig->AntiLag.LoudOffThrottleIntervalMs;

                // if lifted entirely within 200ms
                if ((mLastThrottle - currentThrottle) / MISC::GET_FRAME_TIME() > 1000.0f / 200.0f ||
                    mActiveConfig->AntiLag.LoudOffThrottle && gameTime > loudDelayMs) {
                    loud = true;
                    mLastLoudTime = gameTime;
                }
                runPtfx(mVehicle, loud);
                runSfx(mVehicle, loud);
                mLastFxTime = gameTime;
            }
        }

        // currentBoost slightly decreases, so use a random mult with slight positive bias
        // TODO: Needs to be framerate-insensitive
        float randMult = map(static_cast<float>(rand() % 101),
            0.0f, 100.0f, 0.990f, 1.025f);
        float alBoost = std::clamp(currentBoost * randMult,
                    mActiveConfig->Turbo.MinBoost,
                    limBoost);

        newBoost = alBoost;
    }

    mLastThrottle = currentThrottle;
    return newBoost;
}

void CTurboScript::updateDial(float newBoost) {
    if (DashHook::Available()) {
        VehicleDashboardData dashData{};
        DashHook::GetData(&dashData);

        if (mActiveConfig->Dial.BoostIncludesVacuum) {
            float boost;
            
            if (newBoost >= 0.0f) {
                boost = map(newBoost, 0.0f, mActiveConfig->Turbo.MaxBoost, 0.0f, 1.0f);
                boost *= mActiveConfig->Dial.BoostScale;
            }
            else {
                boost = map(newBoost, mActiveConfig->Turbo.MinBoost, 0.0f, -1.0f, 0.0f);
                boost *= mActiveConfig->Dial.VacuumScale;
            }

            // Attempt at smoothing boost/vacuum transition, but too much headache to get right.
            // float boostScale = map(newBoost,
            //     mActiveConfig->Turbo.MinBoost, -mActiveConfig->Turbo.MinBoost,
            //     mActiveConfig->DialVacuumScale, mActiveConfig->DialBoostScale);
            // float minScale = std::min(mActiveConfig->DialVacuumScale, mActiveConfig->DialBoostScale);
            // float maxScale = std::max(mActiveConfig->DialVacuumScale, mActiveConfig->DialBoostScale);
            // boostScale = std::clamp(boostScale, minScale, maxScale);
            // boost *= boostScale;

            boost += mActiveConfig->Dial.BoostOffset;
            dashData.boost = boost;
        }
        else {
            float boost = std::clamp(map(newBoost, 0.0f, mActiveConfig->Turbo.MaxBoost, 0.0f, 1.0f), 0.0f, 1.0f);
            boost *= mActiveConfig->Dial.BoostScale; // scale (0.0, 1.0)
            boost += mActiveConfig->Dial.BoostOffset; // Add offset
            dashData.boost = boost;

            float vacuum = std::clamp(map(newBoost, mActiveConfig->Turbo.MinBoost, 0.0f, 0.0f, 1.0f), 0.0f, 1.0f);
            vacuum *= mActiveConfig->Dial.VacuumScale; // scale (0.0, 1.0)
            vacuum += mActiveConfig->Dial.VacuumOffset; // Add offset
            dashData.vacuum = vacuum;
        }

        DashHook::SetData(dashData);
    }
}

void CTurboScript::runPtfx(Vehicle vehicle, bool loud) {
    for (const auto& bone : mExhaustBones) {
        int boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(vehicle, bone.c_str());
        if (boneIdx == -1)
            continue;

        Vector3 bonePos = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(vehicle, boneIdx);
        Vector3 boneRot{};
        if (getGameVersion() >= 50) {
            boneRot = ENTITY::_GET_ENTITY_BONE_ROTATION_LOCAL(vehicle, boneIdx);
        }
        Vector3 boneOff = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(vehicle,
            bonePos.x, bonePos.y, bonePos.z);

        float explSz;
        if (loud) {
            explSz = 1.25f;
        }
        else {
            explSz = map(VExt::GetCurrentRPM(mVehicle),
                mActiveConfig->Turbo.RPMSpoolStart, mActiveConfig->Turbo.RPMSpoolEnd,
                0.75f, 1.25f);
            explSz = std::clamp(explSz, 0.75f, 1.25f);
        }

        GRAPHICS::USE_PARTICLE_FX_ASSET("core");
        GRAPHICS::START_PARTICLE_FX_NON_LOOPED_ON_ENTITY("veh_backfire", vehicle,
            boneOff.x, boneOff.y, boneOff.z, boneRot.x, boneRot.y, boneRot.z, explSz, false, false, false);
    }
}

void CTurboScript::runSfx(Vehicle vehicle, bool loud) {
    Vector3 camPos = CAM::GET_FINAL_RENDERED_CAM_COORD();
    Vector3 camRot = CAM::GET_FINAL_RENDERED_CAM_ROT(0);
    Vector3 camDir = RotationToDirection(camRot);

    // UI::DrawSphere(camPos + camDir * 0.25f, 0.0625f, 255, 0, 0, 255);
    mSoundEngine->setSoundVolume(mActiveConfig->AntiLag.Volume);
    mSoundEngine->setListenerPosition(
        irrklang::vec3df(camPos.x, camPos.y, camPos.z),
        irrklang::vec3df(camDir.x, camDir.y, camDir.z),
        irrklang::vec3df(0, 0, 0),
        irrklang::vec3df(0, 0, -1)
    );

    for (const auto& bone : mExhaustBones) {
        int boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(vehicle, bone.c_str());
        if (boneIdx == -1)
            continue;

        Vector3 bonePos = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(vehicle, boneIdx);
        std::string soundName;
        const std::string soundNameBass = "EX_POP_SUB.wav";
        // UI::DrawSphere(bonePos, 0.125f, 0, 255, 0, 255);

        if (mActiveConfig->AntiLag.SoundSet != "NoSound") {
            if (loud) {
                auto randIndex = rand() % mSoundSets[mSoundSetIndex].EffectCount;
                soundName = fmt::format("EX_POP_{}.wav", randIndex);
            }
            else {
                soundName = soundNameBass;
            }

            std::string soundFinalName =
                fmt::format(R"({}\Sounds\{}\{})", Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
                    Constants::ModDir, mActiveConfig->AntiLag.SoundSet, soundName);
            std::string soundBassFinalName =
                fmt::format(R"({}\Sounds\{}\{})", Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
                    Constants::ModDir, mActiveConfig->AntiLag.SoundSet, soundNameBass);

            if (!soundName.empty())
                mSoundEngine->play3D(soundFinalName.c_str(), { bonePos.x, bonePos.y, bonePos.z });

            mSoundEngine->play3D(soundBassFinalName.c_str(), { bonePos.x, bonePos.y, bonePos.z });

            // Just play on one exhaust.
            break;
        }
    }
}

void CTurboScript::updateTurbo() {
    if (!VEHICLE::IS_TOGGLE_MOD_ON(mVehicle, VehicleToggleModTurbo) ||
        !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(mVehicle)) {
        float currentBoost = VExt::GetTurbo(mVehicle);
        float newBoost = lerp(
            currentBoost, 0.0f, 1.0f - pow(1.0f - mActiveConfig->Turbo.UnspoolRate, MISC::GET_FRAME_TIME()));
        if (!mIsNPC)
            updateDial(newBoost);
        VExt::SetTurbo(mVehicle, newBoost);
        return;
    }

    float currentBoost = VExt::GetTurbo(mVehicle);
    currentBoost = std::clamp(currentBoost,
        mActiveConfig->Turbo.MinBoost,
        mActiveConfig->Turbo.MaxBoost);

    // No throttle:
    //   0.2 RPM -> NA
    //   1.0 RPM -> MinBoost
    //
    // Full throttle:
    //   0.2 RPM to RPMSpoolStart -> NA
    //   RPMSpoolEnd to 1.0 RPM -> MaxBoost

    float boostClosed = map(VExt::GetCurrentRPM(mVehicle), 
        0.2f, 1.0f, 
        0.0f, mActiveConfig->Turbo.MinBoost);
    boostClosed = std::clamp(boostClosed, mActiveConfig->Turbo.MinBoost, 0.0f);

    float boostWOT = map(VExt::GetCurrentRPM(mVehicle), 
        mActiveConfig->Turbo.RPMSpoolStart, mActiveConfig->Turbo.RPMSpoolEnd,
        0.0f, mActiveConfig->Turbo.MaxBoost);
    boostWOT = std::clamp(boostWOT, 0.0f, mActiveConfig->Turbo.MaxBoost);

    float now = map(abs(VExt::GetThrottle(mVehicle)), 
        0.0f, 1.0f, 
        boostClosed, boostWOT);

    float lerpRate;
    if (now > currentBoost)
        lerpRate = mActiveConfig->Turbo.SpoolRate;
    else
        lerpRate = mActiveConfig->Turbo.UnspoolRate;

    float newBoost = lerp(currentBoost, now, 1.0f - pow(1.0f - lerpRate, MISC::GET_FRAME_TIME()));

    float limBoost = mActiveConfig->Turbo.MaxBoost;
    if (!mActiveConfig->BoostByGear.Enable || mActiveConfig->BoostByGear.Gear.empty()) {
        newBoost = std::clamp(newBoost,
            mActiveConfig->Turbo.MinBoost,
            mActiveConfig->Turbo.MaxBoost);
    }
    else {
        auto currentGear = VExt::GetGearCurr(mVehicle);
        auto topBoostKvp = mActiveConfig->BoostByGear.Gear.rbegin();

        // Use 1st gear boost limit for reverse.
        if (currentGear == 0)
            currentGear = 1;

        // Use top gear boost limit when missing in config.
        if (currentGear > topBoostKvp->first)
            currentGear = topBoostKvp->first;

        newBoost = std::clamp(newBoost,
            mActiveConfig->Turbo.MinBoost,
            mActiveConfig->BoostByGear.Gear[currentGear]);

        limBoost = mActiveConfig->BoostByGear.Gear[currentGear];
    }

    if (mActiveConfig->AntiLag.Enable) {
        newBoost = updateAntiLag(currentBoost, newBoost, limBoost);
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

void CTurboScript::updateSoundSetIndex(const std::string& soundSet) {
    auto soundSetIt = std::find_if(mSoundSets.begin(), mSoundSets.end(), [soundSet](const auto& other) {
        return other.Name == soundSet;
    });
    if (soundSetIt != mSoundSets.end()) {
        mSoundSetIndex = static_cast<int>(soundSetIt - mSoundSets.begin());
    }
    else {
        mSoundSetIndex = 0;
    }
}
