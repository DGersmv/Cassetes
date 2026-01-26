#ifndef CASSETTESETTINGSPALETTE_HPP
#define CASSETTESETTINGSPALETTE_HPP

// =============================================================================
// CassetteSettingsPalette - Палитра настроек кассет
// =============================================================================

#include "DGModule.hpp"
#include "DGBrowser.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"

class CassetteSettingsPalette : public DG::Palette,\n                                public DG::PanelObserver,
                                public DG::CompoundItemObserver {
public:
    CassetteSettingsPalette();
    ~CassetteSettingsPalette();
    
    // Singleton управление
    static bool HasInstance();
    static void CreateInstance();
    static CassetteSettingsPalette& GetInstance();
    static void DestroyInstance();
    
    // Показать/скрыть
    static void ShowPalette();
    void Show();
    void Hide();
    
    // Регистрация callback
    static GSErrCode RegisterPaletteControlCallBack();
    static GSErrCode PaletteControlCallBack(Int32 referenceID, 
                                            API_PaletteMessageID messageID, 
                                            GS::IntPtr param);
    
private:
    static GS::Ref<CassetteSettingsPalette> instance;
    static const GS::Guid s_guid;
    
    DG::Browser browser;
    
    // DG overrides
    void PanelResized(const DG::PanelResizeEvent& ev) override;
    void PanelCloseRequested(const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    
    // Обновление состояния меню
    void SetMenuItemCheckedState(bool isChecked);
};

#endif // CASSETTESETTINGSPALETTE_HPP
