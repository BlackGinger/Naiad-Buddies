#include <ai.h>
#include <Nb.h>
#include <NbBody.h>
#include <NbBlock.h>
#include <NbFilename.h>
#include <limits>

namespace NbAi
{

void minMaxLocal(const Nb::Vec3f & pos, Nb::Vec3f & min, Nb::Vec3f & max)
{
	if (pos[0] < min[0])
		min[0] = pos[0];
	if (pos[0] > max[0])
		max[0] = pos[0];
	if (pos[1] < min[1])
		min[1] = pos[1];
	if (pos[1] > max[1])
		max[1] = pos[1];
	if (pos[2] < min[2])
		min[2] = pos[2];
	if (pos[2] > max[2])
		max[2] = pos[2];
}

bool empExists(std::string filename)
{
	FILE* emp(std::fopen(filename.c_str(),"rb"));
	bool fileExists(emp!=NULL);
	std::fclose(emp);
	return fileExists;
}

AtNode* loadMesh(const Nb::Body * body, const float frametime = 0, const Nb::Body * bodyNext = NULL)
{
	//Create the node
	AtNode* node = AiNode("polymesh");

	//Load shapes from Naiad body
    const Nb::TriangleShape& triangle = body->constTriangleShape();
    const Nb::PointShape& point = body->constPointShape();

    //Get buffers
    const Nb::Buffer3f& posBuf(point.constBuffer3f("position"));
    const Nb::Buffer3i& triIdxBuf(triangle.constBuffer3i("index"));

    //This is not needed if all polygons are triangles (which is they case in Naiad!)
    /*/Store how many vertices a face got
	AtArrayPtr nsidesArray = AiArrayAllocate(triIdxBuf.size(), 1, AI_TYPE_UINT);
	for (int i = 0; i < triIdxBuf.size(); ++ i)
	    AiArraySetUInt(nsidesArray, i, 3); //Only triangles are allowed in Naiad
	AiNodeSetArray(node, "nsides", nsidesArray);*/

	//Store vertex indices (the three vertices that a triangles uses)
	AtArrayPtr vidxsArray = AiArrayConvert(triIdxBuf.size() * 3, 1, AI_TYPE_UINT, triIdxBuf.data, false);
	AiNodeSetArray(node, "vidxs", vidxsArray);

	//Copy Vertex positions
	AtArrayPtr vlistArray = NULL;
	if (frametime == 0.0f){
		vlistArray = AiArrayConvert(posBuf.size(), 1, AI_TYPE_POINT, posBuf.data, false);
		AiNodeSetArray(node, "vlist", vlistArray);
		std::cerr << body->name() << " : No motion blur! \n";
		return node;
	}
	AtArrayPtr vp0array = AiArrayConvert(posBuf.size(), 1,  AI_TYPE_POINT,posBuf.data,false);
	AtArrayPtr vp1array;
	if (point.hasChannels3f("velocity")){
		std::cerr << "Creating Motion Blur \n";
		//If the body has a velocity for each point, we will use it for motion blur.
		//Allocate arrays
		std::cerr << body->name() << " has a velocity channel! \n";
		vp1array = AiArrayAllocate(posBuf.size(), 1, AI_TYPE_POINT);
		const Nb::Buffer3f& velBuf(point.constBuffer3f("velocity"));

		//Forward euler. x + dx * dt where dt = frametime
		for (int i = 0; i < velBuf.size(); ++i){
			AtPoint p1 = {posBuf(i)[0] + velBuf(i)[0] * frametime,
					posBuf(i)[1] + velBuf(i)[1] * frametime,
					posBuf(i)[2] + velBuf(i)[2] * frametime};
			AiArraySetPnt(vp1array, i, p1);
		}

	}
	else if (bodyNext != NULL) {
		std::cerr << body->name() << " motion blur from next frame: " << bodyNext->name() << "\n";
	    //Get buffers
	    const Nb::Buffer3f& posBufNext(bodyNext->constPointShape().constBuffer3f("position"));
		vp1array = AiArrayConvert(posBufNext.size(), 1,  AI_TYPE_POINT,posBufNext.data,false);
	}
	else
		NB_THROW("Can't set motion key for motion blur. No velocity or no body next frame available.")

	vlistArray = AiArrayAllocate(posBuf.size(), 2, AI_TYPE_POINT);
	AiArraySetKey(vlistArray, 0, vp0array->data);
	AiArraySetKey(vlistArray, 1, vp1array->data);
	AiNodeSetArray(node, "vlist", vlistArray);

	return node;
}

AtNode* loadParticles(const Nb::Body * body, const char * pMode, const float radius, const float motionBlur = 0)
{
	//Create the node
	AtNode* node = AiNode("points");

	//Load Particle shape from Naiad body
	const Nb::ParticleShape & particle = body->constParticleShape();

	//Total amount of particles
	const int64_t nParticles = particle.size();

	//Copy the data from Naiads particle information. Position will always be in channel 0
	const int sizeOfFloat = sizeof(float);
	const Nb::BlockArray3f& blocksPos = particle.constBlocks3f(0);
	const int bcountPos = blocksPos.block_count();
	//int64_t nParticles = bcountPos;
	int pCount = 0.0;
	std::cerr << "Total particles nParticles: " << nParticles << std::endl;
	AtArrayPtr pointsArray = AiArrayAllocate(nParticles, 1, AI_TYPE_POINT);
	char * data = (char * ) pointsArray->data;

	//Create map for parallelism
	int64_t totalOffset = 0;
	int64_t * offset = new int64_t[bcountPos];
	offset[0] = 0;
	for(int b = 0; b < bcountPos-1; ++b) {
		totalOffset += blocksPos(b).size() * 3  * sizeOfFloat;
		offset[b + 1] = totalOffset;
	}

	std::cerr << "Starting copy loop\n";
#pragma omp parallel for schedule(dynamic)
	for(int b = 0; b < bcountPos; ++b) {
		//std::cerr << "b: " << b << std::endl;
		const Nb::Block3f& cb = blocksPos(b);
		const int bufferSize = cb.size() * 3  * sizeOfFloat;
		if(cb.size()==0) continue;

		memcpy(data + offset[b], (char*)cb.data(), bufferSize);
	}
	delete[] offset;
	AiNodeSetArray(node, "points", pointsArray);
	std::cerr<< "Done with copy loop\n";

	//All particles have the same radius (maybe add randomness in the future?)
	AtArrayPtr radiusArray = AiArrayAllocate(nParticles, 1, AI_TYPE_FLOAT);
    for (int i = 0; i < nParticles; ++ i)
        AiArraySetFlt(radiusArray, i, radius);
	AiNodeSetArray(node, "radius", radiusArray);

	return node;
}

void computeMinMax(const Nb::Body * body, Nb::Vec3f & min, Nb::Vec3f & max)
{
	min[0] = min[1] = min[2] = std::numeric_limits<float>::max();
	max[0] = max[1] = max[2] = std::numeric_limits<float>::min();
    //Evaluate bodies
	if(body->matches("Mesh")){
		//todo this can be improved by use bounding box of motion blur too
		const Nb::PointShape& point = body->constPointShape();
        const Nb::Buffer3f& posBuf(point.constBuffer3f("position"));
        for (int j = 0; j < posBuf.size(); ++j)
        	minMaxLocal(posBuf(j),min,max);
    }
    else if (body->matches("Particle")){
    	const Nb::ParticleShape & particle = body->constParticleShape();
		const Nb::BlockArray3f& blocksPos=particle.constBlocks3f(0);
		const int bcountPos=blocksPos.block_count();
		for(int b=0; b<bcountPos; ++b) {
			const Nb::Block3f& cb = blocksPos(b);
			for(int64_t p(0); p<cb.size(); ++p)
				minMaxLocal(cb(p),min,max);
		}
    }
}

AtNode* loadImplicit( const Nb::Body * body
					, const char * channel
					, const char * implicitname
					, const float raybias
					, const float treshold
					, const int samples)
{
	AtNode * nodeND = AiNode("naiad_distance");
	AiNodeSetStr(nodeND, "name", implicitname);
	AiNodeSetStr(nodeND, "channel", channel);

	std::stringstream ss;
	ss << body;
	AiNodeSetStr(nodeND, "pointerBody", ss.str().c_str());

	//Implicit Node
	AtNode* node = AiNode("implicit");
	//Gets the same name as the first body
	AiNodeSetFlt(node, "treshold", treshold);
	AiNodeSetUInt(node, "samples", samples);

	Nb::Vec3f min, max;
	body->bounds(min,max);
	AiNodeSetPnt(node, "min", min[0], min[1], min[2]);
	AiNodeSetPnt(node, "max", max[0], max[1], max[2]);
	AiNodeSetFlt(node, "ray_bias", raybias);
	AiNodeSetStr(node, "solver", "levelset");
	AiNodeSetPtr(node, "field", nodeND);
	return node;
}

AtNode* createProceduralASS( const char * dso
							, const char * data
							, const char * bodies
							, const Nb::Vec3f & min
							, const Nb::Vec3f & max
							, const int frame
							, const int padding
							, const float frametime
							, bool particle = false
							, const float radius = 0.005
							, const char * pMode = "sphere")
{
	AtNode* node = AiNode("procedural");
	AiNodeSetStr(node, "dso", dso);
	AiNodeSetStr(node, "data", data);
	AiNodeSetPnt(node, "min", min[0], min[1], min[2]);
	AiNodeSetPnt(node, "max", max[0], max[1], max[2]);

	AiNodeDeclare(node, "frame", "constant INT");
	AiNodeSetInt(node, "frame", frame);

	AiNodeDeclare(node, "bodies", "constant STRING");
	AiNodeSetStr(node, "bodies", bodies);

	AiNodeDeclare(node, "padding", "constant INT");
	AiNodeSetInt(node, "padding", padding);

	AiNodeDeclare(node, "motionblur", "constant FLOAT");
	AiNodeSetFlt(node, "motionblur", frametime);

	//Extra data for particles
	if (particle){
		AiNodeDeclare(node, "radius", "constant FLOAT");
		AiNodeSetFlt(node, "radius", radius);
		AiNodeDeclare(node, "mode", "constant STRING");
		AiNodeSetStr(node, "mode", pMode);
	}

	return node;
}

AtNode * createImplicitASS(	  const char * ndName
						, const char * empFile
						, const char * channel
						, const int samples
						, const Nb::Vec3f & min
						, const Nb::Vec3f & max
						, const int frame
						, const int padding
						, const float rayBias
						, const char * bodies = "*"
						, const char * shader = ""
						)
{
	//Naiad Distance Node
	AtNode * nodeND = AiNode("naiad_distance");
	if (std::string(ndName).size() > 0)
		AiNodeSetStr(nodeND, "name", ndName);
	else
		AiNodeSetStr(nodeND, "name", "noname"); //dummy so arnold wont crash

	AiNodeSetStr(nodeND, "empcache", empFile);
	AiNodeSetStr(nodeND, "body", bodies);
	AiNodeSetStr(nodeND, "channel", channel);
	AiNodeDeclare(nodeND, "frame", "constant INT");
	AiNodeSetInt(nodeND, "frame", frame);
	AiNodeDeclare(nodeND, "padding", "constant INT");
	AiNodeSetInt(nodeND, "padding", padding);


	//Implicit Node
	AtNode* node = AiNode("implicit");
	//Gets the same name as the first body
	AiNodeSetFlt(node, "treshold", 0);
	AiNodeSetUInt(node, "samples", samples);
	AiNodeSetPnt(node, "min", min[0], min[1], min[2]);
	AiNodeSetPnt(node, "max", max[0], max[1], max[2]);
	AiNodeSetFlt(node, "ray_bias", rayBias);
	AiNodeSetStr(node, "solver", "levelset");
	AiNodeSetPtr(node, "field", nodeND);

	if (std::string(shader).size() > 0)
		AiNodeSetPtr(node, "shader", AiNodeLookUpByName(shader));

	return node;
}

}; //end NbAi namespace
