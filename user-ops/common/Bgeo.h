#ifndef BGEO_H
#define BGEO_H

#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <math.h>
#include <vector>

//For errors
#include <Nbx.h>

using namespace std;

class Bgeo
{
public:
    struct attribute
    {
        string name;
        uint16_t    size,
                    type;
        char* defBuf;
    };
    Bgeo(const char* filename);
    Bgeo(const char* filename,uint32_t * headerParameters);
    ~Bgeo();



    void addPointAttribute(const int index,const char* name,const  uint16_t size,const  uint16_t type, const char * def)
    {
    	addAttribute(&pointAtr[index], name, size, type, def);
    	pointAtrBytes += typeBytes[pointAtr[index].type] * pointAtr[index].size;
    };

    attribute *  addVtxAttribute(const int index,const char* name,const  uint16_t size,const  uint16_t type, const char * def)
    {
    	addAttribute(&vtxArt[index], name, size, type, def);
    	vtxAtrBytes += typeBytes[vtxArt[index].type] * vtxArt[index].size;
    	return &vtxArt[index];
    };

    void addPrimAttribute(const int index,const char* name, const uint16_t size, const uint16_t type, const char * def)
    {
    	addAttribute(&primArt[index], name, size, type, def);
    	primAtrBytes += typeBytes[primArt[index].type] * primArt[index].size;
    };

    void writePointAtr(){for (int i = 0; i < nPointAtr; ++i) writeAttribute(pointAtr[i]);};
    void writeVtxAtr(){for  (int i = 0; i < nVtxAtr;  ++i) writeAttribute(vtxArt[i]);};
    void writePrimAtr(){for  (int i = 0; i < nPrimAtr;  ++i) writeAttribute(primArt[i]);};

    template<class T> void copyBufLocal(const char* src, T*dst,const int count);

    void readPoints();
    void writePoints(char * * pointsData );
    void freePointsBuffer() { delete[] pointsBuf;};
    float * getPoints3v();
    uint32_t getNumberOfPoints(){ return nPoints;};
    uint32_t getNumberOfPointAtr(){ return nPointAtr;};
    attribute* getPointAtr(){return pointAtr;};
    template<class T> T * getPointAtrArr(const int size, const int offset);

    void readPrims(const bool integrityCheck);
    void writePrims(char * * vtxData, attribute * * vtxAtr, const int vtxDataChannels, char * * primData);
    void freePrimsBuffer() { delete[] primsBuf;};
    uint32_t * getIndices3v();
    uint32_t getNumberOfPrims(){ return nPrims;};
    uint32_t getNumberOfPrimAtr(){ return nPrimAtr;};
    attribute* getPrimAtr(){return primArt;};
    uint32_t getNumberOfVtxAtr(){ return nVtxAtr;};
    attribute* getVtxAtr(){return vtxArt;};
    template<class T> T * getPrimAtrArr(const int size, const int offset);
    template<class T> T * getVtxAtrArr(const int size, const int offset);

    void writeOtherInfo(){char begEnd[6] = {0,0,0,0,0,255}; file.write(begEnd,6);};
    void writeDetailAtr();

    int getBytesPerPrimLine(){return 5 + (idxBytes + vtxAtrBytes) * 3 + primAtrBytes;};
    int getIdxBytes(){return idxBytes;};
    int getVtxAtrBytes(){return vtxAtrBytes;};
    int getPrimAtrBytes(){return primAtrBytes;};

    template<class T> void printArray(T* buf, const int n, const int size);


    int type2Bytes(uint16_t type) {return typeBytes.at(type);};


private:
    bool 		pointsRead,primsRead;

    vector<int> typeBytes;
    vector<string> typeStr;

    fstream 	file;
    uint32_t    vNr,
                nPoints,
                nPrims,
                nPointGrps,
                nPrimGrps,
                nPointAtr,
                nVtxAtr,
                nPrimAtr,
                nAtr,
                idxBytes,
                pointAtrBytes,
                vtxAtrBytes,
                primAtrBytes;

    char        * pointsBuf,
                * primsBuf;

    attribute   * pointAtr,
                * vtxArt,
                * primArt;

    void setConstants();
    attribute readAttributeInfo();
    void addAttribute(attribute* atr, const char * name, const uint16_t size, const uint16_t type, const char * def);
    void writeAttribute(const attribute &atr);
    void swpAtrData(const attribute & atr,char  * &dst, char *  src, const int n);

    template<class T> void swap_endianity(T &x);
    template<class T> void readNumber(char* &buf, T & r);
    void copyBufLocaluInt16(const char* src, uint32_t * dst,const int count);
    void copyBufLocaluint32(const char* src, uint16_t * dst,const int count);
    std::string removePrefix(std::string s);

