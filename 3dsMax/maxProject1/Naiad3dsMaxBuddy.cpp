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
#include <NbBodyWriter.h>
#include <NbEmpWriter.h>
#include <NbEmpReader.h>
#include <NbSequenceReader.h>
#include <NbBody.h>
#include <NbString.h>
#include <em_log.h>
#include <sstream>
#include <map>
#include <Windows.h>

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

    void
    setFlipYZ(const bool flipYZ)
    { _flipYZ = flipYZ; }

private:

    static const Nb::String _signature;

    void
    _rebuildMesh(const int frame);

    void
    _clearMesh();

    void
    _buildMesh(const Nb::PointShape &point, const Nb::TriangleShape &triangle);

    static void
    _buildMapVerts1i(MeshMap            &map, 
                     const Nb::Buffer1i &buf,
                     const int           mask[3]);

    static void
    _buildMapVerts1f(MeshMap            &map, 
                     const Nb::Buffer1f &buf,
                     const int           mask[3]);

    static void
    _buildMapVerts3f(MeshMap            &map, 
                     const Nb::Buffer3f &buf,
                     const int           mask[3]);

    static void
    _buildMapFaces(MeshMap            &map, 
                   const Nb::Buffer3i &index);

    static bool
    _hasUVW(const Nb::PointShape &point);

private:    // Member variables.

    Nb::SequenceReader _seqReader; 
    Nb::String         _bodyName;  //!< The name of the body this object tracks.
    bool               _flipYZ;
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

public:

    //! DOCS
    struct Mapping
    {
        int id;
        int mask[3];    // TODO: bitmask.
    };

    //! CTOR.
    EmpMeshObjectClassDesc()
    {
        // Channel 0 is reserved for vertex color. 
        // Channel 1 is the default texture mapping.

        typedef _ChannelMap::value_type value_type;
        Mapping uvMapping;
        uvMapping.id = 1;   // Default 3ds Max texture mapping channel.
        uvMapping.mask[0] = 1;
        uvMapping.mask[1] = 1;
        uvMapping.mask[2] = 0;
        _channelMapping.insert(value_type("uv", uvMapping));

        Mapping uMapping;
        uMapping.id = 1;    // Default 3ds Max texture mapping channel.
        uMapping.mask[0] = 1;
        uMapping.mask[1] = 0;
        uMapping.mask[2] = 0;
        _channelMapping.insert(value_type("u", uMapping));

        Mapping vMapping;
        vMapping.id = 1;    // Default 3ds Max texture mapping channel.
        vMapping.mask[0] = 0;
        vMapping.mask[1] = 1;
        vMapping.mask[2] = 0;
        _channelMapping.insert(value_type("v", vMapping));

        Mapping velocityMapping;
        velocityMapping.id = 2;
        velocityMapping.mask[0] = 1;
        velocityMapping.mask[1] = 1;
        velocityMapping.mask[2] = 1;
        _channelMapping.insert(value_type("velocity", velocityMapping));
    }

    //! Returns true if channelName is not "special". [static]
    //! TODO: Support UVW!?
    bool
    isBlindChannel(const Nb::String &channelName) const
    {
        return (!(channelName == "position") && 
                 _channelMapping.find(channelName) == _channelMapping.end());
    }

    //! Returns mapping id for the provided channel name. Id is -1 for blind
    //! mapping channels.
    Mapping
    mapping(const Nb::String &channelName) const
    {
        Mapping m;
        m.id      = -1;
        m.mask[0] =  1;     // All sub-channels enabled by default  
        m.mask[1] =  1;     // even for blind channels.
        m.mask[2] =  1;
        const _ChannelMap::const_iterator iter =
            _channelMapping.find(channelName);
        if (iter != _channelMapping.end()) {
            m.id      = iter->second.id;
            m.mask[0] = iter->second.mask[0];
            m.mask[1] = iter->second.mask[1];
            m.mask[2] = iter->second.mask[2];
        }
        return m;
    }

