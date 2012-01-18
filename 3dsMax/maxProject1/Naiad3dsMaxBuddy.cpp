// -----------------------------------------------------------------------------
//
// Naiad3dsMaxBuddy.cpp
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

#define NaiadBuddy_CLASS_ID	Class_ID(0x59d78e7e, 0x9b2064f2)


// -----------------------------------------------------------------------------

//! DOCS
class NaiadBuddy : public UtilityObj 
{
public:

    //! Singleton access
    static NaiadBuddy* 
    GetInstance() 
    { 
        static NaiadBuddy instance;
        return &instance; 
    }

public:     // CTOR/DTOR 
    
    NaiadBuddy();

    virtual 
    ~NaiadBuddy();

public:

    virtual void 
    DeleteThis() 
    {}		
    
    virtual void 
    BeginEditParams(Interface *ip,IUtil *iu);

    virtual void 
    EndEditParams(Interface *ip,IUtil *iu);

    virtual void 
    Init(HWND hWnd);

    virtual void 
    Destroy(HWND hWnd);

public:     
    // These are the handles to the custom controls in the rollup page.
    // Public and ugly just like a "proper" plug-in...

    ICustButton *browseButton;
    ICustButton *importButton;
    ICustButton *exportButton;

    ICustEdit *importEdit;
    ICustEdit *exportEdit;

    ISpinnerControl *frameOffsetSpinner;
    ISpinnerControl *paddingSpinner;
    ISpinnerControl *firstFrameSpinner;
    ISpinnerControl *lastFrameSpinner;


private:

    static INT_PTR CALLBACK 
    DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:    // Member variables.

    HWND			hPanel;
    IUtil		   *iu;
    Interface	   *ip;
};

// -----------------------------------------------------------------------------

//! DOCS
class NaiadBuddyClassDesc : public ClassDesc2 
{
public:

    virtual int 
    IsPublic() 							
    { return TRUE; }

    virtual void* 
    Create(BOOL /*loading = FALSE*/) 	
    { return NaiadBuddy::GetInstance(); }

    virtual const TCHAR*	
    ClassName() 			
    { return GetString(IDS_CLASS_NAME); }

    virtual SClass_ID 
    SuperClassID() 				
    { return UTILITY_CLASS_ID; }

    virtual Class_ID 
    ClassID() 						
    { return NaiadBuddy_CLASS_ID; }

    virtual const TCHAR* 
    Category() 				
    { return GetString(IDS_CATEGORY); }

    //! Returns fixed parsable name (scripter-visible name)
    virtual const TCHAR* 
    InternalName() 			
    { return _T("Naiad Buddy"); }	

    //! returns owning module handle
    virtual HINSTANCE 
    HInstance() 					
    { return hInstance; }					
};

// -----------------------------------------------------------------------------

ClassDesc2* 
GetNaiadBuddyDesc() { 
    static NaiadBuddyClassDesc NaiadBuddyDesc;
    return &NaiadBuddyDesc; 
}

// -----------------------------------------------------------------------------
// NaiadBuddy implementation.

//! CTOR
NaiadBuddy::NaiadBuddy()
{
    // TODO: Call base CTOR?
    iu = NULL;
    ip = NULL;	
    hPanel = NULL;
}


//! DTOR.
NaiadBuddy::~NaiadBuddy()
{}


//! DOCS
void 
NaiadBuddy::BeginEditParams(Interface *ip, IUtil *iu) 
{
    this->iu = iu;
    this->ip = ip;
    hPanel = ip->AddRollupPage(
        hInstance,
        MAKEINTRESOURCE(IDD_PANEL),
        DlgProc,
        GetString(IDS_PARAMS),
        0);
}
    

//! DOCS
void 
NaiadBuddy::EndEditParams(Interface *ip, IUtil *iu) 
{
    this->iu = NULL;
    this->ip = NULL;
    ip->DeleteRollupPage(hPanel);
    hPanel = NULL;
}


