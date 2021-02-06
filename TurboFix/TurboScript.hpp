#pragma once
#include "ScriptSettings.hpp"
#include "Config.hpp"
#include "SoundSet.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <irrKlang.h>
#include <vector>
#include <string>

class CTurboScript {
public:
    CTurboScript(
        CScriptSettings& settings,
        std::vector<CConfig>& configs,
        std::vector<SSoundSet>& soundSets);
    virtual ~CTurboScript();
    virtual void Tick();

    CConfig* ActiveConfig() {
        return mActiveConfig;
    }

    float GetCurrentBoost();

    void UpdateActiveConfig(bool playerCheck);

    int& SoundSetIndex() {
        return mSoundSetIndex;
    }

    Vehicle GetVehicle() {
        return mVehicle;
    }

protected:
    void runPtfx(Vehicle vehicle, bool loud);
    void runSfx(Vehicle vehicle, bool loud);
    float updateAntiLag(float currentBoost, float newBoost, float limBoost);
    void updateDial(float newBoost);
    void updateTurbo();

    const CScriptSettings& mSettings;
    std::vector<CConfig>& mConfigs;
    CConfig mDefaultConfig;

    Vehicle mVehicle;
    CConfig* mActiveConfig;

    int mLastFxTime;
    int mLastLoudTime;

    float mLastThrottle;

    const std::vector<SSoundSet>& mSoundSets;
    int mSoundSetIndex;

    irrklang::ISoundEngine* mSoundEngine;
    std::vector<std::string> mExhaustBones;

    bool mIsNPC;
};