    template<class T> T* copyBuffer(char * src,const int n, const int size, const int bytesPerLine, const int offset = 0);
    template<class T> T* copyBuffer32(char * src,const int n, const int size, const int bytesPerLine, const int offset = 0);
    template<class T> T* copyBufferVertex(char * src, const int size);
    template<class T> T* copyBufferVertex16(char * src, const int size);
    template<class T> void printBuffer(T* buf, const int n, const int size);

};

template<class T> inline void Bgeo::swap_endianity(T &x)
{
   assert(sizeof(T)<=8); // should not be called on composite types: instead specialize swap_endianity if needed.
   T old=x;
   //cout << x << endl;
   for(unsigned int k=0; k<sizeof(T); ++k)
      ((char*)&x)[k] = ((char*)&old)[sizeof(T)-1-k];
}


template<class T> void Bgeo::readNumber(char* &buf, T & r)
{
    r = *(T*) buf;
    swap_endianity(r); //Data is stored big endian regardless of the native bit ordering of the platform.
    buf += sizeof(T);
}

template<class T> void Bgeo::copyBufLocal(const char* src, T*dst,const int count)
{
    const int   sizeoft = sizeof(T);
    const char* src_end = src+count*sizeoft;
    int         counter = 0;
#pragma omp parallel for
    for(const char* p = src; p < src_end; p += sizeoft){
        dst[counter] =  *(T*)p ;
        swap_endianity(dst[counter]);
        ++counter;
    }
}

template<class T> T* Bgeo::copyBuffer(char * src,const int n, const int size, const int bytesPerLine, const int offset)
{
    char * buffer = src+offset;
    T* dst = new T[n*size], *dst_start = dst;
    for (unsigned int i = 0; i < n ; ++i){
        copyBufLocal(buffer,dst,size);
        dst +=size;
        buffer += bytesPerLine;
    }
    return dst_start;
}

template<class T> T* Bgeo::copyBuffer32(char * src,const int n, const int size, const int bytesPerLine, const int offset)
{
    char * buffer = src+offset;
    T* dst = new T[n*size], *dst_start = dst;
    for (unsigned int i = 0; i < n ; ++i){
    	copyBufLocaluint32(buffer,dst,size);
        dst +=size;
        buffer += bytesPerLine;
    }
    return dst_start;
}


template<class T> T* Bgeo::copyBufferVertex(char * src, const int size)
{
    const int vertsPerPoly = 3;
    T* dst = new T[nPrims*vertsPerPoly*size], *dst_start = dst;
    for (unsigned int i = 0; i < nPrims ; ++i){
        for(int j = 0; j < vertsPerPoly; ++j){
            copyBufLocal(src,dst,size);
            dst +=size;
            src += idxBytes + vtxAtrBytes;
        }
        src += 5 + primAtrBytes;// 5 since uint32 for number of verts and char for closed/unclosed.
    }
    return dst_start;
}

template<class T> T* Bgeo::copyBufferVertex16(char * src, const int size)
{
    const int vertsPerPoly = 3;
    T* dst = new T[nPrims*vertsPerPoly*size], *dst_start = dst;
    for (unsigned int i = 0; i < nPrims ; ++i){
        for(int j = 0; j < vertsPerPoly; ++j){
            copyBufLocaluInt16(src,dst,size);
            dst +=size;
            src += idxBytes + vtxAtrBytes;
        }
        src += 5 + primAtrBytes;// 5 since uint32 for number of verts and char for closed/unclosed.
    }
    return dst_start;
}

template<class T> void Bgeo::printBuffer(T* buf, const int n, const int size)
{
    for (int i=0;i<n*size;i+=size){
        for(int j=0;j<size;++j)
            cout << buf[i+j] << " ";
        cout << endl;
     }
}

template<class T> void Bgeo::printArray(T* buf, const int n, const int size)
{
    for (int i=0;i<n*size;i+=size){
        for(int j=0;j<size;++j)
            cout << buf[i+j] << " ";
        cout << endl;
     }
}

template<class T> T * Bgeo::getPointAtrArr(const int size, const int offset)
{
	return copyBuffer<T>(pointsBuf+offset,nPoints,size,sizeof(float) * 4 + pointAtrBytes);
}

template<class T> T * Bgeo::getPrimAtrArr(const int size, const int offset)
{
	return copyBuffer<T>(primsBuf+offset,nPrims,size,getBytesPerPrimLine());
}

template<class T> T * Bgeo::getVtxAtrArr(const int size, const int offset)
{
	return copyBufferVertex<T>(primsBuf+offset,size);
}



#endif // BGEO_H
