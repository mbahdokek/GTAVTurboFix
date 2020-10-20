#pragma once
#include "TurboScript.hpp"
#include "ScriptMenu.hpp"

namespace TurboFix {
    void ScriptMain();
    void UpdateNPC();
    void UpdatePatch();
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> BuildMenu();

    CScriptSettings& GetSettings();
    CTurboScript* GetScript();
    uint64_t GetNPCScriptCount();
    const std::vector<CConfig>& GetConfigs();
    const std::vector<std::string>& GetSoundSets();

    uint32_t LoadConfigs();
    uint32_t LoadSoundSets();
}
