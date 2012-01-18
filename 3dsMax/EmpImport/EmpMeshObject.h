// -----------------------------------------------------------------------------
//
// EmpMeshObject.cpp
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

#ifndef EMP_MESH_OBJECT_H_INCLUDED
#define EMP_MESH_OBJECT_H_INCLUDED

#include <simpobj.h>
#include <iparamb2.h>

#include <NbSequenceReader.h>
#include <NbBuffer.h>
#include <NbString.h>

namespace Nb {
    class PointShape;
    class TriangleShape;
}

//------------------------------------------------------------------------------

#define EmpMeshObject_CLASS_ID Class_ID(0x311e0e37, 0x10a162b1)

//------------------------------------------------------------------------------

//extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

//------------------------------------------------------------------------------

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
    { s = "EmpMeshObject"; }

    virtual void 
    DeleteThis() 
    { delete this; }

public:

    const Nb::String&
    bodyName() const;

    void
    setBodyName(const Nb::String &bodyName);

    void
    setSequenceName(const Nb::String &sequenceName);

private:

    void
    _rebuildMesh(TimeValue t);

    void
    _clearMesh();

    void
    _buildMesh(const Nb::PointShape &point, const Nb::TriangleShape &triangle);

    void
    _addMap3f(MeshMap &map, const Nb::Buffer3f &buf, const Nb::Buffer3i &index);

private:    // Member variables.

    Nb::SequenceReader _seqReader; 
    Nb::String         _bodyName;  //!< The name of the body this object tracks.
    TimeValue          _meshTime; 
    BOOL               _validMesh;
};

//------------------------------------------------------------------------------

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
    { return _T("EmpMeshObject"); } // GetString(IDS_CLASS_NAME); 

    virtual SClass_ID 
    SuperClassID() 				
    { return GEOMOBJECT_CLASS_ID; } // TODO: Not sure what to put here!?
    
    virtual Class_ID 
    ClassID() 						
    { return EmpMeshObject_CLASS_ID; }
    
    virtual const TCHAR* 
    Category() 				
    { return _T("Exotic Matter"); } //GetString(IDS_CATEGORY);

    //! Returns fixed parsable name (scripter-visible name).
    virtual const TCHAR* 
    InternalName() 			
    { return _T("EmpMeshObject"); }

    //! Returns owning module handle.
    virtual HINSTANCE 
    HInstance() 					
    { return hInstance; }					
};

//------------------------------------------------------------------------------

ClassDesc2* 
GetEmpMeshObjectDesc();

//------------------------------------------------------------------------------

#endif // EMP_MESH_OBJECT_H_INCLUDED











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
