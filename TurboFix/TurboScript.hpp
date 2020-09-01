#pragma once
#include "ScriptSettings.hpp"
#include "Config.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <vector>
#include <string>
#include "ScriptMenu.hpp"

class CTurboScript {
public:
    CTurboScript(const std::string& settingsFile);

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
    void updateTurbo();

    CScriptSettings mSettings;
    std::vector<CConfig> mConfigs;
    CConfig mDefaultConfig;

    Vehicle mVehicle;
    CConfig* mActiveConfig;
};
