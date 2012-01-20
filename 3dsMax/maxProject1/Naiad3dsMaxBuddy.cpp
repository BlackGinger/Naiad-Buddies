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
#include <simpobj.h>

#include <NbFilename.h>
#include <NbFactory.h>
#include <NbBodyReader.h>
#include <NbEmpReader.h>
#include <NbSequenceReader.h>
#include <NbBody.h>
#include <NbString.h>
#include <em_log.h>

// -----------------------------------------------------------------------------

#define NaiadBuddy_CLASS_ID	   Class_ID(0x59d78e7e, 0x9b2064f2)
#define EmpMeshObject_CLASS_ID Class_ID(0x311e0e37, 0x10a162b1)

//------------------------------------------------------------------------------

//! DOCS
class EmpMeshObject : public SimpleObject2
{
public:

    EmpMeshObject();

    //! DTOR.
    virtual
    ~EmpMeshObject() 
    {}


    // From BaseObject.

    virtual CreateMouseCallBack* 
    GetCreateMouseCallBack() 
    { return NULL; }

    
    // From SimpleObject.

    virtual void 
    BuildMesh(TimeValue t);

    //! Returns true if this object has a valid mesh.
    virtual BOOL
    OKtoDisplay();


    //From Animatable.

    virtual Class_ID 
    ClassID() 
    { return EmpMeshObject_CLASS_ID; }        

    virtual SClass_ID 
    SuperClassID() 
    { return GEOMOBJECT_CLASS_ID; }

    virtual void 
    GetClassName(TSTR &s) 
    { s = GetString(IDS_EMP_MESH_OBJECT_CLASS_NAME); }

    virtual void 
    DeleteThis() 
    { delete this; }

public:

    //! Expose so that we can set up the parameters from outside.
    Nb::SequenceReader&
    sequenceReader()
    { return _seqReader; }

    const Nb::String&
    bodyName() const
    { return _bodyName; }

    //! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
    void
    setBodyName(const Nb::String &bodyName)
    { _bodyName = bodyName; }

    //! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
    void
    setFrameOffset(const int frameOffset)
    { _frameOffset = frameOffset; }

private:

    static const Nb::String _signature;

    void
    _rebuildMesh(const int frame);

    void
    _clearMesh();

    void
    _buildMesh(const Nb::PointShape &point, const Nb::TriangleShape &triangle);

    void
    _addMap3f(MeshMap &map, const Nb::Buffer3f &buf, const Nb::Buffer3i &index);

private:    // Member variables.

    Nb::SequenceReader _seqReader; 
    Nb::String         _bodyName;  //!< The name of the body this object tracks.
    int                _frameOffset;
    int                _meshFrame; 
    BOOL               _validMesh;
};

const Nb::String EmpMeshObject::_signature("Mesh");

//------------------------------------------------------------------------------

//! DOCS
class EmpMeshObjectClassDesc : public ClassDesc2 
{
public:

    virtual int 
    IsPublic() 							
    { return TRUE; }

    virtual void* 
    Create(BOOL /*loading = FALSE*/) 		
    { return new EmpMeshObject(); }

    virtual const TCHAR*	
    ClassName() 			
    { return GetString(IDS_EMP_MESH_OBJECT_CLASS_NAME); }

    virtual SClass_ID 
    SuperClassID() 				
    { return GEOMOBJECT_CLASS_ID; } // TODO: Not sure what to put here!?
    
    virtual Class_ID 
    ClassID() 						
    { return EmpMeshObject_CLASS_ID; }
    
    virtual const TCHAR* 
    Category() 				
    { return GetString(IDS_CATEGORY); }

    //! Returns fixed parsable name (scripter-visible name).
    virtual const TCHAR* 
    InternalName() 			
    { return GetString(IDS_EMP_MESH_OBJECT_INTERNAL_NAME); }

    //! Returns owning module handle.
    virtual HINSTANCE 
    HInstance() 					
    { return hInstance; }					
};

//------------------------------------------------------------------------------

ClassDesc2* 
GetEmpMeshObjectDesc()
{
    static EmpMeshObjectClassDesc instance;
    return &instance; 
}

//------------------------------------------------------------------------------

