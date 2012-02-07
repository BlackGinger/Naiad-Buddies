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
#include <iostream>
#include <iterator>
#include <algorithm>
#include <map>
#include <windows.h>
#include <string>
//#include <atlbase.h>

// -----------------------------------------------------------------------------

#define NaiadBuddy_CLASS_ID	   Class_ID(0x59d78e7e, 0x9b2064f2)
#define EmpMeshObject_CLASS_ID Class_ID(0x311e0e37, 0x10a162b1)


//------------------------------------------------------------------------------

void
getMapChannelName(INode             *node, 
                    const int          id,
                    std::string       &name,
                    const std::string &keyPrefix = std::string("MapChannel"))
{
    if (NULL != node) {
        TSTR key, tname;
        key.printf("%s:%d", keyPrefix.c_str(), id);
        node->GetUserPropString(key, tname);
        //CT2CA tmp(tname);   // Ugly string conversion.
        //name = std::string(tmp);
        name = std::string(tname.data());
    }
}

void
setMapChannelName(INode             *node, 
                    const int          id,
                    const std::string &name,
                    const std::string &keyPrefix = std::string("MapChannel"))
{
    if (NULL != node) {
        TSTR key;
        key.printf("%s:%d", keyPrefix.c_str(), id);
        node->SetUserPropString(key, TSTR(name.c_str()));
    }
}

//------------------------------------------------------------------------------

//! DOCS
template <typename T>
T 
lexical_cast(const std::string &s)
{
    std::stringstream ss(s);
    T result;
    if ((ss >> result).fail() || !(ss >> std::ws).eof()) {
        throw std::bad_cast("bad lexical_cast");
    }
    return result;
}

//------------------------------------------------------------------------------

//! DOCS
class NaiadChannel
{
public:

    static const NaiadChannel blind;

public:

    //! CTOR.
    explicit
    NaiadChannel(const Nb::String &name,      
                 const em::vec3i  &comp = em::vec3i(0, 1, 2))
        : _name(name)
        , _comp(comp)
    {}

public:

    const Nb::String&
    name() const
    { return _name; }

    void
    setName(const Nb::String &name)
    { _name = name; }

    em::vec3i&
    components()
    { return _comp; }

    void
    setComponents(const em::vec3i &comp)
    { _comp = comp; }

    //! No bounds checking!
    int
    component(const int i) const
    { return _comp[i]; }

    //! No bounds checking!
    void
    setComponent(const int i, const int c)
    { _comp[i] = c; }

private:    // Member variables.

    Nb::String _name;
    em::vec3i  _comp;
};


//! DOCS [static]
const NaiadChannel NaiadChannel::blind = NaiadChannel("");


//! Returns true if the provided NaiadChannel is blind.
bool
isBlind(const NaiadChannel &pc)
{ return (pc.name() == NaiadChannel::blind.name()); }


//! Compare names only.
bool
operator<(const NaiadChannel &lhs, const NaiadChannel &rhs)
{ return lhs.name() < rhs.name(); }


//! Convenience
std::ostream&
operator<<(std::ostream &os, const NaiadChannel &nc)
{
    os << "'" << nc.name().str() << "'"
       << "[" << nc.component(0) << nc.component(1) << nc.component(2) << "]";
    return os;
}

//------------------------------------------------------------------------------

//! DOCS
class MaxChannel
{
public:

    static const MaxChannel blind;

public:

    //! CTOR.
    explicit
    MaxChannel(const int        id,      
               const em::vec3i &comp = em::vec3i(0, 1, 2))
        : _id(id)
        , _comp(comp)
    {}

public:

    int
    id() const
    { return _id; }

    void
    setId(const int id)
    { _id = id; }

    em::vec3i&
    components()
    { return _comp; }

    void
    setComponents(const em::vec3i &comp)
    { _comp = comp; }

    //! No bounds checking!
    int
    component(const int i) const
    { return _comp[i]; }

    //! No bounds checking!
    void
    setComponent(const int i, const int c)
    { _comp[i] = c; }

private:    // Member variables.

    int       _id;
    em::vec3i _comp;
};


//! DOCS [static]
const MaxChannel MaxChannel::blind = MaxChannel(-1);


//! Returns true if the provided MaxChannel has the blind id.
bool
isBlind(const MaxChannel &mc)
{ return (mc.id() <= MaxChannel::blind.id()); }


//! Compare Id's only.
bool
operator<(const MaxChannel &lhs, const MaxChannel &rhs)
{ return lhs.id() < rhs.id(); }


