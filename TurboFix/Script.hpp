#pragma once
#include "TurboScript.hpp"
#include "ScriptMenu.hpp"

namespace TurboFix {
    void ScriptMain();
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> BuildMenu();

    CTurboScript* GetScript();
}