//! CTOR.
EmpMeshObject::EmpMeshObject() 
    : SimpleObject2()
    , _bodyName("")
    , _frameOffset(0)
    , _meshFrame(TIME_NegInfinity/GetTicksPerFrame())
    , _validMesh(FALSE)
{
    //this->ivalid = FOREVER;

    //_seqReader.setFormat("emp");
    //_seqReader.setSigFilter("Body");    // TODO: Mesh?
    //_seqReader.setPadding(4);           // TODO: Optional!
}


//! DOCS
void 
EmpMeshObject::BuildMesh(TimeValue t)
{
    const int frame = t/GetTicksPerFrame() + _frameOffset;

    if (frame != _meshFrame) {
        // Current time, t, is different from the time that the current mesh
        // was built at. First, we clear the current mesh, since it is no
        // longer valid. Thereafter we try to build a new mesh for the
        // current time.

        NB_INFO("EmpMeshObject::BuildMesh - frame: " << frame << 
                " (offset: " << _frameOffset << ")");
        _clearMesh();
        _rebuildMesh(frame);
        _meshFrame = frame;
    }
}


//! Returns true if this object has a valid mesh.
BOOL
EmpMeshObject::OKtoDisplay()
{ 
    return _validMesh; 
}

//------------------------------------------------------------------------------

//! DOCS
void
EmpMeshObject::_rebuildMesh(const int frame)
{
    try {
        _seqReader.setFrame(frame);   // May throw.
        std::auto_ptr<Nb::Body> body(
            _seqReader.bodyReader()->ejectBody(_bodyName));
        if (0 != body.get()) {
            // Body was found in sequence on disk.

            NB_INFO("Importing body: '" << body->name() << 
                    "' (signature = '" << body->sig() << "')");

            if (body->matches(_signature)) { // "Mesh"
                // Point and triangle shapes are guaranteed to exist 
                // according to the Mesh-signature.

                _buildMesh(
                    body->constPointShape(), 
                    body->constTriangleShape());
                _validMesh = TRUE;
            }
            else {
                NB_WARNING("Signature '" << body->sig() << 
                            "' not supported");
            }
        }
        else {
            NB_WARNING("Body '" << _bodyName << 
                        "' not found in file '" << _seqReader.fileName());
        }
    }
    catch (std::exception &ex) {
        NB_ERROR("exception: " << ex.what());
    }
    catch (...) {
        NB_ERROR("unknown exception");
    }
}


//! Clear contents of the existing mesh. 
//! TODO: Not sure what the best way to do this is... ?
void
EmpMeshObject::_clearMesh()
{
    this->mesh.FreeAll();
    _validMesh = FALSE;
    //this->mesh.setNumVerts(0);
    //this->mesh.setNumFaces(0);
    //this->mesh.InvalidateGeomCache();
}


