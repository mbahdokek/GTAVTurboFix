#pragma once
#include <inc/natives.h>
#include <string>

namespace UI {
    void Notify(const std::string& message, int* prevNotification);
    void Notify(const std::string& message, bool removePrevious);

    void ShowText(float x, float y, float scale, const std::string& text);

    std::string GetKeyboardResult();

    void DrawSphere(Vector3 p, float scale, int r, int g, int b, int a);
}
