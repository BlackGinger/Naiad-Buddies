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
#include <limits>

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

    //Used for Bgeo-Read
    Bgeo(const char* filename);
    //Used for Bgeo-Write
    Bgeo(const char* filename,uint32_t * headerParameters);
    ~Bgeo();
    void addPointAttribute(const int index,const char* name,const  uint16_t size,const  uint16_t type, const char * def);
    void addPrimAttribute(const int index,const char* name, const uint16_t size, const uint16_t type, const char * def);
    attribute *  addVtxAttribute(const int index,const char* name,const  uint16_t size,const  uint16_t type, const char * def);
    template<class T> void copyBufLocal(const char* src, T*dst,const int count);
    void freePointsBuffer() { delete[] mPointsBuf;};
    void freePrimsBuffer() { delete[] mPrimsBuf;};
    int getBytesPerPrimLine(){return 5 + (mIdxBytes + mVtxAtrBytes) * 3 + mPrimAtrBytes;};
    int getIdxBytes(){return mIdxBytes;};
    uint32_t * getIndices3v();
    uint32_t getNumberOfPoints(){ return mNumPoints;};
    uint32_t getNumberOfPointAtr(){ return mNumPointAtr;};
    uint32_t getNumberOfPrims(){ return mNumPrims;};
    uint32_t getNumberOfPrimAtr(){ return mNumPrimAtr;};
    uint32_t getNumberOfVtxAtr(){ return mNumVtxAtr;};
    attribute* getPointAtr(){return mPointAtr;};
    template<class T> T * getPointAtrArr(const int size, const int offset);
    float * getPoints3v();
    attribute* getPrimAtr(){return mPrimAtr;};
    template<class T> T * getPrimAtrArr(const int size, const int offset);
    int getPrimAtrBytes(){return mPrimAtrBytes;};
    attribute* getVtxAtr(){return mVtxAtr;};
    template<class T> T * getVtxAtrArr(const int size, const int offset);
    int getVtxAtrBytes(){return mVtxAtrBytes;};
    void readPoints();
    void readPrims(const bool integrityCheck);
    int type2Bytes(uint16_t type) {return mTypeBytes.at(type);};
    void writeDetailAtrMesh();
    void writeDetailAtrParticle(bool fromHoudini);
    void writeOtherInfo(){char begEnd[2] = {0,255}; mFile.write(begEnd,2);};
    void writePoints(char * * pointsData );
    void writePointAtr(){for (int i = 0; i < mNumPointAtr; ++i) writeAttribute(mPointAtr[i]);};
    void writePrimAtr(){for  (int i = 0; i < mNumPrimAtr;  ++i) writeAttribute(mPrimAtr[i]);};
    void writePrimsMesh(char * * vtxData, attribute * * vtxAtr, const int vtxDataChannels, char * * primData);
    void writePrimsParticle();
    void writeVtxAtr(){for  (int i = 0; i < mNumVtxAtr;  ++i) writeAttribute(mVtxAtr[i]);};

private:
    bool 			mPointsRead,mPrimsRead;
    vector<int>		mTypeBytes;
    vector<string> 	mTypeStr;
    fstream 		mFile;
    uint32_t    	mVerNr,
					mNumPoints,
					mNumPrims,
					mNumPointGrps,
					mNumPrimGrps,
					mNumPointAtr,
					mNumVtxAtr,
					mNumPrimAtr,
					mNumAtr,
					mIdxBytes,
					mPointAtrBytes,
					mVtxAtrBytes,
					mPrimAtrBytes;
    char        	* mPointsBuf,
					* mPrimsBuf;
    attribute   	* mPointAtr,
					* mVtxAtr,
					* mPrimAtr;

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
   for(unsigned int k=0; k<sizeof(T); ++k)
      ((char*)&x)[k] = ((char*)&old)[sizeof(T)-1-k];
}


template<class T> void Bgeo::readNumber(char* &buf, T & r)
{
    r = *(T*) buf;
    swap_endianity(r); //Data is stored big endian regardless of the native bit ordering of the platform.
    buf += sizeof(T);
}

//Takes a chunk of memory from src and element by element do endian swap and put the output to the dst
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
	//Given a big buffer of data, this function will create a new array of specific attribute data (and also swapped)

	//Offset, where to start at each "line of data" (line as in line of the ascii format GEO used for reference).
    char * buffer = src+offset;
    T* dst = new T[n*size], *dst_start = dst;
    for (unsigned int i = 0; i < n ; ++i){
        copyBufLocal(buffer,dst,size);
        dst +=size;
        //If we jump bytesPerLine, we will get to the next data cluster.
        buffer += bytesPerLine;
    }
    return dst_start;
}

template<class T> T* Bgeo::copyBuffer32(char * src,const int n, const int size, const int bytesPerLine, const int offset)
{
	//Will read data as 16 bit int and create a 32bit int dst for it.
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
    T* dst = new T[mNumPrims*vertsPerPoly*size], *dst_start = dst;
    for (unsigned int i = 0; i < mNumPrims ; ++i){
    	//loop through all vertices
        for(int j = 0; j < vertsPerPoly; ++j){
            copyBufLocal(src,dst,size);
            dst +=size;
            //spacing between the same attribue but on different vertices.
            src += mIdxBytes + mVtxAtrBytes;
        }
        //Jump to the next primitive
        src += 5 + mPrimAtrBytes;// 5 since uint32 for number of verts and char for closed/unclosed.
    }
    return dst_start;
}

template<class T> T* Bgeo::copyBufferVertex16(char * src, const int size)
{
	//Will read data as 32 bit int and create a 16bit int dst for it.
    const int vertsPerPoly = 3;
    T* dst = new T[mNumPrims*vertsPerPoly*size], *dst_start = dst;
    for (unsigned int i = 0; i < mNumPrims ; ++i){
        for(int j = 0; j < vertsPerPoly; ++j){
            copyBufLocaluInt16(src,dst,size);
            dst +=size;
            src += mIdxBytes + mVtxAtrBytes;
        }
        src += 5 + mPrimAtrBytes;// 5 since uint32 for number of verts and char for closed/unclosed.
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

template<class T> T * Bgeo::getPointAtrArr(const int size, const int offset)
{
	return copyBuffer<T>(mPointsBuf+offset,mNumPoints,size,sizeof(float) * 4 + mPointAtrBytes);
}

template<class T> T * Bgeo::getPrimAtrArr(const int size, const int offset)
{
	return copyBuffer<T>(mPrimsBuf+offset,mNumPrims,size,getBytesPerPrimLine());
}

template<class T> T * Bgeo::getVtxAtrArr(const int size, const int offset)
{
	return copyBufferVertex<T>(mPrimsBuf+offset,size);
}



#endif // BGEO_H
