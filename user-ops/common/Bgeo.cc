#include "Bgeo.h"

Bgeo::Bgeo(const char* filename) :
				  pointAtrBytes(0)
				, vtxAtrBytes(0)
				, primAtrBytes(0)
				, pointsRead(false)
				, primsRead(false)
				, file(filename, ios::in|ios::binary)
{
    if(!file.good())
        NB_THROW("Cannot read input file: '" << filename << "'");

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

    setConstants();
}

Bgeo::Bgeo(const char* filename,uint32_t * headerParameters) :
				  pointAtrBytes(0)
				, vtxAtrBytes(0)
				, primAtrBytes(0)
				, pointsRead(false)
				, primsRead(false)
				, vNr(headerParameters[0])
				, nPoints(headerParameters[1])
				, nPrims(headerParameters[2])
				, nPointGrps(headerParameters[3])
				, nPrimGrps(headerParameters[4])
				, nPointAtr(headerParameters[5])
				, nVtxAtr(headerParameters[6])
				, nPrimAtr(headerParameters[7])
				, nAtr(headerParameters[8])
				, file(filename, ios::out|ios::binary)
{
    if(!file.good())
        NB_THROW("Cannot create output file: '" << filename << "'");

	setConstants();

	char headrStr[] = "BgeoV";
	file.write(headrStr,5);

	for (int i = 0; i < 9; ++i)
		swap_endianity(headerParameters[i]);
	file.write( (char*) headerParameters, sizeof(uint32_t)*9);
	pointAtr = new attribute[nPointAtr];
	vtxArt = new attribute[nVtxAtr];
	primArt = new attribute[nPrimAtr];
}

