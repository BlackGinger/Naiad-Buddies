#include "Bgeo.h"

Bgeo::Bgeo(const char* filename) : pointAtrBytes(0), vtxAtrBytes(0) ,primAtrBytes(0), pointsRead(false), primsRead(false)
{
    file.open(filename, ios::in|ios::binary);

    //Read One char "V" and 10 32-bit uints
    const int bufferSize = sizeof(char) + 10*sizeof(uint32_t);
    char * buffer = new char[bufferSize];
    char * p = buffer;
    file.read(buffer,bufferSize);

    //skips the unnecessary magic number & char "V"
    buffer +=  sizeof(uint32_t) + sizeof(char);

    readNumber(buffer,vNr);
    readNumber(buffer,nPoints);
    readNumber(buffer,nPrims);
    readNumber(buffer,nPointGrps);
    readNumber(buffer,nPrimGrps);
    readNumber(buffer,nPointAtr);
    readNumber(buffer,nVtxAtr);
    readNumber(buffer,nPrimAtr);
    readNumber(buffer,nAtr);

    delete[] p;

    cout    << "Version number: "   << vNr << endl
            << "NPoints: "          << nPoints << endl
            << "NPrims: "           << nPrims << endl
            << "NPointGroups: "     << nPointGrps << endl
            << "NPrimGroups: "      << nPrimGrps << endl
            << "NPointAttrib: "     << nPointAtr << endl
            << "NVertexAttrib: "    << nVtxAtr << endl
            << "NPrimAttrib: "      << nPrimAtr << endl
            << "NAttrib: "          << nAtr<< endl;

    //If the number of points is > 65535, uint32 is used
    idxBytes = 2;
    if (nPoints > 65535)
        idxBytes = 4;

    int tmpInt[6] = {4,4,32,1,4,4};
    string tmpStr[6] = {"float","int","string","mixed","index","vector"};

    typeBytes = vector<int>(tmpInt,tmpInt+6);
    typeStr = vector<string>(tmpStr,tmpStr+6);
}

Bgeo::~Bgeo()
{
    file.close();

    if (pointsRead){
    	delete[] pointsBuf;
        //Delete parameter info (stored in buffers)
        for (int i = 0; i < nPointAtr; ++i )
            delete[] pointAtr[i].defBuf;
        delete[] pointAtr;
    }

    if (primsRead){
        delete[] primsBuf;

        for (int i = 0; i < nVtxAtr; ++i )
            delete[] vtxArt[i].defBuf;
        delete[] vtxArt;

        for (int i = 0; i < nPrimAtr; ++i )
            delete[] primArt[i].defBuf;
        delete[] primArt;
    }


}

void Bgeo::readPoints()
{
    pointAtr = new attribute[nPointAtr];
    cout << endl << "PointAttrib" ;
    for (int i = 0; i < nPointAtr; ++i )
    {
        pointAtr[i] = readAttributeInfo();
        pointAtrBytes += typeBytes[pointAtr[i].type] * pointAtr[i].size;
    }

    //Each point has 4 float values (x,y,z,w) and parameterinfo
    int bufferSize = (sizeof(float) * 4 + pointAtrBytes) * nPoints;
    pointsBuf = new char[bufferSize];
    file.read(pointsBuf,bufferSize);

    /*   for debugging purposes.
    cout << endl << "All Points read!" << endl;
    float *pointPos = copyBuffer<float>(pointsBuf,nPoints,3,sizeof(float) * 4 + pointAtrBytes);
    printBuffer(pointPos,nPoints,3);

    float *uvtest = copyBuffer<float>(pointsBuf,nPoints,2,sizeof(float) * 4 + pointAtrBytes,16);
    printBuffer(uvtest,nPoints,2);

    float *attribute = copyBuffer<float>(pointsBuf,nPoints,3,sizeof(float) * 4 + pointAtrBytes,24);
    printBuffer(attribute,nPoints,3);

    delete[] pointPos;
    delete[] uvtest;
    delete[] attribute;*/

    pointsRead = true;

}

