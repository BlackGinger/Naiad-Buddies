#include "Bgeo.h"

Bgeo::Bgeo(const char* filename) :
				  mPointAtrBytes(0)
				, mVtxAtrBytes(0)
				, mPrimAtrBytes(0)
				, mPointsRead(false)
				, mPrimsRead(false)
				, mFile(filename, ios::in|ios::binary)
{
    if(!mFile.good())
        NB_THROW("Cannot read bgeo: '" << filename << "'");

    //Read One char "V" and 10 32-bit uints
    const int bufferSize = sizeof(char) + 10*sizeof(uint32_t);
    char * buffer = new char[bufferSize];
    char * p = buffer;
    mFile.read(buffer,bufferSize);

    //skips the unnecessary magic number & char "V"
    buffer +=  sizeof(uint32_t) + sizeof(char);

    readNumber(buffer,mVerNr);
    readNumber(buffer,mNumPoints);
    readNumber(buffer,mNumPrims);
    readNumber(buffer,mNumPointGrps);
    readNumber(buffer,mNumPrimGrps);
    readNumber(buffer,mNumPointAtr);
    readNumber(buffer,mNumVtxAtr);
    readNumber(buffer,mNumPrimAtr);
    readNumber(buffer,mNumAtr);

    //Enough info to set some constants
    setConstants();
    //delete buffer
    delete[] p;

    /*cout    << "Version number: "   << mVerNr << endl
            << "NPoints: "          << mNumPoints << endl
            << "NPrims: "           << mNumPrims << endl
            << "NPointGroups: "     << mNumPointGrps << endl
            << "NPrimGroups: "      << mNumPrimGrps << endl
            << "NPointAttrib: "     << mNumPointAtr << endl
            << "NVertexAttrib: "    << mNumVtxAtr << endl
            << "NPrimAttrib: "      << mNumPrimAtr << endl
            << "NAttrib: "          << mNumAtr<< endl;*/


}

Bgeo::Bgeo(const char* filename,uint32_t * headerParameters) :
				  mPointAtrBytes(0)
				, mVtxAtrBytes(0)
				, mPrimAtrBytes(0)
				, mPointsRead(false)
				, mPrimsRead(false)
				, mVerNr(headerParameters[0])
				, mNumPoints(headerParameters[1])
				, mNumPrims(headerParameters[2])
				, mNumPointGrps(headerParameters[3])
				, mNumPrimGrps(headerParameters[4])
				, mNumPointAtr(headerParameters[5])
				, mNumVtxAtr(headerParameters[6])
				, mNumPrimAtr(headerParameters[7])
				, mNumAtr(headerParameters[8])
				, mFile(filename, ios::out|ios::binary)
{
    if(!mFile.good())
        NB_THROW("Cannot create bgeo: '" << filename << "'");

    //Set constants right away
	setConstants();

	//Magic number and V
	char headrStr[] = "BgeoV";
	mFile.write(headrStr,5);

	//Write header parameters to file
	for (int i = 0; i < 9; ++i)
		swap_endianity(headerParameters[i]);
	mFile.write( (char*) headerParameters, sizeof(uint32_t)*9);

	//These are allocated here since we won't run any readPoints/readPrims now.
	mPointAtr = new attribute[mNumPointAtr];
	mVtxAtr = new attribute[mNumVtxAtr];
	mPrimAtr = new attribute[mNumPrimAtr];

}

Bgeo::~Bgeo()
{
	//Close the file (can be both in & out)
    mFile.close();

    //If Points data have been loaded, delete
    if (mPointsRead){
    	delete[] mPointsBuf;
        //Delete parameter info (stored in buffers)
        for (int i = 0; i < mNumPointAtr; ++i )
            delete[] mPointAtr[i].defBuf;
        delete[] mPointAtr;
    }

    //If Primitive data have been loaded, delete
    if (mPrimsRead){
        delete[] mPrimsBuf;

        for (int i = 0; i < mNumVtxAtr; ++i )
            delete[] mVtxAtr[i].defBuf;
        delete[] mVtxAtr;

        for (int i = 0; i < mNumPrimAtr; ++i )
            delete[] mPrimAtr[i].defBuf;
        delete[] mPrimAtr;
    }


}

void Bgeo::addPointAttribute(const int index,const char* name,const  uint16_t size,const  uint16_t type, const char * def)
{
	addAttribute(&mPointAtr[index], name, size, type, def);
	//Keep track of how many bytes the point attributes take in total (needed for offset later).
	mPointAtrBytes += mTypeBytes[mPointAtr[index].type] * mPointAtr[index].size;
}

