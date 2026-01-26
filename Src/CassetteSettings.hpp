#ifndef CASSETTESETTINGS_HPP
#define CASSETTESETTINGS_HPP

// =============================================================================
// CassetteSettings - Загрузка и сохранение настроек в JSON
// =============================================================================

#include "CassetteHelper.hpp"
#include "GSRoot.hpp"

namespace CassetteSettings {

// Структура настроек для одного типа
struct TypeSettings {
    int plankWidth;          // Ширина планки в мм
    int slopeWidth;          // Ширина откоса в мм
    int offsetX;             // Смещение X в мм
    int offsetY;             // Смещение Y в мм
    int x2Coeff;             // Коэффициент X2 в мм (только для типа 1-2)
    
    // ID целевых объектов
    GS::UniString cassetteId;
    GS::UniString plankId;
    GS::UniString leftSlopeId;
    GS::UniString rightSlopeId;
};

// Полная структура настроек
struct Settings {
    int defaultType;                    // Тип по умолчанию (0, 1, 2)
    GS::UniString wallIdForFloorHeight; // ID стены для получения высоты этажа
    bool showDuplicateWarning;          // Показывать предупреждение о дубликатах
    
    TypeSettings type0;                 // Настройки для типа 0
    TypeSettings type1_2;               // Настройки для типов 1 и 2
};

// Получить настройки по умолчанию
Settings GetDefaultSettings();

// Загрузить настройки из файла
// Путь: %APPDATA%/GRAPHISOFT/CassettePanel/settings.json
bool LoadSettings(Settings& settings);

// Сохранить настройки в файл
bool SaveSettings(const Settings& settings);

// Сбросить настройки к значениям по умолчанию
bool ResetToDefaults();

// Получить путь к файлу настроек
GS::UniString GetSettingsFilePath();

// Конвертация настроек в CalcParams
CassetteHelper::CalcParams ToCalcParams(const Settings& settings, CassetteHelper::CalcType type);

// Конвертация настроек в TargetObjects
CassetteHelper::TargetObjects ToTargetObjects(const Settings& settings, CassetteHelper::CalcType type);

} // namespace CassetteSettings

#endif // CASSETTESETTINGS_HPP
