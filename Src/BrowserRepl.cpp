// =============================================================================
// BrowserRepl - JavaScript Bridge для Cassette Panel
// =============================================================================

#include "ACAPinc.h"
#include "BrowserRepl.hpp"
#include "DGBrowser.hpp"
#include "CassetteHelper.hpp"
#include "CassetteSettings.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

// =============================================================================
// JS Helper Functions
// =============================================================================

// Extract string from JS::Base
static GS::UniString GetStringFromJs(GS::Ref<JS::Base> p)
{
    if (p == nullptr) return GS::UniString();
    
    if (GS::Ref<JS::Value> v = GS::DynamicCast<JS::Value>(p)) {
        if (v->GetType() == JS::Value::STRING) {
            return v->GetString();
        }
    }
    return GS::UniString();
}

// Extract double from JS::Base
static double GetDoubleFromJs(GS::Ref<JS::Base> p, double def = 0.0)
{
    if (p == nullptr) return def;

	if (GS::Ref<JS::Value> v = GS::DynamicCast<JS::Value>(p)) {
		const auto t = v->GetType();
        if (t == JS::Value::DOUBLE) return v->GetDouble();
        if (t == JS::Value::INTEGER) return static_cast<double>(v->GetInteger());
		if (t == JS::Value::STRING) {
			GS::UniString s = v->GetString();
            for (UIndex i = 0; i < s.GetLength(); ++i) 
                if (s[i] == ',') s[i] = '.';
			double out = def;
			std::sscanf(s.ToCStr().Get(), "%lf", &out);
			return out;
		}
	}
	return def;
}

// Extract integer from JS::Base
static Int32 GetIntFromJs(GS::Ref<JS::Base> p, Int32 def = 0)
{
    if (p == nullptr) return def;

	if (GS::Ref<JS::Value> v = GS::DynamicCast<JS::Value>(p)) {
		const auto t = v->GetType();
        if (t == JS::Value::INTEGER) return v->GetInteger();
        if (t == JS::Value::DOUBLE) return static_cast<Int32>(v->GetDouble());
		if (t == JS::Value::STRING) {
			Int32 out = def;
            std::sscanf(v->GetString().ToCStr().Get(), "%d", &out);
			return out;
		}
	}
	return def;
}

// Extract bool from JS::Base
static bool GetBoolFromJs(GS::Ref<JS::Base> p, bool def = false)
{
    if (p == nullptr) return def;
    
    if (GS::Ref<JS::Value> v = GS::DynamicCast<JS::Value>(p)) {
        if (v->GetType() == JS::Value::BOOL) {
            return v->GetBool();
        }
    }
    return def;
}

// =============================================================================
// RegisterACAPIJavaScriptObject
// Регистрирует объект window.ACAPI с функциями для вызова из JavaScript
// =============================================================================