void Bgeo::addPrimAttribute(const int index,const char* name, const uint16_t size, const uint16_t type, const char * def)
{
	addAttribute(&mPrimAtr[index], name, size, type, def);
	//Keep track of how many bytes the Primitive attributes take in total (needed for offset later).
	mPrimAtrBytes += mTypeBytes[mPrimAtr[index].type] * mPrimAtr[index].size;
}

Bgeo::attribute * Bgeo::addVtxAttribute(const int index,const char* name,const  uint16_t size,const  uint16_t type, const char * def)
{
	addAttribute(&mVtxAtr[index], name, size, type, def);
	//Keep track of how many bytes the Vertex attributes take in total (needed for offset later).
	mVtxAtrBytes += mTypeBytes[mVtxAtr[index].type] * mVtxAtr[index].size;
	return &mVtxAtr[index];
}

void Bgeo::setConstants()
{
    //If the number of points is > 65535, uint32 is used
    mIdxBytes = sizeof(uint16_t);
    if (mNumPoints > numeric_limits<uint16_t>::max())
        mIdxBytes = sizeof(uint32_t);

    //From Bgeo definitions. A vector (type = 5) is of 4 bytes since the attribute size will still be 3 (3 * 4 = 12 in total)
    int tmpInt[6] = {4,4,32,1,4,4};
    string tmpStr[6] = {"float","int","string","mixed","index","vector"};
    mTypeBytes = vector<int>(tmpInt,tmpInt+6);
    mTypeStr = vector<string>(tmpStr,tmpStr+6);
}

void Bgeo::readPoints()
{
	//Allocate memory for attribute parameters and read them
    mPointAtr = new attribute[mNumPointAtr];
    for (int i = 0; i < mNumPointAtr; ++i )
    {
        mPointAtr[i] = readAttributeInfo();
        mPointAtrBytes += mTypeBytes[mPointAtr[i].type] * mPointAtr[i].size;
    }

    //Each point has 4 float values (x,y,z,w) and parameter info
    int bufferSize = (sizeof(float) * 4 + mPointAtrBytes) * mNumPoints;
    mPointsBuf = new char[bufferSize];

    //Store the data in the Point buffer
    mFile.read(mPointsBuf,bufferSize);
    mPointsRead = true;
}

void Bgeo::writeDetailAtrParticle(bool fromHoudini)
{
	//copied from binary mFile
	uint8_t detailAtrInfo[] = { 0, 6, 110, 101, 120, 116, 105, 100, 0, 1, 0, 0, 0,
			1, 0, 0, 0, 1, 0 , 5, 101, 118, 101, 110, 116, 0, 1, 0, 0, 0, 4, 0, 0, 0, 0};

	//no need to swap, 8bit.
	mFile.write((char*) detailAtrInfo, 35);

	if (mNumAtr > 2){
		char * buffer = new char[14], *buffer_start = buffer;

		uint16_t varmapSize = 6;
		swap_endianity(varmapSize);
		memcpy(buffer,(char*)&varmapSize,2);
		buffer += 2;

		char varmap[] = "varmap";
		memcpy(buffer,varmap,6);
		buffer += 6;

		uint16_t sizeAndType[] = {1,0,4};
		swap_endianity(sizeAndType[0]);
		swap_endianity(sizeAndType[2]);
		memcpy(buffer,sizeAndType,6);
		mFile.write(buffer_start,14);
		delete[] buffer_start;

		std::vector<string> strVec;
		string tmpStr,tmpStrUpper;
		int startAtr=0;
		if (fromHoudini)
			startAtr = 6;
		for (int i = startAtr; i < mNumPointAtr; ++i){
			tmpStr = removePrefix(mPointAtr[i].name);
			tmpStrUpper = tmpStr;
			transform(tmpStrUpper.begin(), tmpStrUpper.end(),tmpStrUpper.begin(), ::toupper);
			strVec.push_back(tmpStr + string(" -> ")  + tmpStrUpper);
		}

		int size = 0;
		uint32_t nStrings = strVec.size();
		for (int i = 0; i < nStrings; ++i)
			size += strVec.at(i).length() + 2;

		buffer = new char[size + 4];
		buffer_start = buffer;
		swap_endianity(nStrings);
		memcpy(buffer,(char*)& nStrings,4);
		buffer+= 4;
		for (int i = 0; i < strVec.size(); ++i){
			uint16_t tmpSize = strVec.at(i).length();
			uint16_t tmpSizeSwpd = tmpSize;
			swap_endianity(tmpSizeSwpd);
			memcpy(buffer,(char*)& tmpSizeSwpd,2);
			buffer+= 2;
			memcpy(buffer,(char*) (strVec.at(i).c_str()),tmpSize);
			buffer += tmpSize;
		}
		mFile.write(buffer_start,size+4);
		delete[] buffer_start;

		uint32_t end[] = {mNumPoints+1, 4294967295, 0};
		swap_endianity(end[0]);
		swap_endianity(end[1]);
		mFile.write((char*)end,12);
	}
	else
	{
		uint32_t end[] = {mNumPoints+1, 0};
		swap_endianity(end[0]);
		mFile.write((char*)end,8);
	}
}

