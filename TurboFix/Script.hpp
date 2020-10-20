#pragma once
#include "TurboScript.hpp"
#include "ScriptMenu.hpp"

namespace TurboFix {
    void ScriptMain();
    void UpdateNPC();
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> BuildMenu();

    CTurboScript* GetScript();
    uint64_t GetNPCScriptCount();
}
