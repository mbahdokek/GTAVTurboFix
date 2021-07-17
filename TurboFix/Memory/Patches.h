#pragma once
namespace Patches {
    void SetPatterns();
    bool Test();

    bool PatchBoostLimiter(bool enable);
    extern bool Error;
}