//only write varmap
void Bgeo::writeDetailAtrMesh()
{
	char * buffer = new char[14], *buffer_start = buffer;

	uint16_t varmapSize = 6;
	swap_endianity(varmapSize);
	memcpy(buffer,(char*)&varmapSize,2);
	buffer += 2;

	char varmap[] = "varmap";
	memcpy(buffer,varmap,6);
	buffer += 6;

	uint16_t sizeAndType[] = {1,0,4};
	swap_endianity(sizeAndType[0]);
	swap_endianity(sizeAndType[2]);
	memcpy(buffer,sizeAndType,6);
	mFile.write(buffer_start,14);
	delete[] buffer_start;

	std::vector<string> strVec;
	string tmpStr,tmpStrUpper;
	for (int i = 0; i < mNumPointAtr; ++i){
		tmpStr = removePrefix(mPointAtr[i].name);
		tmpStrUpper = tmpStr;
		transform(tmpStrUpper.begin(), tmpStrUpper.end(),tmpStrUpper.begin(), ::toupper);
		strVec.push_back(tmpStr + string(" -> ")  + tmpStrUpper);
	}
	for (int i = 0; i < mNumPrimAtr; ++i){
		tmpStr = removePrefix(mPrimAtr[i].name);
		tmpStrUpper = tmpStr;
		transform(tmpStrUpper.begin(), tmpStrUpper.end(),tmpStrUpper.begin(), ::toupper);
		strVec.push_back(tmpStr + string(" -> ")  + tmpStrUpper);
	}
	for (int i = 0; i < mNumVtxAtr; ++i){
		tmpStr = removePrefix(mVtxAtr[i].name);
		tmpStrUpper = tmpStr;
		transform(tmpStrUpper.begin(), tmpStrUpper.end(),tmpStrUpper.begin(), ::toupper);
		strVec.push_back(tmpStr + string(" -> ")  + tmpStrUpper);
	}

	int size = 0;
	uint32_t nStrings = strVec.size();
	for (int i = 0; i < nStrings; ++i)
		size += strVec.at(i).length() + 2;

	buffer = new char[size + 8];
	buffer_start = buffer;
	swap_endianity(nStrings);
	memcpy(buffer,(char*)& nStrings,4);
	buffer+= 4;
	for (int i = 0; i < strVec.size(); ++i){
		uint16_t tmpSize = strVec.at(i).length();
		uint16_t tmpSizeSwpd = tmpSize;
		swap_endianity(tmpSizeSwpd);
		memcpy(buffer,(char*)& tmpSizeSwpd,2);
		buffer+= 2;
		memcpy(buffer,(char*) (strVec.at(i).c_str()),tmpSize);
		buffer += tmpSize;
	}
	uint32_t end(0);
	memcpy(buffer,(char*)& end,4);
	mFile.write(buffer_start,size+8);
	delete[] buffer_start;
}
void Bgeo::writePrimsParticle()
{
	//Write prim attribute for generator source
	char * buffer = new char[38], *buffer_start = buffer;
	uint16_t generatorSize = 9;
	swap_endianity(generatorSize);
	memcpy(buffer,(char*)&generatorSize,2);
	buffer += 2;

	char generator[] = "generator";
	memcpy(buffer,generator,9);
	buffer += 9;

	uint16_t parainfo[] = {1,0,4,0,1,7};
	for (int i = 0; i < 6; ++i)
		swap_endianity(parainfo[i]);
	memcpy(buffer,(char*)parainfo,12);
	buffer += 12;

	char source1[] = "source1";
	memcpy(buffer,source1,7);
	buffer += 7;

	uint32_t primKey = 32768; // 0x00008000
	uint32_t nParticles = mNumPoints;
	swap_endianity(primKey);
	swap_endianity(nParticles);
	memcpy(buffer,(char*)&primKey,4);
	buffer += 4;
	memcpy(buffer,(char*)&nParticles,4);

	mFile.write(buffer_start,38);
	delete[] buffer_start;

	if (mIdxBytes == 4){
		int bufferSize = mNumPoints*4;
		uint32_t  * indices = new uint32_t[mNumPoints+1];
		for (uint32_t i = 0; i < mNumPoints; ++i){
			indices[i] = i;
			swap_endianity(indices[i]);
		}
		indices[mNumPoints] = 0;
		mFile.write((char*)indices,bufferSize+4);;
		delete[] indices;
	}
	else {
		int bufferSize = mNumPoints*2 ;
		buffer = new char[bufferSize+4];
		buffer_start = buffer;
		uint16_t* indices = new uint16_t[mNumPoints];
		for (uint16_t i = 0; i < mNumPoints; ++i){
			indices[i] = i;
			swap_endianity(indices[i]);
		}
		memcpy(buffer,(char*)indices,bufferSize);
		buffer += bufferSize;
		uint32_t end = 0;
		swap_endianity(end);
		memcpy(buffer,(char*)&end,4);
		mFile.write(buffer_start,bufferSize+4);
		delete[] buffer_start;
        delete[] indices;
	}
}
//might change name from vtxAtr to something else (member variable is not mVtxAtr yet but misspelledd darrnnn)
void Bgeo::writePrimsMesh(char * * vtxData, attribute * *vtxAtr, const int vtxDataChannels, char * * primData)
{
	//Probably possible to make new functions that are OPTIMIZED for these operations (without the middle step) but I'm going to use
	//old code Bgeo code for now at least //per

	//Copy all data from Naiad store on the primitives
    std::vector<char*> swpdPrimData(mNumPrimAtr), swpPrimDataStart(mNumPrimAtr);
	for (int i = 0; i < mNumPrimAtr;++i){
		swpAtrData(mPrimAtr[i],swpdPrimData[i], primData[i], mNumPrims);
		swpPrimDataStart[i] = swpdPrimData[i];
	}

	//Copy indices. Depending on how many points, they will be packed in 2 or 4 bytes spaces.
	std::vector<char*> swpdVtxData(vtxDataChannels + 1), swpVtxDataStart(vtxDataChannels + 1);
	if (mIdxBytes == 4 ){
		swpdVtxData[0] = (char*) copyBuffer<uint32_t>(vtxData[0],1, mNumPrims*3, 0);
	}
	else {
		swpdVtxData[0] = (char*) copyBuffer32<uint16_t>(vtxData[0],1, mNumPrims*3, 0);
	}
	swpVtxDataStart[0] = swpdVtxData[0];


	//Copy the rest of the vertex data from Naiad.
	//vtxAtr[] goes form 0 to vtxDataChannels-1, while  vtxData[] goes from 1 to vtxData vtxDataChannels.
	//This is because indices don't have attribute info
	for (int i = 0; i < vtxDataChannels;++i){
		swpAtrData(*vtxAtr[i],swpdVtxData[i+1], vtxData[i+1], mNumPrims*3);
		swpVtxDataStart[i+1] = swpdVtxData[i+1];
	}

	//Each line will always look the same in the start: "3 <"
	const uint32_t verticesPerPolygon = 3;
	uint32_t verticesPerPolygonSwpd = verticesPerPolygon;
	swap_endianity(verticesPerPolygonSwpd);
	char primLineStart[5];
	memcpy(primLineStart, (char*)&verticesPerPolygonSwpd ,4);
	primLineStart[4] = '<';


    const int bytesPerPrim = 5 + verticesPerPolygon * (mIdxBytes + mVtxAtrBytes)  + mPrimAtrBytes;
    const int sizeofuint32 = sizeof(uint32_t);
    const int sizeofuint16 = sizeof(uint16_t);

    //Used for Houdini to idenity different elements
    const uint32_t run = 4294967295; // 0xFFFFFFFF
    uint32_t primkey = 1; //1 for polygon. (code is 0x00000001 binary)
    swap_endianity(primkey);

    int primsWritten = 0;
    const int iterations = ceil(((double) mNumPrims / 65535));

    //This is so we don't have to do a lot of if statements later on when copying vertex data per vertex
    //The vtxData have more elements than mNumVtxAtr since they are in some cases split into different channels (see Bgeo-Read help)
	std::vector<bool> vtxUpdate[3];
    vtxUpdate[0].resize(vtxDataChannels);
	vtxUpdate[1].resize(vtxDataChannels);
	vtxUpdate[2].resize(vtxDataChannels);
	
    for (int i = 0; i < vtxDataChannels;++i){
		size_t v0 = vtxAtr[i]->name.find("$v0")
				, v1 = vtxAtr[i]->name.find("$v1")
				, v2 = vtxAtr[i]->name.find("$v2");
		if (v0 != string::npos ){
			vtxUpdate[0][i] = true;
			vtxUpdate[1][i] = false;
			vtxUpdate[2][i] = false;

			vtxUpdate[0][i+1] = false;
			vtxUpdate[1][i+1] = true;
			vtxUpdate[2][i+1] = false;

			vtxUpdate[0][i+2] = false;
			vtxUpdate[1][i+2] = false;
			vtxUpdate[2][i+2] = true;
			i += 2;
		}
		else {
			vtxUpdate[0][i] = true;
			vtxUpdate[1][i] = true;
			vtxUpdate[2][i] = true;
		}
	}

    mPrimsBuf = new char[iterations * 10 + mNumPrims * bytesPerPrim ];
    char * primsBufStart = mPrimsBuf;
	for (int i = 0; i < iterations ; ++i) {
		cout << "Bgeo-Write >> Triangle chunk : " << i << endl;

		//Identify chunk as polygons
		memcpy(mPrimsBuf,(char*)&run,sizeofuint32);
		mPrimsBuf+= sizeofuint32;
		//Write number of triangles in chunk. 65535 is the maximum value for a uint16_t
		uint16_t nTempPrims;
		if (mNumPrims - primsWritten < 65535)
			nTempPrims = mNumPrims - primsWritten;
		else
			nTempPrims = 65535;
		uint16_t nTempPrimsSwpd = nTempPrims;
		swap_endianity(nTempPrimsSwpd);
		memcpy(mPrimsBuf,(char*)&nTempPrimsSwpd,sizeofuint16);
		mPrimsBuf+= sizeofuint16;
		//Another key that tells Houdini that this is polygon data
		memcpy(mPrimsBuf,(char*)&primkey,sizeofuint32);
		mPrimsBuf+= sizeofuint32;

		//For every primitive in chunk
		for (int j = 0; j < nTempPrims; ++j ) {
			//Write default at the start of each "line"
			memcpy(mPrimsBuf,primLineStart,5);
			mPrimsBuf+= 5;

			//For each vertex
			for (int k = 0; k < 3; ++k){
				//Write point index
				memcpy(mPrimsBuf,swpdVtxData[0],mIdxBytes);
				mPrimsBuf+= mIdxBytes;
				swpdVtxData[0]+= mIdxBytes;

				//For every vertex attribute
				for (int l = 0; l < vtxDataChannels;++l){
					//this can maybe be done in a smarter way? //per
					if (vtxUpdate[k][l]){
						const int sizeAtr = mTypeBytes[vtxAtr[l]->type] * vtxAtr[l]->size;
						memcpy(mPrimsBuf,swpdVtxData[l+1],sizeAtr);
						mPrimsBuf += sizeAtr;
						swpdVtxData[l+1] += sizeAtr;
					}
				}
			}

			//At the end, print all primitive attributes.
			for (int k = 0; k < mNumPrimAtr; ++k){
				const int sizeAtr = mTypeBytes[mPrimAtr[k].type] * mPrimAtr[k].size;
				memcpy(mPrimsBuf,swpdPrimData[k],sizeAtr);
				mPrimsBuf += sizeAtr;
				swpdPrimData[k] += sizeAtr;
			}
		}
		//Keep track of total number of primitives written
		primsWritten += nTempPrims;
	}

	//Remove the temporary data we created (with endian swaps)
	for (int i = 0; i < mNumPrimAtr; ++i)
		delete[] swpPrimDataStart[i];
	for (int i = 0; i < vtxDataChannels+1; ++i)
		delete[] swpVtxDataStart[i];

	//Finally write to file from buffer.
	mPrimsBuf = primsBufStart;
	mPrimsRead = true;
	mFile.write(mPrimsBuf,iterations * 10 + mNumPrims * bytesPerPrim);
}

