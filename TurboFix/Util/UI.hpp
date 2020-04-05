#pragma once
#include <string>

namespace UI {
    void Notify(const std::string& message, int* prevNotification);
    void Notify(const std::string& message, bool removePrevious);

    std::string GetKeyboardResult();
}
