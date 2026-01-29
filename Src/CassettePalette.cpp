// =============================================================================
// CassettePalette - Палитра расчёта кассет
// =============================================================================

#include "CassettePalette.hpp"
#include "BrowserRepl.hpp"
#include "ResourceIDs.hpp"
#include "ACAPinc.h"
#include "DGModule.hpp"

// Уникальный GUID палитры
const GS::Guid CassettePalette::s_guid("{c4a55e77-3d1f-4b8a-9c2e-5f6d7a8b9c0d}");

// Singleton instance
GS::Ref<CassettePalette> CassettePalette::instance;

// Загрузка HTML из ресурса
static GS::UniString LoadHtmlFromResource(short resId)
{
    GS::UniString resourceData;
    GSHandle data = RSLoadResource('DATA', ACAPI_GetOwnResModule(), resId);
    if (data != nullptr) {
        GSSize handleSize = BMhGetSize(data);
        resourceData.Append(*data, handleSize);
        BMhKill(&data);
    }
    return resourceData;
}

// =============================================================================
// Конструктор / Деструктор
// =============================================================================

CassettePalette::CassettePalette() :
    DG::Palette(ACAPI_GetOwnResModule(), CassettePaletteResId, 
                ACAPI_GetOwnResModule(), s_guid),
    browser(GetReference(), CassetteBrowserCtrlId)
{
    // Подписываемся на события панели
    Attach(*this);
    BeginEventProcessing();
    
    // Устанавливаем минимальный размер палитры (ширина контента + padding)
    SetMinSize(520, 400);
    
    // Регистрируем JS API
    BrowserRepl::RegisterACAPIJavaScriptObject(browser);
    
    // Загружаем HTML из ресурса
    GS::UniString html = LoadHtmlFromResource(CassetteHtmlResId);
    if (!html.IsEmpty()) {
        browser.LoadHTML(html);
    }
}

CassettePalette::~CassettePalette()
{
    EndEventProcessing();
    Detach(*this);
}

// =============================================================================
// Singleton управление
// =============================================================================

bool CassettePalette::HasInstance()
{
    return instance != nullptr;
}

void CassettePalette::CreateInstance()
{
    if (!HasInstance()) {
        instance = new CassettePalette();
    }
}

CassettePalette& CassettePalette::GetInstance()
{
    return *instance;
}

void CassettePalette::DestroyInstance()
{
    instance = nullptr;
}

// =============================================================================
// Показать / Скрыть
// =============================================================================

void CassettePalette::ShowPalette()
{
    if (!HasInstance()) {
        CreateInstance();
    }
    
    if (GetInstance().IsVisible()) {
        GetInstance().Hide();
    } else {
        GetInstance().Show();
    }
}

void CassettePalette::Show()
{
    DG::Palette::Show();
    SetMenuItemCheckedState(true);
}

void CassettePalette::Hide()
{
    DG::Palette::Hide();
    SetMenuItemCheckedState(false);
}

void CassettePalette::SetMenuItemCheckedState(bool isChecked)
{
    // Отмечаем пункт меню галочкой
    API_MenuItemRef menuRef = {};
    menuRef.menuResID = CassetteMenuResId;
    menuRef.itemIndex = CassetteMenuCalcItem;
    
    GSFlags flags = isChecked ? API_MenuItemChecked : 0;
    ACAPI_MenuItem_SetMenuItemFlags(&menuRef, &flags, nullptr);
}

// =============================================================================
// DG Event Handlers
// =============================================================================

void CassettePalette::PanelResized(const DG::PanelResizeEvent& /*ev*/)
{
    // Растягиваем браузер на всю клиентскую область палитры
    short width = GetClientWidth();
    short height = GetClientHeight();
    
    // Перемещаем браузер в начало и задаём размер равный клиентской области
    browser.Move(0, 0);
    browser.Resize(width, height);
}

void CassettePalette::PanelCloseRequested(const DG::PanelCloseRequestEvent& /*ev*/, 
                                          bool* accepted)
{
    *accepted = true;
    Hide();
}

// =============================================================================
// Palette Control Callback
// =============================================================================

GSErrCode CassettePalette::RegisterPaletteControlCallBack()
{
    return ACAPI_RegisterModelessWindow(
        GS::CalculateHashValue(s_guid),
        PaletteControlCallBack,
        API_PalEnabled_FloorPlan | API_PalEnabled_Section | 
        API_PalEnabled_Elevation | API_PalEnabled_InteriorElevation |
        API_PalEnabled_3D | API_PalEnabled_Detail | API_PalEnabled_Worksheet |
        API_PalEnabled_Layout | API_PalEnabled_DocumentFrom3D,
        GSGuid2APIGuid(s_guid)
    );
}

GSErrCode CassettePalette::PaletteControlCallBack(Int32 /*referenceID*/,
                                                  API_PaletteMessageID messageID,
                                                  GS::IntPtr /*param*/)
{
    switch (messageID) {
        case APIPalMsg_ClosePalette:
            if (HasInstance()) {
                GetInstance().Hide();
            }
            break;
            
        case APIPalMsg_HidePalette_Begin:
            if (HasInstance() && GetInstance().IsVisible()) {
                GetInstance().Hide();
            }
            break;
            
        case APIPalMsg_HidePalette_End:
            break;
            
        default:
            break;
    }
    
    return NoError;
}
