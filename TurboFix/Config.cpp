#include "Config.hpp"
#include "Constants.hpp"
#include "Util/AddonSpawnerCache.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "Util/String.hpp"

#include <simpleini/SimpleIni.h>
#include <fmt/format.h>
#include <filesystem>

#define CHECK_LOG_SI_ERROR(result, operation, file) \
    if ((result) < 0) { \
        logger.Write(ERROR, "[Config] %s Failed to %s, SI_Error [%d]", \
        file, operation, result); \
    }

#define SAVE_VAL(section, key, option) \
    { \
        SetValue(ini, section, key, option); \
    }

#define LOAD_VAL(section, key, option) \
    { \
        option = (GetValue(ini, section, key, option)); \
    }

void SetValue(CSimpleIniA & ini, const char* section, const char* key, int val) {
    ini.SetLongValue(section, key, val);
}

void SetValue(CSimpleIniA & ini, const char* section, const char* key, const std::string & val) {
    ini.SetValue(section, key, val.c_str());
}

void SetValue(CSimpleIniA & ini, const char* section, const char* key, bool val) {
    ini.SetBoolValue(section, key, val);
}

void SetValue(CSimpleIniA & ini, const char* section, const char* key, float val) {
    ini.SetDoubleValue(section, key, static_cast<double>(val));
}

int GetValue(CSimpleIniA & ini, const char* section, const char* key, int val) {
    return ini.GetLongValue(section, key, val);
}

std::string GetValue(CSimpleIniA & ini, const char* section, const char* key, std::string val) {
    return ini.GetValue(section, key, val.c_str());
}

bool GetValue(CSimpleIniA & ini, const char* section, const char* key, bool val) {
    return ini.GetBoolValue(section, key, val);
}

float GetValue(CSimpleIniA & ini, const char* section, const char* key, float val) {
    return static_cast<float>(ini.GetDoubleValue(section, key, val));
}

CConfig CConfig::Read(const std::string& configFile) {
    CConfig config{};

    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load", configFile.c_str());

    config.Name = std::filesystem::path(configFile).stem().string();

    // [ID]
    std::string modelNamesAll = ini.GetValue("ID", "Models", "");

    std::string modelHashStr = ini.GetValue("ID", "ModelHash", "");
    std::string modelName = ini.GetValue("ID", "ModelName", "");

    if (!modelNamesAll.empty() && modelHashStr.empty() && modelName.empty()) {
        config.Legacy = true;
    }

    if (config.Legacy) {
        std::vector<std::string> modelNames = Util::split(modelNamesAll, ' ');

        for (const auto& modelNameLegacy : modelNames) {
            config.ModelHash = Util::joaat(modelNameLegacy.c_str());
            config.ModelName = modelNameLegacy;
            // Only bother with the first one
            break;
        }
    }
    else {
        if (modelHashStr.empty() && modelName.empty()) {
            // This is a no-vehicle config. Nothing to be done.
        }
        else if (modelHashStr.empty()) {
            // This config only has a model name.
            config.ModelHash = Util::joaat(modelName.c_str());
            config.ModelName = modelName;
        }
        else {
            // This config only has a hash.
            Hash modelHash = 0;
            int found = sscanf_s(modelHashStr.c_str(), "%X", &modelHash);

            if (found == 1) {
                config.ModelHash = modelHash;

                auto& asCache = ASCache::Get();
                auto it = asCache.find(modelHash);
                std::string modelName = it == asCache.end() ? std::string() : it->second;
                config.ModelName = modelName;
            }
        }
    }

    std::string platesAll = ini.GetValue("ID", "Plates", "");
    std::string plate = ini.GetValue("ID", "Plate", "");

    if (!platesAll.empty() && plate.empty()) {
        std::vector<std::string> plates = Util::split(platesAll, ' ');

        for (const auto& plate : plates) {
            config.Plate = plate;
            // Only bother with the first one
            break;
        }
    }
    else {
        config.Plate = plate;
    }

#pragma warning(push)
#pragma warning(disable: 4244)
    // [Turbo]
    LOAD_VAL("Turbo", "ForceTurbo", config.Turbo.ForceTurbo);
    LOAD_VAL("Turbo", "RPMSpoolStart", config.Turbo.RPMSpoolStart);
    LOAD_VAL("Turbo", "RPMSpoolEnd", config.Turbo.RPMSpoolEnd);
    LOAD_VAL("Turbo", "MinBoost", config.Turbo.MinBoost);
    LOAD_VAL("Turbo", "MaxBoost", config.Turbo.MaxBoost);
    LOAD_VAL("Turbo", "SpoolRate", config.Turbo.SpoolRate);
    LOAD_VAL("Turbo", "UnspoolRate", config.Turbo.UnspoolRate);

    // [BoostByGear]
    LOAD_VAL("BoostByGear", "Enable", config.BoostByGear.Enable);
    config.BoostByGear.Gear.clear();
    for (int i = 1; i < 11; ++i) {
        double boost = ini.GetDoubleValue("BoostByGear", fmt::format("{}", i).c_str(), -2.0);
        if (boost < -1.0)
            break;
        config.BoostByGear.Gear[i] = boost;
    }

    // [AntiLag]
    LOAD_VAL("AntiLag", "Enable", config.AntiLag.Enable);
    LOAD_VAL("AntiLag", "MinRPM", config.AntiLag.MinRPM);

    LOAD_VAL("AntiLag", "Effects", config.AntiLag.Effects);
    LOAD_VAL("AntiLag", "PeriodMs", config.AntiLag.PeriodMs);
    LOAD_VAL("AntiLag", "RandomMs", config.AntiLag.RandomMs);

    LOAD_VAL("AntiLag", "LoudOffThrottle", config.AntiLag.LoudOffThrottle);
    LOAD_VAL("AntiLag", "LoudOffThrottleIntervalMs", config.AntiLag.LoudOffThrottleIntervalMs);

    LOAD_VAL("AntiLag", "SoundSet", config.AntiLag.SoundSet);
    LOAD_VAL("AntiLag", "Volume", config.AntiLag.Volume);

    // [Dial]
    LOAD_VAL("Dial", "BoostOffset", config.Dial.BoostOffset);
    LOAD_VAL("Dial", "BoostScale", config.Dial.BoostScale);
    LOAD_VAL("Dial", "VacuumOffset", config.Dial.VacuumOffset);
    LOAD_VAL("Dial", "VacuumScale", config.Dial.VacuumScale);
    LOAD_VAL("Dial", "BoostIncludesVacuum", config.Dial.BoostIncludesVacuum);
#pragma warning(pop)

    return config;
}