private:    // Member variables.

    typedef std::map<Nb::String,Mapping> _ChannelMap;
    _ChannelMap _channelMapping;
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
    , _flipYZ(true)
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
    const int frame = t/GetTicksPerFrame();
    if (frame != _meshFrame) {
        // Current time, t, is different from the time that the current mesh
        // was built at. First, we clear the current mesh, since it is no
        // longer valid. Thereafter we try to build a new mesh for the
        // current time.

        NB_INFO("EmpMeshObject::BuildMesh - frame: " << frame /*<< 
                "TODO (offset: " << _seqReader.frameOffset() << ")"*/);
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

            NB_INFO("mesh: Importing body: '" << body->name() << 
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
                NB_WARNING("EmpMeshObject: Signature '" << body->sig() << 
                            "' not supported");
            }
        }
        else {
            NB_WARNING("EmpMeshObject: Body '" << _bodyName << 
                        "' not found in file '" << _seqReader.fileName());
        }
    }
    catch (std::exception &ex) {
        NB_ERROR("EmpMeshObject: exception: " << ex.what());
    }
    catch (...) {
        NB_ERROR("EmpMeshObject: unknown exception");
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
        
    // Mesh vertices (positions) [required].

    this->mesh.setNumVerts(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        const em::vec3f pos = position[i];
        _flipYZ ?
            this->mesh.setVert(i, Point3(pos[0], pos[2], pos[1])) :
            this->mesh.setVert(i, Point3(pos[0], pos[1], pos[2]));
    }

    // Mesh faces [required].

    this->mesh.setNumFaces(numFaces);
    for (int f = 0; f < numFaces; ++f) {
        Face &face = this->mesh.faces[f];
        face.setEdgeVisFlags(1, 1, 1);
        for (int i = 0; i < 3; ++i) {
            face.v[i] = index[f][i];
        }
    }

    // Count number of blind channels.

    EmpMeshObjectClassDesc *desc = 
        static_cast<EmpMeshObjectClassDesc*>(GetEmpMeshObjectDesc());

    int blindChannelCount = 0;
    const int pointChannelCount = point.channelCount();
    for (int pc = 0; pc < pointChannelCount; ++pc) {
        const Nb::Channel *chan = point.channel(pc)();
        if (desc->isBlindChannel(chan->name())) {
            ++blindChannelCount;
        }
    }

    if ((blindChannelCount + 2) > (MAX_MESHMAPS - 1)) {
        // TODO: Warning!
    }
    
    this->mesh.setNumMaps(3 + blindChannelCount);

    // TODO: check if bc > 99
    //MAX_MESHMAPS

    int bc = 3;
    //const int pointChannelCount = point.channelCount();
    for (int pc = 0; pc < pointChannelCount; ++pc) {
        const Nb::Channel *chan = point.channel(pc)();
        if (!(chan->name() == "position")) {
            // Ignore position channel, it is dealt with above...

            const EmpMeshObjectClassDesc::Mapping mapping = 
                desc->mapping(chan->name());
            
            NB_INFO("mesh: point.channel: Name '" << chan->name() << 
                    "', Type: '" << chan->typeName() << 
                    "', Map Channel: " << mapping.id << 
                    " , Mask: [" << 
                    mapping.mask[0]<<mapping.mask[1]<<mapping.mask[2] << "]");

            switch (chan->type()) {
            case Nb::ValueBase::IntType:
                {
                const Nb::Buffer1i& buf1i = point.constBuffer1i(pc);
                if (mapping.id < 0) {
                    // Blind channel.

                    this->mesh.setMapSupport(bc, TRUE);
                    MeshMap &meshMap = this->mesh.Map(bc);
                    _buildMapVerts1i(meshMap, buf1i, mapping.mask);
                    _buildMapFaces(meshMap, index);
                    ++bc;
                }
                else {
                    this->mesh.setMapSupport(mapping.id, TRUE);
                    MeshMap &meshMap = this->mesh.Map(mapping.id);
                    _buildMapVerts1i(meshMap, buf1i, mapping.mask);
                    _buildMapFaces(meshMap, index);
                }
                }
                break;
            case Nb::ValueBase::FloatType:
                {
                const Nb::Buffer1f& buf1f = point.constBuffer1f(pc);
                if (mapping.id < 0) {
                    // Blind channel.

                    this->mesh.setMapSupport(bc, TRUE);
                    MeshMap &meshMap = this->mesh.Map(bc);
                    _buildMapVerts1f(meshMap, buf1f, mapping.mask);
                    _buildMapFaces(meshMap, index);
                    ++bc;
                }
                else {
                    this->mesh.setMapSupport(mapping.id, TRUE);
                    MeshMap &meshMap = this->mesh.Map(mapping.id);
                    _buildMapVerts1f(meshMap, buf1f, mapping.mask);
                    _buildMapFaces(meshMap, index);
                }
                }
                break;
            case Nb::ValueBase::Vec3fType:
                {
                const Nb::Buffer3f& buf3f = point.constBuffer3f(pc);
                if (mapping.id < 0) {
                    // Blind channel.

                    this->mesh.setMapSupport(bc, TRUE);
                    MeshMap &meshMap = this->mesh.Map(bc);
                    _buildMapVerts3f(meshMap, buf3f, mapping.mask);
                    _buildMapFaces(meshMap, index);
                    ++bc;
                }
                else {
                    this->mesh.setMapSupport(mapping.id, TRUE);
                    MeshMap &meshMap = this->mesh.Map(mapping.id);
                    _buildMapVerts3f(meshMap, buf3f, mapping.mask);
                    _buildMapFaces(meshMap, index);
                }
                }
                break;
            default:
                NB_WARNING(
                    "mesh: Point channel type: '" << chan->typeName() << 
                    "' not supported yet!");
                break;
            }
        }
    }

    this->mesh.buildNormals();
    this->mesh.buildBoundingBox();
    this->mesh.InvalidateEdgeList();
    this->mesh.InvalidateGeomCache();
}


