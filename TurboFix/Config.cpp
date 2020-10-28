#include "Config.hpp"
#include "Constants.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "Util/String.hpp"

#include <simpleini/SimpleIni.h>
#include <fmt/format.h>
#include <filesystem>

#define CHECK_LOG_SI_ERROR(result, operation) \
    if ((result) < 0) { \
        logger.Write(ERROR, "[Config] %s Failed to %s, SI_Error [%d]", \
        __FUNCTION__, operation, result); \
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
    CHECK_LOG_SI_ERROR(result, "load");

    config.Name = std::filesystem::path(configFile).stem().string();

    // [ID]
    std::string modelNamesAll = ini.GetValue("ID", "Models", "");
    std::vector<std::string> modelNames = Util::split(modelNamesAll, ' ');

    for (const auto& modelName : modelNames) {
        config.Models.push_back(Util::joaat(modelName.c_str()));
        config.ModelNames.push_back(modelName);
    }

    std::string platesAll = ini.GetValue("ID", "Plates", "");
    std::vector<std::string> plates = Util::split(platesAll, ' ');

    for (const auto& plate : plates) {
        config.Plates.push_back(plate);
    }

#pragma warning(push)
#pragma warning(disable: 4244)
    // [Turbo]
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
    LOAD_VAL("AntiLag", "Effects", config.AntiLag.Effects);
    LOAD_VAL("AntiLag", "PeriodMs", config.AntiLag.PeriodMs);
    LOAD_VAL("AntiLag", "RandomMs", config.AntiLag.RandomMs);
    LOAD_VAL("AntiLag", "RandomLoudIntervalMs", config.AntiLag.RandomLoudIntervalMs);
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

void CConfig::Write() {
    Write({}, {});
}

bool CConfig::Write(const std::string& newName, const std::string& model) {
    std::string name = newName.empty() ? Name : newName;
    const std::string configsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Configs";
    const std::string configFile = fmt::format("{}\\{}.ini", configsPath, newName);

    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [ID]
    if (!model.empty())
        ini.SetValue("ID", "Models", model.c_str());

#pragma warning(push)
#pragma warning(disable: 4244)
    // [Turbo]
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
    SAVE_VAL("AntiLag", "Effects", AntiLag.Effects);
    SAVE_VAL("AntiLag", "PeriodMs", AntiLag.PeriodMs);
    SAVE_VAL("AntiLag", "RandomMs", AntiLag.RandomMs);
    SAVE_VAL("AntiLag", "RandomLoudIntervalMs", AntiLag.RandomLoudIntervalMs);

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
    CHECK_LOG_SI_ERROR(result, "save");
    if (result < 0)
        return false;
    return true;
}