void Bgeo::writePoints(char * * oldPointsData )
{
	//Probably possible to make new functions that are OPTIMIZED for these operations (without the middle step) but I'm going to use
	//old code Bgeo code for now at least. One future speed up could be writing new copyBuffer funtctions that add the W=1 in the swap //per

	// go from a buffer with [x y z]' to one with [x y z w]'
	const int sizeofFloat = sizeof(float);
	const int sizePerXYZ = sizeofFloat * 3;
	int bufferSize = mNumPoints * sizeofFloat * 4;
	char * tmpPoints = new char[mNumPoints * sizeofFloat * 4];
	char * bufStart = tmpPoints;
	const float W = 1.f;
	for (int i = 0; i < mNumPoints; ++i){
		memcpy(tmpPoints,oldPointsData[0],sizePerXYZ);
		tmpPoints +=  sizePerXYZ;
		oldPointsData[0] +=  sizePerXYZ;
		memcpy(tmpPoints,(char*) &W,sizeofFloat);
		tmpPoints += sizeofFloat;
	}

	//Get swapped buffers
	std::vector<char*> swpdPointData(mNumPointAtr+1), swpPointDataStart(mNumPointAtr+1);
	swpdPointData[0] = new char[bufferSize];
	swpPointDataStart[0] = swpdPointData[0];
	swpdPointData[0] = (char*) copyBuffer<float>(bufStart,1, mNumPoints*4, 0);
	delete[] bufStart; // delete tmpPoints
	for (int i = 0; i < mNumPointAtr;++i){

		swpAtrData(mPointAtr[i],swpdPointData[i + 1], oldPointsData[i + 1], mNumPoints);
		swpPointDataStart[i + 1] = swpdPointData[i + 1];
	}

	// copy everything to one big buffer, the Point buffer
	const int sizePerXYZW = sizeofFloat * 4;
    bufferSize = (sizeofFloat * 4 + mPointAtrBytes) * mNumPoints;
    mPointsBuf = new char[bufferSize];
    bufStart = mPointsBuf;

    //For all points
	for (int i = 0; i < mNumPoints; ++i) {
		//Write position (X Y Z W)
		memcpy(mPointsBuf,swpdPointData[0],sizePerXYZW);
		mPointsBuf += sizePerXYZW;
		swpdPointData[0] += sizePerXYZW;
		//For all point attributes
		for (int j = 0; j < mNumPointAtr; ++j) {
			//Write position attribute
			const int sizeAtr = mTypeBytes[mPointAtr[j].type] * mPointAtr[j].size;
			memcpy(mPointsBuf,swpdPointData[j+1],sizeAtr);
			mPointsBuf += sizeAtr;
			swpdPointData[j+1] += sizeAtr;
		}
	}
	//Delete swap buffers
	for (int i = 0; i < mNumPointAtr; ++i)
		delete[] swpPointDataStart[i];

	mPointsBuf = bufStart; //let class delete this and attributes
	mPointsRead = true;
	//Finally write to mFile.
	mFile.write(mPointsBuf,bufferSize);
}