//! Convenience
std::ostream&
operator<<(std::ostream &os, const MaxChannel &mc)
{
    os << mc.id() << ":map"
       << "[" << mc.component(0) << mc.component(1) << mc.component(2) << "]";
    return os;
}

//------------------------------------------------------------------------------

class NaiadToMaxMapping 
{
public:

    typedef std::map<NaiadChannel,MaxChannel>   MappingType;
    typedef MappingType::const_iterator         MappingConstIterator;
    typedef std::set<NaiadChannel>              NaiadChannelSetType;
    typedef NaiadChannelSetType::const_iterator NaiadChannelConstIterator;
    typedef std::set<int>                       IdSetType;
    typedef IdSetType::const_iterator           IdConstIterator;

public:

    //! DOCS
    class Entry
    {
    public:

        //! CTOR.
        explicit
        Entry(const NaiadChannel &pc, const MaxChannel &mc)
            : _nc(pc)
            , _mc(mc)
        {}

        const NaiadChannel&
        naiadChannel() const
        { return _nc; }

        MaxChannel&
        maxChannel()
        { return _mc; }

    private:    // Member variables.

        NaiadChannel _nc;
        MaxChannel   _mc;
    };

public:

    //! CTOR.
    explicit
    NaiadToMaxMapping(const int minId = 0, 
                      const int maxId = (MAX_MESHMAPS - 1))
        : _minId(minId)
        , _maxId(maxId)
    {
        reset();
    }

public:

    //! DOCS
    bool
    map(const NaiadChannel &nc, const MaxChannel &mc)
    { 
        // Blind channels cannot be used with mappings.

        if (!isBlind(nc) && !isBlind(mc) && 
            (_minId <= mc.id() && mc.id() <= _maxId)) {
            _mapping.insert(MappingType::value_type(nc, mc)); 
            _mappedIds.insert(mc.id());
            _excluded.erase(nc); // Remove exclusion.
            _blindIds.erase(mc.id());
            return true;
        }
        return false;
    }

    //! DOCS
    Entry
    find(const Nb::String &name) const
    {
        NaiadChannel nc(name); // Naiad-channel with name + default settings.
        const MappingType::const_iterator iter = _mapping.find(nc);
        if (iter != _mapping.end()) {
            return Entry(iter->first, iter->second);
        }
        else {
            return Entry(nc, MaxChannel::blind);
        }
    }

    //! DOCS
    bool
    exclude(const NaiadChannel &nc)
    {
        // Blind channels cannot be excluded.

        if (!isBlind(nc)) {
            const MappingType::const_iterator iter = _mapping.find(nc);
            if (iter != _mapping.end()) {
                // Remove mapping.

                _mappedIds.erase(iter->second.id());
                _blindIds.insert(iter->second.id());
                _mapping.erase(iter);
            }
            _excluded.insert(nc);
            return true;
        }
        return false;
    }

    //! DOCS
    bool
    excluded(const Nb::String &name) const
    { return (_excluded.find(NaiadChannel(name)) != _excluded.end()); }


    //! Invalidates ALL iterators!
    void
    reset()
    {
        _mapping.clear();
        _mappedIds.clear();
        _excluded.clear();

        for (int i = _minId; i <= _maxId; ++i) { // All Id's blind initially.
            _blindIds.insert(i);
        }
    }

public:

    int
    minMappedId() const
    { return *_mappedIds.begin(); }

    int
    maxMappedId() const
    { return *_mappedIds.rbegin(); }

    int
    numMappedIds() const
    { return static_cast<int>(_mappedIds.size()); }

public:

    const IdSetType&
    blindIds() const
    { return _blindIds; }

    IdConstIterator
    blindIdBegin() const
    { return _blindIds.begin(); }

    IdConstIterator
    blindIdEnd() const
    { return _blindIds.end(); }

public:

    std::ostream&
    print(std::ostream &os) const
    {
        os << "Valid Id range: [" << _minId << ", " << _maxId << "]\n";
        for (MappingConstIterator iter = _mapping.begin();
            iter != _mapping.end(); ++iter) {
            os << iter->first << " --> " << iter->second << "\n";
        }
        os << "Excluded: {";
        if (!_excluded.empty()) {
            for (NaiadChannelConstIterator iter = _excluded.begin();
                iter != _excluded.end(); ++iter) {
                os << *iter << ((iter != --_excluded.end()) ? ", " : "");
            }
        }
        os << "}\n";
        os << "Mapped Id's: {";
        if (!_mappedIds.empty()) {
            for (IdConstIterator iter = _mappedIds.begin();
                iter != _mappedIds.end(); ++iter) {
                os << *iter << ((iter != --_mappedIds.end()) ? "," : "");
            }
        }
        os << "}\n";
        os << "Blind Id's: {";
        if (!_blindIds.empty()) {
            for (IdConstIterator iter = _blindIds.begin();
                iter != _blindIds.end(); ++iter) {
                os << *iter << ((iter != --_blindIds.end()) ? "," : "");
            }
        }
        os << "}\n";
        return os;
    }

private:    // Member variables.

