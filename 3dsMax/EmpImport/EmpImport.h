// -----------------------------------------------------------------------------
//
// EmpImport.cpp
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

#ifndef EMP_IMPORT_H_INCLUDED
#define EMP_IMPORT_H_INCLUDED

#include <impexp.h>
#include <iparamb2.h>

//------------------------------------------------------------------------------

#define EmpImport_CLASS_ID Class_ID(0xa84bdfb3, 0x4fc93524)

//------------------------------------------------------------------------------

//extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

//------------------------------------------------------------------------------

//! DOCS
class EmpImport : public SceneImport 
{
public:

    static HWND hParams;
        
public:     // CTOR/DTOR
    
    //! CTOR.
    EmpImport()
    {}

    //! DTOR.
    virtual
    ~EmpImport()
    {}

public:

    //! Number of extensions supported.
    virtual int				
    ExtCount()
    { return 1; }

    //! Extension #n (i.e. "3DS").
    virtual const TCHAR*	
    Ext(int n)
    {
        switch (n) {
        case 0:
        return _T("emp");
        default:
        return _T("");  // Bad call!
        }
    }

    //! Long ASCII description (i.e. "Autodesk 3D Studio File").
    virtual const TCHAR*	
    LongDesc()
    { return _T("Exotic Matter EMP File"); }

    //! Short ASCII description (i.e. "3D Studio").
    virtual const TCHAR*	
    ShortDesc()
    { return _T("Exotic Matter"); }

    //! ASCII Author name.
    virtual const TCHAR*	
    AuthorName()
    { return _T("Exotic Matter"); }

    //! ASCII Copyright message.
    virtual const TCHAR*	
    CopyrightMessage()
    { return _T("Copyright Exotic Matter 2012"); }

    //! Other message #1, if any.
    virtual const TCHAR*	
    OtherMessage1()
    { return _T(""); }

    //! Other message #2, if any.
    virtual const TCHAR*	
    OtherMessage2()
    { return _T(""); }

    //! Version number * 100 (i.e. v3.01 = 301).
    virtual unsigned int	
    Version()
    { return 10; } // Version 0.1

    //! Show DLL's "About..." box. Optional.
    virtual void			
    ShowAbout(HWND hWnd)
    {}
    
    virtual int				
    DoImport(const TCHAR  *filename,
             ImpInterface *i,
             Interface    *gi, 
             BOOL          suppressPrompts = FALSE);	

private:

    BOOL
    _importEmpFile(const TCHAR *filename, Interface *i, ImpInterface *ii);

    //GeomObject*
    //_importBody(const Nb::Body *nbBody, Interface *i);

    //BOOL
    //_buildMesh(const Nb::PointShape    &point, 
    //           const Nb::TriangleShape &triangle,
    //           Mesh                    &mesh);

    //BOOL
    //_buildParticle(const Nb::ParticleShape &particle,
    //               ParticleSys             &psys);

    //BOOL
    //_buildCamera();

    //BOOL
    //_addGeomObjectToScene(GeomObject   &geomObject, 
    //                      const char   *name, 
    //                      ImpInterface *ii);
};

//------------------------------------------------------------------------------

class EmpImportClassDesc : public ClassDesc2 
{
public:

    virtual int 
    IsPublic() 							
    { return TRUE; }

    virtual void* 
    Create(BOOL /*loading = FALSE*/) 		
    { return new EmpImport(); }

    virtual const TCHAR*	
    ClassName() 			
    { return _T("EmpImport"); } // GetString(IDS_CLASS_NAME);

    virtual SClass_ID 
    SuperClassID() 				
    { return SCENE_IMPORT_CLASS_ID; }
    
    virtual Class_ID 
    ClassID() 						
    { return EmpImport_CLASS_ID; }
    
    virtual const TCHAR* 
    Category() 				
    { return _T("Exotic Matter"); } // GetString(IDS_CATEGORY);

    //! Returns fixed parsable name (scripter-visible name).
    virtual const TCHAR* 
    InternalName() 			
    { return _T("EmpImport"); }

    //! Returns owning module handle.
    virtual HINSTANCE 
    HInstance() 					
    { return hInstance; }					
};

//------------------------------------------------------------------------------

ClassDesc2* 
GetEmpImportDesc();

//------------------------------------------------------------------------------

INT_PTR CALLBACK 
EmpImportOptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); 

//------------------------------------------------------------------------------

#endif // EMP_IMPORT_H_INCLUDED