void Bgeo::swpAtrData(const attribute & atr,char * &dst, char *  src, const int n)
{
	//Allocates a new buffer for endian swap data (has to be deleted later)
	dst = new char[(mTypeBytes[atr.type] * atr.size ) * n];
	switch(atr.type) {
		//Since type=5 is vector and only stores float.
		case 5:
		case 0: {
			dst = (char*) copyBuffer<float>(src,1, n * atr.size , 0);
			break;
		}
		case 1: {
			dst = (char*) copyBuffer<uint32_t>(src,1, n * atr.size, 0);
			break;
		}
		default:
			cout << "Type was wrong!: " << atr.type <<endl;
			NB_THROW("Bgeo-Write error; Can't read the type of attribute (Name: "<< atr.name <<" Type# = " << atr.type << ")");
	}
}

std::string Bgeo::removePrefix(std::string s)
{
	//Removes all the prefixes ($ followed by info) from a string
	size_t pos3f = s.find("$3f")
			, posV = s.find("$v");
	if (pos3f != string::npos)
		s.erase(pos3f,3);
	//Actualyl we don't have to check for $v1 and $v2 since all
	//attributes will only store the $v0 in their names
	if (posV != string::npos) {
		if (s.find("$v0") != string::npos)
			s.erase(posV,3);
		else
			s.erase(posV,2);
	}
	return s;
}

