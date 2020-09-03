#pragma once
#include <string>

namespace UI {
    void Notify(const std::string& message, int* prevNotification);
    void Notify(const std::string& message, bool removePrevious);

    void ShowText(float x, float y, float scale, const std::string& text);

    std::string GetKeyboardResult();
}