//! DOCS
void
EmpMeshObject::_buildMesh(const Nb::PointShape    &point, 
                          const Nb::TriangleShape &triangle)
{
    const Nb::Buffer3f &position = point.constBuffer3f("position");
    const Nb::Buffer3i &index    = triangle.constBuffer3i("index");

    const int numVerts = position.size();
    const int numFaces = index.size();
        
    // Mesh vertices.

    this->mesh.setNumVerts(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        const em::vec3f pos = position[i];
        this->mesh.setVert(i, Point3(pos[0], pos[1], pos[2]));
    }

    // Mesh faces.

    this->mesh.setNumFaces(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        const int i0 = index[i][0];
        const int i1 = index[i][1];
        const int i2 = index[i][2];
        Face &face = this->mesh.faces[i];
        face.setVerts(i0, i1, i2);
        face.setEdgeVisFlags(1, 1, 1);
    }

    // Now create 

    const int pointChannelCount = point.channelCount();
    for (int i = 0; i < pointChannelCount; ++i) {
        NB_INFO("point.channel: '" << point.channel(i)->name() << 
                "': type: '" << point.channel(i)->typeName() << "'");
    }

    const bool pointUV = point.hasChannels3f("uv");
    const bool pointU = point.hasChannels1f("u");
    const bool pointV = point.hasChannels1f("v");

    if (pointUV) {
        // Body has UV coordinates.

        const Nb::Buffer3f &uvw = point.constBuffer3f("uv");

        // Set UVs
        // TODO: multiple map channels?
        // Channel 0 is reserved for vertex color. 
        // Channel 1 is the default texture mapping.

        this->mesh.setNumMaps(2);
        this->mesh.setMapSupport(1, TRUE);  // enable map channel
        MeshMap &uvwMap = this->mesh.Map(1);
        uvwMap.setNumVerts(numVerts);
        for (int i = 0; i < numVerts; ++i) {
            UVVert &uvwVert = uvwMap.tv[i];
            uvwVert.x = uvw[i][0];
            uvwVert.y = uvw[i][1];
            uvwVert.z = uvw[i][2];
        }

        uvwMap.setNumFaces(numFaces);
        for (int i = 0; i < numFaces; ++i) {
            const int i0 = index[i][0];
            const int i1 = index[i][1];
            const int i2 = index[i][2];
            TVFace &uvwFace = uvwMap.tf[i];
            uvwFace.t[0] = i0;
            uvwFace.t[1] = i1;
            uvwFace.t[2] = i2;
        }
    }
    else if (pointU && pointV) {
        // Body has UV coordinates.

        const Nb::Buffer3f &uvw = point.constBuffer3f("uv");

        // Set UVs
        // TODO: multiple map channels?
        // Channel 0 is reserved for vertex color. 
        // Channel 1 is the default texture mapping.

        this->mesh.setNumMaps(2);
        this->mesh.setMapSupport(1, TRUE);  // enable map channel
        MeshMap &uvwMap = this->mesh.Map(1);
        uvwMap.setNumVerts(numVerts);
        for (int i = 0; i < numVerts; ++i) {
            UVVert &uvwVert = uvwMap.tv[i];
            uvwVert.x = uvw[i][0];
            uvwVert.y = uvw[i][1];
            uvwVert.z = uvw[i][2];
        }

        uvwMap.setNumFaces(numFaces);
        for (int i = 0; i < numFaces; ++i) {
            const int i0 = index[i][0];
            const int i1 = index[i][1];
            const int i2 = index[i][2];
            TVFace &uvwFace = uvwMap.tf[i];
            uvwFace.t[0] = i0;
            uvwFace.t[1] = i1;
            uvwFace.t[2] = i2;
        }
    }

    this->mesh.buildNormals();
    this->mesh.buildBoundingBox();
    this->mesh.InvalidateEdgeList();
    this->mesh.InvalidateGeomCache();
}


//! DOCS
void
EmpMeshObject::_addMap3f(MeshMap            &map, 
                         const Nb::Buffer3f &buf, 
                         const Nb::Buffer3i &index)
{
    // Set map vertices.

    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        UVVert &uvVert = map.tv[i];
        uvVert.x = buf[i][0];
        uvVert.y = buf[i][1];
        uvVert.z = buf[i][2];
    }

    // Set map faces.

    const int numFaces = static_cast<int>(index.size());
    map.setNumFaces(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        TVFace &tvFace = map.tf[i];
        tvFace.t[0] = index[i][0];
        tvFace.t[1] = index[i][1];
        tvFace.t[2] = index[i][1];
    }
}


























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

    virtual void
    Command(HWND hWnd, WPARAM wParam, LPARAM lParam);

    virtual void
    Import();

    virtual void
    Export();


public:     // Member variables (!?)

    // These are the handles to the custom controls in the rollup page.
    // Public and ugly just like a "proper" plug-in...


private:

    struct _General
    {
        ICustEdit   *projectPathEdit;
        Nb::String   projectPath;

        ICustButton *projectPathBrowseButton;
    };

    struct _Import
    {
        ICustEdit       *sequenceNameEdit;
        Nb::String       sequenceName;
        ICustButton     *browseButton;
        bool             sequence;

        ICustEdit       *signatureFilterEdit;
        Nb::String       signatureFilter;

        ICustEdit       *bodyNameFilterEdit;
        Nb::String       bodyNameFilter;

        ISpinnerControl *frameOffsetSpinner;
        int              frameOffset;

        ICustButton     *importButton;
    };

    struct _Export
    {
        ICustEdit   *sequenceNameEdit;
        Nb::String   sequenceName;
        ICustButton *browseButton;

        ISpinnerControl *paddingSpinner;
        int              padding;

        ISpinnerControl *firstFrameSpinner;
        ISpinnerControl *lastFrameSpinner;

        ICustButton *exportButton;
    };