void Bgeo::writeAttribute(const attribute &atr)
{
	//Remove the prefixes added in naiad (if added)
	string name = removePrefix(atr.name);

	//All this with the assumption of string lengths are LESS than numeric_limits<uint16_t)::max()
	uint16_t strSize = name.length(), strSizeSwpd = strSize;
	swap_endianity(strSizeSwpd);
	const int sizeofUint16 = sizeof(uint16_t);
	const int defBytes = mTypeBytes[atr.type] * atr.size;
	const int bufferSize = 4 * sizeofUint16 + strSize +  defBytes;
	char * buffer= new char[bufferSize], * bufferStart = buffer;
	//Copy String size indicator
	memcpy(buffer,(char*) &strSizeSwpd,sizeofUint16);
	buffer += sizeofUint16;
	//Copy name
	memcpy(buffer,name.c_str(),strSize);
	buffer += strSize;
	//Copy size and type info. setting TypeInfo to 0
	uint16_t sizeAndType[3] = {atr.size,0,atr.type };
	swap_endianity(sizeAndType[0]);
	swap_endianity(sizeAndType[2]);
	memcpy(buffer,(char*) sizeAndType, 3 * sizeofUint16);
	buffer+= 3 * sizeofUint16;

	//Copy default values
	char * tempDef;
	if (atr.type == 0 || atr.type==5)
		tempDef = (char * )copyBuffer<float>(atr.defBuf,1,atr.size,0);
	else
		tempDef = (char * )copyBuffer<uint32_t>(atr.defBuf,1,atr.size,0);
	memcpy(buffer,tempDef,defBytes);
	delete[] tempDef;

	//Write to mFile
	mFile.write(bufferStart,bufferSize);
	delete[] bufferStart;
}

