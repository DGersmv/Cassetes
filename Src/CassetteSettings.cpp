// =============================================================================
// CassetteSettings - Загрузка и сохранение настроек
// =============================================================================

#include "CassetteSettings.hpp"
#include "ACAPinc.h"

#include <Windows.h>
#include <ShlObj.h>
#include <fstream>
#include <sstream>
#include <map>

namespace CassetteSettings {

// =============================================================================
// Вспомогательные функции
// =============================================================================

// Получить путь к папке APPDATA
static GS::UniString GetAppDataPath()
{
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        return GS::UniString(path);
    }
    return GS::UniString();
}

GS::UniString GetSettingsFilePath()
{
    GS::UniString appData = GetAppDataPath();
    if (appData.IsEmpty()) {
        return GS::UniString();
    }
    return appData + "\\GRAPHISOFT\\CassettePanel\\settings.txt";
}

// Создать директорию если не существует
static bool EnsureDirectoryExists(const GS::UniString& dirPath)
{
    std::wstring wpath(dirPath.ToUStr().Get());
    return CreateDirectoryW(wpath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// =============================================================================
// GetDefaultSettings
// =============================================================================

Settings GetDefaultSettings()
{
    Settings s;
    
    s.defaultType = 0;
    s.wallIdForFloorHeight = "СН-МД1";
    s.showDuplicateWarning = true;
    
    // Тип 0
    s.type0.plankWidth = 285;
    s.type0.slopeWidth = 285;
    s.type0.offsetX = 165;
    s.type0.offsetY = 50;
    s.type0.x2Coeff = 745;
    s.type0.cassetteId = "";
    s.type0.plankId = "OK-0_PLNK";
    s.type0.leftSlopeId = "OK-0_LOTK";
    s.type0.rightSlopeId = "OK-0_ROTK";
    
    // Тип 1-2
    s.type1_2.plankWidth = 160;
    s.type1_2.slopeWidth = 225;
    s.type1_2.offsetX = 165;
    s.type1_2.offsetY = 50;
    s.type1_2.x2Coeff = 745;
    s.type1_2.cassetteId = "OK-1_2_CASS";
    s.type1_2.plankId = "OK-1_2_PLNK";
    s.type1_2.leftSlopeId = "OK-1_2_LOTK";
    s.type1_2.rightSlopeId = "OK-1_2_ROTK";
    
    return s;
}

// =============================================================================
// LoadSettings
// =============================================================================

bool LoadSettings(Settings& settings)
{
    GS::UniString filePath = GetSettingsFilePath();
    if (filePath.IsEmpty()) {
        settings = GetDefaultSettings();
        return false;
    }
    
    std::wifstream file(filePath.ToUStr().Get());
    if (!file.is_open()) {
        settings = GetDefaultSettings();
        return false;
    }
    
    // Начинаем с defaults
    settings = GetDefaultSettings();
    
    // Парсим простой формат key=value
    std::map<std::wstring, std::wstring> values;
    std::wstring line;
    while (std::getline(file, line)) {
        size_t pos = line.find(L'=');
        if (pos != std::wstring::npos) {
            std::wstring key = line.substr(0, pos);
            std::wstring value = line.substr(pos + 1);
            values[key] = value;
        }
    }
    file.close();
    
    // Применяем значения
    auto getInt = [&](const std::wstring& key, int def) -> int {
        auto it = values.find(key);
        if (it != values.end()) {
            return std::stoi(it->second);
        }
        return def;
    };
    
    auto getString = [&](const std::wstring& key, const GS::UniString& def) -> GS::UniString {
        auto it = values.find(key);
        if (it != values.end()) {
            return GS::UniString(it->second.c_str());
        }
        return def;
    };
    
    settings.defaultType = getInt(L"defaultType", 0);
    settings.wallIdForFloorHeight = getString(L"wallIdForFloorHeight", "СН-МД1");
    settings.showDuplicateWarning = getInt(L"showDuplicateWarning", 1) != 0;
    
    // Тип 0
    settings.type0.plankWidth = getInt(L"type0.plankWidth", 285);
    settings.type0.slopeWidth = getInt(L"type0.slopeWidth", 285);
    settings.type0.offsetX = getInt(L"type0.offsetX", 165);
    settings.type0.offsetY = getInt(L"type0.offsetY", 50);
    settings.type0.plankId = getString(L"type0.plankId", "OK-0_PLNK");
    settings.type0.leftSlopeId = getString(L"type0.leftSlopeId", "OK-0_LOTK");
    settings.type0.rightSlopeId = getString(L"type0.rightSlopeId", "OK-0_ROTK");
    
    // Тип 1-2
    settings.type1_2.plankWidth = getInt(L"type1_2.plankWidth", 160);
    settings.type1_2.slopeWidth = getInt(L"type1_2.slopeWidth", 225);
    settings.type1_2.offsetX = getInt(L"type1_2.offsetX", 165);
    settings.type1_2.offsetY = getInt(L"type1_2.offsetY", 50);
    settings.type1_2.x2Coeff = getInt(L"type1_2.x2Coeff", 745);
    settings.type1_2.cassetteId = getString(L"type1_2.cassetteId", "OK-1_2_CASS");
    settings.type1_2.plankId = getString(L"type1_2.plankId", "OK-1_2_PLNK");
    settings.type1_2.leftSlopeId = getString(L"type1_2.leftSlopeId", "OK-1_2_LOTK");
    settings.type1_2.rightSlopeId = getString(L"type1_2.rightSlopeId", "OK-1_2_ROTK");
    
    return true;
}

// =============================================================================
// SaveSettings
// =============================================================================

bool SaveSettings(const Settings& settings)
{
    GS::UniString filePath = GetSettingsFilePath();
    if (filePath.IsEmpty()) {
        return false;
    }
    
    // Создаём директорию
    GS::UniString dirPath = GetAppDataPath() + "\\GRAPHISOFT\\CassettePanel";
    EnsureDirectoryExists(GetAppDataPath() + "\\GRAPHISOFT");
    EnsureDirectoryExists(dirPath);
    
    std::wofstream file(filePath.ToUStr().Get());
    if (!file.is_open()) {
        return false;
    }
    
    // Записываем в формате key=value
    file << L"defaultType=" << settings.defaultType << std::endl;
    file << L"wallIdForFloorHeight=" << settings.wallIdForFloorHeight.ToUStr().Get() << std::endl;
    file << L"showDuplicateWarning=" << (settings.showDuplicateWarning ? 1 : 0) << std::endl;
    
    // Тип 0
    file << L"type0.plankWidth=" << settings.type0.plankWidth << std::endl;
    file << L"type0.slopeWidth=" << settings.type0.slopeWidth << std::endl;
    file << L"type0.offsetX=" << settings.type0.offsetX << std::endl;
    file << L"type0.offsetY=" << settings.type0.offsetY << std::endl;
    file << L"type0.plankId=" << settings.type0.plankId.ToUStr().Get() << std::endl;
    file << L"type0.leftSlopeId=" << settings.type0.leftSlopeId.ToUStr().Get() << std::endl;
    file << L"type0.rightSlopeId=" << settings.type0.rightSlopeId.ToUStr().Get() << std::endl;
    
    // Тип 1-2
    file << L"type1_2.plankWidth=" << settings.type1_2.plankWidth << std::endl;
    file << L"type1_2.slopeWidth=" << settings.type1_2.slopeWidth << std::endl;
    file << L"type1_2.offsetX=" << settings.type1_2.offsetX << std::endl;
    file << L"type1_2.offsetY=" << settings.type1_2.offsetY << std::endl;
    file << L"type1_2.x2Coeff=" << settings.type1_2.x2Coeff << std::endl;
    file << L"type1_2.cassetteId=" << settings.type1_2.cassetteId.ToUStr().Get() << std::endl;
    file << L"type1_2.plankId=" << settings.type1_2.plankId.ToUStr().Get() << std::endl;
    file << L"type1_2.leftSlopeId=" << settings.type1_2.leftSlopeId.ToUStr().Get() << std::endl;
    file << L"type1_2.rightSlopeId=" << settings.type1_2.rightSlopeId.ToUStr().Get() << std::endl;
    
    file.close();
    return true;
}

// =============================================================================
// ResetToDefaults
// =============================================================================

bool ResetToDefaults()
{
    Settings defaults = GetDefaultSettings();
    return SaveSettings(defaults);
}

// =============================================================================
// Конвертации
// =============================================================================

CassetteHelper::CalcParams ToCalcParams(const Settings& settings, CassetteHelper::CalcType type)
{
    CassetteHelper::CalcParams params;
    params.type = type;
    params.floorHeight = 3.3;  // По умолчанию, должно быть получено из стены
    
    // Параметры для типа 0
    params.plankWidth0 = settings.type0.plankWidth;
    params.slopeWidth0 = settings.type0.slopeWidth;
    
    // Параметры для типов 1-2
    params.plankWidth12 = settings.type1_2.plankWidth;
    params.slopeWidth12 = settings.type1_2.slopeWidth;
    
    // Общие параметры (используем из типа 0 или 1-2, не важно)
    params.offsetX = settings.type0.offsetX;
    params.offsetY = settings.type0.offsetY;
    params.offsetTop = settings.type1_2.x2Coeff;  // Используем offsetTop вместо x2Coeff
    
    return params;
}

CassetteHelper::TargetObjects ToTargetObjects(const Settings& settings, CassetteHelper::CalcType type)
{
    CassetteHelper::TargetObjects targets;
    
    // Объекты для типа 0
    targets.plankId0 = settings.type0.plankId;
    targets.leftSlopeId0 = settings.type0.leftSlopeId;
    targets.rightSlopeId0 = settings.type0.rightSlopeId;
    
    // Объекты для типов 1-2
    targets.cassetteId12 = settings.type1_2.cassetteId;
    targets.plankId12 = settings.type1_2.plankId;
    targets.leftSlopeId12 = settings.type1_2.leftSlopeId;
    targets.rightSlopeId12 = settings.type1_2.rightSlopeId;
    
    return targets;
}

} // namespace CassetteSettings