//! DOCS
void
EmpMeshObject::_buildMapVerts1i(MeshMap            &map, 
                                const Nb::Buffer1i &buf,
                                const int           mask[3])
{
    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int v = 0; v < numVerts; ++v) { // Set map vertices.
        UVVert &uvVert = map.tv[v];
        for (int i = 0; i < 3; ++i) { // Check mask before setting.
            if (mask[i] > 0) {
                uvVert[i] = static_cast<float>(buf[v]);
            }
        }
    }
}


//! DOCS [static]
void
EmpMeshObject::_buildMapVerts1f(MeshMap            &map, 
                                const Nb::Buffer1f &buf,
                                const int           mask[3])
{
    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int v = 0; v < numVerts; ++v) { // Set map vertices.
        UVVert &uvVert = map.tv[v];
        for (int i = 0; i < 3; ++i) { // Check mask before setting.
            if (mask[i] > 0) {
                uvVert[i] = buf[v];
            }
        }
    }
}


//! DOCS [static]
void
EmpMeshObject::_buildMapVerts3f(MeshMap            &map, 
                                const Nb::Buffer3f &buf,
                                const int           mask[3])
{
    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int v = 0; v < numVerts; ++v) { // Set map vertices.
        UVVert &uvVert = map.tv[v];
        for (int i = 0; i < 3; ++i) { // Check mask before setting.
            if (mask[i] > 0) {
                uvVert[i] = buf[v][i];
            }
        }
    }
}


//! DOCS [static]
void
EmpMeshObject::_buildMapFaces(MeshMap            &map,
                              const Nb::Buffer3i &index)
{
    const int numFaces = static_cast<int>(index.size());
    map.setNumFaces(numFaces);
    for (int f = 0; f < numFaces; ++f) { // Set map faces.
        TVFace &tvFace = map.tf[f];
        for (int i = 0; i < 3; ++i) {
            tvFace.t[i] = index[f][i];
        }
    }
}


//! Returns true if the point shape has texture coordinate channel(s). [static]
bool
EmpMeshObject::_hasUVW(const Nb::PointShape &point)
{
    return (point.hasChannels("uv") || 
            point.hasChannels("u")  || 
            point.hasChannels("v"));
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

    virtual void
    ExportNode(INode *node, int frame, 
               bool flipYZ, bool selectedOnly, 
               Nb::EmpWriter &emp,
               int &numExportedNodes, int &numExportedMeshNodes,
               int &numExportedParticleNodes, int &numExportedCameraNodes);

public:     // Member variables (!?)

    // These are the handles to the custom controls in the rollup page.
    // Public and ugly just like a "proper" plug-in...


private:

    struct _General
    {
        ICustEdit   *projectPathEdit;
        Nb::String   projectPath;

        ICustButton *projectPathBrowseButton;

        ICustButton *showLogButton;
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
        bool             flipYZ;

        ICustButton     *importButton;
        //ICustStatusEdit *importStatusEdit;
        ICustStatus     *importStatus;
    };

    struct _Export
    {
        ICustEdit       *sequenceNameEdit;
        Nb::String       sequenceName;
        ICustButton     *browseButton;

        ISpinnerControl *paddingSpinner;
        int              padding;

        ISpinnerControl *firstFrameSpinner;
        ISpinnerControl *lastFrameSpinner;

        bool             flipYZ;
        bool             selection;

        ICustButton     *exportButton;
        //ICustStatusEdit *exportStatusEdit;
        ICustStatus     *exportStatus;
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
    _imp.signatureFilter = Nb::String("Mesh");
    _imp.bodyNameFilter  = Nb::String("*");
    _imp.frameOffset     = 0;
    _imp.flipYZ          = true;

    _exp.sequenceName    = Nb::String("");
    _exp.padding         = 4;
    _exp.flipYZ          = true;
    _exp.selection       = false;
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

    _gen.showLogButton =
        GetICustButton(GetDlgItem(hWnd, IDC_GEN_SHOW_LOG_BUTTON));


    // Import.

    _imp.sequenceNameEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_IMP_EMP_SEQUENCE_EDIT));
    _imp.sequenceNameEdit->SetText(_imp.sequenceName.c_str());

    _imp.browseButton = 
        GetICustButton(GetDlgItem(hWnd, IDC_IMP_BROWSE_BUTTON));

    CheckDlgButton(
        hWnd, 
        IDC_IMP_SEQUENCE_CHECK, 
        _imp.sequence ? BST_CHECKED : BST_UNCHECKED);

    _imp.signatureFilterEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_IMP_SIGNATURE_FILTER_EDIT));
    _imp.signatureFilterEdit->SetText(_imp.signatureFilter.c_str());

    _imp.bodyNameFilterEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_IMP_BODY_NAME_FILTER_EDIT));
    _imp.bodyNameFilterEdit->SetText(_imp.bodyNameFilter.c_str());

    _imp.frameOffsetSpinner = 
        SetupIntSpinner(
            hWnd,                           // Window handle.
            IDC_IMP_FRAME_OFFSET_SPINNER,   // Spinner Id.
            IDC_IMP_FRAME_OFFSET_EDIT,      // Edit Id.
            TIME_NegInfinity,               // Min.
            TIME_PosInfinity,               // Max.
            _imp.frameOffset);              // Value.

    CheckDlgButton(
        hWnd, 
        IDC_IMP_FLIP_YZ_CHECK, 
        _imp.flipYZ ? BST_CHECKED : BST_UNCHECKED);

    _imp.importButton = 
        GetICustButton(GetDlgItem(hWnd, IDC_IMP_IMPORT_BUTTON));

    //_imp.importStatusEdit = 
    //    GetICustStatusEdit(GetDlgItem(hWnd, IDC_IMP_STATUS_EDIT));
    //_imp.importStatusEdit->SetText("Ready");
    _imp.importStatus = 
        GetICustStatus(GetDlgItem(hWnd, IDC_IMP_STATUS_EDIT));
    _imp.importStatus->SetText("Ready");

    // Export.

    _exp.sequenceNameEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_EXP_EMP_SEQUENCE_EDIT));

    _exp.browseButton = 
        GetICustButton(GetDlgItem(hWnd, IDC_EXP_BROWSE_BUTTON));

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
            hWnd,                                           // Window handle.
            IDC_EXP_FIRST_FRAME_SPINNER,                    // Spinner Id.
            IDC_EXP_FIRST_FRAME_EDIT,                       // Edit Id.
            TIME_NegInfinity/GetTicksPerFrame(),            // Min.
            TIME_PosInfinity/GetTicksPerFrame(),            // Max.
            ip->GetAnimRange().Start()/GetTicksPerFrame()); // Value.
    _exp.lastFrameSpinner = 
        SetupIntSpinner(
            hWnd,                                           // Window handle.
            IDC_EXP_LAST_FRAME_SPINNER,                     // Spinner Id.
            IDC_EXP_LAST_FRAME_EDIT,                        // Edit Id.
            TIME_NegInfinity/GetTicksPerFrame(),            // Min.
            TIME_PosInfinity/GetTicksPerFrame(),            // Max.
            ip->GetAnimRange().End()/GetTicksPerFrame());   // Value.

    CheckDlgButton(
        hWnd, 
        IDC_EXP_FLIP_YZ_CHECK, 
        _exp.flipYZ ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(
        hWnd, 
        IDC_EXP_SELECTION_CHECK, 
        _exp.selection ? BST_CHECKED : BST_UNCHECKED);

    _exp.exportButton = GetICustButton(GetDlgItem(hWnd, IDC_EXP_EXPORT_BUTTON));

    _exp.exportStatus = 
        GetICustStatus(GetDlgItem(hWnd, IDC_EXP_STATUS_EDIT));
    _exp.exportStatus->SetText("Ready");
}