void Bgeo::addAttribute(attribute* atr, const char* name, const uint16_t size, const uint16_t type, const char * def)
{
	atr[0].name = string(name);
	atr[0].size = size;
	atr[0].type = type;

	//Have to copy data from the default buffers in Naiad
	const int bufferSize = mTypeBytes[type] * size;
	atr[0].defBuf = new char[bufferSize];
	if (type == 0 || type == 5)
		memcpy(atr[0].defBuf, def, bufferSize);
	else
	{
		//This can be corrected in future versions of Naiad. Atm, Default is only stored as floats (even if vec3i)
		std::vector<uint32_t> tempIntArr(size);
		for (int i = 0; i < size; ++i){
			float temp = *(float *)(def + sizeof(uint32_t) * i);
			tempIntArr[i] = (uint32_t ) temp;
		}
		memcpy(atr[0].defBuf,(char*)&tempIntArr[0], bufferSize);
	}
	//cout << atr[0].name << " " << atr[0].size<< " "  << atr[0].type << endl;
}

Bgeo::attribute Bgeo::readAttributeInfo()
{
    attribute atr;
    //Read length of Name string(max of 32k chars, uint32 not needed)
    int bufferSize = sizeof(uint16_t);
    char * buffer = new char[bufferSize], *bufferStart = buffer;
    mFile.read(buffer,bufferSize);
    uint16_t strLen;
    readNumber(buffer,strLen);
    delete[] bufferStart;

    //Read Name
    char * name_arr = new char[strLen];
    mFile.read(name_arr,strLen);
    atr.name = string(name_arr,name_arr + strLen);

    //Size && Type
    bufferSize = sizeof(uint16_t) + sizeof(uint32_t);
    buffer = new char[bufferSize], bufferStart = buffer;
    mFile.read(buffer,bufferSize);
    readNumber(buffer,atr.size);

    //This byte is later on splitted in two uint16 components
    uint32_t Type;
    readNumber(buffer,Type);
    atr.type = Type & 0xffff;

    // If type is string, char or index -> output error
    switch (atr.type){
		case 2:
		case 3:
		case 4:
			NB_THROW("Error when reading type from Bgeo. No support for type of " << mTypeStr[atr.type] << ".");
    }

    //dont know what typeinfo do (can be 0 or 1(index pair))
    uint16_t typeinfo = Type >> 16;
    delete[] bufferStart;

    //Defaults - store them in a buffer (size and type decides how to output them)
    bufferSize = mTypeBytes[atr.type] * atr.size;
    atr.defBuf = new char[bufferSize];
    mFile.read(atr.defBuf,bufferSize);

    return atr;
}