    int _minId;
    int _maxId;

    MappingType         _mapping;
    NaiadChannelSetType _excluded;
    IdSetType           _mappedIds;
    IdSetType           _blindIds;
};


//! Print operator.
std::ostream&
operator<<(std::ostream &os, NaiadToMaxMapping &rhs)
{ return rhs.print(os); }

//------------------------------------------------------------------------------

//! DOCS
class MaxToNaiadMapping 
{
public:

    typedef std::map<MaxChannel,NaiadChannel> MappingType;
    typedef MappingType::const_iterator       MappingConstIterator;
    typedef std::set<MaxChannel>              MaxChannelSetType;
    typedef MaxChannelSetType::const_iterator MaxChannelConstIterator;

public:

    //! DOCS
    class Entry
    {
    public:

        //! CTOR.
        explicit
        Entry(const MaxChannel &mc, const NaiadChannel &nc)
            : _mc(mc)
            , _nc(nc)
        {}

    public:

        const MaxChannel&
        maxChannel() const
        { return _mc; }

        NaiadChannel&
        naiadChannel()
        { return _nc; }

    private:    // Member variables.

        MaxChannel   _mc;
        NaiadChannel _nc;
    };

public:

    //! CTOR.
    explicit
    MaxToNaiadMapping(const int minId = 0, 
                      const int maxId = (MAX_MESHMAPS - 1))
        : _minId(minId)
        , _maxId(maxId)
    {
        reset();
    }

public:

    //! DOCS
    bool
    map(const MaxChannel &mc, const NaiadChannel &nc)
    { 
        if (!isBlind(mc) && (_minId <= mc.id() && mc.id() <= _maxId) &&
            !isBlind(nc)) {
            _mapping.insert(MappingType::value_type(mc, nc)); 
            return true;
        }
        return false;
    }

    //! DOCS
    Entry
    find(const int id) const
    {
        const MaxChannel mc(id);
        const MappingType::const_iterator iter = _mapping.find(mc);
        if (iter != _mapping.end()) {
            return Entry(
                iter->first, 
                iter->second);
        }
        else { // Blind.
            return Entry(        
                mc,
                NaiadChannel::blind);
        }
    }

    //! DOCS
    bool
    exclude(const MaxChannel &mc)
    {
        if (!isBlind(mc)) {
            const MappingType::const_iterator iter = _mapping.find(mc);
            if (iter != _mapping.end()) {
                _mapping.erase(iter);
            }
            _excluded.insert(mc);
            return true;
        }
        return false;
    }

    //! DOCS
    bool
    excluded(const int id) const
    { return (_excluded.find(MaxChannel(id)) != _excluded.end()); }

    //! Invalidates ALL iterators!
    void
    reset()
    {
        _mapping.clear();
        _excluded.clear();
    }

public:

    std::ostream&
    print(std::ostream &os) const
    {
        os << "Valid Id range: [" << _minId << ", " << _maxId << "]\n";
        for (MappingConstIterator iter = _mapping.begin();
            iter != _mapping.end(); ++iter) {
            os << iter->first << " --> " << iter->second << "\n";
        }
        os << "Excluded: {";
        if (!_excluded.empty()) {
            for (MaxChannelConstIterator iter = _excluded.begin();
                iter != _excluded.end(); ++iter) {
                os << *iter << ((iter != --_excluded.end()) ? ", " : "");
            }
        }
        os << "}\n";
        return os;
    }

private:    // Member variables.

    int _minId;
    int _maxId;

    MappingType       _mapping;
    MaxChannelSetType _excluded;
};


//! Print operator.
std::ostream&
operator<<(std::ostream &os, MaxToNaiadMapping &rhs)
{ return rhs.print(os); }

//------------------------------------------------------------------------------

//! DOCS
void
parseComponents(const std::string &expr, em::vec3i &comp)
{
    using std::size_t;
    using std::min;

    const int size = static_cast<int>(min<size_t>(expr.size(), 3));
    for (int i = 0; i < size; ++i) { 
        switch (expr[i]) {
        case '0':
        case 'r':
        case 'u':
        case 'x':
            comp[i] = 0;
            break;
        case '1':
        case 'g':
        case 'v':
        case 'y':
            comp[i] = 1;
            break;
        case '2':
        case 'b':
        case 'w':
        case 'z':
            comp[i] = 2;
            break;
        default:
            comp[i] = i;
            break;
        }
    }
}


