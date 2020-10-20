#pragma once
#include "TurboScript.hpp"

class CTurboScriptNPC : public CTurboScript {
public:
    CTurboScriptNPC(Vehicle vehicle, CScriptSettings& settings, std::vector<CConfig>& configs);

    void Tick() override;
};