Bgeo::~Bgeo()
{
	cout<< "Closing file" << endl;
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

void Bgeo::setConstants()
{
    //If the number of points is > 65535, uint32 is used
    idxBytes = 2;
    if (nPoints > 65535)
        idxBytes = 4;

    int tmpInt[6] = {4,4,32,1,4,4};
    string tmpStr[6] = {"float","int","string","mixed","index","vector"};

    typeBytes = vector<int>(tmpInt,tmpInt+6);
    typeStr = vector<string>(tmpStr,tmpStr+6);
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
//only write varmap
void Bgeo::writeDetailAtr()
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
	file.write(buffer_start,14);
	delete[] buffer_start;

	std::vector<string> strVec;
	string tmpStr,tmpStrUpper;
	for (int i = 0; i < nPointAtr; ++i){
		tmpStr = removePrefix(pointAtr[i].name);
		tmpStrUpper = tmpStr;
		transform(tmpStrUpper.begin(), tmpStrUpper.end(),tmpStrUpper.begin(), ::toupper);
		strVec.push_back(tmpStr + string(" -> ")  + tmpStrUpper);
	}
	for (int i = 0; i < nPrimAtr; ++i){
		tmpStr = removePrefix(primArt[i].name);
		tmpStrUpper = tmpStr;
		transform(tmpStrUpper.begin(), tmpStrUpper.end(),tmpStrUpper.begin(), ::toupper);
		strVec.push_back(tmpStr + string(" -> ")  + tmpStrUpper);
	}
	for (int i = 0; i < nVtxAtr; ++i){
		tmpStr = removePrefix(vtxArt[i].name);
		tmpStrUpper = tmpStr;
		transform(tmpStrUpper.begin(), tmpStrUpper.end(),tmpStrUpper.begin(), ::toupper);
		strVec.push_back(tmpStr + string(" -> ")  + tmpStrUpper);
	}

	int size = 0;
	uint32_t nStrings = strVec.size();
	for (int i = 0; i < nStrings; ++i)
		size += strVec.at(i).length() + 2;

	for (int i = 0; i < nStrings; ++i)
		cout <<strVec.at(i)<<endl;

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
	file.write(buffer_start,size+4);
}

//might change name from vtxAtr to something else (member variable is not mVtxAtr yet but misspelledd darrnnn)
void Bgeo::writePrims(char * * vtxData, attribute * *vtxAtr, const int vtxDataChannels, char * * primData)
{
	//Probably possible to make new functions that are OPTIMIZED for these operations (without the middle step) but I'm going to use
	//old code Bgeo code for now at least //per
	char * swpdPrimData[nPrimAtr], *swpPrimData_start[nPrimAtr];

	for (int i = 0; i < nPrimAtr;++i){
		swpAtrData(primArt[i],swpdPrimData[i], primData[i], nPrims);
		swpPrimData_start[i] = swpdPrimData[i];
	}

	//Copy indices
	char * swpdVtxData[vtxDataChannels + 1], *swpVtxData_start[vtxDataChannels + 1];
	if (idxBytes == 4 ){
		//swpdVtxData[0] = new char[nPrims * sizeof(uint32_t) * 3];
		swpdVtxData[0] = (char*) copyBuffer<uint32_t>(vtxData[0],1, nPrims*3, 0);
	}
	else {
		//swpdVtxData[0] = new char[nPrims * sizeof(uint16_t) * 3];
		swpdVtxData[0] = (char*) copyBuffer32<uint16_t>(vtxData[0],1, nPrims*3, 0);
	}
	swpVtxData_start[0] = swpdVtxData[0];

	for (int i = 0; i < vtxDataChannels;++i){
		swpAtrData(*vtxAtr[i],swpdVtxData[i+1], vtxData[i+1], nPrims*3);
		swpVtxData_start[i+1] = swpdVtxData[i+1];
	}

	const uint32_t verticesPerPolygon = 3;
	uint32_t verticesPerPolygonSwpd = verticesPerPolygon;
	swap_endianity(verticesPerPolygonSwpd);
	char primLineStart[5];
	memcpy(primLineStart, (char*)&verticesPerPolygonSwpd ,4);
	primLineStart[4] = '<';
    const int bytesPerPrim = 5 + verticesPerPolygon * (idxBytes + vtxAtrBytes)  + primAtrBytes;


    const int sizeofuint32 = sizeof(uint32_t);
    const int sizeofuint16 = sizeof(uint16_t);
    const uint32_t run = 4294967295; // 0xFFFFFFFF
    uint32_t primkey = 1; //1 for polygon.		if (vtxAtr[i]->name.find("$v0") != string::npos)
    swap_endianity(primkey);

    int primsWritten = 0;
    const int iterations = ceil(((double) nPrims / 65535));

	bool vtxUpdate[3][vtxDataChannels];
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
			i +=2;
		}
		else {
			vtxUpdate[0][i] = true;
			vtxUpdate[1][i] = true;
			vtxUpdate[2][i] = true;
		}
	}

    primsBuf = new char[iterations * 10 + nPrims * bytesPerPrim ];


    char * primsBuf_start = primsBuf;
	for (int i = 0; i < iterations ; ++i) {
		cout << "Write prims loop: " << i << endl;

		memcpy(primsBuf,(char*)&run,sizeofuint32);
		primsBuf+= sizeofuint32;

		uint16_t nTempPrims;
		if (nPrims - primsWritten < 65535)
			nTempPrims = nPrims - primsWritten;
		else
			nTempPrims = 65535;
		uint16_t nTempPrimsSwpd = nTempPrims;
		swap_endianity(nTempPrimsSwpd);
		memcpy(primsBuf,(char*)&nTempPrimsSwpd,sizeofuint16);
		primsBuf+= sizeofuint16;

		memcpy(primsBuf,(char*)&primkey,sizeofuint32);
		primsBuf+= sizeofuint32;

		for (int j = 0; j < nTempPrims; ++j ) {
			memcpy(primsBuf,primLineStart,5);
			primsBuf+= 5;

			for (int k = 0; k < 3; ++k){
				memcpy(primsBuf,swpdVtxData[0],idxBytes);
				primsBuf+= idxBytes;
				swpdVtxData[0]+= idxBytes;

				for (int l = 0; l < vtxDataChannels;++l){
					//this can maybe be done in a smarter way //per
					if (vtxUpdate[k][l]){
						const int sizeAtr = typeBytes[vtxAtr[l]->type] * vtxAtr[l]->size;
						memcpy(primsBuf,swpdVtxData[l+1],sizeAtr);
						primsBuf += sizeAtr;
						swpdVtxData[l+1] += sizeAtr;
					}
				}
			}

			for (int k = 0; k < nPrimAtr; ++k){
				const int sizeAtr = typeBytes[primArt[k].type] * primArt[k].size;
				memcpy(primsBuf,swpdPrimData[k],sizeAtr);
				primsBuf += sizeAtr;
				swpdPrimData[k] += sizeAtr;
			}
		}
		primsWritten += nTempPrims;
	}
	primsBuf = primsBuf_start;
	primsRead = true;
	file.write(primsBuf,iterations * 10 + nPrims * bytesPerPrim);
}