//! DOCS
bool
parseNaiadChannel(const std::string &expr, NaiadChannel &nc)
{
    using std::size_t;
    using std::string;
    using std::exception;

    if (expr.empty()) {
        return false; // Failure!
    }

    const size_t lb = expr.find_first_of('[');
    const size_t rb = expr.find_last_of(']');
    const string name = expr.substr(0, lb);
    if (!name.empty()) {
        nc.setName(name);
        if (lb < rb) { // Bracket pair present in expression.
            parseComponents(expr.substr(lb + 1, rb - lb - 1), nc.components());
        }
        return true;    // Success!
    }
    else {
        NB_WARNING("parse: Empty Naiad Channel name");
        return false; // Failure!
    }
}


//! DOCS
bool
parseMaxChannel(const std::string &expr, MaxChannel &mc)
{
    using std::size_t;
    using std::string;
    using std::exception;

    if (expr.empty()) {
        return false; // Failure!
    }

    const size_t lb = expr.find_first_of('[');
    const size_t rb = expr.find_last_of(']');
    const string id = expr.substr(0, lb);
    if (1 <= id.size() && id.size() <= 2) {
        try {
            mc.setId(lexical_cast<int>(id)); // May throw.
        }
        catch (const exception &ex) {
            NB_WARNING("parse: Invalid Max Channel Id: '" << id << "':" <<
                        ex.what());
            return false; // Failure!
        }

        if (!(0 <= mc.id() && mc.id() <= (MAX_MESHMAPS - 1))) {
            NB_WARNING(
                "parse: Invalid Max Channel Id: " << 
                mc.id() << " (must be in the range [0, " 
                << (MAX_MESHMAPS - 1) << "])");
            return false; // Failure!
        }

        if (lb < rb) { // Bracket pair present in expression.
            parseComponents(expr.substr(lb + 1, rb - lb - 1), mc.components());
        }

        return true;    // Success!
    }
    else {
        NB_WARNING(
            "parse: Invalid Max Channel Id string: '" << 
            id << "' (must have size [1,2])");
        return false; // Failure!
    }
}


//! DOCS
void
parseNaiadToMax(const std::string &expr, NaiadToMaxMapping &mapping)
{
    using std::string;
    using std::stringstream;
    using std::vector;
    using std::map;
    using std::istream_iterator;
    using std::atoi;
    using std::size_t;
    using std::count;

    mapping.reset();

    if (expr.empty()) {
        return; // Early exit.
    }

    stringstream ss(expr); // Create a stream from the string.

    // Use stream iterators to copy the stream to the vector 
    // as whitespace separated strings.

    istream_iterator<string> iter(ss);
    istream_iterator<string> iend;
    vector<string> tokens(iter, iend);

    typedef vector<string>::const_iterator TokenIterator;
    for (TokenIterator token = tokens.begin(); token != tokens.end(); ++token) {
        const size_t numColons = count(token->begin(), token->end(), ':');
        const size_t numCarets = count(token->begin(), token->end(), '^');
        if (numColons == 1) {
            const Nb::String nbToken = *token;
            const string naiadChannelStr = 
                nbToken.parent(":").str(); // Left of ':'.
            const string maxChannelStr = 
                nbToken.child(":").str(); // Right of ':'.
            if (!naiadChannelStr.empty() && !maxChannelStr.empty()) {
                NaiadChannel nc = NaiadChannel::blind;
                MaxChannel mc = MaxChannel::blind;
                if (parseNaiadChannel(naiadChannelStr, nc) &&
                    parseMaxChannel(maxChannelStr, mc)) {
                    NB_INFO("parse: Map: Naiad Channel: " << nc << " --> " << 
                            "Max Channel: " << mc);
                    mapping.map(nc, mc);
                }
            }
            else {
                NB_WARNING("parse: Empty channel token: " << 
                           "Naiad Channel: '" << naiadChannelStr << "'" << 
                           "Max Channel: '" << maxChannelStr << "'");
            }
        }
        else if (numCarets == 1) {
            const Nb::String nbToken = *token;
            const string naiadChannelStr = 
                nbToken.child("^").str(); // Right of '^'.
            if (!naiadChannelStr.empty()) {
                NaiadChannel nc = NaiadChannel::blind;
                if (parseNaiadChannel(naiadChannelStr, nc)) {
                    NB_INFO("parse: Exclude: Naiad Channel: " << nc);
                    mapping.exclude(nc);
                }
            }
            else {
                NB_WARNING("parse: Empty Naiad Channel token");
            }
        }
        else {
            NB_WARNING("parse: Invalid token '" << *token << "' ignored." << 
                       "Tokens must contain exactly one ':' or one '^'");
        }
    }
}


