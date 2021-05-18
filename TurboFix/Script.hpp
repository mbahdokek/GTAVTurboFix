#pragma once
#include "TurboScript.hpp"
#include "ScriptMenu.hpp"

namespace TurboFix {
    void ScriptMain();
    void ScriptInit();
    void ScriptTick();
    void UpdateNPC();
    void UpdateActiveConfigs();
    void UpdatePatch();
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> BuildMenu();

    CScriptSettings& GetSettings();
    CTurboScript* GetScript();
    uint64_t GetNPCScriptCount();
    const std::vector<CConfig>& GetConfigs();
    const std::vector<SSoundSet>& GetSoundSets();

    uint32_t LoadConfigs();
    uint32_t LoadSoundSets();
}
