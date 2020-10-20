#include "TurboScriptNPC.hpp"

CTurboScriptNPC::CTurboScriptNPC(Vehicle vehicle, CScriptSettings& settings, std::vector<CConfig>& configs)
    : CTurboScript(settings, configs) {
    mIsNPC = true;
    mVehicle = vehicle;
}

void CTurboScriptNPC::Tick() {
    updateTurbo();
}