void Bgeo::readPrims(const bool integrityCheck)
{
    //Read vertex attributes
	mVtxAtr = new attribute[mNumVtxAtr];
    cout << endl <<"VertAttr" ;
    for (int i = 0; i < mNumVtxAtr; ++i )
    {
        mVtxAtr[i] = readAttributeInfo();
        mVtxAtrBytes += mTypeBytes.at(mVtxAtr[i].type) * mVtxAtr[i].size;
    }

    //Read primitives attributes
    mPrimAtr = new attribute[mNumPrimAtr];
    cout << endl << "PrimAttrib";
    for (int i = 0; i < mNumPrimAtr; ++i )
    {
        mPrimAtr[i] = readAttributeInfo();
        mPrimAtrBytes += mTypeBytes.at(mPrimAtr[i].type) * mPrimAtr[i].size;
    }

    //Allocate memory for the buffer
    const int verticesPerPolygon = 3;
    const int bytesPerPrim = (sizeof(uint32_t) + sizeof(char) + verticesPerPolygon * (mIdxBytes + mVtxAtrBytes)  + mPrimAtrBytes);
    mPrimsBuf = new char[mNumPrims * bytesPerPrim];
    char * primsBufStart = mPrimsBuf;

    //shorten this
    const int bufferSizeKey = sizeof(uint32_t)*2 + sizeof(uint16_t) ;
    char * bufferKey = new char[bufferSizeKey], *bufferKeyStart = bufferKey;
    mFile.read(bufferKey,bufferSizeKey);
    uint32_t run;
	readNumber(bufferKey,run);

	int bufferSize;
	char * buffer, *bufferStart;
	cerr << "Bgeo-Read: Loading polygons("<< mNumPrims <<"): ";
    while (run == 4294967295){// 4294967295 = 0xFFFFFFFF
    	uint16_t nPrimPolygons;
		readNumber(bufferKey,nPrimPolygons);

		uint32_t PrimKey;
		readNumber(bufferKey,PrimKey);

		//Read the primtive data and store in Primitive buffer
		bufferSize = nPrimPolygons * bytesPerPrim;
		mFile.read(mPrimsBuf,bufferSize);

		//validate vertices per polygon
		if (integrityCheck){
			char * bufferTmp = mPrimsBuf;
			uint32_t nVtxPerPoly;
			for (int i = 0; i < nPrimPolygons ; ++i){
				readNumber(mPrimsBuf,nVtxPerPoly);
				mPrimsBuf+= bytesPerPrim - 4;//sizeof uint32-t
				if (nVtxPerPoly != 3)
					NB_THROW("Bgeo-Read error! There is at the moment only support for objects with triangles. A polygon with "	<< nVtxPerPoly << " vertices was detected.");
			}
			mPrimsBuf = bufferTmp;
		}
		mPrimsBuf += bufferSize;

		//Read data for next chunk
		bufferKey = bufferKeyStart;
	    mFile.read(bufferKey,bufferSizeKey);
		readNumber(bufferKey,run);
    }
    delete[] bufferKeyStart;
    mPrimsBuf = primsBufStart;
    mPrimsRead = true;
}


void Bgeo::copyBufLocaluInt16(const char* src, uint32_t * dst,const int count)
{
	//Takes a chunk of memory from src and element by element do endian swap and put the output to the dst
	//This will convert data from 16bit int to 32bit int. (Bgeo can have indices as 16bit int, naiad always use 32bit.
    const int   sizeoft = sizeof(uint16_t);
    const int   q_end = count*sizeoft;
    int         counter = 0;
    for(int q=0; q<q_end; q += sizeoft){
        const char* p = src+q;
        uint16_t tmp =  *(uint16_t*)p ;
        swap_endianity(tmp);
        dst[counter] = tmp;
        ++counter;
    }
}

void Bgeo::copyBufLocaluint32(const char* src, uint16_t * dst,const int count)
{
	//Takes a chunk of memory from src and element by element do endian swap and put the output to the dst
	//This will convert data from 32bit int to 16bit int. (Bgeo can have indices as 16bit int, naiad always use 32bit.
    const int   sizeoft = sizeof(uint32_t);
    const int   q_end = count*sizeoft;
    int         counter = 0;
    for(int q=0; q<q_end; q += sizeoft){
        const char* p = src+q;
        uint32_t tmp =  *(uint32_t*)p ;
        dst[counter] = (uint16_t)tmp;
        swap_endianity(dst[counter]);
        ++counter;
    }
}

float * Bgeo::getPoints3v()
{
	return copyBuffer<float>(mPointsBuf,mNumPoints,3,sizeof(float) * 4 + mPointAtrBytes);
}

uint32_t * Bgeo::getIndices3v()
{
    if (mIdxBytes == 2)
        return copyBufferVertex16<uint32_t>(mPrimsBuf+5,1);
    else
        return copyBufferVertex<uint32_t>(mPrimsBuf+5,1);
}

