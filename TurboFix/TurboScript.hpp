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

    unsigned LoadConfigs();
    void UpdateActiveConfig();

protected:
    void DoExplodyThing(Vehicle c_veh, float explSz, bool loud);
    void updateTurbo();

    CScriptSettings mSettings;
    std::vector<CConfig> mConfigs;
    CConfig mDefaultConfig;

    Vehicle mVehicle;
    CConfig* mActiveConfig;

    irrklang::ISoundEngine* mSoundEngine;
    std::vector<std::string> mSoundNames;
    std::vector<irrklang::ISound*> mSounds;
};