//! DOCS
void NaiadBuddy::Destroy(HWND hWnd)
{
    // General.

    ReleaseICustEdit(_gen.projectPathEdit);
    ReleaseICustButton(_gen.projectPathBrowseButton);
    ReleaseICustButton(_gen.showLogButton);


    // Import.

    ReleaseICustEdit(_imp.sequenceNameEdit);
    ReleaseICustButton(_imp.browseButton);
    ReleaseICustEdit(_imp.signatureFilterEdit);
    ReleaseICustEdit(_imp.bodyNameFilterEdit);
    ReleaseISpinner(_imp.frameOffsetSpinner);
    ReleaseICustButton(_imp.importButton);
    ReleaseICustStatus(_imp.importStatus);


    // Export.

    ReleaseICustEdit(_exp.sequenceNameEdit);
    ReleaseICustButton(_exp.browseButton);
    ReleaseISpinner(_exp.paddingSpinner);
    ReleaseISpinner(_exp.firstFrameSpinner);
    ReleaseISpinner(_exp.lastFrameSpinner);
    ReleaseICustButton(_exp.exportButton);
    ReleaseICustStatus(_exp.exportStatus);
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
                    _T("Choose Shot Path..."),
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
    case IDC_GEN_SHOW_LOG_BUTTON:
        {
            try {
                em::close_log(); // Close to flush any contents to disk.

                // Read log contents so that we can restore them later.

                const std::string logFileName = GetLogFileName();
                std::string logContents;
                FILE *fptr(std::fopen(logFileName.c_str(), "r"));
                if (0 != fptr) {
                    std::fseek(fptr, 0, SEEK_END);
                    const int length(std::ftell(fptr));
                    logContents.resize(length + 1);
                    std::fseek(fptr, 0, SEEK_SET);
                    std::fread(&logContents[0], length, 1, fptr);
                    std::fclose(fptr);
                    logContents[length] = '\n';
                }

                // Some API initialization stuff...

                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                ZeroMemory(&pi, sizeof(pi));

                // Create command line.

                const std::string cmdline(
                    "\"notepad.exe\" \"" + logFileName + "\"");
                LPTSTR szCmdline = _tcsdup(TEXT(cmdline.c_str()));

                // Start the child process. 

                if (CreateProcess(
                        NULL,      // No module name (use command line)
                        szCmdline, // Command line
                        NULL,      // Process handle not inheritable
                        NULL,      // Thread handle not inheritable
                        FALSE,     // Set handle inheritance to FALSE
                        0,         // No creation flags
                        NULL,      // Use parent's environment block
                        NULL,      // Use parent's starting directory 
                        &si,       // Pointer to STARTUPINFO structure
                        &pi)) {    // Pointer to PROCESS_INFORMATION structure
                    // Wait until child process exits.

                    WaitForSingleObject(pi.hProcess, INFINITE);

                    // Close process and thread handles. 

                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                else {
                    std::stringstream ss;
                    ss << "\nCreateProcess failed: " << GetLastError() << "\n";
                    logContents += ss.str();
                }

                em::open_log(logFileName);   // Open Log again, may throw.
                em::output_log(logContents); // Restore Log contents.
            }
            catch (...) {
                // Something bad happened, but there is nowhere to report it!
            }
        }
        break;
    // Import stuff.
    case IDC_IMP_BROWSE_BUTTON:
        {
        TSTR filename;
        TSTR initialDir(_gen.projectPath.c_str());
        FilterList extensionList;
        extensionList.Append(_T("EMP (*.emp)"));
        extensionList.Append(_T("*.emp"));
        Interface8 *i8 = GetCOREInterface8();
        if (0 != i8 && 
            i8->DoMaxOpenDialog(
                hWnd, 
                _T("Import EMP Sequence..."), 
                filename, 
                initialDir, 
                extensionList)) {
            // Check if we should convert the chosen filename into a sequence.

            const bool sequence = 
                (BST_CHECKED==IsDlgButtonChecked(hWnd, IDC_IMP_SEQUENCE_CHECK));
            if (sequence) {
                _imp.sequenceName = Nb::hashifyFilename(filename.data());
            }
            else {
                _imp.sequenceName = Nb::String(filename.data());
            }
            _imp.sequenceNameEdit->SetText(_imp.sequenceName.c_str());
        }
        }
        break;
    case IDC_IMP_SEQUENCE_CHECK:
        {
        _imp.sequence = 
            (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_IMP_SEQUENCE_CHECK));
        }
        break;
    case IDC_IMP_SIGNATURE_FILTER_EDIT:
        {
        MSTR signatureFilter;
        _imp.signatureFilterEdit->GetText(signatureFilter);
        _imp.signatureFilter = Nb::String(signatureFilter.data());
        }
        break;
    case IDC_IMP_BODY_NAME_FILTER_EDIT:
        {
        MSTR bodyNameFilter;
        _imp.bodyNameFilterEdit->GetText(bodyNameFilter);
        _imp.bodyNameFilter = Nb::String(bodyNameFilter.data());
        }
        break;
    case IDC_IMP_FRAME_OFFSET_SPINNER:
    case IDC_IMP_FRAME_OFFSET_EDIT:
        {
        _imp.frameOffset = _imp.frameOffsetSpinner->GetIVal();
        }
        break;
    case IDC_IMP_FLIP_YZ_CHECK:
        {
        _imp.flipYZ = 
            (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_IMP_FLIP_YZ_CHECK));
        }
        break;
    case IDC_IMP_IMPORT_BUTTON:
        Import(); // The import button was pressed, do the import...
        break;
    // Export stuff.
    case IDC_EXP_BROWSE_BUTTON:
        {
        TSTR filename;
        TSTR initialDir(_gen.projectPath.c_str());
        FilterList extensionList;
        extensionList.Append(_T("EMP (*.emp)"));
        extensionList.Append(_T("*.emp"));
        //extensionList.Append("All (*.*)");
        //extensionList.Append("*.*");
        Interface8 *i8 = GetCOREInterface8();
        if (0 != i8 && 
            i8->DoMaxSaveAsDialog(
                hWnd, 
                _T("Export EMP Sequence..."),
                filename,
                initialDir,
                extensionList)) {
            _exp.sequenceName = Nb::String(filename.data());
            _exp.sequenceNameEdit->SetText(_exp.sequenceName.c_str());
        }
        }
        break;
    case IDC_EXP_FLIP_YZ_CHECK:
        {
        _exp.flipYZ = 
            (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_EXP_FLIP_YZ_CHECK));
        }
        break;
    case IDC_EXP_SELECTION_CHECK:
        {
        _exp.selection = 
            (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_EXP_SELECTION_CHECK));
        }
        break;
    case IDC_EXP_EXPORT_BUTTON:
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
    try {
        // Information about the provided file...

        MSTR sequenceName;
        _imp.sequenceNameEdit->GetText(sequenceName);
        if (sequenceName.isNull()) {
            _imp.importStatus->SetText(_T("ERROR!"));
            _imp.importStatus->SetTooltip(true, _T("Empty sequence name."));
            NB_ERROR("import: Empty sequence name.");
            return; // Early exit, no sequenceName provided.
        }

        MSTR bodyNameFilter;
        _imp.bodyNameFilterEdit->GetText(bodyNameFilter);
        if (bodyNameFilter.isNull()) {
            bodyNameFilter = _T("*");
        }
        
        MSTR signatureFilter;
        _imp.signatureFilterEdit->GetText(signatureFilter);
        if (signatureFilter.isNull()) {
            signatureFilter = _T("Mesh");
        }

        const int frameOffset = _imp.frameOffsetSpinner->GetIVal();
        const bool flipYZ = 
            (BST_CHECKED == IsDlgButtonChecked(hPanel, IDC_IMP_FLIP_YZ_CHECK));

        Nb::SequenceReader seqReader;
        try {
            // Create our sequence reader.

            NB_INFO("Nb::SequenceReader::setSigFilter");
            seqReader.setSigFilter("Body");           // HACK! Read as bodies...
            NB_INFO("Nb::SequenceReader::setProjectPath");
            seqReader.setProjectPath("<non-empty>");  // HACK!
            NB_INFO("Nb::SequenceReader::setSequenceName");
            seqReader.setSequenceName(sequenceName.data());
            NB_INFO("Nb::SequenceReader::setFormat");
            seqReader.setFormat(Nb::extractExt(sequenceName.data()));
            NB_INFO("Nb::SequenceReader::setFrameOffset");
            seqReader.setFrameOffset(frameOffset);
            //seqReader.setFrame(
            //    ip->GetTime()/GetTicksPerFrame()); // May throw.
            //seqReader.refresh();
        }
        catch (std::exception &ex) {
            NB_WARNING("import: exception: " << ex.what());
        }

        std::vector<Nb::String> bodyNames;
        std::vector<Nb::String> bodySignatures;
        seqReader.allBodyInfo(bodyNames, bodySignatures);
        if (bodyNames.size() != bodySignatures.size()) {
            _imp.importStatus->SetText(_T("ERROR!"));
            _imp.importStatus->SetTooltip(true, _T("Invalid body info."));
            NB_ERROR("import: Invalid body info.");
            return; // Early exit, invalid body info.
        }

        int numCreatedNodes = 0;
        int numCreatedMeshNodes = 0;
        int numCreatedParticleNodes = 0;
        int numCreatedCameraNodes = 0;
        for (unsigned int b = 0; b < bodyNames.size(); ++b) {
            const Nb::String &bodyName = bodyNames[b];
            const Nb::String &bodySig = bodySignatures[b];
            const bool nameMatch = bodyName.listed_in(bodyNameFilter.data());
            const bool sigMatch = bodySig.listed_in(signatureFilter.data());

            if (nameMatch && sigMatch) {
                if ("Mesh" == bodySig) {
                    // Create an EmpMeshObject for each body. We own this object 
                    // until it is attached to a node and added to the scene. 
                    // If it can't be added we must delete it below.

                    EmpMeshObject *empMeshObj = 
                        static_cast<EmpMeshObject*>(
                            ip->CreateInstance(
                                GEOMOBJECT_CLASS_ID, EmpMeshObject_CLASS_ID));

                    // Set information required by the object's 
                    // internal sequence reader.

                    empMeshObj->setBodyName(bodyName);
                    empMeshObj->setFlipYZ(flipYZ);

                    Nb::SequenceReader &sr = empMeshObj->sequenceReader();
                    sr.setSigFilter("Body"); // HACK!
                    sr.setProjectPath(seqReader.projectPath());
                    sr.setSequenceName(seqReader.sequenceName());
                    sr.setFormat(seqReader.format());
                    sr.setFrameOffset(frameOffset/*TODO seqReader.frameOffset()*/);
                    sr.setPadding(seqReader.padding());

                    INode *node = ip->CreateObjectNode(empMeshObj);
                    if (NULL != node) {
                        Matrix3 tm; // TMP! Hard-coded transform and time-value. 
                        tm.IdentityMatrix();// Body data already in world-space.
                        const TimeValue t = 0; // No time-varying XForm.
                        node->SetNodeTM(t, tm);
                        MSTR nodeName = _M(empMeshObj->bodyName().c_str());
                        ip->MakeNameUnique(nodeName);
                        node->SetName(nodeName);
                        ++numCreatedMeshNodes;
                        ++numCreatedNodes;
                    }
                    else {
                        // For some reason the node could not be added to the 
                        // scene. Simply delete the EmpMeshObject and move on, 
                        // hoping for better luck with the rest of the 
                        // Nb::Body objects.

                        delete empMeshObj;
                    }
                }
                else if ("Particle" == bodySig) {
                    NB_WARNING("import: Body signature: '" 
                                << bodySig << "' not supported yet!");
                    //++numCreatedParticleNodes;
                    //++numCreatedNodes;
                }
                else if ("Camera" == bodySig) {
                    NB_WARNING("import: Body signature: '" 
                                << bodySig << "' not supported yet!");
                    //++numCreatedCameraNodes;
                    //++numCreatedNodes;
                }
                else {
                    NB_WARNING("import: Unsupported body signature: " 
                                << bodySig);
                }
            }
        }

        std::stringstream ss;
        ss << "Created " << numCreatedNodes << " nodes\n";
        _imp.importStatus->SetText(ss.str().c_str());
        ss << "Mesh: " << numCreatedMeshNodes << "\n"
           << "Particle: " << numCreatedParticleNodes << "\n"
           << "Camera: " << numCreatedCameraNodes << "\n";
        _imp.importStatus->SetTooltip(true, _T(ss.str().c_str()));
        NB_INFO(ss.str());

        ip->RedrawViews(ip->GetTime());
        //seqReader.close();    // TODO??
    }
    catch (std::exception &ex) {
        _imp.importStatus->SetText(_T("ERROR!"));
        _imp.importStatus->SetTooltip(true, ex.what());
        NB_ERROR("import: exception: " << ex.what());
    }
    catch (...) {
        _imp.importStatus->SetText(_T("ERROR!"));
        _imp.importStatus->SetTooltip(true, _T("unknown exception"));
        NB_ERROR("import: unknown exception");
    }
}


