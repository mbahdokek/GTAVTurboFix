#include "UI.hpp"
#include <inc/natives.h>

namespace {
    int notificationId;
}

void UI::Notify(const std::string& message, int* prevNotification) {
    if (prevNotification != nullptr && *prevNotification != 0) {
        UI::_REMOVE_NOTIFICATION(*prevNotification);
    }
    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");

    UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char*)message.c_str());

    int id = UI::_DRAW_NOTIFICATION(false, false);
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

std::string UI::GetKeyboardResult() {
    WAIT(1);
    GAMEPLAY::DISPLAY_ONSCREEN_KEYBOARD(true, "FMMC_KEY_TIP8", "", "", "", "", "", 127);
    while (GAMEPLAY::UPDATE_ONSCREEN_KEYBOARD() == 0) 
        WAIT(0);
    if (!GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify("Cancelled input", true);
        return std::string();
    }
    auto result = GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT();
    if (result == nullptr)
        return std::string();
    return result;
}