void BrowserRepl::RegisterACAPIJavaScriptObject(DG::Browser& browser)
{
	JS::Object* jsACAPI = new JS::Object("ACAPI");

    // ------------------------------------------------------------
    // Ping - тестовая функция
    // ------------------------------------------------------------
    
    jsACAPI->AddItem(new JS::Function("Ping", [](GS::Ref<JS::Base>) -> GS::Ref<JS::Base> {
        return new JS::Value("Pong from Cassette Panel!");
    }));

    // ------------------------------------------------------------
    // GetCassetteSelection - получить выделенные окна/двери
    // ------------------------------------------------------------
    
    jsACAPI->AddItem(new JS::Function("GetCassetteSelection", [](GS::Ref<JS::Base>) -> GS::Ref<JS::Base> {
        GS::Ref<JS::Object> result = new JS::Object();
        
        // Получаем окна/двери
        GS::Array<CassetteHelper::WindowDoorInfo> windows = 
            CassetteHelper::GetSelectedWindowsDoors();
        
        // Конвертируем в JS массив
        GS::Ref<JS::Array> jsWindows = new JS::Array();
        for (const CassetteHelper::WindowDoorInfo& w : windows) {
            GS::Ref<JS::Object> jsWindow = new JS::Object();
            jsWindow->AddItem("id", new JS::Value(w.id));
            jsWindow->AddItem("elemType", new JS::Value(w.elemType));
            jsWindow->AddItem("width", new JS::Value(w.width));
            jsWindow->AddItem("height", new JS::Value(w.height));
            jsWindow->AddItem("sillHeight", new JS::Value(w.sillHeight));
            jsWindow->AddItem("x", new JS::Value(w.x));
            jsWindow->AddItem("y", new JS::Value(w.y));
            jsWindow->AddItem("angle", new JS::Value(w.angle));
            jsWindow->AddItem("calcType", new JS::Value(static_cast<Int32>(w.calcType)));
            jsWindows->AddItem(jsWindow);
        }
        result->AddItem("windows", jsWindows);
        
        // Дубликаты
        GS::Array<GS::UniString> duplicates = CassetteHelper::FindDuplicateIds(windows);
        GS::Ref<JS::Array> jsDuplicates = new JS::Array();
        for (const GS::UniString& dup : duplicates) {
            jsDuplicates->AddItem(new JS::Value(dup));
        }
        result->AddItem("duplicates", jsDuplicates);
        result->AddItem("count", new JS::Value(static_cast<Int32>(windows.GetSize())));
        result->AddItem("success", new JS::Value(true));
        
        return result;
    }));

    // ------------------------------------------------------------
    // GetFloorHeightFromWall - получить высоту этажа из стены
    // ------------------------------------------------------------
    
    jsACAPI->AddItem(new JS::Function("GetFloorHeightFromWall", [](GS::Ref<JS::Base> param) -> GS::Ref<JS::Base> {
        GS::UniString wallId = GetStringFromJs(param);
        if (wallId.IsEmpty()) wallId = "СН-МД1";
        
        double height = CassetteHelper::GetFloorHeightFromWall(wallId);
        
        GS::Ref<JS::Object> result = new JS::Object();
        result->AddItem("height", new JS::Value(height));
        result->AddItem("found", new JS::Value(height > 0));
        
        return result;
    }));

    // ------------------------------------------------------------
    // CalculateCassettes и WriteCassetteResults - временно отключены
    // Требуется исправление API для работы с JS::Object/JS::Array
    // TODO: Найти правильный способ доступа к свойствам JS::Object
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // GetCassetteSettings - получить настройки
    // ------------------------------------------------------------
    
    jsACAPI->AddItem(new JS::Function("GetCassetteSettings", [](GS::Ref<JS::Base>) -> GS::Ref<JS::Base> {
        CassetteSettings::Settings settings;
        CassetteSettings::LoadSettings(settings);
        
        GS::Ref<JS::Object> result = new JS::Object();
        result->AddItem("defaultType", new JS::Value(settings.defaultType));
        result->AddItem("wallIdForFloorHeight", new JS::Value(settings.wallIdForFloorHeight));
        result->AddItem("showDuplicateWarning", new JS::Value(settings.showDuplicateWarning));
        
        // Тип 0
        GS::Ref<JS::Object> type0 = new JS::Object();
        type0->AddItem("plankWidth", new JS::Value(settings.type0.plankWidth));
        type0->AddItem("slopeWidth", new JS::Value(settings.type0.slopeWidth));
        type0->AddItem("offsetX", new JS::Value(settings.type0.offsetX));
        type0->AddItem("offsetY", new JS::Value(settings.type0.offsetY));
        type0->AddItem("plankId", new JS::Value(settings.type0.plankId));
        type0->AddItem("leftSlopeId", new JS::Value(settings.type0.leftSlopeId));
        type0->AddItem("rightSlopeId", new JS::Value(settings.type0.rightSlopeId));
        result->AddItem("type0", type0);
        
        // Тип 1-2
        GS::Ref<JS::Object> type12 = new JS::Object();
        type12->AddItem("plankWidth", new JS::Value(settings.type1_2.plankWidth));
        type12->AddItem("slopeWidth", new JS::Value(settings.type1_2.slopeWidth));
        type12->AddItem("offsetX", new JS::Value(settings.type1_2.offsetX));
        type12->AddItem("offsetY", new JS::Value(settings.type1_2.offsetY));
        type12->AddItem("x2Coeff", new JS::Value(settings.type1_2.x2Coeff));
        type12->AddItem("cassetteId", new JS::Value(settings.type1_2.cassetteId));
        type12->AddItem("plankId", new JS::Value(settings.type1_2.plankId));
        type12->AddItem("leftSlopeId", new JS::Value(settings.type1_2.leftSlopeId));
        type12->AddItem("rightSlopeId", new JS::Value(settings.type1_2.rightSlopeId));
        result->AddItem("type1_2", type12);
		
		return result;
	}));

    // ------------------------------------------------------------
    // CalculateCassettes - выполнить расчёт кассет
    // ------------------------------------------------------------
    
    jsACAPI->AddItem(new JS::Function("CalculateCassettes", [](GS::Ref<JS::Base> param) -> GS::Ref<JS::Base> {
        GS::Ref<JS::Object> result = new JS::Object();
        
        // Парсим параметры из JS объекта
        if (GS::Ref<JS::Object> jsParam = GS::DynamicCast<JS::Object>(param)) {
            // Получаем хеш-таблицу элементов объекта
            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& itemTable = jsParam->GetItemTable();
            
            // Получаем окна
            GS::Array<CassetteHelper::WindowDoorInfo> windows;
            GS::Ref<JS::Base> windowsBase;
            if (itemTable.Get("windows", &windowsBase)) {
                if (GS::Ref<JS::Array> jsWindows = GS::DynamicCast<JS::Array>(windowsBase)) {
                    const GS::Array<GS::Ref<JS::Base>>& windowItems = jsWindows->GetItemArray();
                    for (UIndex i = 0; i < windowItems.GetSize(); ++i) {
                        if (GS::Ref<JS::Object> jsWindow = GS::DynamicCast<JS::Object>(windowItems[i])) {
                            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& windowTable = jsWindow->GetItemTable();
                            CassetteHelper::WindowDoorInfo w;
                            
                            GS::Ref<JS::Base> item;
                            if (windowTable.Get("id", &item)) w.id = GetStringFromJs(item);
                            if (windowTable.Get("elemType", &item)) w.elemType = GetStringFromJs(item);
                            if (windowTable.Get("width", &item)) w.width = GetDoubleFromJs(item);
                            if (windowTable.Get("height", &item)) w.height = GetDoubleFromJs(item);
                            if (windowTable.Get("sillHeight", &item)) w.sillHeight = GetDoubleFromJs(item);
                            if (windowTable.Get("x", &item)) w.x = GetDoubleFromJs(item);
                            if (windowTable.Get("y", &item)) w.y = GetDoubleFromJs(item);
                            if (windowTable.Get("angle", &item)) w.angle = GetDoubleFromJs(item);
                            if (windowTable.Get("calcType", &item)) w.calcType = GetIntFromJs(item);
                            
                            windows.Push(w);
                        }
                    }
                }
            }
            
            // Получаем параметры расчёта
            CassetteHelper::CalcParams params;
            GS::Ref<JS::Base> paramsBase;
            if (itemTable.Get("params", &paramsBase)) {
                if (GS::Ref<JS::Object> jsParams = GS::DynamicCast<JS::Object>(paramsBase)) {
                    const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& paramsTable = jsParams->GetItemTable();
                    GS::Ref<JS::Base> item;
                    if (paramsTable.Get("type", &item)) params.type = static_cast<CassetteHelper::CalcType>(GetIntFromJs(item));
                    if (paramsTable.Get("floorHeight", &item)) params.floorHeight = GetDoubleFromJs(item);
                    // Параметры для типа 0
                    if (paramsTable.Get("plankWidth0", &item)) params.plankWidth0 = GetIntFromJs(item); else params.plankWidth0 = 285;
                    if (paramsTable.Get("slopeWidth0", &item)) params.slopeWidth0 = GetIntFromJs(item); else params.slopeWidth0 = 285;
                    // Параметры для типов 1-2
                    if (paramsTable.Get("plankWidth12", &item)) params.plankWidth12 = GetIntFromJs(item); else params.plankWidth12 = 160;
                    if (paramsTable.Get("slopeWidth12", &item)) params.slopeWidth12 = GetIntFromJs(item); else params.slopeWidth12 = 225;
                    // Общие параметры
                    if (paramsTable.Get("offsetX", &item)) params.offsetX = GetIntFromJs(item);
                    if (paramsTable.Get("offsetY", &item)) params.offsetY = GetIntFromJs(item);
                    if (paramsTable.Get("offsetTop", &item)) params.offsetTop = GetIntFromJs(item); else params.offsetTop = 745;
                }
            }
            
            // Выполняем расчёт
            CassetteHelper::CalculationResult calcResult = 
                CassetteHelper::Calculate(windows, params);
            
            // Конвертируем результат в JS
            result->AddItem("success", new JS::Value(calcResult.success));
            result->AddItem("errorMessage", new JS::Value(calcResult.errorMessage));
            
            // Кассеты
            GS::Ref<JS::Array> jsCassettes = new JS::Array();
            for (const CassetteHelper::CassetteSize& cs : calcResult.cassettes) {
                GS::Ref<JS::Object> jsCs = new JS::Object();
                jsCs->AddItem("x", new JS::Value(cs.x));
                jsCs->AddItem("y", new JS::Value(cs.y));
                jsCs->AddItem("count", new JS::Value(cs.count));
                jsCassettes->AddItem(jsCs);
            }
            result->AddItem("cassettes", jsCassettes);
            
            // Планки
            GS::Ref<JS::Array> jsPlanks = new JS::Array();
            for (const CassetteHelper::PlankSize& ps : calcResult.planks) {
                GS::Ref<JS::Object> jsPs = new JS::Object();
                jsPs->AddItem("width", new JS::Value(ps.width));
                jsPs->AddItem("length", new JS::Value(ps.length));
                jsPs->AddItem("count", new JS::Value(ps.count));
                jsPs->AddItem("calcType", new JS::Value(ps.calcType));
                jsPlanks->AddItem(jsPs);
            }
            result->AddItem("planks", jsPlanks);
            
            // Левые откосы
            GS::Ref<JS::Array> jsLeftSlopes = new JS::Array();
            for (const CassetteHelper::PlankSize& ps : calcResult.leftSlopes) {
                GS::Ref<JS::Object> jsPs = new JS::Object();
                jsPs->AddItem("width", new JS::Value(ps.width));
                jsPs->AddItem("length", new JS::Value(ps.length));
                jsPs->AddItem("count", new JS::Value(ps.count));
                jsPs->AddItem("calcType", new JS::Value(ps.calcType));
                jsLeftSlopes->AddItem(jsPs);
            }
            result->AddItem("leftSlopes", jsLeftSlopes);
            
            // Правые откосы
            GS::Ref<JS::Array> jsRightSlopes = new JS::Array();
            for (const CassetteHelper::PlankSize& ps : calcResult.rightSlopes) {
                GS::Ref<JS::Object> jsPs = new JS::Object();
                jsPs->AddItem("width", new JS::Value(ps.width));
                jsPs->AddItem("length", new JS::Value(ps.length));
                jsPs->AddItem("count", new JS::Value(ps.count));
                jsPs->AddItem("calcType", new JS::Value(ps.calcType));
                jsRightSlopes->AddItem(jsPs);
            }
            result->AddItem("rightSlopes", jsRightSlopes);
            
            // Дубликаты
            GS::Ref<JS::Array> jsDuplicates = new JS::Array();
            for (const GS::UniString& dup : calcResult.duplicateIds) {
                jsDuplicates->AddItem(new JS::Value(dup));
            }
            result->AddItem("duplicates", jsDuplicates);
        }
        
        return result;
    }));

    // ------------------------------------------------------------
    // WriteCassetteResults - записать результаты в GDL объекты
    // ------------------------------------------------------------
    
    jsACAPI->AddItem(new JS::Function("WriteCassetteResults", [](GS::Ref<JS::Base> param) -> GS::Ref<JS::Base> {
        GS::Ref<JS::Object> result = new JS::Object();
        bool success = false;
        GS::UniString errorMessage;
        
        if (GS::Ref<JS::Object> jsParam = GS::DynamicCast<JS::Object>(param)) {
            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& itemTable = jsParam->GetItemTable();
            
            // Парсим результат расчёта
            CassetteHelper::CalculationResult calcResult;
            calcResult.success = true;
            
            // Получаем вложенный объект result
            GS::Ref<JS::Base> resultBase;
            if (!itemTable.Get("result", &resultBase)) {
                errorMessage = "Отсутствует объект result";
                result->AddItem("success", new JS::Value(false));
                result->AddItem("errorMessage", new JS::Value(errorMessage));
                return result;
            }
            
            GS::Ref<JS::Object> jsResultObj = GS::DynamicCast<JS::Object>(resultBase);
            if (!jsResultObj) {
                errorMessage = "result не является объектом";
                result->AddItem("success", new JS::Value(false));
                result->AddItem("errorMessage", new JS::Value(errorMessage));
                return result;
            }
            
            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& resultTable = jsResultObj->GetItemTable();
            
            // Парсим кассеты
            GS::Ref<JS::Base> cassettesBase;
            if (resultTable.Get("cassettes", &cassettesBase)) {
                if (GS::Ref<JS::Array> jsCassettes = GS::DynamicCast<JS::Array>(cassettesBase)) {
                    const GS::Array<GS::Ref<JS::Base>>& items = jsCassettes->GetItemArray();
                    for (UIndex i = 0; i < items.GetSize(); ++i) {
                        if (GS::Ref<JS::Object> jsCs = GS::DynamicCast<JS::Object>(items[i])) {
                            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& csTable = jsCs->GetItemTable();
                            CassetteHelper::CassetteSize cs;
                            GS::Ref<JS::Base> item;
                            if (csTable.Get("x", &item)) cs.x = GetIntFromJs(item);
                            if (csTable.Get("y", &item)) cs.y = GetIntFromJs(item);
                            if (csTable.Get("count", &item)) cs.count = GetIntFromJs(item);
                            calcResult.cassettes.Push(cs);
                        }
                    }
                }
            }
            
            // Парсим планки
            GS::Ref<JS::Base> planksBase;
            if (resultTable.Get("planks", &planksBase)) {
                if (GS::Ref<JS::Array> jsPlanks = GS::DynamicCast<JS::Array>(planksBase)) {
                    const GS::Array<GS::Ref<JS::Base>>& items = jsPlanks->GetItemArray();
                    for (UIndex i = 0; i < items.GetSize(); ++i) {
                        if (GS::Ref<JS::Object> jsPs = GS::DynamicCast<JS::Object>(items[i])) {
                            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& psTable = jsPs->GetItemTable();
                            CassetteHelper::PlankSize ps;
                            GS::Ref<JS::Base> item;
                            if (psTable.Get("width", &item)) ps.width = GetIntFromJs(item);
                            if (psTable.Get("length", &item)) ps.length = GetIntFromJs(item);
                            if (psTable.Get("count", &item)) ps.count = GetIntFromJs(item);
                            if (psTable.Get("calcType", &item)) ps.calcType = GetIntFromJs(item); else ps.calcType = 0;
                            calcResult.planks.Push(ps);
                        }
                    }
                }
            }
            
            // Парсим левые откосы
            GS::Ref<JS::Base> leftSlopesBase;
            if (resultTable.Get("leftSlopes", &leftSlopesBase)) {
                if (GS::Ref<JS::Array> jsLeftSlopes = GS::DynamicCast<JS::Array>(leftSlopesBase)) {
                    const GS::Array<GS::Ref<JS::Base>>& items = jsLeftSlopes->GetItemArray();
                    for (UIndex i = 0; i < items.GetSize(); ++i) {
                        if (GS::Ref<JS::Object> jsPs = GS::DynamicCast<JS::Object>(items[i])) {
                            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& psTable = jsPs->GetItemTable();
                            CassetteHelper::PlankSize ps;
                            GS::Ref<JS::Base> item;
                            if (psTable.Get("width", &item)) ps.width = GetIntFromJs(item);
                            if (psTable.Get("length", &item)) ps.length = GetIntFromJs(item);
                            if (psTable.Get("count", &item)) ps.count = GetIntFromJs(item);
                            if (psTable.Get("calcType", &item)) ps.calcType = GetIntFromJs(item); else ps.calcType = 0;
                            calcResult.leftSlopes.Push(ps);
                        }
                    }
                }
            }
            
            // Парсим правые откосы
            GS::Ref<JS::Base> rightSlopesBase;
            if (resultTable.Get("rightSlopes", &rightSlopesBase)) {
                if (GS::Ref<JS::Array> jsRightSlopes = GS::DynamicCast<JS::Array>(rightSlopesBase)) {
                    const GS::Array<GS::Ref<JS::Base>>& items = jsRightSlopes->GetItemArray();
                    for (UIndex i = 0; i < items.GetSize(); ++i) {
                        if (GS::Ref<JS::Object> jsPs = GS::DynamicCast<JS::Object>(items[i])) {
                            const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& psTable = jsPs->GetItemTable();
                            CassetteHelper::PlankSize ps;
                            GS::Ref<JS::Base> item;
                            if (psTable.Get("width", &item)) ps.width = GetIntFromJs(item);
                            if (psTable.Get("length", &item)) ps.length = GetIntFromJs(item);
                            if (psTable.Get("count", &item)) ps.count = GetIntFromJs(item);
                            if (psTable.Get("calcType", &item)) ps.calcType = GetIntFromJs(item); else ps.calcType = 0;
                            calcResult.rightSlopes.Push(ps);
                        }
                    }
                }
            }
            
            // Парсим целевые объекты
            CassetteHelper::TargetObjects targets;
            GS::Ref<JS::Base> targetsBase;
            if (itemTable.Get("targets", &targetsBase)) {
                if (GS::Ref<JS::Object> jsTargets = GS::DynamicCast<JS::Object>(targetsBase)) {
                    const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& targetsTable = jsTargets->GetItemTable();
                    GS::Ref<JS::Base> item;
                    // Объекты для типа 0
                    if (targetsTable.Get("plankId0", &item)) targets.plankId0 = GetStringFromJs(item);
                    if (targetsTable.Get("leftSlopeId0", &item)) targets.leftSlopeId0 = GetStringFromJs(item);
                    if (targetsTable.Get("rightSlopeId0", &item)) targets.rightSlopeId0 = GetStringFromJs(item);
                    // Объекты для типов 1-2
                    if (targetsTable.Get("cassetteId12", &item)) targets.cassetteId12 = GetStringFromJs(item);
                    if (targetsTable.Get("plankId12", &item)) targets.plankId12 = GetStringFromJs(item);
                    if (targetsTable.Get("leftSlopeId12", &item)) targets.leftSlopeId12 = GetStringFromJs(item);
                    if (targetsTable.Get("rightSlopeId12", &item)) targets.rightSlopeId12 = GetStringFromJs(item);
                }
            }
            
            // Парсим параметры для определения типа
            CassetteHelper::CalcParams params;
            GS::Ref<JS::Base> paramsBase;
            if (itemTable.Get("params", &paramsBase)) {
                if (GS::Ref<JS::Object> jsParams = GS::DynamicCast<JS::Object>(paramsBase)) {
                    const GS::HashTable<GS::UniString, GS::Ref<JS::Base>>& paramsTable = jsParams->GetItemTable();
                    GS::Ref<JS::Base> item;
                    if (paramsTable.Get("type", &item)) params.type = static_cast<CassetteHelper::CalcType>(GetIntFromJs(item));
                }
            } else {
                // Пытаемся определить тип из результата (если есть кассеты, то тип 1-2)
                params.type = (calcResult.cassettes.GetSize() > 0) 
                    ? CassetteHelper::CalcType::Type1And2 
                    : CassetteHelper::CalcType::Type0;
            }
            
            // Записываем результаты
            success = CassetteHelper::WriteToTargetObjects(calcResult, targets, params);
            if (!success) {
                errorMessage = "Не удалось записать результаты в объекты";
            }
        } else {
            errorMessage = "Неверные параметры";
        }
        
        result->AddItem("success", new JS::Value(success));
        result->AddItem("errorMessage", new JS::Value(errorMessage));
        
        return result;
    }));

    // ------------------------------------------------------------
    // Регистрируем объект в браузере
    // ------------------------------------------------------------
    browser.RegisterAsynchJSObject(jsACAPI);
}
