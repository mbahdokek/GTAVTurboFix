#include "TurboFix.hpp"
#include "Constants.hpp"

#include "Memory/VehicleExtensions.hpp"
#include "Memory/Versions.hpp"
#include "Util/FileVersion.hpp"
#include "Util/Logger.hpp"
#include "Util/Paths.hpp"

#include <inc/main.h>

#include <windows.h>
#include <Psapi.h>

#include <filesystem>

namespace fs = std::filesystem;

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
            int scriptingVersion = getGameVersion();
            logger.Clear();
            logger.Write(INFO, "Turbo Fix %s (built %s %s)", Constants::DisplayVersion, __DATE__, __TIME__);
            logger.Write(INFO, "Game version " + eGameVersionToString(scriptingVersion));
            if (scriptingVersion < G_VER_1_0_877_1_STEAM) {
                logger.Write(WARN, "Unsupported game version! Update your game.");
            }

            SVersion exeVersion = getExeInfo();
            int actualVersion = findNextLowest(ExeVersionMap, exeVersion);
            if (scriptingVersion % 2) {
                scriptingVersion--;
            }
            if (actualVersion != scriptingVersion) {
                logger.Write(WARN, "Version mismatch!");
                logger.Write(WARN, "    Detected: %s", eGameVersionToString(actualVersion).c_str());
                logger.Write(WARN, "    Reported: %s", eGameVersionToString(scriptingVersion).c_str());
                if (actualVersion == -1) {
                    logger.Write(WARN, "Version detection failed");
                    logger.Write(WARN, "    Using reported version (%s)", eGameVersionToString(scriptingVersion).c_str());
                    actualVersion = scriptingVersion;
                }
                else {
                    logger.Write(WARN, "Using detected version (%s)", eGameVersionToString(actualVersion).c_str());
                }
                VehicleExtensions::ChangeVersion(actualVersion);
            }

            scriptRegister(hInstance, TurboFix::ScriptMain);

            logger.Write(INFO, "Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            scriptUnregister(hInstance);
            break;
        }
        default:
            // Do nothing
            break;
    }
    return TRUE;
}