void Bgeo::writePoints(char * * oldPointsData )
{
	//BIG fix needs to be done. bad copy to tmpPoints!!

	// go from a buffer with [x y z]' to one with [x y z w]'
	const int sizeofFloat = sizeof(float);
	const int sizePerXYZ = sizeofFloat * 3;
	int bufferSize = nPoints * sizeofFloat * 4;
	char * tmpPoints = new char[nPoints * sizeofFloat * 4];
	char * buf_start = tmpPoints;
	const float W = 1.f;
	for (int i = 0; i < nPoints; ++i){
		memcpy(tmpPoints,oldPointsData[0],sizePerXYZ);
		tmpPoints +=  sizePerXYZ;
		oldPointsData[0] +=  sizePerXYZ;
		memcpy(tmpPoints,(char*) &W,sizeofFloat);
		tmpPoints += sizeofFloat;
	}

	char * swpdPointData[nPointAtr+1], *swpPointData_start[nPointAtr+1];
	swpdPointData[0] = new char[bufferSize];
	swpPointData_start[0] = swpdPointData[0];
	swpdPointData[0] = (char*) copyBuffer<float>(buf_start,1, nPoints*4, 0);
	delete[] buf_start; // delete tmpPoints

	//Probably possible to make new functions that are OPTIMIZED for these operations (without the middle step) but I'm going to use
	//old code Bgeo code for now at least //per

	for (int i = 0; i < nPointAtr;++i){

		swpAtrData(pointAtr[i],swpdPointData[i + 1], oldPointsData[i + 1], nPoints);
		swpPointData_start[i + 1] = swpdPointData[i + 1];
	}

	// copy everything to one big buffer
	const int sizePerXYZW = sizeofFloat * 4;
    bufferSize = (sizeofFloat * 4 + pointAtrBytes) * nPoints;
    pointsBuf = new char[bufferSize];
    buf_start = pointsBuf;
	for (int i = 0; i < nPoints; ++i) {
		memcpy(pointsBuf,swpdPointData[0],sizePerXYZW);
		pointsBuf += sizePerXYZW;
		swpdPointData[0] += sizePerXYZW;
		for (int j = 0; j < nPointAtr; ++j) {
			const int sizeAtr = typeBytes[pointAtr[j].type] * pointAtr[j].size;
			memcpy(pointsBuf,swpdPointData[j+1],sizeAtr);
			pointsBuf += sizeAtr;
			swpdPointData[j+1] += sizeAtr;
			//cout << "Name: " << pointAtr[j].name  <<"Type: " << pointAtr[j].type << " Size: " << pointAtr[j].size << " Bytes: "<< sizeAtr << endl;
		}
	}
	for (int i = 0; i < nPointAtr; ++i)
		delete[] swpPointData_start[i];

	pointsBuf = buf_start; //let class delete this and attributes
	pointsRead = true;
	//Finally write to file.
	file.write(pointsBuf,bufferSize);
	//delete[] buf_start; // delete pointsBuf
}

