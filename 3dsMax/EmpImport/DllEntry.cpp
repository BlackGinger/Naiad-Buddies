//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Contains the Dll Entry stuff
// AUTHOR: 
//***************************************************************************/

#include "EmpImport.h"
#include <Nb.h>
#include <NbLog.h>
#include <em_log.h>

//------------------------------------------------------------------------------

extern ClassDesc2* GetEmpImportDesc();
extern ClassDesc2* GetEmpObjectDesc();

HINSTANCE hInstance;
int controlsInit = FALSE;

//------------------------------------------------------------------------------

//! This function is called by Windows when the DLL is loaded.  This 
//! function may also be called many times during time critical operations
//! like rendering.  Therefore developers need to be careful what they
//! do inside this function.  In the code below, note how after the DLL is
//! loaded the first time only a few statements are executed.

BOOL WINAPI 
DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID /*lpvReserved*/)
{
    if( fdwReason == DLL_PROCESS_ATTACH ) {
        // Hang on to this DLL's instance handle.

        hInstance = hinstDLL;
        DisableThreadLibraryCalls(hInstance);
    }
    return(TRUE);
}


//! This function returns a string that describes the DLL and where the user
//! could purchase the DLL if they don't have it.
__declspec(dllexport) const TCHAR* 
LibDescription()
{
    return "Exotic Matter's EMP importer for 3ds Max 2012 x64 (release)";
}


//! This function returns the number of plug-in classes this DLL contains.
//! TODO: Must change this number when adding a new class
__declspec(dllexport) int 
LibNumberClasses()
{
    return 2;
}


//! Return descriptions of plug-in classes in DLL.
__declspec(dllexport) ClassDesc* 
LibClassDesc(int i)
{
    switch(i) {
    case 0: return GetEmpImportDesc();
    case 1: return GetEmpObjectDesc();
    default: return 0;
    }
}


//! This function returns a pre-defined constant indicating the version of 
//! the system under which it was compiled. It is used to allow the system
//! to catch obsolete DLLs.
__declspec(dllexport) ULONG 
LibVersion()
{
    return VERSION_3DSMAX;
}


//! This function is called once, right after your plugin has been loaded by 
//! 3ds Max. Perform one-time plugin initialization in this method.
//! Return TRUE if you deem your plugin successfully loaded, or FALSE otherwise. 
//! If the function returns FALSE, the system will NOT load the plugin, it will 
//! then call FreeLibrary on your DLL, and send you a message.
__declspec(dllexport) int 
LibInitialize(void)
{
    try {
        const std::string naiadPath = std::getenv("NAIAD_PATH");
        em::open_log(naiadPath + "/EmpImportLog.txt"); // May throw.
    }
    catch (...) { // Pokemon: Gotta catch 'em all...
        return FALSE;   
    }

    // Log is available below.

    try {
        Nb::verboseLevel = "VERBOSE";
        Nb::begin(true);    // Register signatures.
        return TRUE; // Success.
    }
    catch (std::exception &ex) {
        NB_ERROR("exception: " << ex.what());
        return FALSE;   // Failure.
    }
    catch (...) { // Pokemon: Gotta catch 'em all!
        NB_ERROR("unknown exception");
        return FALSE;   // Failure.
    }
}


//! This function is called once, just before the plugin is unloaded. 
//! Perform one-time plugin un-initialization in this method."
//! The system doesn't pay attention to a return value.
__declspec(dllexport) int 
LibShutdown(void)
{
    try {
        // Perform un-initialization here.	

        Nb::end();
        em::close_log();
    }
    catch (...) {
    }

    return TRUE;    // Ignored.
}

//------------------------------------------------------------------------------

//TCHAR*
//GetString(int id)
//{
//    static TCHAR buf[256];
//    if (hInstance) {
//        return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
//    }
//    return NULL;
//}