//! DOCS
void
parseMaxToNaiad(const std::string &expr, MaxToNaiadMapping &mapping)
{
    using std::string;
    using std::stringstream;
    using std::vector;
    using std::map;
    using std::istream_iterator;
    using std::atoi;
    using std::size_t;
    using std::count;

    mapping.reset();

    if (expr.empty()) {
        return; // Early exit.
    }

    stringstream ss(expr); // Create a stream from the string.

    // Use stream iterators to copy the stream to the vector 
    // as whitespace separated strings.

    istream_iterator<string> iter(ss);
    istream_iterator<string> iend;
    vector<string> tokens(iter, iend);

    typedef vector<string>::const_iterator TokenIterator;
    for (TokenIterator token = tokens.begin(); token != tokens.end(); ++token) {
        const size_t numColons = count(token->begin(), token->end(), ':');
        const size_t numCarets = count(token->begin(), token->end(), '^');
        if (numColons == 1) {
            const Nb::String nbToken = *token;
            const string maxChannelStr = 
                nbToken.parent(":").str(); // Left of ':'.
            const string naiadChannelStr = 
                nbToken.child(":").str(); // Right of ':'.
            if (!naiadChannelStr.empty() && !maxChannelStr.empty()) {
                MaxChannel mc = MaxChannel::blind;
                NaiadChannel nc = NaiadChannel::blind;
                if (parseMaxChannel(maxChannelStr, mc) &&
                    parseNaiadChannel(naiadChannelStr, nc)) {
                    NB_INFO("parse: Map: Max Channel: " << mc << " --> " << 
                            "Naiad Channel: " << nc);
                    mapping.map(mc, nc);
                }
            }
            else {
                NB_WARNING("parse: Empty channel token: " << 
                           "Max Channel: '" << maxChannelStr << "' " << 
                           "Naiad Channel: '" << naiadChannelStr << "'");
            }
        }
        else if (numCarets == 1) {
            const Nb::String nbToken = *token;
            const string maxChannelStr = 
                nbToken.child("^").str(); // Right of '^'.
            if (!maxChannelStr.empty()) {
                MaxChannel mc = MaxChannel::blind;
                if (parseMaxChannel(maxChannelStr, mc)) {
                    NB_INFO("parse: Exclude: Max Channel: " << mc);
                    mapping.exclude(mc);
                }
            }
            else {
                NB_WARNING("parse: Empty Max Channel token");
            }
        }
        else {
            NB_WARNING("parse: Invalid token '" << *token << "' ignored." << 
                       "Tokens must contain exactly one ':' or one '^'");
        }
    }
}

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

    void
    setNode(INode *node)
    { _node = node; }

    void
    setChannelMapping(const Nb::String &expr)
    {
        parseNaiadToMax(expr, _mapping);
        NB_INFO("EmpMeshObject: NaiadToMaxMapping:\n" << _mapping);
    }

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
                     const NaiadChannel &nc,
                     const MaxChannel   &mc);

    static void
    _buildMapVerts1f(MeshMap            &map, 
                     const Nb::Buffer1f &buf,
                     const NaiadChannel &nc,
                     const MaxChannel   &mc);

    static void
    _buildMapVerts3f(MeshMap            &map, 
                     const Nb::Buffer3f &buf,
                     const NaiadChannel &nc,
                     const MaxChannel   &mc);

    static void
    _buildMapFaces(MeshMap            &map, 
                   const Nb::Buffer3i &index);