Bgeo::attribute Bgeo::readAttributeInfo()
{
    //Dont bother handle strings etc

    attribute atr;

    //Name
    //Read length of string(max of 32k chars, uint32 not needed)
    int bufferSize = sizeof(uint16_t);
    char * buffer = new char[bufferSize], *buffer_start = buffer;
    file.read(buffer,bufferSize);
    uint16_t strLen;
    readNumber(buffer,strLen);
    delete[] buffer_start;

    char * name_arr = new char[strLen];
    file.read(name_arr,strLen);
    atr.name = string(name_arr,name_arr + strLen);

    //Size && Type
    bufferSize = sizeof(uint16_t) + sizeof(uint32_t);
    buffer = new char[bufferSize], buffer_start = buffer;
    file.read(buffer,bufferSize);
    readNumber(buffer,atr.size);

    uint32_t Type;
    readNumber(buffer,Type);
    atr.type = Type & 0xffff;

    //dont know what index do (todo)
    uint16_t typeinfo = Type >> 16;
    delete[] buffer_start;


    //Todo, error if type = 3 4 5

    cout << endl << "Parameter--> Name: " << atr.name << " Size: " << atr.size<< " Type: " << typeStr[atr.type] ;

    //Defaults - store them in a buffer (size and type decides how to output them)
    bufferSize = typeBytes[atr.type] * atr.size;
    atr.defBuf = new char[bufferSize];
    file.read(atr.defBuf,bufferSize);
    return atr;
}

void Bgeo::readPrims()
{
    //Read vertex attributes
	vtxArt = new attribute[nVtxAtr];
    cout << endl <<"VertAttr" ;
    for (int i = 0; i < nVtxAtr; ++i )
    {
        vtxArt[i] = readAttributeInfo();
        vtxAtrBytes += typeBytes.at(vtxArt[i].type) * vtxArt[i].size;
    }

    //Read primitives attributes
    primArt = new attribute[nPrimAtr];
    cout << endl << "PrimAttrib";
    for (int i = 0; i < nPrimAtr; ++i )
    {
        primArt[i] = readAttributeInfo();
        primAtrBytes += typeBytes.at(primArt[i].type) * primArt[i].size;
    }

    //Control that the bgeo is a polygon model
    int bufferSize = sizeof(uint32_t) * 2 + sizeof(uint16_t) ;
    char * buffer = new char[bufferSize], *buffer_start = buffer;
    file.read(buffer,bufferSize);
    uint32_t run;
    readNumber(buffer,run);
    // 4294967295 = 0xFFFFFFFF
    if (run != 4294967295)
        cout << "FAIL";

    uint16_t nPrimsLocal;
    readNumber(buffer,nPrimsLocal);

    if (nPrimsLocal != nPrims)
        cout << "FAIL";

    uint32_t PrimKey;
    readNumber(buffer,PrimKey);
    if (PrimKey != 1)
        cout << "No polygon data, bye!";

    delete[] buffer_start;
    cout << endl << nPrims << " primitives as polygons:" << endl;

    //Restricted to triangles only
    const int verticesPerPolygon = 3;
    bufferSize = nPrims * (sizeof(uint32_t) + sizeof(char) + verticesPerPolygon * (idxBytes + vtxAtrBytes)  + primAtrBytes);
    primsBuf = new char[bufferSize];
    file.read(primsBuf,bufferSize);

    //Copy
    cout << "idxBytes: " << idxBytes << " vtxAtrBytes: " << vtxAtrBytes << " primAtrBytes: " << primAtrBytes << endl;
    uint32_t * indicesBuffer;


    /* For debugging purposes
    if (idxBytes == 2)
        indicesBuffer = copyBufferVertex16<uint32_t>(primsBuf+5,1);
    else
        indicesBuffer = copyBufferVertex<uint32_t>(primsBuf+5,1);

    cout << endl << "Primitives read!" << endl;
    printBuffer(indicesBuffer,nPrims,3);

    float * uv = copyBufferVertex<float>(primsBuf+5+idxBytes,3);
    printBuffer(uv,nPrims,9);

    int bytesPerLine = 5 + verticesPerPolygon * (idxBytes + vtxAtrBytes) + primAtrBytes;
    float * testPrimPara = copyBuffer<float>(primsBuf,nPrims,4, bytesPerLine,bytesPerLine - primAtrBytes);
    printBuffer(testPrimPara,nPrims,4);

    delete[] testPrimPara;
    delete[] uv;
    delete[] indicesBuffer;
*/
    primsRead = true;

}


void Bgeo::copyBufLocaluInt16(const char* src, uint32_t * dst,const int count)
{
    const int   sizeoft = sizeof(uint16_t);
    const char* src_end = src+count*sizeoft;
    int         counter = 0;
#pragma omp parallel for
    for(const char* p = src; p<src_end; p += sizeoft){
        uint16_t tmp =  *(uint16_t*)p ;
        swap_endianity(tmp);
        dst[counter] = tmp;
        ++counter;
    }
}

float * Bgeo::getPoints3v()
{
	return copyBuffer<float>(pointsBuf,nPoints,3,sizeof(float) * 4 + pointAtrBytes);
}

uint32_t * Bgeo::getIndices3v()
{
    if (idxBytes == 2)
        return copyBufferVertex16<uint32_t>(primsBuf+5,1);
    else
        return copyBufferVertex<uint32_t>(primsBuf+5,1);
}

