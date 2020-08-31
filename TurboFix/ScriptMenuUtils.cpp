#include "ScriptMenuUtils.h"

#include "Util/UI.hpp"

#include <inc/natives.h>
#include <fmt/format.h>

bool MenuUtils::GetKbFloat(float& val) {
    UI::Notify("Enter value", true);
    MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
        fmt::format("{:f}", val).c_str(), "", "", "", 64);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    std::string floatStr = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (floatStr.empty()) {
        UI::Notify("Cancelled value entry", true);
        return false;
    }

    char* pEnd;
    float parsedValue = strtof(floatStr.c_str(), &pEnd);

    if (parsedValue == 0.0f && *pEnd != 0) {
        UI::Notify("Failed to parse entry.", true);
        return false;
    }

    val = parsedValue;
    return true;
}
