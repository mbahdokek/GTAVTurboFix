#include "TurboFix.h"

#include "Script.hpp"
#include "Util/Math.hpp"

bool TF_Active() {
    if (!TurboFix::GetScript())
        return false;
    auto* activeConfig = TurboFix::GetScript()->ActiveConfig();
    return activeConfig;
}

float TF_GetNormalizedBoost() {
    if (!TurboFix::GetScript())
        return 0.0f;
    auto* activeConfig = TurboFix::GetScript()->ActiveConfig();
    if (activeConfig) {
        return map(TurboFix::GetScript()->GetCurrentBoost(),
            activeConfig->Turbo.MinBoost, activeConfig->Turbo.MaxBoost,
            -1.0f, 1.0f);
    }
    return 0.0f;
}

float TF_GetAbsoluteBoost() {
    if (!TurboFix::GetScript())
        return 0.0f;
    auto* activeConfig = TurboFix::GetScript()->ActiveConfig();
    if (activeConfig) {
        return TurboFix::GetScript()->GetCurrentBoost();
    }
    return 0.0f;
}

float TF_GetAbsoluteBoostMin() {
    if (!TurboFix::GetScript())
        return 0.0f;
    auto* activeConfig = TurboFix::GetScript()->ActiveConfig();
    if (activeConfig) {
        return activeConfig->Turbo.MinBoost;
    }
    return 0.0f;
}

float TF_GetAbsoluteBoostMax() {
    if (!TurboFix::GetScript())
        return 0.0f;
    auto* activeConfig = TurboFix::GetScript()->ActiveConfig();
    if (activeConfig) {
        return activeConfig->Turbo.MaxBoost;
    }
    return 0.0f;
}
