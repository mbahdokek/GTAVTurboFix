#include "UI.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <vector>

namespace {
    int notificationId;

    float GetStringWidth(const std::string& text, float scale, int font) {
        HUD::BEGIN_TEXT_COMMAND_GET_SCREEN_WIDTH_OF_DISPLAY_TEXT("STRING");
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
        HUD::SET_TEXT_FONT(font);
        HUD::SET_TEXT_SCALE(scale, scale);
        return HUD::END_TEXT_COMMAND_GET_SCREEN_WIDTH_OF_DISPLAY_TEXT(true);
    }
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
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT({ x, y }, 0);
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

void UI::DrawSphere(Vector3 p, float scale, int r, int g, int b, int a) {
    GRAPHICS::DRAW_MARKER(eMarkerType::MarkerTypeDebugSphere,
        p,
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { scale, scale, scale },
        r, g, b, a,
        false, false, 2, false, nullptr, nullptr, false);
}

void UI::ShowText3D(Vector3 location, const std::vector<std::string>& textLines) {
    float height = 0.0125f;

    GRAPHICS::SET_DRAW_ORIGIN(location, 0);
    int i = 0;

    float szX = 0.060f;
    for (const auto& line : textLines) {
        ShowText(0, 0 + height * static_cast<float>(i), 0.2f, line);
        float currWidth = GetStringWidth(line, 0.2f, 0);
        if (currWidth > szX) {
            szX = currWidth;
        }
        i++;
    }

    float szY = (height * static_cast<float>(i)) + 0.02f;
    GRAPHICS::DRAW_RECT({ 0.027f, (height * static_cast<float>(i)) / 2.0f },
        szX, szY,
        0, 0, 0, 92, 0);
    GRAPHICS::CLEAR_DRAW_ORIGIN();
}

