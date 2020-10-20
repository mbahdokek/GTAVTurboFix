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

    uint32_t LoadConfigs();
    const std::vector<CConfig>& GetConfigs();
}
