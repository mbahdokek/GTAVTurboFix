#include "TurboScriptNPC.hpp"

CTurboScriptNPC::CTurboScriptNPC(
    Vehicle vehicle,
    CScriptSettings& settings,
    std::vector<CConfig>& configs,
    std::vector<SSoundSet>& soundSets)
    : CTurboScript(settings, configs, soundSets) {
    mIsNPC = true;
    mVehicle = vehicle;
}

void CTurboScriptNPC::Tick() {
    updateTurbo();
}
