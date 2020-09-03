#include "UI.hpp"
#include <inc/natives.h>

namespace {
    int notificationId;
}

void UI::Notify(const std::string& message, int* prevNotification) {
    if (prevNotification != nullptr && *prevNotification != 0) {
        HUD::THEFEED_REMOVE_ITEM(*prevNotification);
    }
    HUD::BEGIN_TEXT_COMMAND_THEFEED_POST("STRING");

    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(message.c_str());

    int id = HUD::END_TEXT_COMMAND_THEFEED_POST_TICKER(false, false);
    if (prevNotification != nullptr) {
        *prevNotification = id;
    }
}

void UI::Notify(const std::string& message, bool removePrevious) {
    int* notifHandleAddr = nullptr;
    if (removePrevious) {
        notifHandleAddr = &notificationId;
    }
    Notify(message, notifHandleAddr);
}

void UI::ShowText(float x, float y, float scale, const std::string& text) {
    HUD::SET_TEXT_FONT(0);
    HUD::SET_TEXT_SCALE(scale, scale);
    HUD::SET_TEXT_COLOUR(255, 255, 255, 255);
    HUD::SET_TEXT_WRAP(0.0, 1.0);
    HUD::SET_TEXT_CENTRE(0);
    HUD::SET_TEXT_OUTLINE();
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
}

std::string UI::GetKeyboardResult() {
    WAIT(1);
    MISC::DISPLAY_ONSCREEN_KEYBOARD(true, "FMMC_KEY_TIP8", "", "", "", "", "", 127);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0)
        WAIT(0);
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify("Cancelled input", true);
        return std::string();
    }
    auto result = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (result == nullptr)
        return std::string();
    return result;
}