private:

    static INT_PTR CALLBACK 
    DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:    // Member variables.

    HWND			hPanel;
    IUtil		   *iu;
    Interface	   *ip;

    _General _gen;
    _Import  _imp;
    _Export  _exp;
};

// -----------------------------------------------------------------------------

//! DOCS
class NaiadBuddyClassDesc : public ClassDesc2 
{
public:

    virtual int 
    IsPublic() 							
    { return TRUE; }

    //! Don't create anything, return pointer to singleton instance.
    virtual void* 
    Create(BOOL /*loading = FALSE*/) 	
    { return NaiadBuddy::GetInstance(); }

    virtual const TCHAR*	
    ClassName() 			
    { return GetString(IDS_NAIAD_BUDDY_CLASS_NAME); }

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
    { return GetString(IDS_NAIAD_BUDDY_INTERNAL_NAME); }	

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
    // TODO: Call base CTOR?
{
    iu = NULL;
    ip = NULL;	
    hPanel = NULL;

    _gen.projectPath     = Nb::String("");

    _imp.sequenceName    = Nb::String("");
    _imp.sequence        = true;
    _imp.signatureFilter = Nb::String("Body");
    _imp.bodyNameFilter  = Nb::String("*");
    _imp.frameOffset     = 0;

    _exp.sequenceName    = Nb::String("");
    _exp.padding         = 4;
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
void 
NaiadBuddy::Init(HWND hWnd)
{
    // General.

    _gen.projectPathEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_GEN_PROJECT_PATH_EDIT));
    _gen.projectPathEdit->SetText(_gen.projectPath.c_str());

    _gen.projectPathBrowseButton =
        GetICustButton(GetDlgItem(hWnd, IDC_GEN_PROJECT_PATH_BROWSE_BUTTON));


    // Import.

    _imp.sequenceNameEdit = GetICustEdit(GetDlgItem(hWnd, IDC_IMPORT_EDIT));
    _imp.sequenceNameEdit->SetText(_imp.sequenceName.c_str());

    _imp.browseButton = 
        GetICustButton(GetDlgItem(hWnd, IDC_IMPORT_BROWSE_BUTTON));

    CheckDlgButton(
        hWnd, 
        IDC_SEQUENCE_CHECK, 
        _imp.sequence ? BST_CHECKED : BST_UNCHECKED);

    _imp.signatureFilterEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_SIGNATURE_FILTER_EDIT));
    _imp.signatureFilterEdit->SetText(_imp.signatureFilter.c_str());

    _imp.bodyNameFilterEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_BODY_NAME_FILTER_EDIT));
    _imp.bodyNameFilterEdit->SetText(_imp.bodyNameFilter.c_str());

    _imp.frameOffsetSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_FRAME_OFFSET_SPINNER,   // Spinner Id.
            IDC_FRAME_OFFSET_EDIT,      // Edit Id.
            TIME_NegInfinity,           // Min.
            TIME_PosInfinity,           // Max.
            _imp.frameOffset);          // Value.

    _imp.importButton = GetICustButton(GetDlgItem(hWnd, IDC_IMPORT_BUTTON));


    // Export.

    _exp.sequenceNameEdit = GetICustEdit(GetDlgItem(hWnd, IDC_EXPORT_EDIT));

    _exp.browseButton = 
        GetICustButton(GetDlgItem(hWnd, IDC_EXPORT_BROWSE_BUTTON));

    _exp.paddingSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_EXP_PADDING_SPINNER,    // Spinner Id.
            IDC_EXP_PADDING_EDIT,       // Edit Id.
            1,                          // Min.
            8,                          // Max.
            _exp.padding);              // Value.
    
    _exp.firstFrameSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_FIRST_FRAME_SPINNER,    // Spinner Id.
            IDC_FIRST_FRAME_EDIT,       // Edit Id.
            TIME_NegInfinity,           // Min.
            TIME_PosInfinity,           // Max.
            ip->GetAnimRange().Start()/GetTicksPerFrame()); // Value.
    _exp.lastFrameSpinner = 
        SetupIntSpinner(
            hWnd,                       // Window handle.
            IDC_LAST_FRAME_SPINNER,     // Spinner Id.
            IDC_LAST_FRAME_EDIT,        // Edit Id.
            TIME_NegInfinity,           // Min.
            TIME_PosInfinity,           // Max.
            ip->GetAnimRange().End()/GetTicksPerFrame());  // Value.

    _exp.exportButton = GetICustButton(GetDlgItem(hWnd, IDC_EXPORT_BUTTON));
}


