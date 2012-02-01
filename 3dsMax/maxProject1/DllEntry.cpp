// -----------------------------------------------------------------------------
//
// DllEntry.cpp
//
// DOCS
//
// Copyright (c) 2009-2012 Exotic Matter AB.  All rights reserved.
//
// This material contains the confidential and proprietary information of
// Exotic Matter AB and may not be disclosed, copied or duplicated in
// any form, electronic or hardcopy, in whole or in part, without the
// express prior written consent of Exotic Matter AB.  This copyright notice
// does not imply publication.
// 
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT 
//    LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
//    FOR  A  PARTICULAR  PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL THE
//    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//    BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS  OR  SERVICES; 
//    LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER
//    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT
//    LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN
//    ANY  WAY OUT OF THE USE OF  THIS SOFTWARE,  EVEN IF ADVISED OF  THE
//    POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------------

#include "Naiad3dsMaxBuddy.h"

#include <Nb.h>
#include <NbLog.h>
#include <em_log.h>

//------------------------------------------------------------------------------

extern ClassDesc2* GetNaiadBuddyDesc();
extern ClassDesc2* GetEmpMeshObjectDesc();

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
    if (fdwReason == DLL_PROCESS_ATTACH) {
        hInstance = hinstDLL; // Hang on to this DLL's instance handle.
        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}


//! This function returns a string that describes the DLL and where the user
//! could purchase the DLL if they don't have it.
__declspec(dllexport) const TCHAR* 
LibDescription()
{ return GetString(IDS_LIBDESCRIPTION); }


//! This function returns the number of plug-in classes this DLL contains.
//! TODO: Must change this number when adding a new class
__declspec(dllexport) int 
LibNumberClasses()
{ return 2; }


//! Return descriptions of plug-in classes in DLL.
__declspec(dllexport) ClassDesc* 
LibClassDesc(int i)
{
    switch (i) {
    case  0: return GetNaiadBuddyDesc();
    case  1: return GetEmpMeshObjectDesc();
    default: return 0;
    }
}


//! This function returns a pre-defined constant indicating the version of 
//! the system under which it was compiled. It is used to allow the system
//! to catch obsolete DLLs.
__declspec(dllexport) ULONG 
LibVersion()
{ return VERSION_3DSMAX; }


//! This function is called once, right after your plugin has been loaded by 
//! 3ds Max. Perform one-time plugin initialization in this method.
//! Return TRUE if you deem your plugin successfully loaded, or FALSE otherwise. 
//! If the function returns FALSE, the system will NOT load the plugin, it will 
//! then call FreeLibrary on your DLL, and send you a message.
__declspec(dllexport) int 
LibInitialize(void)
{
    try {
        em::open_log(GetLogFileName()); // May throw.
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

    return TRUE; // TODO: Perform initialization here.
}


//! This function is called once, just before the plugin is unloaded. 
//! Perform one-time plugin un-initialization in this method.
//! The system doesn't pay attention to a return value.
__declspec(dllexport) int 
LibShutdown(void)
{
    try {         // Perform un-initialization here.	
        Nb::end();
        em::close_log();
    }
    catch (...) { // Pokemon: Gotta catch 'em all!
    }
    return TRUE;    // Ignored.
}

//------------------------------------------------------------------------------

//! DOCS
TCHAR*
GetString(int id)
{
    static TCHAR buf[256];
    if (hInstance) {
        return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
    }
    return NULL;
}

//------------------------------------------------------------------------------

std::string
GetLogFileName()
{
    const std::string naiadPath = 
        std::getenv("NAIAD_PATH");
    const std::string logFileName = 
        std::string(naiadPath + "/" + GetString(IDS_NAIAD_BUDDY_LOG_FILE_NAME));
    return logFileName;
}

//------------------------------------------------------------------------------
