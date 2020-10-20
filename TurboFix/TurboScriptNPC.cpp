#include "TurboScriptNPC.hpp"

CTurboScriptNPC::CTurboScriptNPC(Vehicle vehicle, const std::string& settingsFile)
    : CTurboScript(settingsFile) {
    mIsNPC = true;
    mVehicle = vehicle;
}

void CTurboScriptNPC::Tick() {
    updateTurbo();
}
