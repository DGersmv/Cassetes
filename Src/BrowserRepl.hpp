#ifndef BROWSERREPL_HPP
#define BROWSERREPL_HPP

// =============================================================================
// BrowserRepl - JavaScript Bridge для Cassette Panel
// Минимальная версия без toolbar, только JS API регистрация
// =============================================================================

#include "DGModule.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"

namespace DG { class Browser; }

namespace BrowserRepl {
    // Регистрация JavaScript API объекта "window.ACAPI" в браузере палитры
    void RegisterACAPIJavaScriptObject(DG::Browser& targetBrowser);
}

#endif // BROWSERREPL_HPP
