// =============================================================================
// CassetteHelper - Реализация логики расчёта кассет
// =============================================================================

#include "CassetteHelper.hpp"
#include "ACAPinc.h"
#include "APICommon.h"
#include <cmath>
#include <map>
#include <cstdio>

namespace CassetteHelper {

// =============================================================================
// Вспомогательные функции
// =============================================================================

// Определить тип расчёта из ID элемента
int GetCalcTypeFromId(const GS::UniString& id)
{
    // Ищем паттерны ОК-0, ОК-1, ОК-2, ДВ-0, ДВ-1, ДВ-2
    if (id.Contains("ОК-0") || id.Contains("OK-0") || 
        id.Contains("ДВ-0") || id.Contains("DV-0")) {
        return 0;
    }
    if (id.Contains("ОК-1") || id.Contains("OK-1") || 
        id.Contains("ДВ-1") || id.Contains("DV-1")) {
        return 1;
    }
    if (id.Contains("ОК-2") || id.Contains("OK-2") || 
        id.Contains("ДВ-2") || id.Contains("DV-2")) {
        return 2;
    }
    return -1; // Неизвестный тип
}

// Получить параметры по умолчанию
CalcParams GetDefaultParams(CalcType type)
{
    CalcParams params;
    params.type = type;
    params.floorHeight = 2.99;  // 2.99 м (2990 мм) по умолчанию
    params.offsetX = 165;      // Низ плюс к низу этажа (мм)
    params.offsetY = 50;       // мм
    params.offsetTop = 745;    // Верх плюс к высоте этажа (мм)
    
    // Параметры для типа 0
    params.plankWidth0 = 285;   // мм
    params.slopeWidth0 = 285;   // мм
    
    // Параметры для типов 1-2
    params.plankWidth12 = 160;   // мм
    params.slopeWidth12 = 225;   // мм
    
    return params;
}

// Получить ID целевых объектов по умолчанию
TargetObjects GetDefaultTargets(CalcType type)
{
    TargetObjects targets;
    
    // Устанавливаем значения по умолчанию для обоих типов
    targets.plankId0 = "OK-0_PLNK";
    targets.leftSlopeId0 = "OK-0_LOTK";
    targets.rightSlopeId0 = "OK-0_ROTK";
    targets.cassetteId12 = "OK-1_2_CASS";
    targets.plankId12 = "OK-1_2_PLNK";
    targets.leftSlopeId12 = "OK-1_2_LOTK";
    targets.rightSlopeId12 = "OK-1_2_ROTK";
    
    return targets;
}

// =============================================================================
// GetSelectedWindowsDoors - получить выделенные окна/двери
// =============================================================================

GS::Array<WindowDoorInfo> GetSelectedWindowsDoors()
{
    GS::Array<WindowDoorInfo> result;
    
    // Получаем выделение
    API_SelectionInfo selectionInfo;
    GS::Array<API_Neig> selNeigs;
    
    GSErrCode err = ACAPI_Selection_Get(&selectionInfo, &selNeigs, false);
    if (err != NoError || selectionInfo.typeID == API_SelEmpty) {
        return result;
    }
    
    // Обрабатываем каждый выделенный элемент
    for (const API_Neig& neig : selNeigs) {
        API_Element element = {};
        element.header.guid = neig.guid;
        
        err = ACAPI_Element_Get(&element);
        if (err != NoError) continue;
        
        // Фильтруем только окна и двери
        API_ElemTypeID typeID = element.header.type.typeID;
        if (typeID == API_WindowID || typeID == API_DoorID) {
            WindowDoorInfo info;
            info.guid = element.header.guid;
            
            // Инициализация
            info.width = 0;
            info.height = 0;
            info.sillHeight = 0;
            
            // Получаем размеры из API (openingBase для окон/дверей)
            if (typeID == API_WindowID) {
                info.width = element.window.openingBase.width;   // Ширина в метрах
                info.height = element.window.openingBase.height; // Высота в метрах
                
                // Получаем высоту подоконника из lower (Parapet height)
                info.sillHeight = element.window.lower; // Parapet height = Sill to Storey
                
            } else if (typeID == API_DoorID) {
                info.width = element.door.openingBase.width;    // Ширина в метрах
                info.height = element.door.openingBase.height;  // Высота в метрах
                
                // Получаем высоту подоконника из lower (Parapet height)
                info.sillHeight = element.door.lower; // Parapet height = Sill to Storey
            }
            
            // Устанавливаем тип элемента
            if (typeID == API_WindowID) {
                info.elemType = "Window";
            } else {
                info.elemType = "Door";
            }
            // Координаты не критичны для расчёта, устанавливаем 0
            info.x = 0;
            info.y = 0;
            info.angle = 0;
            
            // Получаем ID элемента через свойства
            // Сначала пробуем пользовательские свойства
            GS::Array<API_PropertyDefinition> definitions;
            err = ACAPI_Element_GetPropertyDefinitions(element.header.guid, API_PropertyDefinitionFilter_UserDefined, definitions);
            
            // Ищем свойство ID в пользовательских свойствах
            bool idFound = false;
            for (const API_PropertyDefinition& def : definitions) {
                if (def.name.Contains("ID") || def.name.Contains("id") || 
                    def.name.Contains("ИД") || def.name.Contains("идентификатор")) {
                    API_Property property;
                    err = ACAPI_Element_GetPropertyValue(element.header.guid, def.guid, property);
                    if (err == NoError && property.isDefault == false) {
                        if (property.value.singleVariant.variant.type == API_PropertyStringValueType) {
                            info.id = property.value.singleVariant.variant.uniStringValue;
                            idFound = true;
                            break;
                        }
                    }
                }
            }
            
            // Если не нашли в пользовательских, пробуем все свойства
            if (!idFound) {
                err = ACAPI_Element_GetPropertyDefinitions(element.header.guid, API_PropertyDefinitionFilter_All, definitions);
                for (const API_PropertyDefinition& def : definitions) {
                    if (def.name.Contains("ID") || def.name.Contains("id") || 
                        def.name.Contains("ИД") || def.name.Contains("идентификатор")) {
                        API_Property property;
                        err = ACAPI_Element_GetPropertyValue(element.header.guid, def.guid, property);
                        if (err == NoError && property.isDefault == false) {
                            if (property.value.singleVariant.variant.type == API_PropertyStringValueType) {
                                info.id = property.value.singleVariant.variant.uniStringValue;
                                break;
                            }
                        }
                    }
                }
            }
            
            // Определяем тип расчёта из ID
            info.calcType = GetCalcTypeFromId(info.id);
            
            // Добавляем элемент, даже если ID не соответствует паттерну (calcType будет -1)
            // Это позволит видеть все окна/двери, а не только с правильными ID
            result.Push(info);
        }
    }
    
    return result;
}

// =============================================================================
// GetFloorHeightFromWall - получить высоту этажа (упрощённая версия)
// =============================================================================
// Теперь просто возвращает значение по умолчанию 2.99 м (2990 мм)
// Высоту можно изменить вручную через UI
// Параметр wallIdPattern игнорируется, но оставлен для совместимости

double GetFloorHeightFromWall(const GS::UniString& wallIdPattern)
{
    // Возвращаем значение по умолчанию 2.99 м (2990 мм)
    return 2.99;
}

// =============================================================================
// FindDuplicateIds - найти дубликаты ID
// =============================================================================

GS::Array<GS::UniString> FindDuplicateIds(const GS::Array<WindowDoorInfo>& windows)
{
    GS::Array<GS::UniString> duplicates;
    std::map<GS::UniString, int> idCount;
    
    // Подсчитываем количество каждого ID
    for (const WindowDoorInfo& w : windows) {
        idCount[w.id]++;
    }
    
    // Находим дубликаты
    for (const auto& pair : idCount) {
        if (pair.second > 1) {
            duplicates.Push(pair.first);
        }
    }
    
    return duplicates;
}

// =============================================================================
// Calculate - выполнить расчёт
// =============================================================================

CalculationResult Calculate(
    const GS::Array<WindowDoorInfo>& windows,
    const CalcParams& params)
{
    CalculationResult result;
    result.success = true;
    
    // Проверяем дубликаты
    result.duplicateIds = FindDuplicateIds(windows);
    
    // Временные структуры для группировки с учётом типа элемента
    std::map<std::pair<int, int>, int> cassetteGroups;  // (X, Y) -> count (только для типов 1-2)
    std::map<std::pair<int, int>, int> plankGroups0;   // (length, width) -> count для типа 0
    std::map<std::pair<int, int>, int> plankGroups12;  // (length, width) -> count для типов 1-2
    std::map<std::pair<int, int>, int> leftSlopeGroups0;   // (length, width) -> count для типа 0
    std::map<std::pair<int, int>, int> leftSlopeGroups12;  // (length, width) -> count для типов 1-2
    std::map<std::pair<int, int>, int> rightSlopeGroups0;  // (length, width) -> count для типа 0
    std::map<std::pair<int, int>, int> rightSlopeGroups12; // (length, width) -> count для типов 1-2
    
    // Обрабатываем каждое окно (обрабатываем все элементы подряд без фильтрации по типу расчёта)
    for (const WindowDoorInfo& w : windows) {
        // Определяем ширину планки и откоса в зависимости от типа элемента
        int plankWidth = (w.calcType == 0) ? params.plankWidth0 : params.plankWidth12;
        int slopeWidth = (w.calcType == 0) ? params.slopeWidth0 : params.slopeWidth12;
        
        // Расчёт планок: длина = B * 1000 + offsetY
        int plankLength = static_cast<int>(w.width * 1000) + params.offsetY;
        if (w.calcType == 0) {
            plankGroups0[{plankLength, plankWidth}] += 2;  // По 2 на каждое окно
        } else {
            plankGroups12[{plankLength, plankWidth}] += 2;  // По 2 на каждое окно
        }
        
        // Расчёт откосов: длина = C * 1000
        int slopeLength = static_cast<int>(w.height * 1000);
        if (w.calcType == 0) {
            leftSlopeGroups0[{slopeLength, slopeWidth}] += 1;
            rightSlopeGroups0[{slopeLength, slopeWidth}] += 1;
        } else {
            leftSlopeGroups12[{slopeLength, slopeWidth}] += 1;
            rightSlopeGroups12[{slopeLength, slopeWidth}] += 1;
        }
        
        // Расчёт кассет (только для типов 1 и 2)
        if (w.calcType == 1 || w.calcType == 2) {
            // X = D * 1000 + offsetX, Y = B * 1000 + offsetY
            int cassetteX = static_cast<int>(w.sillHeight * 1000) + params.offsetX;
            int cassetteY = static_cast<int>(w.width * 1000) + params.offsetY;
            cassetteGroups[{cassetteX, cassetteY}]++;
            
            // Для типа 2 добавляем вторую (верхнюю) кассету
            if (w.calcType == 2) {
                // X2 = I2*1000 - (190 + C*1000 + D*1000 + 20) + offsetTop
                int cassetteX2 = static_cast<int>(params.floorHeight * 1000) 
                    - (190 + static_cast<int>(w.height * 1000) + static_cast<int>(w.sillHeight * 1000) + 20) 
                    + params.offsetTop;
                cassetteGroups[{cassetteX2, cassetteY}]++;
            }
        }
    }
    
    // Конвертируем группы в результат
    for (const auto& pair : cassetteGroups) {
        CassetteSize cs;
        cs.x = pair.first.first;
        cs.y = pair.first.second;
        cs.count = pair.second;
        result.cassettes.Push(cs);
    }
    
    // Планки для типа 0
    for (const auto& pair : plankGroups0) {
        PlankSize ps;
        ps.width = pair.first.second;
        ps.length = pair.first.first;
        ps.count = pair.second;
        ps.calcType = 0;  // Тип 0
        result.planks.Push(ps);
    }
    
    // Планки для типов 1-2
    for (const auto& pair : plankGroups12) {
        PlankSize ps;
        ps.width = pair.first.second;
        ps.length = pair.first.first;
        ps.count = pair.second;
        ps.calcType = 1;  // Тип 1 или 2 (будет уточнено при необходимости, но для записи это не важно - оба идут в объекты типа 1-2)
        result.planks.Push(ps);
    }
    
    // Левые откосы для типа 0
    for (const auto& pair : leftSlopeGroups0) {
        PlankSize ps;
        ps.width = pair.first.second;
        ps.length = pair.first.first;
        ps.count = pair.second;
        ps.calcType = 0;  // Тип 0
        result.leftSlopes.Push(ps);
    }
    
    // Левые откосы для типов 1-2
    for (const auto& pair : leftSlopeGroups12) {
        PlankSize ps;
        ps.width = pair.first.second;
        ps.length = pair.first.first;
        ps.count = pair.second;
        ps.calcType = 1;  // Тип 1 или 2
        result.leftSlopes.Push(ps);
    }
    
    // Правые откосы для типа 0
    for (const auto& pair : rightSlopeGroups0) {
        PlankSize ps;
        ps.width = pair.first.second;
        ps.length = pair.first.first;
        ps.count = pair.second;
        ps.calcType = 0;  // Тип 0
        result.rightSlopes.Push(ps);
    }
    
    // Правые откосы для типов 1-2
    for (const auto& pair : rightSlopeGroups12) {
        PlankSize ps;
        ps.width = pair.first.second;
        ps.length = pair.first.first;
        ps.count = pair.second;
        ps.calcType = 1;  // Тип 1 или 2
        result.rightSlopes.Push(ps);
    }
    
    return result;
}

// =============================================================================
// WriteToTargetObjects - записать результаты в GDL объекты
// =============================================================================

bool WriteToTargetObjects(
    const CalculationResult& result,
    const TargetObjects& targets,
    const CalcParams& params)
{
    // Получаем выделение
    API_SelectionInfo selectionInfo;
    GS::Array<API_Neig> selNeigs;
    
    GSErrCode err = ACAPI_Selection_Get(&selectionInfo, &selNeigs, false);
    if (err != NoError) {
        return false;
    }
    
    // Функция для форматирования строки (3 параметра)
    auto formatLine3 = [](const char* format, int a, int b, int c) -> GS::UniString {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), format, a, b, c);
        return GS::UniString(buffer);
    };
    
    // Функция для форматирования строки (4 параметра)
    auto formatLine4 = [](const char* format, int a, int b, int c, int d) -> GS::UniString {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), format, a, b, c, d);
        return GS::UniString(buffer);
    };
    
    // Функция для поиска объекта по ID и записи параметров
    auto writeToObject = [&](const GS::UniString& targetId, 
                             const GS::Array<GS::UniString>& lines,
                             int maxLines) -> bool {
        if (targetId.IsEmpty()) {
            WriteReport("=== writeToObject: targetId пустой, пропуск");
            return true;
        }
        
        WriteReport("=== writeToObject: поиск объекта ID='%s', lines=%d, maxLines=%d", 
            targetId.ToCStr().Get(), lines.GetSize(), maxLines);
        WriteReport("  Выделено элементов: %d", selNeigs.GetSize());
        
        for (UIndex neigIdx = 0; neigIdx < selNeigs.GetSize(); neigIdx++) {
            const API_Neig& neig = selNeigs[neigIdx];
            API_Element element = {};
            element.header.guid = neig.guid;
            
            WriteReport("  Проверка элемента [%d/%d]: guid=%s", 
                neigIdx + 1, selNeigs.GetSize(), 
                APIGuidToString(neig.guid).ToCStr().Get());
            
            err = ACAPI_Element_Get(&element);
            if (err != NoError) {
                WriteReport("    Ошибка получения элемента: err=%d", err);
                continue;
            }
            
            WriteReport("    Тип элемента: typeID=%d", element.header.type.typeID);
            
            // Проверяем что это Object
            if (element.header.type.typeID != API_ObjectID) {
                WriteReport("    Пропуск: не Object (typeID=%d)", element.header.type.typeID);
                continue;
            }
            
            WriteReport("    Это Object, ищем ID...");
            
            // Получаем ID объекта - пробуем все свойства
            GS::Array<API_PropertyDefinition> definitions;
            err = ACAPI_Element_GetPropertyDefinitions(element.header.guid, 
                API_PropertyDefinitionFilter_All, definitions);
            
            if (err != NoError) {
                WriteReport("    Ошибка получения свойств: err=%d", err);
                continue;
            }
            
            WriteReport("    Найдено свойств: %d", definitions.GetSize());
            
            GS::UniString objectId;
            bool idFound = false;
            
            // Сначала пробуем пользовательские свойства
            for (const API_PropertyDefinition& def : definitions) {
                GS::UniString propName = def.name;
                
                // Логируем все свойства с ID в названии
                if (propName.Contains("ID", GS::UniString::CaseInsensitive) || 
                    propName.Contains("id", GS::UniString::CaseInsensitive) ||
                    propName.Contains("ИД", GS::UniString::CaseInsensitive) ||
                    propName.Contains("идентификатор", GS::UniString::CaseInsensitive)) {
                    
                    WriteReport("    Найдено свойство с ID: '%s'", propName.ToCStr().Get());
                    
                    API_Property property;
                    err = ACAPI_Element_GetPropertyValue(element.header.guid, def.guid, property);
                    if (err == NoError && property.isDefault == false) {
                        if (property.value.singleVariant.variant.type == API_PropertyStringValueType) {
                            objectId = property.value.singleVariant.variant.uniStringValue;
                            WriteReport("    *** ID объекта: '%s' (ищем '%s')", 
                                objectId.ToCStr().Get(), targetId.ToCStr().Get());
                            idFound = true;
                            break;
                        }
                    }
                }
            }
            
            if (!idFound) {
                WriteReport("    ВНИМАНИЕ: ID не найден в свойствах!");
                // Пробуем получить через libInd и имя библиотечной части
                if (element.object.libInd != 0) {
                    API_LibPart libPart;
                    libPart.index = element.object.libInd;
                    err = ACAPI_LibraryPart_Get(&libPart);
                    if (err == NoError) {
                        WriteReport("    Имя библиотечной части: '%s'", 
                            GS::UniString(libPart.docu_UName).ToCStr().Get());
                    }
                }
                continue;
            }
            
            if (objectId != targetId) {
                WriteReport("    Пропуск: ID не совпадает ('%s' != '%s')", 
                    objectId.ToCStr().Get(), targetId.ToCStr().Get());
                continue;
            }
            
            WriteReport("    *** СОВПАДЕНИЕ! ID='%s' == targetId='%s'", 
                objectId.ToCStr().Get(), targetId.ToCStr().Get());
            
            // Нашли объект, записываем параметры Text_3...Text_N
            WriteReport("  *** НАЙДЕН объект для записи: ID='%s'", objectId.ToCStr().Get());
            WriteReport("  Количество строк для записи: %d", lines.GetSize());
            for (UIndex li = 0; li < lines.GetSize(); li++) {
                WriteReport("    lines[%d] = '%s'", li, lines[li].ToCStr().Get());
            }
            
            // Используем ACAPI_LibraryPart_ChangeAParameter для изменения параметров
            // Это правильный способ изменения параметров размещенного объекта
            WriteReport("  ========================================");
            WriteReport("  ИСПОЛЬЗУЕМ ACAPI_LibraryPart_ChangeAParameter");
            WriteReport("  ========================================");
            
            // Открываем параметры размещенного объекта
            API_ParamOwnerType paramOwner = {};
            paramOwner.guid = element.header.guid;  // GUID размещенного элемента
            paramOwner.libInd = 0;                  // 0 для размещенного элемента
            paramOwner.type = element.header.type;   // Тип элемента
            
            err = ACAPI_LibraryPart_OpenParameters(&paramOwner);
            if (err != NoError) {
                WriteReport("  ❌ ОШИБКА ACAPI_LibraryPart_OpenParameters: err=%d", err);
                return false;
            }
            
            WriteReport("  ✅ Параметры открыты успешно");
            
            // Получаем список параметров для проверки
            API_GetParamsType getParamsCheck = {};
            err = ACAPI_LibraryPart_GetActParameters(&getParamsCheck);
            if (err != NoError || getParamsCheck.params == nullptr) {
                WriteReport("  ❌ ОШИБКА получения списка параметров: err=%d", err);
                ACAPI_LibraryPart_CloseParameters();
                return false;
            }
            
            Int32 nParams = BMGetHandleSize((GSHandle)getParamsCheck.params) / sizeof(API_AddParType);
            WriteReport("  Найдено параметров в открытом списке: %d", nParams);
            
            // Ищем индексы параметров Text_3...Text_18
            std::map<int, short> textParamIndices;  // textNum -> index
            for (Int32 i = 0; i < nParams; i++) {
                API_AddParType& par = (*getParamsCheck.params)[i];
                GS::UniString parName(par.name);
                if (parName.BeginsWith("Text_")) {
                    GS::UniString numStr = parName.GetSubstring(5, parName.GetLength() - 5);
                    int textNum = 0;
                    std::sscanf(numStr.ToCStr().Get(), "%d", &textNum);
                    if (textNum >= 3 && textNum <= 18) {
                        textParamIndices[textNum] = par.index;
                        WriteReport("    Найден параметр %s: index=%d, typeID=%d", 
                            parName.ToCStr().Get(), par.index, par.typeID);
                    }
                }
            }
            ACAPI_DisposeAddParHdl(&getParamsCheck.params);
            
            if (textParamIndices.empty()) {
                WriteReport("  ❌ ОШИБКА: не найдено ни одного параметра Text_3...Text_18!");
                ACAPI_LibraryPart_CloseParameters();
                return false;
            }
            
            bool success = true;
            
            // Записываем строки в Text_3...Text_18 используя индекс
            for (int lineIdx = 0; lineIdx < maxLines && lineIdx < static_cast<int>(lines.GetSize()); lineIdx++) {
                int textNum = 3 + lineIdx;  // Text_3, Text_4, ..., Text_18
                
                if (textNum > 18) break;  // Не выходим за пределы Text_18
                
                // Ищем индекс параметра
                auto it = textParamIndices.find(textNum);
                if (it == textParamIndices.end()) {
                    WriteReport("  ⚠ Параметр Text_%d не найден в списке параметров", textNum);
                    continue;
                }
                
                short paramIndex = it->second;
                char paramName[API_NameLen];
                std::snprintf(paramName, sizeof(paramName), "Text_%d", textNum);
                
                GS::UniString lineStr = lines[lineIdx];
                USize strLen = lineStr.GetLength();
                
                WriteReport("  Запись в %s (index=%d): строка='%s' (len=%d)", 
                    paramName, paramIndex, lineStr.ToCStr().Get(), strLen);
                
                // Подготавливаем буфер для строки
                GS::uchar_t* uStrBuffer = nullptr;
                if (strLen < API_UAddParStrLen) {
                    uStrBuffer = (GS::uchar_t*)BMAllocatePtr((API_UAddParStrLen + 1) * sizeof(GS::uchar_t), ALLOCATE_CLEAR, 0);
                    if (uStrBuffer != nullptr) {
                        GS::ucscpy(uStrBuffer, lineStr.ToUStr());
                    }
                } else {
                    // Обрезаем если слишком длинная
                    GS::UniString truncated = lineStr.GetSubstring(0, API_UAddParStrLen - 1);
                    uStrBuffer = (GS::uchar_t*)BMAllocatePtr((API_UAddParStrLen + 1) * sizeof(GS::uchar_t), ALLOCATE_CLEAR, 0);
                    if (uStrBuffer != nullptr) {
                        GS::ucscpy(uStrBuffer, truncated.ToUStr());
                    }
                    WriteReport("    ⚠ ВНИМАНИЕ: строка обрезана до %d символов", API_UAddParStrLen - 1);
                }
                
                if (uStrBuffer == nullptr) {
                    WriteReport("    ❌ ОШИБКА: не удалось выделить память для строки");
                    success = false;
                    continue;
                }
                
                // Изменяем параметр через ACAPI_LibraryPart_ChangeAParameter
                // Используем index вместо name для надежности
                API_ChangeParamType changeParam = {};
                changeParam.name[0] = '\0';  // Пустое имя
                changeParam.index = paramIndex;  // Используем index
                changeParam.ind1 = 0;
                changeParam.ind2 = 0;
                changeParam.uStrValue = uStrBuffer;
                
                err = ACAPI_LibraryPart_ChangeAParameter(&changeParam);
                if (err != NoError) {
                    WriteReport("    ❌ ОШИБКА ACAPI_LibraryPart_ChangeAParameter для %s (index=%d): err=%d (%s)", 
                        paramName, paramIndex, err, ErrID_To_Name(err));
                    success = false;
                } else {
                    WriteReport("    ✅ Параметр %s (index=%d) изменен успешно", paramName, paramIndex);
                }
                
                // Освобождаем память
                BMKillPtr((GSPtr*)&uStrBuffer);
            }
            
            // Очищаем остальные параметры (если lines.GetSize() < maxLines)
            for (int lineIdx = static_cast<int>(lines.GetSize()); lineIdx < maxLines; lineIdx++) {
                int textNum = 3 + lineIdx;
                if (textNum > 18) break;
                
                // Ищем индекс параметра
                auto it = textParamIndices.find(textNum);
                if (it == textParamIndices.end()) {
                    continue;  // Параметр не найден, пропускаем
                }
                
                short paramIndex = it->second;
                char paramName[API_NameLen];
                std::snprintf(paramName, sizeof(paramName), "Text_%d", textNum);
                
                WriteReport("  Очистка %s (index=%d)", paramName, paramIndex);
                
                // Выделяем память для пробела
                GS::uchar_t* uStrBuffer = (GS::uchar_t*)BMAllocatePtr((API_UAddParStrLen + 1) * sizeof(GS::uchar_t), ALLOCATE_CLEAR, 0);
                if (uStrBuffer != nullptr) {
                    GS::ucscpy(uStrBuffer, GS::UniString(" ").ToUStr());
                    
                    API_ChangeParamType changeParam = {};
                    changeParam.name[0] = '\0';  // Пустое имя
                    changeParam.index = paramIndex;  // Используем index
                    changeParam.ind1 = 0;
                    changeParam.ind2 = 0;
                    changeParam.uStrValue = uStrBuffer;
                    
                    err = ACAPI_LibraryPart_ChangeAParameter(&changeParam);
                    if (err != NoError) {
                        WriteReport("    ❌ ОШИБКА очистки %s (index=%d): err=%d (%s)", 
                            paramName, paramIndex, err, ErrID_To_Name(err));
                    } else {
                        WriteReport("    ✅ Параметр %s (index=%d) очищен", paramName, paramIndex);
                    }
                    
                    BMKillPtr((GSPtr*)&uStrBuffer);
                }
            }
            
            // Получаем измененные параметры и применяем через ACAPI_Element_Change
            API_GetParamsType getParams = {};
            err = ACAPI_LibraryPart_GetActParameters(&getParams);
            if (err != NoError) {
                WriteReport("  ❌ ОШИБКА ACAPI_LibraryPart_GetActParameters: err=%d", err);
                ACAPI_LibraryPart_CloseParameters();
                return false;
            }
            
            WriteReport("  ✅ Получены измененные параметры");
            
            // Закрываем параметры
            ACAPI_LibraryPart_CloseParameters();
            
            // Применяем изменения через ACAPI_Element_Change
            if (success && getParams.params != nullptr) {
                WriteReport("  ========================================");
                WriteReport("  ПРИМЕНЕНИЕ ИЗМЕНЕНИЙ через ACAPI_Element_Change");
                WriteReport("  ========================================");
                
                err = ACAPI_CallUndoableCommand("Change Cassette Parameters", [&]() -> GSErrCode {
                    API_ElementMemo memo = {};
                    memo.params = getParams.params;
                    
                    API_Element mask = {};
                    ACAPI_ELEMENT_MASK_CLEAR(mask);
                    
                    WriteReport("  Вызов ACAPI_Element_Change...");
                    WriteReport("    element.header.guid = %s", APIGuidToString(element.header.guid).ToCStr().Get());
                    
                    GSErrCode changeErr = ACAPI_Element_Change(&element, &mask, &memo, 
                        APIMemoMask_AddPars, true);
                    
                    // НЕ освобождаем memo.params здесь - они принадлежат getParams
                    
                    if (changeErr != NoError) {
                        WriteReport("  ❌ ОШИБКА ACAPI_Element_Change: err=%d", changeErr);
                        return changeErr;
                    }
                    
                    WriteReport("  ✅ УСПЕХ: ACAPI_Element_Change завершился успешно!");
                    return NoError;
                });
                
                if (err != NoError) {
                    WriteReport("  ❌ ОШИБКА ACAPI_CallUndoableCommand: err=%d", err);
                    ACAPI_DisposeAddParHdl(&getParams.params);
                    return false;
                } else {
                    WriteReport("  ✅ УСПЕХ: ACAPI_CallUndoableCommand завершился успешно!");
                }
                
                // Освобождаем память параметров
                ACAPI_DisposeAddParHdl(&getParams.params);
            } else {
                WriteReport("  ⚠ ВНИМАНИЕ: success=false или getParams.params==nullptr");
                if (getParams.params != nullptr) {
                    ACAPI_DisposeAddParHdl(&getParams.params);
                }
            }
            
            return success;
        }
        
        return false; // Объект не найден
    };
    
    // Разделяем результаты по типам элементов (по ширине)
    // Тип 0: планки width=285, откосы width=285
    // Типы 1-2: планки width=160, откосы width=225
    
    // Формируем строки для кассет (только для типов 1-2)
    GS::Array<GS::UniString> cassetteLines;
    WriteReport("=== Формирование строк для записи ===");
    WriteReport("  Кассеты: count=%d", result.cassettes.GetSize());
    for (const CassetteSize& cs : result.cassettes) {
        GS::UniString line = formatLine3("Размер: U x V : %dx%d мм; Количество: %d шт.", 
            cs.x, cs.y, cs.count);
        cassetteLines.Push(line);
        WriteReport("    Кассета: %s", line.ToCStr().Get());
    }
    
    // Формируем строки для планок - разделяем по типу элемента (из calcType)
    GS::Array<GS::UniString> plankLines0;   // Для типа 0
    GS::Array<GS::UniString> plankLines12; // Для типов 1-2
    WriteReport("  Планки: count=%d", result.planks.GetSize());
    for (const PlankSize& ps : result.planks) {
        GS::UniString line;
        // Определяем тип по calcType: 0 → тип 0, 1 или 2 → типы 1-2
        if (ps.calcType == 0) {
            // Тип 0: "Размер: 285×{length} мм; Длина Z = {length} мм; Количество: {count} шт."
            line = formatLine4("Размер: %dx%d мм; Длина Z = %d мм; Количество: %d шт.",
                ps.width, ps.length, ps.length, ps.count);
            plankLines0.Push(line);
            WriteReport("    Планка (тип 0): width=%d, length=%d, count=%d", ps.width, ps.length, ps.count);
        } else {
            // Типы 1-2: "Размер: 160×{length} мм; Длина W = {length} мм; Количество: {count} шт."
            line = formatLine4("Размер: %dx%d мм; Длина W = %d мм; Количество: %d шт.",
                ps.width, ps.length, ps.length, ps.count);
            plankLines12.Push(line);
            WriteReport("    Планка (тип 1-2): width=%d, length=%d, count=%d", ps.width, ps.length, ps.count);
        }
        WriteReport("      -> %s", line.ToCStr().Get());
    }
    
    // Формируем строки для левых откосов - разделяем по типу элемента (из calcType)
    GS::Array<GS::UniString> leftSlopeLines0;   // Для типа 0
    GS::Array<GS::UniString> leftSlopeLines12; // Для типов 1-2
    WriteReport("  Левые откосы: count=%d", result.leftSlopes.GetSize());
    for (const PlankSize& ps : result.leftSlopes) {
        GS::UniString line = formatLine4("Размер: %dx%d мм; Длина Z = %d мм; Количество: %d шт.",
            ps.width, ps.length, ps.length, ps.count);
        // Определяем тип по calcType: 0 → тип 0, 1 или 2 → типы 1-2
        if (ps.calcType == 0) {
            leftSlopeLines0.Push(line);
            WriteReport("    Левый откос (тип 0): width=%d, length=%d, count=%d", ps.width, ps.length, ps.count);
        } else {
            leftSlopeLines12.Push(line);
            WriteReport("    Левый откос (тип 1-2): width=%d, length=%d, count=%d", ps.width, ps.length, ps.count);
        }
        WriteReport("      -> %s", line.ToCStr().Get());
    }
    
    // Формируем строки для правых откосов - разделяем по типу элемента (из calcType)
    GS::Array<GS::UniString> rightSlopeLines0;   // Для типа 0
    GS::Array<GS::UniString> rightSlopeLines12; // Для типов 1-2
    WriteReport("  Правые откосы: count=%d", result.rightSlopes.GetSize());
    for (const PlankSize& ps : result.rightSlopes) {
        GS::UniString line = formatLine4("Размер: %dx%d мм; Длина Z = %d мм; Количество: %d шт.",
            ps.width, ps.length, ps.length, ps.count);
        // Определяем тип по calcType: 0 → тип 0, 1 или 2 → типы 1-2
        if (ps.calcType == 0) {
            rightSlopeLines0.Push(line);
            WriteReport("    Правый откос (тип 0): width=%d, length=%d, count=%d", ps.width, ps.length, ps.count);
        } else {
            rightSlopeLines12.Push(line);
            WriteReport("    Правый откос (тип 1-2): width=%d, length=%d, count=%d", ps.width, ps.length, ps.count);
        }
        WriteReport("      -> %s", line.ToCStr().Get());
    }
    
    // Записываем в объекты с соответствующими лимитами
    bool success = true;
    
    // Лимиты для типа 0: 8 параметров (Text_3...Text_10)
    int maxPlanks0 = 8;
    int maxSlopes0 = 8;
    
    // Лимиты для типов 1-2: 16 для кассет и откосов, 8 для планок
    int maxCassettes12 = 16;
    int maxPlanks12 = 8;
    int maxSlopes12 = 16;
    
    WriteReport("=== Запись в объекты ===");
    WriteReport("  Тип 0: maxPlanks=%d, maxSlopes=%d", maxPlanks0, maxSlopes0);
    WriteReport("  Типы 1-2: maxCassettes=%d, maxPlanks=%d, maxSlopes=%d", maxCassettes12, maxPlanks12, maxSlopes12);
    
    // Записываем результаты для типа 0
    if (!targets.plankId0.IsEmpty() && plankLines0.GetSize() > 0) {
        success &= writeToObject(targets.plankId0, plankLines0, maxPlanks0);
    }
    if (!targets.leftSlopeId0.IsEmpty() && leftSlopeLines0.GetSize() > 0) {
        success &= writeToObject(targets.leftSlopeId0, leftSlopeLines0, maxSlopes0);
    }
    if (!targets.rightSlopeId0.IsEmpty() && rightSlopeLines0.GetSize() > 0) {
        success &= writeToObject(targets.rightSlopeId0, rightSlopeLines0, maxSlopes0);
    }
    
    // Записываем результаты для типов 1-2
    if (!targets.cassetteId12.IsEmpty() && cassetteLines.GetSize() > 0) {
        success &= writeToObject(targets.cassetteId12, cassetteLines, maxCassettes12);
    }
    if (!targets.plankId12.IsEmpty() && plankLines12.GetSize() > 0) {
        success &= writeToObject(targets.plankId12, plankLines12, maxPlanks12);
    }
    if (!targets.leftSlopeId12.IsEmpty() && leftSlopeLines12.GetSize() > 0) {
        success &= writeToObject(targets.leftSlopeId12, leftSlopeLines12, maxSlopes12);
    }
    if (!targets.rightSlopeId12.IsEmpty() && rightSlopeLines12.GetSize() > 0) {
        success &= writeToObject(targets.rightSlopeId12, rightSlopeLines12, maxSlopes12);
    }
    
    return success;
}

} // namespace CassetteHelper
