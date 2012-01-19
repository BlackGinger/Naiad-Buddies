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

#include "EmpImport.h"
#include "EmpMeshObject.h"
#include <Max.h>

#include <NbEmpReader.h>
#include <NbFilename.h>
#include <NbFactory.h>
#include <NbBodyReader.h>
#include <NbSequenceReader.h>
#include <NbBody.h>
#include <NbString.h>
#include <em_log.h>

#include <memory> // std::auto_ptr

//------------------------------------------------------------------------------

//! Import file.
//! One of the following three values should be returned:
//! #define IMPEXP_FAIL    0
//! #define IMPEXP_SUCCESS 1
//! #define IMPEXP_CANCEL  2
int 
EmpImport::DoImport(const TCHAR  *filename,
                    ImpInterface *i,
                    Interface    *gi, 
                    BOOL          suppressPrompts /* = FALSE */)
{
    //#pragma message(TODO("Implement the actual file import here and"))	

    return _importEmpFile(filename, gi, i) ? IMPEXP_SUCCESS : IMPEXP_FAIL;

    //if(!suppressPrompts)
    //    DialogBoxParam(hInstance, 
    //            MAKEINTRESOURCE(IDD_PANEL), 
    //            GetActiveWindow(), 
    //            MaxEmpImportOptionsDlgProc, (LPARAM)this);
    //#pragma message(TODO("return TRUE If the file is imported properly"))
}
    
//------------------------------------------------------------------------------

//! DOCS
BOOL
EmpImport::_importEmpFile(const TCHAR  *filename, 
                          Interface    *i,
                          ImpInterface *ii)
{
    // (1) - Open file.
    // (2) - Discover list of bodies in file (with time-stamps?)
    // (3) - Iterate over bodies, using body signatures to determine
    //       which type of 3ds Max object to create, e.g. Mesh or Points.
    //   (3a) - Add created objects to scene.
    // (4) - Close file.

    try {
        // Information about the provided file...

        Nb::EmpReader empReader(filename, "*", "Body"); // May throw.
        NB_INFO("EMP time: " << empReader.time());
        NB_INFO("EMP revision: " << empReader.revision());
        NB_INFO("EMP body count: " << empReader.bodyCount());
        int frame = 0;
        int timestep = 0;
        Nb::extractFrameAndTimestep(filename, frame, timestep);
        NB_INFO("Frame: " << frame);
        NB_INFO("Timestep: " << timestep);
        const Nb::String sequenceName = Nb::hashifyFilename(filename);
        NB_INFO("Sequence name: '" << sequenceName << "'\n");

        // Scrub to the frame in the EMP file name.

        i->SetTime(frame*GetTicksPerFrame(), FALSE);

        while (empReader.bodyCount() > 0) {
            // Get an Nb::Body from the EMP archive. We get ownership of
            // the retrieved Nb::Body. 

            std::auto_ptr<Nb::Body> body(empReader.popBody());

            // Create an EmpObject for each body. We own this object 
            // until it is attached to a node and added to the scene. If it 
            // can't be added we must delete it below.

            EmpMeshObject *empMeshObject = 
                static_cast<EmpMeshObject*>(
                    i->CreateInstance(
                        GEOMOBJECT_CLASS_ID, EmpMeshObject_CLASS_ID));

            // Set information required by the object's 
            // internal sequence reader.

            empMeshObject->setBodyName(body->name());
            empMeshObject->setSequenceName(sequenceName);
            //empObject->BuildMesh(i->GetTime());

            ImpNode *impNode = ii->CreateNode();
            if (NULL != impNode) {
                Matrix3 xf; // TMP! Hard-coded transform and time-value. 
                xf.IdentityMatrix();   // Body data already in world-space.
                const TimeValue t = 0; // No time-varying XForm.
                impNode->SetTransform(t, xf);
                impNode->Reference(empMeshObject);
                MSTR nodeName = _M(empMeshObject->bodyName().c_str());
                i->MakeNameUnique(nodeName);
                impNode->SetName(nodeName);
                ii->AddNodeToScene(impNode);
            }
            else {
                // For some reason the node could not be added to the scene.
                // Simply delete the EmpObject and move on, hoping
                // for better luck with the rest of the Nb::Body objects.

                delete empMeshObject;
            }
        }

        ii->RedrawViews();
        empReader.close();
        return TRUE;   // Success.
    }
    catch (std::exception &ex) {
        NB_ERROR("exception: " << ex.what());
        return FALSE;   // Failure.
    }
    catch (...) {
        NB_ERROR("unknown exception");
        return FALSE;   // Failure.
    }
}

//------------------------------------------------------------------------------

ClassDesc2* 
GetEmpImportDesc() 
{ 
    static EmpImportClassDesc EmpImportDesc;
    return &EmpImportDesc; 
}

//------------------------------------------------------------------------------

INT_PTR CALLBACK 
EmpImportOptionsDlgProc(HWND   hWnd,
                        UINT   message,
                        WPARAM wParam,
                        LPARAM lParam) 
{
    static EmpImport *imp = NULL;
    switch (message) {
    case WM_INITDIALOG:
        imp = (EmpImport *)lParam;
        CenterWindow(hWnd, GetParent(hWnd));
        return TRUE;
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
