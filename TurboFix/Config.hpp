#pragma once
#include <inc/types.h>
#include <string>
#include <vector>
#include <map>

class CConfig {
public:
    CConfig() = default;
    static CConfig Read(const std::string& configFile);
    void Write();
    bool Write(const std::string& newName, const std::string& model);

    std::string Name;
    std::vector<Hash> Models;

    // just used for menu overview
    std::vector<std::string> ModelNames;
    std::vector<std::string> Plates;

    // Turbo
    struct {
        // RPM where boost may build
        float RPMSpoolStart = 0.2f;

        // RPM where boost may be max
        float RPMSpoolEnd = 0.5f;

        // -1.0f, but -0.8f for pretty
        float MinBoost = -0.8f;

        // 1.0f max
        float MaxBoost = 1.0f;

        // How many % of full boost after 1 second: 0.999f
        float SpoolRate = 0.999f;

        // How many % of no boost after 1 second: 0.97f
        float UnspoolRate = 0.97f;
    } Turbo;

    // BoostByGear
    struct {
        bool Enable;
        std::map<int, float> Gear;
    } BoostByGear;

    // AntiLag
    struct {
        bool Enable = false;

        // Ptfx and Sfx
        bool Effects = true;

        // Delay = PeriodMs + rand() % RandomMs
        int PeriodMs = 50;
        int RandomMs = 150;
        int RandomLoudIntervalMs = 500;

        // "Default", "NoSound" or some custom stuff
        std::string SoundSet = "Default";
        float Volume = 0.25f;
    } AntiLag;

    // DashHook
    struct {
        float BoostOffset = 0.0f;
        float BoostScale = 1.0f;

        float VacuumOffset = 0.0f;
        float VacuumScale = 1.0f;

        bool BoostIncludesVacuum = false;
    } Dial;
};
