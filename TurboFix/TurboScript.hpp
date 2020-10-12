#pragma once
#include "ScriptSettings.hpp"
#include "Config.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <irrKlang.h>
#include <vector>
#include <string>

class CTurboScript {
public:
    CTurboScript(const std::string& settingsFile);
    ~CTurboScript();
    void Tick();

    CScriptSettings& Settings() {
        return mSettings;
    }

    const std::vector<CConfig>& Configs() {
        return mConfigs;
    }

    CConfig* ActiveConfig() {
        return mActiveConfig;
    }

    float GetCurrentBoost();

    uint32_t LoadSoundSets();
    uint32_t LoadConfigs();
    void UpdateActiveConfig();

    const std::vector<std::string>& GetSoundSets() {
        return mSoundSets;
    }

    int& SoundSetIndex() {
        return mSoundSetIndex;
    }

protected:
    void runEffects(Vehicle vehicle, uint32_t popCount, float currentThrottle);
    float updateAntiLag(float currentBoost, float newBoost);
    void updateDial(float newBoost);
    void updateTurbo();

    CScriptSettings mSettings;
    std::vector<CConfig> mConfigs;
    CConfig mDefaultConfig;

    Vehicle mVehicle;
    CConfig* mActiveConfig;

    uint32_t mLastAntilagDelay;
    uint32_t mPopCount;

    float mLastThrottle;

    std::vector<std::string> mSoundSets;
    int mSoundSetIndex;

    irrklang::ISoundEngine* mSoundEngine;
    std::vector<std::string> mSoundNames;
    std::vector<std::string> mExhaustBones;
};
