#ifndef CASSETTEHELPER_HPP
#define CASSETTEHELPER_HPP

// =============================================================================
// CassetteHelper - Логика расчёта кассет, планок и откосов
// =============================================================================

#include "APIEnvir.h"
#include "ACAPinc.h"

namespace CassetteHelper {

// =============================================================================
// Типы и перечисления
// =============================================================================

// Тип расчёта
enum class CalcType {
    Type0 = 0,      // Только планки и откосы (без кассет)
    Type1 = 1,      // Одна кассета на окно
    Type2 = 2,      // Две кассеты на окно (верх + низ)
    Type1And2 = 3   // Типы 1 и 2 вместе
};

// =============================================================================
// Структуры данных
// =============================================================================

// Информация об окне/двери
struct WindowDoorInfo {
    API_Guid guid;           // Уникальный GUID элемента
    GS::UniString id;        // ID (может повторяться!)
    GS::UniString elemType;  // "Window" / "Door"
    double width;            // Ширина (B) в метрах
    double height;           // Высота (C) в метрах
    double sillHeight;       // Высота подоконника (D) в метрах
    double x;                // Координата X
    double y;                // Координата Y
    double angle;            // Угол
    int calcType;            // 0, 1 или 2 (определяется из ID)
};

// Параметры расчёта
struct CalcParams {
    CalcType type;           // Тип расчёта (для совместимости, не используется для фильтрации)
    double floorHeight;      // Высота этажа (I2) в метрах
    // Параметры для типа 0
    int plankWidth0;         // Ширина планки для типа 0 в мм
    int slopeWidth0;         // Ширина откоса для типа 0 в мм
    // Параметры для типов 1-2
    int plankWidth12;        // Ширина планки для типов 1-2 в мм
    int slopeWidth12;        // Ширина откоса для типов 1-2 в мм
    // Общие параметры
    int offsetX;             // Низ плюс к низу этажа (default 165 мм)
    int offsetY;             // Смещение Y (default 50)
    int offsetTop;           // Верх плюс к высоте этажа (default 745 мм)
};

// Размер кассеты (X × Y)
struct CassetteSize {
    int x;                   // Ширина кассеты в мм
    int y;                   // Высота кассеты в мм
    int count;               // Количество штук
};

// Размер планки/откоса
struct PlankSize {
    int width;               // Ширина профиля в мм
    int length;              // Длина в мм
    int count;               // Количество штук
    int calcType;            // Тип элемента: 0, 1 или 2 (определяется из ID: ОК-0/ДВ-0 → 0, ОК-1/ДВ-1 → 1, ОК-2/ДВ-2 → 2)
};

// Целевые GDL объекты для записи результатов
struct TargetObjects {
    // Объекты для типа 0
    GS::UniString plankId0;       // OK-0_PLNK
    GS::UniString leftSlopeId0;   // OK-0_LOTK
    GS::UniString rightSlopeId0;  // OK-0_ROTK
    // Объекты для типов 1-2
    GS::UniString cassetteId12;  // OK-1_2_CASS
    GS::UniString plankId12;     // OK-1_2_PLNK
    GS::UniString leftSlopeId12; // OK-1_2_LOTK
    GS::UniString rightSlopeId12; // OK-1_2_ROTK
};

// Результат расчёта
struct CalculationResult {
    GS::Array<CassetteSize> cassettes;       // Кассеты (пусто для типа 0)
    GS::Array<PlankSize> planks;             // Планки
    GS::Array<PlankSize> leftSlopes;         // Левые откосы
    GS::Array<PlankSize> rightSlopes;        // Правые откосы
    GS::Array<GS::UniString> duplicateIds;   // Найденные дубликаты ID
    GS::UniString errorMessage;              // Сообщение об ошибке (если есть)
    bool success;                            // Успех операции
};

// =============================================================================
// Функции
// =============================================================================

// Получить выделенные окна и двери
// Фильтрует по ID: ОК-0, ОК-1, ОК-2, ДВ-0, ДВ-1, ДВ-2
GS::Array<WindowDoorInfo> GetSelectedWindowsDoors();

// Получить высоту этажа из стены с указанным ID
// wallIdPattern - часть ID стены для поиска (например "СН-МД1")
// Возвращает высоту в метрах, или 0 если не найдено
double GetFloorHeightFromWall(const GS::UniString& wallIdPattern);

// Выполнить расчёт кассет, планок и откосов
CalculationResult Calculate(
    const GS::Array<WindowDoorInfo>& windows,
    const CalcParams& params
);

// Записать результаты в GDL объекты
// Ищет объекты по ID в выделении и записывает в параметры Text_3...Text_N
bool WriteToTargetObjects(
    const CalculationResult& result,
    const TargetObjects& targets,
    const CalcParams& params
);

// Определить тип расчёта из ID элемента (ОК-0 → 0, ОК-1 → 1, ОК-2 → 2)
int GetCalcTypeFromId(const GS::UniString& id);

// Проверить есть ли дубликаты ID в списке окон
GS::Array<GS::UniString> FindDuplicateIds(const GS::Array<WindowDoorInfo>& windows);

// Получить параметры по умолчанию для типа расчёта
CalcParams GetDefaultParams(CalcType type);

// Получить ID целевых объектов по умолчанию для типа расчёта
TargetObjects GetDefaultTargets(CalcType type);

} // namespace CassetteHelper

#endif // CASSETTEHELPER_HPP
