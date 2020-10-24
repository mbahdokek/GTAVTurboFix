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
    config.RPMSpoolStart = ini.GetDoubleValue("Turbo", "RPMSpoolStart", config.RPMSpoolStart);
    config.RPMSpoolEnd = ini.GetDoubleValue("Turbo", "RPMSpoolEnd", config.RPMSpoolEnd);
    config.MinBoost = ini.GetDoubleValue("Turbo", "MinBoost", config.MinBoost);
    config.MaxBoost = ini.GetDoubleValue("Turbo", "MaxBoost", config.MaxBoost);
    config.SpoolRate = ini.GetDoubleValue("Turbo", "SpoolRate", config.SpoolRate);
    config.UnspoolRate = ini.GetDoubleValue("Turbo", "UnspoolRate", config.UnspoolRate);

    // [BoostByGear]
    config.BoostByGearEnable = ini.GetBoolValue("BoostByGear", "Enable", config.BoostByGearEnable);
    config.BoostByGear.clear();
    for (int i = 1; i < 11; ++i) {
        double boost = ini.GetDoubleValue("BoostByGear", fmt::format("{}", i).c_str(), -2.0);
        if (boost < -1.0)
            break;
        config.BoostByGear[i] = boost;
    }

    // [AntiLag]
    config.AntiLag = ini.GetBoolValue("AntiLag", "AntiLag", config.AntiLag);
    config.AntiLagEffects = ini.GetBoolValue("AntiLag", "Effects", config.AntiLagEffects);
    config.AntiLagSoundSet = ini.GetValue("AntiLag", "SoundSet", config.AntiLagSoundSet.c_str());
    config.AntiLagSoundTicks = ini.GetLongValue("AntiLag", "SoundFrequency", config.AntiLagSoundTicks);
    config.AntiLagSoundVolume = ini.GetDoubleValue("AntiLag", "SoundVolume", config.AntiLagSoundVolume);

    // [Dial]
    config.DialBoostOffset = ini.GetDoubleValue("Dial", "BoostOffset", config.DialBoostOffset);
    config.DialBoostScale = ini.GetDoubleValue("Dial", "BoostScale", config.DialBoostScale);
    config.DialVacuumOffset = ini.GetDoubleValue("Dial", "VacuumOffset", config.DialVacuumOffset);
    config.DialVacuumScale = ini.GetDoubleValue("Dial", "VacuumScale", config.DialVacuumScale);
    config.DialBoostIncludesVacuum = ini.GetBoolValue("Dial", "BoostIncludesVacuum", config.DialBoostIncludesVacuum);
#pragma warning(pop)

    return config;
}

void CConfig::Write() {
    const std::string configsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Configs";
    const std::string configFile = fmt::format("{}\\{}.ini", configsPath, Name);

    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

#pragma warning(push)
#pragma warning(disable: 4244)
    // [Turbo]
    ini.SetDoubleValue("Turbo", "RPMSpoolStart", RPMSpoolStart);
    ini.SetDoubleValue("Turbo", "RPMSpoolEnd", RPMSpoolEnd);
    ini.SetDoubleValue("Turbo", "MinBoost", MinBoost);
    ini.SetDoubleValue("Turbo", "MaxBoost", MaxBoost);
    ini.SetDoubleValue("Turbo", "SpoolRate", SpoolRate);
    ini.SetDoubleValue("Turbo", "UnspoolRate", UnspoolRate);

    // [BoostByGear]
    ini.SetBoolValue("BoostByGear", "Enable", BoostByGearEnable);
    for (int i = 1; i <= 10; ++i) {
        ini.DeleteValue("BoostByGear", fmt::format("{}", i).c_str(), nullptr);
    }

    for (const auto& p : BoostByGear) {
        ini.SetDoubleValue("BoostByGear", fmt::format("{}", p.first).c_str(), p.second);
    }

    // [AntiLag]
    ini.SetBoolValue("AntiLag", "AntiLag", AntiLag);
    ini.SetBoolValue("AntiLag", "Effects", AntiLagEffects);
    ini.SetValue("AntiLag", "SoundSet", AntiLagSoundSet.c_str());
    ini.SetLongValue("AntiLag", "SoundFrequency", AntiLagSoundTicks);
    ini.SetDoubleValue("AntiLag", "SoundVolume", AntiLagSoundVolume);

    // [Dial]
    ini.SetDoubleValue("Dial", "BoostOffset", DialBoostOffset);
    ini.SetDoubleValue("Dial", "BoostScale", DialBoostScale);
    ini.SetDoubleValue("Dial", "VacuumOffset", DialVacuumOffset);
    ini.SetDoubleValue("Dial", "VacuumScale", DialVacuumScale);
    ini.SetBoolValue("Dial", "BoostIncludesVacuum", DialBoostIncludesVacuum);
#pragma warning(pop)


    result = ini.SaveFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

bool CConfig::Write(const std::string& newName, const std::string& model) {
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
    ini.SetValue("ID", "Models", model.c_str());

#pragma warning(push)
#pragma warning(disable: 4244)
    // [Turbo]
    ini.SetDoubleValue("Turbo", "RPMSpoolStart", RPMSpoolStart);
    ini.SetDoubleValue("Turbo", "RPMSpoolEnd", RPMSpoolEnd);
    ini.SetDoubleValue("Turbo", "MinBoost", MinBoost);
    ini.SetDoubleValue("Turbo", "MaxBoost", MaxBoost);
    ini.SetDoubleValue("Turbo", "SpoolRate", SpoolRate);
    ini.SetDoubleValue("Turbo", "UnspoolRate", UnspoolRate);

    // [AntiLag]
    ini.SetBoolValue("AntiLag", "AntiLag", AntiLag);
    ini.SetBoolValue("AntiLag", "Effects", AntiLagEffects);
    ini.SetValue("AntiLag", "SoundSet", AntiLagSoundSet.c_str());
    ini.SetLongValue("AntiLag", "SoundFrequency", AntiLagSoundTicks);
    ini.SetDoubleValue("AntiLag", "SoundVolume", AntiLagSoundVolume);

    // [Dial]
    ini.SetDoubleValue("Dial", "BoostOffset", DialBoostOffset);
    ini.SetDoubleValue("Dial", "BoostScale", DialBoostScale);
    ini.SetDoubleValue("Dial", "VacuumOffset", DialVacuumOffset);
    ini.SetDoubleValue("Dial", "VacuumScale", DialVacuumScale);
    ini.SetBoolValue("Dial", "BoostIncludesVacuum", DialBoostIncludesVacuum);
#pragma warning(pop)

    result = ini.SaveFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
    if (result < 0)
        return false;
    return true;
}
