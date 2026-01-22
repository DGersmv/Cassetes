// *****************************************************************************
// Source code for Cassette Panel Add-On
// *****************************************************************************

// =============================================================================
// API Includes
// =============================================================================

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "ResourceIDs.hpp"
#include "APICommon.h"

// Cassette Panel includes
#include "CassettePalette.hpp"

// -----------------------------------------------------------------------------
// MenuCommandHandler
//		called to perform the user-asked command
// -----------------------------------------------------------------------------

GSErrCode MenuCommandHandler (const API_MenuParams *menuParams)
{
	switch (menuParams->menuItemRef.menuResID) {
		case CassetteMenuResId:
			switch (menuParams->menuItemRef.itemIndex) {
				case CassetteMenuCalcItem:  // "Расчёт кассет"
					CassettePalette::ShowPalette();
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return NoError;
}


// =============================================================================
// Required functions
// =============================================================================

// -----------------------------------------------------------------------------
// Dependency definitions
// -----------------------------------------------------------------------------

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, AddOnNameStrResId, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, AddOnNameStrResId, 2, ACAPI_GetOwnResModule ());

	return APIAddon_Preload;
}


// -----------------------------------------------------------------------------
// Interface definitions
// -----------------------------------------------------------------------------

GSErrCode RegisterInterface (void)
{
	GSErrCode err = ACAPI_MenuItem_RegisterMenu (CassetteMenuResId, 0, MenuCode_UserDef, MenuFlag_Default);
	if (DBERROR (err != NoError))
		return err;

	return err;
}


// -----------------------------------------------------------------------------
// Initialize
//		called after the Add-On has been loaded into memory
// -----------------------------------------------------------------------------

GSErrCode Initialize ()
{
	// 1) Установка обработчика меню
	GSErrCode err = ACAPI_MenuItem_InstallMenuHandler (CassetteMenuResId, MenuCommandHandler);
	if (DBERROR (err != NoError))
		return err;

	// 2) Регистрация палитр
	GSErrCode palErr = CassettePalette::RegisterPaletteControlCallBack();
	
	if (DBERROR (palErr != NoError))
		return palErr;

	return NoError;
}


// -----------------------------------------------------------------------------
// FreeData
//		called when the Add-On is going to be unloaded
// -----------------------------------------------------------------------------

GSErrCode FreeData (void)
{
	return NoError;
}
