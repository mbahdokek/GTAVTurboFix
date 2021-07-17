#pragma once
#include <string>

class CScriptSettings {
public:
    CScriptSettings(std::string settingsFile);

    void Load();
    void Save();

    struct {

    } Main;

    struct {
        bool NPCDetails = false;
    } Debug;

private:
    std::string mSettingsFile;
};