private:    // Member variables.

    INode              *_node;
    Nb::SequenceReader  _seqReader; 
    Nb::String          _bodyName;  //!< The name of the body this object tracks.
    bool                _flipYZ;
    int                 _meshFrame; 
    BOOL                _validMesh;
    NaiadToMaxMapping   _mapping;
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
    , _node(NULL)
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

            NB_INFO("EmpMeshObject: Importing body: '" << body->name() << 
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

    // Import Max-channels from Naiad-channels.

    NaiadToMaxMapping::IdConstIterator blindIdIter = _mapping.blindIdBegin();
    const int pointChannelCount = point.channelCount();
    for (int pc = 0; pc < pointChannelCount; ++pc) {
        const Nb::Channel *chan = point.channel(pc)();
        const Nb::String channelName = chan->name();
        if (!(channelName == "position")) { // Ignore position channel.
            if (!_mapping.excluded(channelName)) {
                NaiadToMaxMapping::Entry entry = _mapping.find(channelName);
                const NaiadChannel &nc = entry.naiadChannel();
                MaxChannel &mc = entry.maxChannel();

                const bool blind = isBlind(mc);
                if (blind) {
                    if (blindIdIter == _mapping.blindIdEnd()) {
                        NB_WARNING(
                            "EmpMeshObject: No more available Map Channels");
                        continue; // There might be mapped channels after this.
                    }
                    mc.setId(*blindIdIter++);
                }
                    
                NB_INFO("EmpMeshObject: Naiad Channel<" 
                        << chan->typeName() << ">: " 
                        << nc << " --> Max Channel: " << mc << 
                        (blind ? " (blind)" : ""));  

                switch (chan->type()) {
                case Nb::ValueBase::IntType:
                    {
                    const Nb::Buffer1i &b1i = point.constBuffer1i(nc.name());
                    this->mesh.setMapSupport(mc.id(), TRUE);
                    MeshMap &meshMap = this->mesh.Map(mc.id());
                    _buildMapVerts1i(meshMap, b1i, nc, mc);
                    _buildMapFaces(meshMap, index);
                    setMapChannelName(_node, mc.id(), nc.name());
                    }
                    break;
                case Nb::ValueBase::FloatType:
                    {
                    const Nb::Buffer1f &b1f = point.constBuffer1f(nc.name());
                    this->mesh.setMapSupport(mc.id(), TRUE);
                    MeshMap &meshMap = this->mesh.Map(mc.id());
                    _buildMapVerts1f(meshMap, b1f, nc, mc);
                    _buildMapFaces(meshMap, index);
                    setMapChannelName(_node, mc.id(), nc.name());
                    }
                    break;
                case Nb::ValueBase::Vec3fType:
                    {
                    const Nb::Buffer3f &b3f = point.constBuffer3f(nc.name());
                    this->mesh.setMapSupport(mc.id(), TRUE);
                    MeshMap &meshMap = this->mesh.Map(mc.id());
                    _buildMapVerts3f(meshMap, b3f, nc, mc);
                    _buildMapFaces(meshMap, index);
                    setMapChannelName(_node, mc.id(), nc.name());
                    }
                    break;
                default:
                    NB_WARNING(
                        "EmpMeshObject: Naiad Channel type: <" << 
                        chan->typeName() << "> not supported yet!");
                    break;
                }
            }
            else {
                NB_INFO("EmpMeshObject: Excluded Naiad Channel<" 
                        << chan->typeName() << ">: '" << channelName << "'");  
            }
        }
    }

    this->mesh.buildNormals();
    this->mesh.buildBoundingBox();
    this->mesh.InvalidateEdgeList();
    this->mesh.InvalidateGeomCache();
}


//! DOCS [static]
void
EmpMeshObject::_buildMapVerts1i(MeshMap            &map, 
                                const Nb::Buffer1i &buf,
                                const NaiadChannel &nc,
                                const MaxChannel   &mc)
{
    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int v = 0; v < numVerts; ++v) { // Set map vertices.
        UVVert &uvVert = map.tv[v];
        for (int i = 0; i < 3; ++i) { 
            uvVert[mc.component(i)] = static_cast<float>(buf[v]); // Swizzle!
        }
    }
}


//! DOCS [static]
void
EmpMeshObject::_buildMapVerts1f(MeshMap            &map, 
                                const Nb::Buffer1f &buf,
                                const NaiadChannel &nc,
                                const MaxChannel   &mc)
{
    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int v = 0; v < numVerts; ++v) { // Set map vertices.
        UVVert &uvVert = map.tv[v];
        for (int i = 0; i < 3; ++i) { 
            uvVert[mc.component(i)] = buf[v]; // Swizzle!
        }
    }
}


