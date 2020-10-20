#pragma once
#include "TurboScript.hpp"

class CTurboScriptNPC : public CTurboScript {
public:
    CTurboScriptNPC(Vehicle vehicle, const std::string& settingsFile);

    void Tick() override;
};
