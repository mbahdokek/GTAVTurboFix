#include "Compatibility.h"
#include "Util/Logger.hpp"
#include "Util/Paths.hpp"

#include <Windows.h>

namespace DashHook {
    HMODULE g_DashHookModule = nullptr;
    void(*g_DashHook_GetData)(VehicleDashboardData*);
    void(*g_DashHook_SetData)(VehicleDashboardData);
}

template <typename T>
T CheckAddr(HMODULE lib, const std::string& funcName)
{
    FARPROC func = GetProcAddress(lib, funcName.c_str());
    if (!func)
    {
        logger.Write(ERROR, "[Compat] Couldn't get function [%s]", funcName.c_str());
        return nullptr;
    }
    logger.Write(DEBUG, "[Compat] Found function [%s]", funcName.c_str());
    return reinterpret_cast<T>(func);
}

void setupDashHook() {
    logger.Write(INFO, "[Compat] Setting up DashHook");
    const std::string dashHookPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + "\\DashHook.dll";
    DashHook::g_DashHookModule = LoadLibraryA(dashHookPath.c_str());
    if (!DashHook::g_DashHookModule) {
        logger.Write(INFO, "DashHook.dll not found");
        return;
    }

    DashHook::g_DashHook_GetData = CheckAddr<void(*)(VehicleDashboardData*)>(DashHook::g_DashHookModule, "DashHook_GetData");
    DashHook::g_DashHook_SetData = CheckAddr<void(*)(VehicleDashboardData)>(DashHook::g_DashHookModule, "DashHook_SetData");
}

void Compatibility::Setup() {
    setupDashHook();
}

void Compatibility::Release() {
    logger.Write(DEBUG, "[Compat] DashHook.dll FreeLibrary");
    if (FreeLibrary(DashHook::g_DashHookModule)) {
        DashHook::g_DashHookModule = nullptr;
    }
    else {
        logger.Write(ERROR, "[Compat] DashHook.dll FreeLibrary failed [%ul]", GetLastError());
    }
    DashHook::g_DashHook_GetData = nullptr;
    DashHook::g_DashHook_SetData = nullptr;
}

bool DashHook::Available() {
    return g_DashHook_GetData && g_DashHook_SetData;
}

void DashHook::GetData(VehicleDashboardData* data) {
    if (g_DashHook_GetData) {
        g_DashHook_GetData(data);
    }
}

void DashHook::SetData(VehicleDashboardData data) {
    if (g_DashHook_SetData) {
        g_DashHook_SetData(data);
    }
}