//! DOCS
void
NaiadBuddy::Export()
{
    MSTR sequenceName;
    _exp.sequenceNameEdit->GetText(sequenceName);
    if (sequenceName.isNull()) {
        _exp.exportStatus->SetText(_T("ERROR!"));
        _exp.exportStatus->SetTooltip(true, _T("Empty sequence name."));
        NB_ERROR("export: Empty sequence name.");
        return; // Early exit, no sequenceName provided.
    }
    // TODO: warning if sequence name does not contain exactly one '#'?
    //MessageBox(NULL,"I need to have one and only one object selected !",
    //           "Warning", MB_OK);


    const int firstFrame = _exp.firstFrameSpinner->GetIVal();
    const int lastFrame = _exp.lastFrameSpinner->GetIVal();


    if (firstFrame >= lastFrame) {
        _exp.exportStatus->SetText(_T("ERROR!"));
        _exp.exportStatus->SetTooltip(true, _T("Invalid frame range"));
        NB_ERROR("export: Invalid frame range");
        return; // Early exit, invalid frame range.
    }

    const int padding = 
        _exp.paddingSpinner->GetIVal();
    const bool flipYZ = 
        (BST_CHECKED == IsDlgButtonChecked(hPanel, IDC_EXP_FLIP_YZ_CHECK));
    const bool selectedOnly = 
        (BST_CHECKED == IsDlgButtonChecked(hPanel, IDC_EXP_SELECTION_CHECK));

    NB_INFO("export: ===== START EXPORT =====");
    NB_INFO("export: First Frame: " << firstFrame);
    NB_INFO("export: Last Frame: " << lastFrame);
    NB_INFO("export: Flip YZ: " << (flipYZ ? "true" : "false"));
    NB_INFO("export: Selected Only: " << (selectedOnly ? "true" : "false"));

    int numExportedFrames = 0;
    for (int frame = firstFrame; frame <= lastFrame; ++frame) {
        try {
            NB_INFO("export: ===== START FRAME " << frame << " =====");
            Nb::EmpWriter emp(
                sequenceName.data(),
                frame, 
                -1, 
                padding); // TODO: Max seconds!
            // TODO: set time!!

            // Recursively export nodes in scene, starting at the scene root.
            // TODO: Use for selection only?!
            //       int Interface::GetSelNodeCount();
            //       INode* Interface::GetSelNode(int i);

            int numExportedNodes = 0;
            int numExportedMeshNodes = 0;
            int numExportedParticleNodes = 0;
            int numExportedCameraNodes = 0;
            INode *root = ip->GetRootNode();
            for (int i = 0; i < root->NumberOfChildren(); ++i) {
                ExportNode(root->GetChildNode(i), frame, 
                           flipYZ, selectedOnly, 
                           emp,
                           numExportedNodes, numExportedMeshNodes,
                           numExportedParticleNodes, numExportedCameraNodes);
            }

            emp.close();
            ++numExportedFrames;

            NB_INFO("export: "<<numExportedNodes<<" node(s)");
            NB_INFO("export: "<<numExportedMeshNodes<<" mesh node(s)");
            NB_INFO("export: "<<numExportedParticleNodes<<" particle node(s)");
            NB_INFO("export: "<<numExportedCameraNodes<<" camera node(s)");
            NB_INFO("export: ===== END FRAME " << frame << " =====");
        }
        catch (std::exception &ex) {
            NB_ERROR("export: exception: " << ex.what());
        }
        catch (...) {
            NB_ERROR("export: unknown exception");
        }
    }

    std::stringstream ss;
    ss << "Exported " << numExportedFrames << " frame(s)";
    NB_INFO("export: " << ss.str());
    NB_INFO("export: ===== END EXPORT =====");
    _exp.exportStatus->SetText(_T(ss.str().c_str()));
}


