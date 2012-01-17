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
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "EmpImport.h"
#include <simpobj.h>

#include <NbEmpReader.h>
#include <NbFilename.h>
#include <NbFactory.h>
#include <NbBodyReader.h>
#include <NbSequenceReader.h>
#include <NbBody.h>
#include <NbString.h>
#include <em_log.h>

#include <memory>

#define EmpImport_CLASS_ID Class_ID(0xa84bdfb3, 0x4fc93524)
#define EmpObject_CLASS_ID Class_ID(0x311e0e37, 0x10a162b1)

//------------------------------------------------------------------------------

int
ticksToFrame(const int ticks)
{ return ticks/GetTicksPerFrame(); }

int 
frameToTicks(const int frame)
{ return frame*GetTicksPerFrame(); }

//------------------------------------------------------------------------------

class EmpImport : public SceneImport 
{
public:

    static HWND hParams;
        
public:     // CTOR/DTOR
    
    EmpImport();

    virtual
    ~EmpImport();		

public:

    virtual int				
    ExtCount();					

    virtual const TCHAR*	
    Ext(int n);					

    virtual const TCHAR*	
    LongDesc();

    virtual const TCHAR*	
    ShortDesc();

    virtual const TCHAR*	
    AuthorName();				

    virtual const TCHAR*	
    CopyrightMessage();			

    virtual const TCHAR*	
    OtherMessage1();			

    virtual const TCHAR*	
    OtherMessage2();	

    virtual unsigned int	
    Version();					

    virtual void			
    ShowAbout(HWND hWnd);		
    
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
    { return "EmpImport";/*GetString(IDS_CLASS_NAME);*/ }

    virtual SClass_ID 
    SuperClassID() 				
    { return SCENE_IMPORT_CLASS_ID; }
    
    virtual Class_ID 
    ClassID() 						
    { return EmpImport_CLASS_ID; }
    
    virtual const TCHAR* 
    Category() 				
    { return "Exotic Matter";/*GetString(IDS_CATEGORY);*/ }

    //! Returns fixed parsable name (scripter-visible name).
    virtual const TCHAR* 
    InternalName() 			
    { return _T("EmpImport"); }

    //! Returns owning module handle.
    virtual HINSTANCE 
    HInstance() 					
    { return hInstance; }					
};

ClassDesc2* 
GetEmpImportDesc() 
{ 
    static EmpImportClassDesc EmpImportDesc;
    return &EmpImportDesc; 
}

//------------------------------------------------------------------------------

class EmpObject : public SimpleObject2
{
public:

    //! CTOR.
    EmpObject() 
        : SimpleObject2()
        , _meshTime(TIME_NegInfinity)
        , _validMesh(FALSE)
    {
        //this->ivalid = FOREVER;

        _seqReader.setFormat("emp");
        _seqReader.setSigFilter("Body");    // TODO: Mesh?
        _seqReader.setPadding(4);           // TODO: Optional!
    }

    //! DTOR.
    virtual
    ~EmpObject() 
    {}

    // From BaseObject.
    virtual CreateMouseCallBack* 
    GetCreateMouseCallBack() 
    { return NULL; }

    // From SimpleObject.
    virtual void 
    BuildMesh(TimeValue t)
    {
        if (t != _meshTime) {
            // Current time, t, is different from the time that the current mesh
            // was built at. First, we clear the current mesh, since it is no
            // longer valid. Thereafter we try to build a new mesh for the
            // current time.

            NB_INFO("EmpObject::BuildMesh - t: " << t << 
                    " f: " << ticksToFrame(t));
            _clearMesh();
            _rebuildMesh(t);
            _meshTime = t;
        }
    }

    //! Returns true if this object has a valid mesh.
    virtual BOOL
    OKtoDisplay()
    { return _validMesh; }

    //From Animatable.
    virtual Class_ID 
    ClassID() 
    { return EmpObject_CLASS_ID; }        

    virtual SClass_ID 
    SuperClassID() 
    { return GEOMOBJECT_CLASS_ID; }

    virtual void 
    GetClassName(TSTR &s) 
    { s = "EmpObject"; }

    virtual void 
    DeleteThis() 
    { delete this; }

public:

    //! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
    const Nb::String&
    bodyName() const
    { return _bodyName; }

    //! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
    void
    setBodyName(const Nb::String &bodyName) 
    { _bodyName = bodyName; }

    //! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
    void
    setSequenceName(const Nb::String &sequenceName)
    { _seqReader.setSequenceName(sequenceName); }

private:

    void
    _rebuildMesh(const TimeValue t)
    {
        try {
            _seqReader.setFrame(ticksToFrame(t));   // May throw.
            Nb::BodyReader *bodyReader = _seqReader.bodyReader();
            std::auto_ptr<Nb::Body> body(bodyReader->ejectBody(_bodyName));
            if (0 != body.get()) {
                // Body was found in sequence on disk.

                NB_INFO("Importing body: '" << body->name() << 
                        "' (signature = '" << body->sig() << "')");

                if (body->matches("Mesh")) {
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
            //delete body;
            // TODO: delete bodyReader; ??
        }
        catch (std::exception &ex) {
            // TODO: cleanup?
            NB_ERROR("exception: " << ex.what());
        }
        catch (...) {
            // TODO: cleanup?
            NB_ERROR("unknown exception");
        }
    }


    //! Clear contents of the existing mesh. 
    //! TODO: Not sure what the best way to do this is... ?
    void
    _clearMesh()
    {
        this->mesh.FreeAll();
        _validMesh = FALSE;
        //this->mesh.setNumVerts(0);
        //this->mesh.setNumFaces(0);
        //this->mesh.InvalidateGeomCache();
    }


    void
    _buildMesh(const Nb::PointShape    &point, 
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

    void
    _addMap3f(MeshMap &map, const Nb::Buffer3f &buf, const Nb::Buffer3i &index)
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

private:    // Member variables.

    Nb::SequenceReader _seqReader; 
    Nb::String         _bodyName;  //!< The name of the body this object tracks.
    TimeValue          _meshTime; 
    BOOL               _validMesh;
};

//------------------------------------------------------------------------------

class EmpObjectClassDesc : public ClassDesc2 
{
public:

    virtual int 
    IsPublic() 							
    { return TRUE; }

    virtual void* 
    Create(BOOL /*loading = FALSE*/) 		
    { return new EmpObject(); }

    virtual const TCHAR*	
    ClassName() 			
    { return "EmpObject";/*GetString(IDS_CLASS_NAME);*/ }   // TODO: What goes here?

    virtual SClass_ID 
    SuperClassID() 				
    { return GEOMOBJECT_CLASS_ID; } // TODO: Not sure what to put here!?
    
    virtual Class_ID 
    ClassID() 						
    { return EmpObject_CLASS_ID; }
    
    virtual const TCHAR* 
    Category() 				
    { return "Exotic Matter"; }//GetString(IDS_CATEGORY); }     // TODO: What goes here?

    //! Returns fixed parsable name (scripter-visible name).
    virtual const TCHAR* 
    InternalName() 			
    { return _T("EmpObject"); }

    //! Returns owning module handle.
    virtual HINSTANCE 
    HInstance() 					
    { return hInstance; }					
};

ClassDesc2* 
GetEmpObjectDesc() 
{ 
    static EmpObjectClassDesc EmpObjectDesc;
    return &EmpObjectDesc; 
}

//------------------------------------------------------------------------------

//class EmpParticleObject : public SimpleParticle 
//{
//public:
//
//    //! CTOR.
//    EmpParticleObject()
//        : SimpleParticle()
//    {}
//
//    //! DTOR.
//    virtual
//    ~EmpParticleObject() 
//    {}
//
//public:
//
//    virtual CreateMouseCallBack*
//    GetCreateMouseCallBack()
//    { 
//        //return SimpleParticle::GetCreateMouseCallBack(); 
//        return 0;
//    }
//
//    virtual void
//    UpdateParticles(TimeValue t, INode *node)
//    {
//        //SimpleParticle::UpdateParticles(t, node);
//    }
//
//    virtual void
//    BuildEmitter(TimeValue t, Mesh &amesh)
//    {
//        //SimpleParticle::BuildEmitter(t, amesh);
//    }
//
//    virtual Interval
//    GetValidity(TimeValue t)
//    { 
//        return Interval();
//        //return SimpleParticle::GetValidity(t);
//    }
//};

//------------------------------------------------------------------------------

INT_PTR CALLBACK 
MaxEmpImportOptionsDlgProc(HWND   hWnd,
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

//! CTOR.
EmpImport::EmpImport()
{}


//! DTOR.
EmpImport::~EmpImport() 
{}


//! Number of extensions supported.
int 
EmpImport::ExtCount()
{ 
    return 1; 
}


//! Extension #n (i.e. "3DS").
const TCHAR*
EmpImport::Ext(int n)
{
    switch (n) {
    case 0:
        return _T("emp");
    default:
        return _T("");  // Bad call!
    }
}


//! Long ASCII description (i.e. "Autodesk 3D Studio File").
const TCHAR*
EmpImport::LongDesc()
{
    return _T("Exotic Matter EMP File");
}
 

//! Short ASCII description (i.e. "3D Studio").
const TCHAR*
EmpImport::ShortDesc() 
{			
    return _T("Exotic Matter");
}


//! ASCII Author name.
const TCHAR*
EmpImport::AuthorName()
{			
    return _T("Exotic Matter");
}


//! ASCII Copyright message.
const TCHAR*
EmpImport::CopyrightMessage() 
{	
    return _T("Copyright Exotic Matter 2012");
}


//! Other message #1, if any.
const TCHAR*
EmpImport::OtherMessage1() 
{		
    return _T("");
}


//! Other message #2, if any.
const TCHAR*
EmpImport::OtherMessage2() 
{		
    return _T("");
}


//! Version number * 100 (i.e. v3.01 = 301).
unsigned int 
EmpImport::Version()
{				
    return 10; // Version 0.1
}


//! Show DLL's "About..." box. Optional.
void 
EmpImport::ShowAbout(HWND hWnd)
{}


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

            EmpObject *empObject = 
                static_cast<EmpObject*>(
                    i->CreateInstance(GEOMOBJECT_CLASS_ID, EmpObject_CLASS_ID));

            // Set information required by the object's 
            // internal sequence reader.

            empObject->setBodyName(body->name());
            empObject->setSequenceName(sequenceName);
            //empObject->BuildMesh(i->GetTime());

            ImpNode *impNode = ii->CreateNode();
            if (NULL != impNode) {
                Matrix3 xf; // TMP! Hard-coded transform and time-value. 
                xf.IdentityMatrix();   // Body data already in world-space.
                const TimeValue t = 0; // No time-varying XForm.
                impNode->SetTransform(t, xf);
                impNode->Reference(empObject);
                MSTR nodeName = _M(empObject->bodyName().c_str());
                i->MakeNameUnique(nodeName);
                impNode->SetName(nodeName);
                ii->AddNodeToScene(impNode);
            }
            else {
                // For some reason the node could not be added to the scene.
                // Simply delete the EmpObject and move on, hoping
                // for better luck with the rest of the Nb::Body objects.

                delete empObject;
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


//GeomObject*
//MaxEmpImport::_importBody(const Nb::Body *nbBody, Interface *i)
//{
//    NB_INFO("Importing body: '" << nbBody->name() << 
//            "' (signature = '" << nbBody->sig() << "')");
//
//    if (nbBody->matches("Mesh")) {
//        // We own the memory for the newly created object. Note that if the mesh
//        // cannot be created properly we must remember to delete this object.
//
//        //SampleGObject* myGeomObj = (SampleGObject*)ip->CreateInstance(GEOMOBJECT_CLASS_ID, SampleGObject_CLASS_ID); 
//        EmpObject *empObject = 
//            static_cast<EmpObject*>(
//                i->CreateInstance(GEOMOBJECT_CLASS_ID, EmpObject_CLASS_ID)); 
//        //TriObject *triObject = CreateNewTriObject();
//        if (0 != empObject) {
//            // Valid object.
//            // Point and triangle shapes are guaranteed to exist according to 
//            // the Mesh-signature.
//
//            const Nb::PointShape    &point    = nbBody->constPointShape();
//            const Nb::TriangleShape &triangle = nbBody->constTriangleShape();
//
//            if (_buildMesh(point, triangle, empObject->mesh/*triObject->GetMesh()*/)) {
//                return empObject; // Successful mesh creation.
//            }
//            else {
//                // For some reason the mesh could not be created. Free memory
//                // and return null pointer.
//
//                delete empObject;   
//                return NULL;
//            }
//        }
//        else {
//            return NULL;
//        }
//    }
//    //else if (nbBody->matches("Particle")) {
//    //    EmpParticleObject *particleObject = new EmpParticleObject;
//    //    if (0 != particleObject) {
//    //        const Nb::ParticleShape &particle = nbBody->constParticleShape();
//
//    //        if (_buildParticle(particle, particleObject->parts)) {
//    //            return particleObject;
//    //        }
//    //        else {
//    //            delete particleObject;
//    //            return NULL;
//    //        }
//    //    }
//    //    else {
//    //        return NULL;   
//    //    }
//    //}
//    // else if (nbBody->matches("Camera") {
//    // }
//    else {
//        NB_WARNING("Body signature '" << nbBody->sig() << "' not supported");
//    }
//
//    return NULL;   // Null.
//}


//BOOL
//MaxEmpImport::_buildParticle(const Nb::ParticleShape &particle,
//                             ParticleSys             &psys)
//{
//    return FALSE;
//}