void Bgeo::swpAtrData(const attribute & atr,char * &dst, char *  src, const int n)
{
	dst = new char[(typeBytes[atr.type] * atr.size ) * n];
	switch(atr.type) {
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
	size_t pos3f = s.find("$3f")
			, posV = s.find("$v");
	if (pos3f != string::npos)
		s.erase(pos3f,3);
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

	uint16_t strSize = name.length(), strSizeSwpd = strSize;
	swap_endianity(strSizeSwpd);
	const int sizeofUint16 = sizeof(uint16_t);
	const int defBytes = typeBytes[atr.type] * atr.size;
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
	//Write to file
	file.write(bufferStart,bufferSize);



	delete[] bufferStart;
}

void Bgeo::addAttribute(attribute* atr, const char* name, const uint16_t size, const uint16_t type, const char * def)
{
	atr[0].name = string(name);
	atr[0].size = size;
	atr[0].type = type;
	const int bufferSize = typeBytes[type] * size;
	atr[0].defBuf = new char[bufferSize];
	if (type == 0 || type == 5){
		memcpy(atr[0].defBuf,def,bufferSize);
	}
	else
	{
		//This can be corrected in future versions of Naiad. Atm, Default is only stored as floats (even if vec3i)
		uint32_t tempIntArr[size];
		for (int i = 0; i < size; ++i){
			float temp = *(float *)(def + sizeof(uint32_t) * i);
			tempIntArr[i] = (uint32_t ) temp;
		}
		memcpy(atr[0].defBuf,(char*)tempIntArr,bufferSize);
	}
	//cout << atr[0].name << " " << atr[0].size<< " "  << atr[0].type << endl;
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

    //dont know what index do (t odo)
    uint16_t typeinfo = Type >> 16;
    delete[] buffer_start;

    //Defaults - store them in a buffer (size and type decides how to output them)
    bufferSize = typeBytes[atr.type] * atr.size;
    atr.defBuf = new char[bufferSize];
    file.read(atr.defBuf,bufferSize);
    //aa, error if type = 3 4 5
    //cout << "Parameter--> Name: " << atr.name << " Size: " << atr.size<< " Type: "
    		//<< typeStr[atr.type]  << " Defaultsize in bytes: "<< endl;
    return atr;
}

void Bgeo::readPrims(const bool integrityCheck)
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

    const int verticesPerPolygon = 3;
    const int bytesPerPrim = (sizeof(uint32_t) + sizeof(char) + verticesPerPolygon * (idxBytes + vtxAtrBytes)  + primAtrBytes);
    primsBuf = new char[nPrims * bytesPerPrim];
    char * primsBuf_start = primsBuf;



    //shorten this
    int bufferSize = sizeof(uint32_t)*2 + sizeof(uint16_t) ;
    char * buffer = new char[bufferSize], *buffer_start = buffer;
    file.read(buffer,bufferSize);
    uint32_t run;
	readNumber(buffer,run);

	cout << "Bgeo-Read: Loading polygons("<< nPrims <<"): ";
    while (run == 4294967295){
    	cout << ".";
    	// 4294967295 = 0xFFFFFFFF
        //Control that the bgeo is a polygon model

        //if (run != 4294967295)
        	//NB_THROW("BGEO File Corrupt; No primitives");

    	uint16_t nPrimPolygons;
		readNumber(buffer,nPrimPolygons);
		//if (nPrimPolygons != nPrims) {
			//NB_THROW("BGEO File Corrupt; primitive count mismatch (nPrimPolygons=" <<  nPrimPolygons << ", nPrims=" << nPrims << ")");
		//}

		uint32_t PrimKey;
		readNumber(buffer,PrimKey);
		//if (PrimKey != 1)
			//NB_THROW("BGEO File Corrupt; No polygon primitives ");
		delete[] buffer_start;

		//Restricted to triangles only
		bufferSize = nPrimPolygons * bytesPerPrim;
		file.read(primsBuf,bufferSize);

		//validate vertices per polygon
		if (integrityCheck){
			char * bufferTmp = primsBuf;
			uint32_t nVtxPerPoly;
			for (int i = 0; i < nPrimPolygons ; ++i){
				readNumber(primsBuf,nVtxPerPoly);
				primsBuf+= bytesPerPrim - 4;//sizeof uint32-t
				if (nVtxPerPoly != 3)
					NB_THROW("Bgeo-Read error! There is at the moment only support for objects with triangles. A polygon with "	<< nVtxPerPoly << " vertices was detected.");
			}
			primsBuf = bufferTmp;
		}
		primsBuf += bufferSize;


		//shorten this
		bufferSize = sizeof(uint32_t)*2 + sizeof(uint16_t) ;
	    buffer = new char[bufferSize], buffer_start = buffer;
	    file.read(buffer,bufferSize);
		readNumber(buffer,run);
    }
    delete[] buffer_start;
    primsBuf = primsBuf_start;
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

void Bgeo::copyBufLocaluint32(const char* src, uint16_t * dst,const int count)
{
    const int   sizeoft = sizeof(uint32_t);
    const char* src_end = src+count*sizeoft;
    int         counter = 0;
#pragma omp parallel for
    for(const char* p = src; p<src_end; p += sizeoft){
        uint32_t tmp =  *(uint32_t*)p ;
        dst[counter] = (uint16_t)tmp;
        swap_endianity(dst[counter]);
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