//! DOCS [static]
void
EmpMeshObject::_buildMapVerts3f(MeshMap            &map, 
                                const Nb::Buffer3f &buf,
                                const NaiadChannel &nc,
                                const MaxChannel   &mc)
{
    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int v = 0; v < numVerts; ++v) { // Set map vertices.
        UVVert &uvVert = map.tv[v];
        for (int i = 0; i < 3; ++i) { 
            uvVert[mc.component(i)] = buf[v][nc.component(i)]; // Swizzle!
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
               const MaxToNaiadMapping &mapping,
               Nb::EmpWriter &emp,
               int &numExportedNodes, int &numExportedMeshNodes,
               int &numExportedParticleNodes, int &numExportedCameraNodes);

    void
    ExportMapChannel(const MaxChannel   &mc, 
                     const NaiadChannel &pc, 
                     /* const */ Mesh   &mesh, 
                     Nb::PointShape     &point);

private:

    //! DOCS
    struct _General
    {
        ICustEdit   *projectPathEdit;
        Nb::String   projectPath;

        ICustButton *projectPathBrowseButton;

        ICustButton *showLogButton;
    };

    //! DOCS
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

        ICustEdit       *channelMappingEdit;
        Nb::String       channelMapping;

        ISpinnerControl *frameOffsetSpinner;
        int              frameOffset;
        bool             flipYZ;

        ICustButton     *importButton;
        //ICustStatusEdit *importStatusEdit;
        ICustStatus     *importStatus;
    };

    //! DOCS
    struct _Export
    {
        ICustEdit       *sequenceNameEdit;
        Nb::String       sequenceName;
        ICustButton     *browseButton;

        ICustEdit       *channelMappingEdit;
        Nb::String       channelMapping;

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
    _imp.channelMapping  = Nb::String(
        "rgb:0 "
        "uv[uvv]:1[uvv] "
        "velocity[xyz]:2[xzy]"
        );
    _imp.frameOffset     = 0;
    _imp.flipYZ          = true;

    _exp.sequenceName    = Nb::String("");
    _exp.channelMapping  = Nb::String(
        "0:rgb "
        "1[uvv]:uv[uvv] "
        "2[xyz]:velocity[xzy]"
        );
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

    _imp.channelMappingEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_IMP_CHANNEL_MAPPING_EDIT));
    _imp.channelMappingEdit->SetText(_imp.channelMapping.c_str());

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
    _exp.sequenceNameEdit->SetText(_exp.sequenceName.c_str());

    _exp.browseButton = 
        GetICustButton(GetDlgItem(hWnd, IDC_EXP_BROWSE_BUTTON));

    _exp.channelMappingEdit = 
        GetICustEdit(GetDlgItem(hWnd, IDC_EXP_CHANNEL_MAPPING_EDIT));
    _exp.channelMappingEdit->SetText(_exp.channelMapping.c_str());

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
    ReleaseICustEdit(_imp.channelMappingEdit);
    ReleaseISpinner(_imp.frameOffsetSpinner);
    ReleaseICustButton(_imp.importButton);
    ReleaseICustStatus(_imp.importStatus);


    // Export.

    ReleaseICustEdit(_exp.sequenceNameEdit);
    ReleaseICustEdit(_exp.channelMappingEdit);
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
    case IDC_IMP_EMP_SEQUENCE_EDIT:
        {
        MSTR sequenceName;
        _imp.sequenceNameEdit->GetText(sequenceName);
        _imp.sequenceName = Nb::String(sequenceName.data());
        }
        break;
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
    case IDC_IMP_CHANNEL_MAPPING_EDIT:
        {
        MSTR channelMapping;
        _imp.channelMappingEdit->GetText(channelMapping);
        _imp.channelMapping = Nb::String(channelMapping.data());
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
    case IDC_EXP_EMP_SEQUENCE_EDIT:
        {
        MSTR sequenceName;
        _exp.sequenceNameEdit->GetText(sequenceName);
        _exp.sequenceName = Nb::String(sequenceName.data());
        }
        break;
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
    case IDC_EXP_CHANNEL_MAPPING_EDIT:
        {
        MSTR channelMapping;
        _exp.channelMappingEdit->GetText(channelMapping);
        _exp.channelMapping = Nb::String(channelMapping.data());
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

        MSTR channelMapping;
        _imp.channelMappingEdit->GetText(channelMapping);

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
                    empMeshObj->setChannelMapping(
                        Nb::String(channelMapping.data()));

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
                        empMeshObj->setNode(node);
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

    MSTR channelMapping;
    _exp.channelMappingEdit->GetText(channelMapping);
    MaxToNaiadMapping mapping;
    parseMaxToNaiad(std::string(channelMapping.data()), mapping);

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
    NB_INFO("export: MaxToNaiadMapping:\n" << mapping);

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
                           mapping,
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
        catch (const std::exception &ex) {
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
NaiadBuddy::ExportNode(INode                   *node, 
                       const int                frame, 
                       const bool               flipYZ, 
                       const bool               selectedOnly,
                       const MaxToNaiadMapping &mapping,
                       Nb::EmpWriter           &emp,
                       int                     &numExportedNodes,
                       int                     &numExportedMeshNodes,
                       int                     &numExportedParticleNodes,
                       int                     &numExportedCameraNodes)
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
                            ", Map Count: " << numMaps);

                    // Matrix3 is actually a 4x3 matrix. Order of 
                    // multiplication with vertex is irrelevant (?!).

                    const Matrix3 tm = 
                        node->GetObjTMAfterWSM(frame*GetTicksPerFrame());
                    //node->GetNodeTM(frame*GetTicksPerFrame());

                    try {
                        std::auto_ptr<Nb::Body> body(
                            Nb::Factory::createBody(
                                "Mesh", Nb::String(node->GetName())));

                        // These shapes and channels are guaranteed by the
                        // 'Mesh' signature.

                        Nb::PointShape &points = 
                            body->mutablePointShape();
                        Nb::TriangleShape &triangles = 
                            body->mutableTriangleShape();
                        Nb::Buffer3f &positions = 
                            points.mutableBuffer3f("position");
                        Nb::Buffer3i &indices = 
                            triangles.mutableBuffer3i("index");

                        // Export vertex positions. [required]

                        positions.resize(numVerts);
                        for (int v = 0; v < numVerts; ++v) {
                            const Point3 vtx = tm*tri->mesh.verts[v];
                            if (flipYZ) {
                                positions[v][0] = vtx[0];
                                positions[v][1] = vtx[2];
                                positions[v][2] = vtx[1];
                            }
                            else {
                                positions[v][0] = vtx[0];
                                positions[v][1] = vtx[1];
                                positions[v][2] = vtx[2];
                            }
                        }

                        // Export triangle faces. [required]

                        indices.resize(numFaces);
                        for (int f = 0; f < numFaces; ++f) {
                            indices[f][0] = tri->mesh.faces[f].v[0];
                            indices[f][1] = tri->mesh.faces[f].v[1];
                            indices[f][2] = tri->mesh.faces[f].v[2];
                        }

                        // Export map channels.

                        for (int m = 0; m < numMaps; ++m) {
                            if (tri->mesh.mapSupport(m)) {
                                if (!mapping.excluded(m)) {
                                    MaxToNaiadMapping::Entry entry = 
                                        mapping.find(m);
                                    const MaxChannel &mc = entry.maxChannel();
                                    NaiadChannel &nc = entry.naiadChannel();
                                    const bool blind = isBlind(nc);
                                    if (blind) {
                                        std::string name;
                                        getMapChannelName(node, m, name);
                                        if (name.empty()) {
                                            std::stringstream ss;
                                            ss << "map" << m;
                                            name = ss.str();
                                        }
                                        nc.setName(name);
                                    }

                                    NB_INFO("export: Max Channel: " << mc << 
                                            " --> Naiad Channel: " << nc << 
                                            (blind ? " (blind)" : ""));  

                                    ExportMapChannel(
                                        mc,
                                        nc,
                                        tri->mesh,
                                        points);
                                }
                                else {
                                    NB_INFO("export: Excluded Max Channel: " << 
                                            MaxChannel(m));
                                }
                            }
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
                   mapping,
                   emp,
                   numExportedNodes, numExportedMeshNodes,
                   numExportedParticleNodes, numExportedCameraNodes); 
    }
}



void
NaiadBuddy::ExportMapChannel(const MaxChannel   &mc, 
                             const NaiadChannel &nc, 
                             /* const */ Mesh   &mesh, 
                             Nb::PointShape     &point)
{
    if (mesh.mapSupport(mc.id()) && !nc.name().empty()) {
        MeshMap &meshMap = mesh.Map(mc.id());
        point.guaranteeChannel3f(nc.name());
        Nb::Buffer3f &b3f = point.mutableBuffer3f(nc.name());
        b3f.resize(meshMap.getNumVerts());
        const int numFaces = meshMap.getNumFaces();
        for (int f = 0; f < numFaces; ++f) { // Map faces.
            TVFace &face = meshMap.tf[f];   
            for (int i = 0; i < 3; ++i) { // Map face-vertices.
                UVVert    &uvw = meshMap.tv[face.t[i]];
                em::vec3f &pnt = b3f[face.t[i]];
                for (int j = 0; j < 3; ++j) { // Components.
                    pnt[nc.component(j)] = uvw[mc.component(j)]; // Swizzle!
                }
            }
        }
    }
    else {
        NB_WARNING("export: Failed exporting: Max Channel: " << mc 
                   << " --> Naiad Channel: " << nc);
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
