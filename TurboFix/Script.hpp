#pragma once
#include "TurboScript.hpp"

namespace TurboFix {
    void ScriptMain();
    std::vector<CScriptMenu<CTurboScript>::CSubmenu> BuildMenu();

    CTurboScript& GetScript();
}
