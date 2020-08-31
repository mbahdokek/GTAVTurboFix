#include "Patches.h"

#include "Patcher.h"
#include "PatternInfo.h"

// When disabled, shift-up doesn't trigger.
MemoryPatcher::PatternInfo boostLimiter;
MemoryPatcher::Patcher BoostLimiterPatcher("Boost Limiter", boostLimiter, true);

bool Patches::Error = false;

void Patches::SetPatterns() {
    boostLimiter = MemoryPatcher::PatternInfo("\xC7\x43\x64\x00\x00\x80\x3F", "xxxxxxx",
                                         { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
}

bool Patches::Test() {
    bool success = true;
    success &= 0 != BoostLimiterPatcher.Test();
    return success;
}

bool Patches::BoostLimiter(bool enable) {
    if (Error)
        return false;

    if (enable) {
        if (!BoostLimiterPatcher.Patched()) {
            return BoostLimiterPatcher.Patch();
        }
        else {
            return true;
        }
    }
    else {
        if (BoostLimiterPatcher.Patched()) {
            return BoostLimiterPatcher.Restore();
        }
        else {
            return true;
        }
    }
}