//! DOCS
void NaiadBuddy::Destroy(HWND hWnd)
{
    // General.

    ReleaseICustEdit(_gen.projectPathEdit);
    ReleaseICustButton(_gen.projectPathBrowseButton);


    // Import.

    ReleaseICustEdit(_imp.sequenceNameEdit);
    ReleaseICustButton(_imp.browseButton);
    ReleaseICustEdit(_imp.signatureFilterEdit);
    ReleaseICustEdit(_imp.bodyNameFilterEdit);
    ReleaseISpinner(_imp.frameOffsetSpinner);
    ReleaseICustButton(_imp.importButton);


    // Export.

    ReleaseICustEdit(_exp.sequenceNameEdit);
    ReleaseICustButton(_exp.browseButton);
    ReleaseISpinner(_exp.paddingSpinner);
    ReleaseISpinner(_exp.firstFrameSpinner);
    ReleaseISpinner(_exp.lastFrameSpinner);
    ReleaseICustButton(_exp.exportButton);
}


//! DOCS
void
NaiadBuddy::Command(HWND hWnd, WPARAM wParam, LPARAM lParam)
{

    // Useful standard dialogs in Max.
    // Interface8::DoMaxSaveAsDialog()
    // Interface8::DoMaxOpenDialog()
    // Interface9::DoMaxBrowseForFolder()

    switch (LOWORD(wParam)) {
    // General stuff.
    case IDC_GEN_PROJECT_PATH_BROWSE_BUTTON:
        {
            MSTR dir;
            Interface9 *i9 = GetCOREInterface9();
            if (0 != i9 && 
                i9->DoMaxBrowseForFolder(
                    hWnd, 
                    "Choose Project Path...",
                    dir)) {
                _gen.projectPath = Nb::String(dir.data());
                _gen.projectPathEdit->SetText(_gen.projectPath.c_str());
            }
        }
        break;
    case IDC_GEN_PROJECT_PATH_EDIT:
        {
            MSTR projectPath;
            _gen.projectPathEdit->GetText(projectPath);
            _gen.projectPath = Nb::String(projectPath.data());
        }
        break;
    // Import stuff.
    case IDC_IMPORT_BROWSE_BUTTON:
        {
        TSTR filename;
        TSTR initialDir(_gen.projectPath.c_str());
        FilterList extensionList;
        extensionList.Append("EMP (*.emp)");
        extensionList.Append("*.emp");
        Interface8 *i8 = GetCOREInterface8();
        if (0 != i8 && 
            i8->DoMaxOpenDialog(
                hWnd, 
                "Import EMP Sequence...", 
                filename, 
                initialDir, 
                extensionList)) {
            // Check if we should convert the chosen filename into a sequence.

            if (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_SEQUENCE_CHECK)) {
                _imp.sequenceName = Nb::hashifyFilename(filename.data());
            }
            else {
                _imp.sequenceName = Nb::String(filename.data());
            }
            _imp.sequenceNameEdit->SetText(_imp.sequenceName.c_str());
        }
        }
        break;
    case IDC_SIGNATURE_FILTER_EDIT:
        {
        MSTR signatureFilter;
        _imp.signatureFilterEdit->GetText(signatureFilter);
        _imp.signatureFilter = Nb::String(signatureFilter.data());
        }
        break;
    case IDC_BODY_NAME_FILTER_EDIT:
        {
        MSTR bodyNameFilter;
        _imp.bodyNameFilterEdit->GetText(bodyNameFilter);
        _imp.bodyNameFilter = Nb::String(bodyNameFilter.data());
        }
        break;
    case IDC_FRAME_OFFSET_SPINNER:
    case IDC_FRAME_OFFSET_EDIT:
        {
        _imp.frameOffset = _imp.frameOffsetSpinner->GetIVal();
        }
        break;
    case IDC_IMPORT_BUTTON:
        Import(); // The import button was pressed, do the import...
        break;
    // Export stuff.
    case IDC_EXPORT_BROWSE_BUTTON:
        {
        TSTR filename;
        TSTR initialDir(_gen.projectPath.c_str());
        FilterList extensionList;
        extensionList.Append("EMP (*.emp)");
        extensionList.Append("*.emp");
        extensionList.Append("All (*.*)");
        extensionList.Append("*.*");
        Interface8 *i8 = GetCOREInterface8();
        if (0 != i8 && 
            i8->DoMaxSaveAsDialog(
                hWnd, 
                "Export EMP Sequence...",
                filename,
                initialDir,
                extensionList)) {
            _exp.sequenceName = Nb::String(filename.data());
            _exp.sequenceNameEdit->SetText(_exp.sequenceName.c_str());
        }
        }
        break;
    case IDC_EXPORT_BUTTON:
        Export(); // The export button was pressed, do the export...        
        break;
    default:
        break;
    }
}


