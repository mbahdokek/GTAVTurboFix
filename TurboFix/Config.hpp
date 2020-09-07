#pragma once
#include <inc/types.h>
#include <string>
#include <vector>

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

    bool AntiLag = false;
    uint32_t BaseLoudCount = 4;

};