void CConfig::Write(ESaveType saveType) {
    Write(Name, 0, std::string(), saveType);
}

bool CConfig::Write(const std::string& newName, Hash model, std::string plate, ESaveType saveType) {
    const std::string configsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Configs";
    const std::string configFile = fmt::format("{}\\{}.ini", configsPath, newName);

    CSimpleIniA ini;
    ini.SetUnicode();

    // This here MAY fail on first save, in which case, it can be ignored.
    // _Not_ having this just nukes the entire file, including any comments.
    SI_Error result = ini.LoadFile(configFile.c_str());
    if (result < 0) {
        logger.Write(WARN, "[Config] %s Failed to load, SI_Error [%d]. (No problem if no file exists yet)", 
            configFile.c_str(), result); 
    }

    // [ID]
    if (saveType != ESaveType::GenericNone) {
        if (model != 0) {
            ModelHash = model;
        }

        ini.SetValue("ID", "ModelHash", fmt::format("{:X}", ModelHash).c_str());

        auto& asCache = ASCache::Get();
        auto it = asCache.find(ModelHash);
        std::string modelName = it == asCache.end() ? std::string() : it->second;
        if (!modelName.empty()) {
            ModelName = modelName;
            ini.SetValue("ID", "ModelName", modelName.c_str());
        }

        if (saveType == ESaveType::Specific) {
            Plate = plate;
            ini.SetValue("ID", "Plate", plate.c_str());
        }
    }

#pragma warning(push)
#pragma warning(disable: 4244)
    // [Turbo]
    SAVE_VAL("Turbo", "ForceTurbo", Turbo.ForceTurbo);
    SAVE_VAL("Turbo", "RPMSpoolStart", Turbo.RPMSpoolStart);
    SAVE_VAL("Turbo", "RPMSpoolEnd", Turbo.RPMSpoolEnd);
    SAVE_VAL("Turbo", "MinBoost", Turbo.MinBoost);
    SAVE_VAL("Turbo", "MaxBoost", Turbo.MaxBoost);
    SAVE_VAL("Turbo", "SpoolRate", Turbo.SpoolRate);
    SAVE_VAL("Turbo", "UnspoolRate", Turbo.UnspoolRate);

    // [BoostByGear]
    SAVE_VAL("BoostByGear", "Enable", BoostByGear.Enable);
    for (int i = 1; i <= 10; ++i) {
        ini.DeleteValue("BoostByGear", fmt::format("{}", i).c_str(), nullptr);
    }

    for (const auto& p : BoostByGear.Gear) {
        ini.SetDoubleValue("BoostByGear", fmt::format("{}", p.first).c_str(), p.second);
    }

    // [AntiLag]
    SAVE_VAL("AntiLag", "Enable", AntiLag.Enable);
    SAVE_VAL("AntiLag", "MinRPM", AntiLag.MinRPM);

    SAVE_VAL("AntiLag", "Effects", AntiLag.Effects);
    SAVE_VAL("AntiLag", "PeriodMs", AntiLag.PeriodMs);
    SAVE_VAL("AntiLag", "RandomMs", AntiLag.RandomMs);

    SAVE_VAL("AntiLag", "LoudOffThrottle", AntiLag.LoudOffThrottle);
    SAVE_VAL("AntiLag", "LoudOffThrottleIntervalMs", AntiLag.LoudOffThrottleIntervalMs);

    SAVE_VAL("AntiLag", "SoundSet", AntiLag.SoundSet);
    SAVE_VAL("AntiLag", "Volume", AntiLag.Volume);

    // [Dial]
    SAVE_VAL("Dial", "BoostOffset", Dial.BoostOffset);
    SAVE_VAL("Dial", "BoostScale", Dial.BoostScale);
    SAVE_VAL("Dial", "VacuumOffset", Dial.VacuumOffset);
    SAVE_VAL("Dial", "VacuumScale", Dial.VacuumScale);
    SAVE_VAL("Dial", "BoostIncludesVacuum", Dial.BoostIncludesVacuum);
#pragma warning(pop)

    result = ini.SaveFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save", configFile.c_str());
    if (result < 0)
        return false;
    return true;
}
