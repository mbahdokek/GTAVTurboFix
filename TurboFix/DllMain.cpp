#include "Script.hpp"
#include "Constants.hpp"

#include "Memory/Patches.h"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/Versions.hpp"
#include "Util/FileVersion.hpp"
#include "Util/Logger.hpp"
#include "Util/Paths.hpp"

#include <inc/main.h>

#include <windows.h>
#include <Psapi.h>

#include <filesystem>

#include "Compatibility.h"

namespace fs = std::filesystem;

void resolveVersion() {
    int shvVersion = getGameVersion();

    logger.Write(INFO, "SHV API Game version: %s (%d)", eGameVersionToString(shvVersion).c_str(), shvVersion);
    // Also prints the other stuff, annoyingly.
    SVersion exeVersion = getExeInfo();

    if (shvVersion < G_VER_1_0_877_1_STEAM) {
        logger.Write(WARN, "Outdated game version! Update your game.");
    }

    // Version we *explicitly* support
    std::vector<int> exeVersionsSupp = findNextLowest(ExeVersionMap, exeVersion);
    if (exeVersionsSupp.empty() || exeVersionsSupp.size() == 1 && exeVersionsSupp[0] == -1) {
        logger.Write(ERROR, "Failed to find a corresponding game version.");
        logger.Write(WARN, "    Using SHV API version [%s] (%d)",
            eGameVersionToString(shvVersion).c_str(), shvVersion);
        Patches::SetPatterns(/*actualVersion*/);
        return;
    }

    int highestSupportedVersion = *std::max_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion > highestSupportedVersion) {
        logger.Write(WARN, "Game newer than last supported version");
        logger.Write(WARN, "    You might experience instabilities or crashes");
        logger.Write(WARN, "    Using SHV API version [%s] (%d)",
            eGameVersionToString(shvVersion).c_str(), shvVersion);
        Patches::SetPatterns(/*actualVersion*/);
        return;
    }

    int lowestSupportedVersion = *std::min_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion < lowestSupportedVersion) {
        logger.Write(WARN, "SHV API reported lower version than actual EXE version.");
        logger.Write(WARN, "    EXE version     [%s] (%d)",
            eGameVersionToString(lowestSupportedVersion).c_str(), lowestSupportedVersion);
        logger.Write(WARN, "    SHV API version [%s] (%d)",
            eGameVersionToString(shvVersion).c_str(), shvVersion);
        logger.Write(WARN, "    Using EXE version, or highest supported version [%s] (%d)",
            eGameVersionToString(lowestSupportedVersion).c_str(), lowestSupportedVersion);
        Patches::SetPatterns(/*actualVersion*/);
        // Actually need to change stuff
        VehicleExtensions::ChangeVersion(lowestSupportedVersion);
        return;
    }

    logger.Write(INFO, "Using offsets based on SHV API version [%s] (%d)",
        eGameVersionToString(shvVersion).c_str(), shvVersion);
    Patches::SetPatterns(/*actualVersion*/);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    const std::string modPath = Paths::GetModuleFolder(hInstance) + Constants::ModDir;
    const std::string logFile = modPath + "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";

    if (!fs::is_directory(modPath) || !fs::exists(modPath)) {
        fs::create_directory(modPath);
    }

    logger.SetFile(logFile);
    logger.SetMinLevel(DEBUG);
    Paths::SetOurModuleHandle(hInstance);

    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            logger.Clear();
            logger.Write(INFO, "Turbo Fix %s (built %s %s)", Constants::DisplayVersion, __DATE__, __TIME__);

            resolveVersion();

            scriptRegister(hInstance, TurboFix::ScriptMain);

            logger.Write(INFO, "Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            logger.Write(INFO, "[PATCH] Restore patches");
            const uint8_t expected = 1;
            uint8_t actual = 0;

            if (Patches::BoostLimiter(false))
                actual++;

            if (actual == expected) {
                logger.Write(INFO, "[PATCH] Script shut down cleanly");
            }
            else {
                logger.Write(ERROR, "[PATCH] Script shut down with unrestored patches!");
            }

            Compatibility::Release();
            scriptUnregister(hInstance);
            break;
        }
        default:
            // Do nothing
            break;
    }
    return TRUE;
}