//! DOCS
void
NaiadBuddy::Import()
{
    // (1) - Open file.
    // (2) - Discover list of bodies in file (with time-stamps?)
    // (3) - Iterate over bodies, using body signatures to determine
    //       which type of 3ds Max object to create, e.g. Mesh or Points.
    //   (3a) - Add created objects to scene.
    // (4) - Close file.

    try {
        // Information about the provided file...

        MSTR sequenceName;
        _imp.sequenceNameEdit->GetText(sequenceName);
        if (sequenceName.isNull()) {
            return; // Early exit, no sequenceName provided.
        }

        Nb::SequenceReader seqReader;
        seqReader.setFormat(Nb::extractExt(sequenceName.data()));
        seqReader.setSequenceName(sequenceName.data());
        seqReader.setSigFilter("Body"); // Read as bodies...
        seqReader.setFrame(
            ip->GetTime()/GetTicksPerFrame() + 
            _imp.frameOffsetSpinner->GetIVal());

        MSTR bodyNameFilter;
        _imp.bodyNameFilterEdit->GetText(bodyNameFilter);
        if (bodyNameFilter.isNull()) {
            bodyNameFilter = "*";
        }
        
        MSTR signatureFilter;
        _imp.signatureFilterEdit->GetText(signatureFilter);
        if (signatureFilter.isNull()) {
            signatureFilter = "Body";
        }

        while (seqReader.bodyReader()->bodyCount() > 0) {
            // Get an Nb::Body from the EMP archive. We get ownership of
            // the retrieved Nb::Body. 

            std::auto_ptr<Nb::Body> body(seqReader.bodyReader()->popBody());

            if (body->name().listed_in(Nb::String(bodyNameFilter)) && 
                body->matches(Nb::String(signatureFilter))) {
                if ("Mesh" == body->sig()) {
                    // Create an EmpObject for each body. We own this object 
                    // until it is attached to a node and added to the scene. 
                    // If it can't be added we must delete it below.

                    EmpMeshObject *empMeshObj = 
                        static_cast<EmpMeshObject*>(
                            ip->CreateInstance(
                                GEOMOBJECT_CLASS_ID, EmpMeshObject_CLASS_ID));

                    // Set information required by the object's 
                    // internal sequence reader.

                    empMeshObj->setBodyName(body->name());
                    empMeshObj->setFrameOffset(
                        _imp.frameOffsetSpinner->GetIVal());

                    Nb::SequenceReader &sr = empMeshObj->sequenceReader();
                    sr.setFormat(seqReader.format());
                    sr.setSigFilter("Body"); // TODO: seqReader.sigFilter() ??
                    sr.setSequenceName(seqReader.sequenceName());
                    
                    INode *node = ip->CreateObjectNode(empMeshObj);
                    if (NULL != node) {
                        Matrix3 tm; // TMP! Hard-coded transform and time-value. 
                        tm.IdentityMatrix();// Body data already in world-space.
                        const TimeValue t = 0; // No time-varying XForm.
                        node->SetNodeTM(t, tm);
                        MSTR nodeName = _M(empMeshObj->bodyName().c_str());
                        ip->MakeNameUnique(nodeName);
                        node->SetName(nodeName);
                    }
                    else {
                        // For some reason the node could not be added to the 
                        // scene. Simply delete the EmpObject and move on, 
                        // hoping for better luck with the rest of the 
                        // Nb::Body objects.

                        delete empMeshObj;
                    }
                }
                //else if ("Particle" == body->sig()) {
                //}
                //else if ("Camera" == body->sig()) {
                //}
            }
        }

        ip->RedrawViews(ip->GetTime());
        //seqReader.close();    // TODO??

        /*
        Nb::EmpReader empReader(
            Nb::String(filename.data()), 
            Nb::String(bodyNameFilter.data()), 
            "Body"); // May throw.
        NB_INFO("EMP time: " << empReader.time());
        NB_INFO("EMP revision: " << empReader.revision());
        NB_INFO("EMP body count: " << empReader.bodyCount());
        int frame = 0;
        int timestep = 0;
        Nb::extractFrameAndTimestep(filename.data(), frame, timestep);
        NB_INFO("Frame: " << frame);
        NB_INFO("Timestep: " << timestep);
        const Nb::String sequenceName = Nb::hashifyFilename(filename.data());
        NB_INFO("Sequence name: '" << sequenceName << "'\n");

        // Scrub to the frame in the EMP file name.

        ip->SetTime(frame*GetTicksPerFrame(), FALSE);

        while (empReader.bodyCount() > 0) {
            // Get an Nb::Body from the EMP archive. We get ownership of
            // the retrieved Nb::Body. 

            std::auto_ptr<Nb::Body> body(empReader.popBody());

            // Create an EmpObject for each body. We own this object 
            // until it is attached to a node and added to the scene. If it 
            // can't be added we must delete it below.

            EmpMeshObject *empMeshObject = 
                static_cast<EmpMeshObject*>(
                    ip->CreateInstance(
                        GEOMOBJECT_CLASS_ID, EmpMeshObject_CLASS_ID));

            // Set information required by the object's 
            // internal sequence reader.

            empMeshObject->setBodyName(body->name());
            empMeshObject->setSequenceName(sequenceName);
            //empObject->BuildMesh(i->GetTime());

            INode *node = ip->CreateObjectNode(empMeshObject);
            if (NULL != node) {
                Matrix3 tm; // TMP! Hard-coded transform and time-value. 
                tm.IdentityMatrix();   // Body data already in world-space.
                const TimeValue t = 0; // No time-varying XForm.
                node->SetNodeTM(t, tm);
                //node->Reference(empMeshObject);
                MSTR nodeName = _M(empMeshObject->bodyName().c_str());
                ip->MakeNameUnique(nodeName);
                node->SetName(nodeName);
                //ii->AddNodeToScene(impNode);
            }
            else {
                // For some reason the node could not be added to the scene.
                // Simply delete the EmpObject and move on, hoping
                // for better luck with the rest of the Nb::Body objects.

                delete empMeshObject;
            }
        }

        ip->RedrawViews(ip->GetTime());
        //ii->RedrawViews();
        empReader.close();
        //return TRUE;   // Success.
        */
    }
    catch (std::exception &ex) {
        NB_ERROR("exception: " << ex.what());
    }
    catch (...) {
        NB_ERROR("unknown exception");
    }
}


//! DOCS
void
NaiadBuddy::Export()
{
    // TODO!        
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
    case CC_SPINNER_CHANGE:
    case WM_CUSTEDIT_ENTER: 
        // This message is sent when the user presses ENTER on
        // a custom edit control...
    case WM_COMMAND:
        //#pragma message(TODO("React to the user interface commands.  
        // A utility plug-in is controlled by the user from here."))
        NaiadBuddy::GetInstance()->Command(hWnd, wParam, lParam);
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
