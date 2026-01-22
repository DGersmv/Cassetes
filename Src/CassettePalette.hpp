#ifndef CASSETTEPALETTE_HPP
#define CASSETTEPALETTE_HPP

// =============================================================================
// CassettePalette - Палитра расчёта кассет
// =============================================================================

#include "DGModule.hpp"
#include "DGBrowser.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"

class CassettePalette : public DG::Palette,
                        public DG::PanelObserver,
                        public DG::CompoundItemObserver {
public:
    CassettePalette();
    ~CassettePalette();
    
    // Singleton управление
    static bool HasInstance();
    static void CreateInstance();
    static CassettePalette& GetInstance();
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
    static GS::Ref<CassettePalette> instance;
    static const GS::Guid s_guid;
    
    DG::Browser browser;
    
    // DG overrides
    void PanelResized(const DG::PanelResizeEvent& ev) override;
    void PanelCloseRequested(const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    
    // Обновление состояния меню
    void SetMenuItemCheckedState(bool isChecked);
};

#endif // CASSETTEPALETTE_HPP
