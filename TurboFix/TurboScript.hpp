#pragma once
#include "ScriptSettings.hpp"
#include "Config.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <irrKlang.h>
#include <vector>
#include <string>

class CTurboScript {
public:
    CTurboScript(
        CScriptSettings& settings,
        std::vector<CConfig>& configs,
        std::vector<std::string>& soundSets);
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
    void runEffects(Vehicle vehicle, uint32_t popCount, float currentThrottle);
    float updateAntiLag(float currentBoost, float newBoost);
    void updateDial(float newBoost);
    void updateTurbo();

    const CScriptSettings& mSettings;
    std::vector<CConfig>& mConfigs;
    CConfig mDefaultConfig;

    Vehicle mVehicle;
    CConfig* mActiveConfig;

    uint32_t mLastAntilagDelay;
    uint32_t mPopCount;

    float mLastThrottle;

    std::vector<std::string>& mSoundSets;
    int mSoundSetIndex;

    irrklang::ISoundEngine* mSoundEngine;
    std::vector<std::string> mSoundNames;
    std::vector<std::string> mExhaustBones;

    bool mIsNPC;
};
