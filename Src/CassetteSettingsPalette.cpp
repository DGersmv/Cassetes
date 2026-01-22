// =============================================================================
// CassetteSettingsPalette - Палитра настроек кассет
// =============================================================================

#include "CassetteSettingsPalette.hpp"
#include "BrowserRepl.hpp"
#include "ResourceIDs.hpp"
#include "ACAPinc.h"
#include "DGModule.hpp"

// Уникальный GUID палитры
const GS::Guid CassetteSettingsPalette::s_guid("{d5b66f88-4e2a-5c9b-0d3f-6a7e8b9c0d1e}");

// Singleton instance
GS::Ref<CassetteSettingsPalette> CassetteSettingsPalette::instance;

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

CassetteSettingsPalette::CassetteSettingsPalette() :
    DG::Palette(ACAPI_GetOwnResModule(), CassetteSettingsPaletteResId, 
                ACAPI_GetOwnResModule(), s_guid),
    browser(GetReference(), CassetteSettingsBrowserCtrlId)
{
    // Подписываемся на события панели
    Attach(*this);
    BeginEventProcessing();
    
    // Регистрируем JS API
    BrowserRepl::RegisterACAPIJavaScriptObject(browser);
    
    // Загружаем HTML из ресурса
    GS::UniString html = LoadHtmlFromResource(CassetteSettingsHtmlResId);
    if (!html.IsEmpty()) {
        browser.LoadHTML(html);
    }
}

CassetteSettingsPalette::~CassetteSettingsPalette()
{
    EndEventProcessing();
    Detach(*this);
}

// =============================================================================
// Singleton управление
// =============================================================================

bool CassetteSettingsPalette::HasInstance()
{
    return instance != nullptr;
}

void CassetteSettingsPalette::CreateInstance()
{
    if (!HasInstance()) {
        instance = new CassetteSettingsPalette();
    }
}

CassetteSettingsPalette& CassetteSettingsPalette::GetInstance()
{
    return *instance;
}

void CassetteSettingsPalette::DestroyInstance()
{
    instance = nullptr;
}

// =============================================================================
// Показать / Скрыть
// =============================================================================

void CassetteSettingsPalette::ShowPalette()
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

void CassetteSettingsPalette::Show()
{
    DG::Palette::Show();
    SetMenuItemCheckedState(true);
}

void CassetteSettingsPalette::Hide()
{
    DG::Palette::Hide();
    SetMenuItemCheckedState(false);
}

void CassetteSettingsPalette::SetMenuItemCheckedState(bool isChecked)
{
    // Отмечаем пункт меню галочкой
    API_MenuItemRef menuRef = {};
    menuRef.menuResID = CassetteMenuResId;
    menuRef.itemIndex = CassetteMenuSettingsItem;
    
    GSFlags flags = isChecked ? API_MenuItemChecked : 0;
    ACAPI_MenuItem_SetMenuItemFlags(&menuRef, &flags, nullptr);
}

// =============================================================================
// DG Event Handlers
// =============================================================================

void CassetteSettingsPalette::PanelResized(const DG::PanelResizeEvent& ev)
{
    // Растягиваем браузер на всю палитру
    short width = ev.GetHorizontalChange() + GetOriginalClientWidth();
    short height = ev.GetVerticalChange() + GetOriginalClientHeight();
    
    browser.Resize(width, height);
}

void CassetteSettingsPalette::PanelCloseRequested(const DG::PanelCloseRequestEvent& /*ev*/, 
                                                  bool* accepted)
{
    *accepted = true;
    Hide();
}

// =============================================================================
// Palette Control Callback
// =============================================================================

GSErrCode CassetteSettingsPalette::RegisterPaletteControlCallBack()
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

GSErrCode CassetteSettingsPalette::PaletteControlCallBack(Int32 /*referenceID*/,
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
