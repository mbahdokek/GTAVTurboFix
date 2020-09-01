#pragma once
#include <DashHook/DashHook.h>
namespace DashHook {
    bool Available();
    void GetData(VehicleDashboardData* data);
    void SetData(VehicleDashboardData data);
}

namespace Compatibility {
    void Setup();
    void Release();
}