//! DOCS
void NaiadBuddy::Init(HWND hWnd)
{
    browseButton = GetICustButton(GetDlgItem(hWnd, IDC_IMPORT_BROWSE_BUTTON));
    importButton = GetICustButton(GetDlgItem(hWnd, IDC_IMPORT_BUTTON));
    exportButton = GetICustButton(GetDlgItem(hWnd, IDC_EXPORT_BUTTON));
    
    importEdit = GetICustEdit(GetDlgItem(hWnd, IDC_IMPORT_EDIT));
    exportEdit = GetICustEdit(GetDlgItem(hWnd, IDC_EXPORT_EDIT));

    paddingSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_PADDING_SPINNER,        // Spinner Id.
            IDC_PADDING_EDIT,           // Edit Id.
            1,                          // Min.
            8,                          // Max.
            4);                         // Value.
    frameOffsetSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_FRAME_OFFSET_SPINNER,   // Spinner Id.
            IDC_FRAME_OFFSET_EDIT,      // Edit Id.
            -10000,                     // Min.
            10000,                      // Max.
            0);                         // Value.
    firstFrameSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_FIRST_FRAME_SPINNER,    // Spinner Id.
            IDC_FIRST_FRAME_EDIT,       // Edit Id.
            -10000,                     // Min.
            10000,                      // Max.
            ip->GetAnimRange().Start()/GetTicksPerFrame());// Value.
    lastFrameSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_LAST_FRAME_SPINNER,     // Spinner Id.
            IDC_LAST_FRAME_EDIT,        // Edit Id.
            -10000,                     // Min.
            10000,                      // Max.
            ip->GetAnimRange().End()/GetTicksPerFrame());  // Value.
}


//! DOCS
void NaiadBuddy::Destroy(HWND hWnd)
{
    // Release all our Custom Controls.

    ReleaseICustButton(browseButton);
    ReleaseICustButton(importButton);
    ReleaseICustButton(exportButton);

    ReleaseICustEdit(importEdit);
    ReleaseICustEdit(exportEdit);

    ReleaseISpinner(paddingSpinner);
    ReleaseISpinner(frameOffsetSpinner);
    ReleaseISpinner(firstFrameSpinner);
    ReleaseISpinner(lastFrameSpinner);

    //ReleaseISpinner(u->ccSpin);
    //ReleaseIColorSwatch(u->ccColorSwatch);
    //ReleaseICustEdit(u->ccEdit);
    //ReleaseICustEdit(u->ccTCBEdit);
    //ReleaseISpinner(u->ccTCBSpin);
    //ReleaseICustButton(u->ccButtonP);
    //ReleaseICustButton(u->ccButtonC);
    //ReleaseICustButton(u->ccFlyOff);
    //ReleaseICustStatus(u->ccStatus);
    //ReleaseICustToolbar(u->ccToolbar);
    //ReleaseICustImage(u->ccImage);
    //ReleaseISlider(u->ccSlider);
    //ReleaseICustEdit(u->ccSliderEdit);
    //ReleaseISlider(u->ccSlider1);
    //ReleaseICustEdit(u->ccSliderEdit1);
    //ReleaseISlider(u->ccSlider2);
    //ReleaseICustEdit(u->ccSliderEdit2);
}


//! DOCS [static]
INT_PTR CALLBACK 
NaiadBuddy::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
    {
    case WM_INITDIALOG:
        NaiadBuddy::GetInstance()->Init(hWnd);
        break;
    case WM_DESTROY:
        NaiadBuddy::GetInstance()->Destroy(hWnd);
        break;
    case WM_COMMAND:
        {
        //#pragma message(TODO("React to the user interface commands.  A utility plug-in is controlled by the user from here."))

        TSTR filename;
        TSTR dir;
        FilterList filter;
        Interface8 *i8 = GetCOREInterface8();
        i8->DoMaxOpenDialog(hWnd, "blarh", filename, dir, filter);
        //i8->DoMaxSaveAsDialog(hWnd, "blarh", filename, dir, filter);
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        NaiadBuddy::GetInstance()->ip->RollupMouseMessage(
            hWnd, msg, wParam, lParam); 
        break;
    default:
        return 0;
    }
    return 1;
}