//! DOCS
void
NaiadBuddy::ExportNode(INode         *node, 
                       const int      frame, 
                       const bool     flipYZ, 
                       const bool     selectedOnly,
                       Nb::EmpWriter &emp,
                       int           &numExportedNodes,
                       int           &numExportedMeshNodes,
                       int           &numExportedParticleNodes,
                       int           &numExportedCameraNodes)
{
    if (!selectedOnly || (selectedOnly && 0 != node->Selected())) {
        const ObjectState &os = node->EvalWorldState(frame*GetTicksPerFrame());
        Object *obj = os.obj;
        if (obj != NULL) {
            switch (obj->SuperClassID()) {
            case GEOMOBJECT_CLASS_ID:
                {
                // We have a geometric object. Now look for triangles...

                if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) {
                    TriObject *tri = 
                        static_cast<TriObject*>(
                            obj->ConvertToType(
                                frame*GetTicksPerFrame(), 
                                Class_ID(TRIOBJ_CLASS_ID, 0)));

                    MSTR className;
                    obj->GetClassName(className);
                    const int numVerts = tri->mesh.getNumVerts();
                    const int numFaces = tri->mesh.getNumFaces();
                    const int numVData = tri->mesh.getNumVData();
                    const int numMaps = tri->mesh.getNumMaps();
                    NB_INFO("export: Node: " << node->GetName() << 
                            ", Class: " << className);
                    NB_INFO("export: Vertex Count: " << numVerts << 
                            ", Face Count: " << numFaces <<
                            ", VData Count: " << numVData << 
                            ", Map Count: " << numMaps);

                    // Matrix3 is actually a 4x3 matrix. Order of 
                    // multiplication with vertex is irrelevant (?!).

                    const Matrix3 tm = 
                        //node->GetNodeTM(frame*GetTicksPerFrame());
                        node->GetObjTMAfterWSM(frame*GetTicksPerFrame());

                    try {
                        std::auto_ptr<Nb::Body> body(
                            Nb::Factory::createBody(
                                "Mesh", Nb::String(node->GetName())));
                        //body->guaranteeChannel3f(Nb::String("Point.position"));
                        //body->guaranteeChannel3i(Nb::String("Triangle.index"));

                        Nb::PointShape &points = 
                            body->mutablePointShape();
                        Nb::TriangleShape &triangles = 
                            body->mutableTriangleShape();
                        Nb::Buffer3f &positions = 
                            points.mutableBuffer3f("position");
                        Nb::Buffer3i &indices = 
                            triangles.mutableBuffer3i("index");

                        positions.resize(numVerts);
                        for (int v = 0; v < numVerts; ++v) {
                            const Point3 vtx = tm*tri->mesh.verts[v];
                            if (flipYZ) {
                                positions[v][0] = vtx.x;
                                positions[v][1] = vtx.z;
                                positions[v][2] = vtx.y;
                            }
                            else {
                                positions[v][0] = vtx.x;
                                positions[v][1] = vtx.y;
                                positions[v][2] = vtx.z;
                            }
                        }

                        for (int m = 0; m < numMaps; ++m) {

                        }

                        indices.resize(numFaces);
                        for (int f = 0; f < numFaces; ++f) {
                            indices[f][0] = tri->mesh.faces[f].v[0];
                            indices[f][1] = tri->mesh.faces[f].v[1];
                            indices[f][2] = tri->mesh.faces[f].v[2];
                        }

                        NB_INFO("export: Writing body '" << body->name() << 
                                "' to file '" << emp.fileName() << "'");

                        emp.write(body.get(), "*.*"); // All shapes & channels.
                        ++numExportedNodes;
                        ++numExportedMeshNodes;
                    }
                    catch (std::exception &ex) {
                        NB_ERROR("export: exception: " << ex.what());
                    }
                    catch (...) {
                        NB_ERROR("export: unknown exception");
                    }

                    // NB: 3ds Max magic...
                    // Note that the TriObject should only be deleted
                    // if the pointer to it is not equal to the object
                    // pointer that called ConvertToType().

                    if (obj != tri) {
                        tri->DeleteMe();
                        //delete tri;
                    }
                }
                }
                break;
            //case CAMERA_CLASS_ID:
            //    ++numExportedNodes;
            //    ++numExportedMeshNodes;
            //    break;
            default:
                break;
            }
        }
    }

    // Process node's children recursively.

    for (int i = 0; i < node->NumberOfChildren(); ++i) {
        ExportNode(node->GetChildNode(i), frame, 
                   flipYZ, selectedOnly, 
                   emp,
                   numExportedNodes, numExportedMeshNodes,
                   numExportedParticleNodes, numExportedCameraNodes); 
    }
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
    case BN_CLICKED:    // Check boxes.
    case CC_SPINNER_CHANGE:
    case WM_CUSTEDIT_ENTER: 
        // This message is sent when the user presses ENTER on
        // a custom edit control...
    case WM_COMMAND:
        // React to the user interface commands.  
        // A utility plug-in is controlled by the user from here.
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
