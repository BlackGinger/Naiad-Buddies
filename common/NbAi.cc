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
    //Store how many vertices a face got
	AtArrayPtr nsidesArray = AiArrayAllocate(triIdxBuf.size(), 1, AI_TYPE_UINT);
	for (int i = 0; i < triIdxBuf.size(); ++ i)
	    AiArraySetUInt(nsidesArray, i, 3); //Only triangles are allowed in Naiad
	AiNodeSetArray(node, "nsides", nsidesArray);

	//Store vertex indices (the three vertices that a triangles uses)
	AtArrayPtr vidxsArray = AiArrayConvert(triIdxBuf.size() * 3, 1, AI_TYPE_UINT, triIdxBuf.data, false);
	AiNodeSetArray(node, "vidxs", vidxsArray);

	//Check if UV coordinates are available
	if (triangle.hasChannels3f("u") && triangle.hasChannels3f("v")){
		std::cerr << "NbAi:: Found UV channels!\n";
		const Nb::Buffer3f& uBuf(triangle.constBuffer3f("u"));
		const Nb::Buffer3f& vBuf(triangle.constBuffer3f("v"));

		//We don't want any vertices to have shared uv coords
		AtArrayPtr uvidxsArray = AiArrayAllocate(uBuf.size() * 3, 1, AI_TYPE_UINT);
		for (int i = 0; i < uBuf.size() * 3; ++ i)
		    AiArraySetUInt(uvidxsArray, i, i);
		AiNodeSetArray(node, "uvidxs", uvidxsArray);

		AtArrayPtr uvlistArray = AiArrayAllocate(uBuf.size() * 3, 1, AI_TYPE_POINT2);
		for (int i = 0; i < uBuf.size(); ++i){
			Nb::Vec3f u = uBuf(i), v = vBuf(i);
			for (int j = 0; j < 3 ; ++j){
				AtPoint2 uv = {u[j], v[j]};
				//std::cerr << "NbAi:: uv(tri = " << i << ", vert = " << j<<") = (" << uv.x << ", " << uv.y << ")\n";
				AiArraySetPnt2(uvlistArray, i * 3 + j, uv);
			}
		}
		//Only triangles are allowed in Naiad
		AiNodeSetArray(node, "uvlist", uvlistArray);
	}

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

AtNode* loadParticles(const Nb::Body * body, const char * pMode, const float radius, const float frametime = 0)
{

	//Create the node
	AtNode* node = AiNode("points");
	AiNodeSetStr(node, "mode" ,pMode);
	//AiNodeSetStr(node, "name", "hej");
	//Load Particle shape from Naiad body
	const Nb::ParticleShape & particle = body->constParticleShape();

	//Total amount of particles
	const int64_t nParticles = particle.size();

	//Copy the data from Naiads particle information. Position will always be in channel 0
	const int sizeOfFloat = sizeof(float);
	const Nb::BlockArray3f& blocksPos = particle.constBlocks3f(0);
	const int bcountPos = blocksPos.block_count();
	std::cerr << "NbAi:: Total amount of particles: " << nParticles << std::endl;

	//Create map for parallel loops later on.
	int64_t totalOffset = 0;
	int64_t * offset = new int64_t[bcountPos];
	offset[0] = 0;
	for(int b = 0; b < bcountPos-1; ++b) {
		totalOffset += blocksPos(b).size() * 3  * sizeOfFloat;
		offset[b + 1] = totalOffset;
	}

	AtArrayPtr pointsArray;
	//No motion blur
	if (frametime == 0){
		pointsArray = AiArrayAllocate(nParticles, 1, AI_TYPE_POINT);
		char * data = (char * ) pointsArray->data;

		std::cerr << "NbAi:: Copying particle data (no motion blur)...\n";
#pragma omp parallel for schedule(dynamic)
		for(int b = 0; b < bcountPos; ++b) {
			//std::cerr << "b: " << b << std::endl;
			const Nb::Block3f& cb = blocksPos(b);
			const int bufferSize = cb.size() * 3  * sizeOfFloat;
			if(cb.size()==0) continue;

			memcpy(data + offset[b], (char*)cb.data(), bufferSize);
		}

		AiNodeSetArray(node, "points", pointsArray);
		std::cerr<< "NbAi:: Copy done.\n";
	}
	else {
		pointsArray = AiArrayAllocate(nParticles, 2, AI_TYPE_POINT);
		char * data = (char * ) pointsArray->data;

		//Velocities
		const Nb::BlockArray3f& blocksVel = particle.constBlocks3f("velocity");
		const int halfBuffer = nParticles * sizeOfFloat * 3;

		std::cerr << "NbAi:: Copying particle data (motion blur)...\n";
#pragma omp parallel for schedule(dynamic)
		for(int b = 0; b < bcountPos; ++b) {
			//std::cerr << "b: " << b << std::endl;
			const Nb::Block3f& cb = blocksPos(b);
			const int bufferSize = cb.size() * 3  * sizeOfFloat;
			if(cb.size()==0) continue;
			memcpy(data + offset[b] , (char*)cb.data(), bufferSize);

			//Velocity block
			const Nb::Block3f& vb = blocksVel(b);

			for (int p(0); p < cb.size(); ++p){
				float vel[3] = {  cb(p)[0] +  frametime * vb(p)[0]
								, cb(p)[1] +  frametime * vb(p)[1]
								, cb(p)[2] +  frametime * vb(p)[2]};
				memcpy(data + offset[b] + halfBuffer + p * 3 * sizeOfFloat, (char*)vel, 3 * sizeOfFloat);
			}

		}

		AiNodeSetArray(node, "points", pointsArray);
		std::cerr<< "NbAi:: Copy done.\n";

	}
	delete[] offset;

	//All particles have the same radius (maybe add static randomness in the future?)
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
					, const int samples
					, const Nb::Vec3f min
					, const Nb::Vec3f max)
{
	AtNode * nodeND = AiNode("naiad_distance_field");
	AiNodeSetStr(nodeND, "name", implicitname);
	AiNodeSetStr(nodeND, "channel", channel);

	if (body != NULL){
		std::stringstream ss;
		ss << body;
		AiNodeSetStr(nodeND, "pointerBody", ss.str().c_str());
	}

	//Implicit Node
	AtNode* node = AiNode("implicit");
	//Gets the same name as the first body
	AiNodeSetFlt(node, "threshold", treshold);
	AiNodeSetUInt(node, "samples", samples);

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

	AiNodeDeclare(node, "body", "constant STRING");
	AiNodeSetStr(node, "body", bodies);

	AiNodeDeclare(node, "padding", "constant INT");
	AiNodeSetInt(node, "padding", padding);

	AiNodeDeclare(node, "frametime", "constant FLOAT");
	AiNodeSetFlt(node, "frametime", frametime);

	//Extra data for particles
	if (particle){
		AiNodeDeclare(node, "radius", "constant FLOAT");
		AiNodeSetFlt(node, "radius", radius);
		AiNodeDeclare(node, "mode", "constant STRING");
		AiNodeSetStr(node, "mode", pMode);
	}

	return node;
}


AtNode * createProceduralImplicitASS(	  const char * dso
										, const char * data
										, const char * ndName
										, const char * channel
										, const int samples
										, const Nb::Vec3f & min
										, const Nb::Vec3f & max
										, const int frame
										, const int padding
										, const float rayBias
										, const float treshold
										, const char * implicitShaderPlugin
										, const char * bodies
										, const char * shader = ""
										)
{
	AtNode* node = AiNode("procedural");
	AiNodeSetStr(node, "dso", dso);
	AiNodeSetStr(node, "data", data);
	AiNodeSetPnt(node, "min", min[0], min[1], min[2]);
	AiNodeSetPnt(node, "max", max[0], max[1], max[2]);

	AiNodeDeclare(node, "frame", "constant INT");
	AiNodeSetInt(node, "frame", frame);

	AiNodeDeclare(node, "body", "constant STRING");
	AiNodeSetStr(node, "body", bodies);

	AiNodeDeclare(node, "padding", "constant INT");
	AiNodeSetInt(node, "padding", padding);

	AiNodeDeclare(node, "samples", "constant INT");
	AiNodeSetInt(node, "samples", samples);

	AiNodeDeclare(node, "ray_bias", "constant FLOAT");
	AiNodeSetFlt(node, "ray_bias", rayBias);

	AiNodeDeclare(node, "frametime", "constant FLOAT");
	AiNodeSetFlt(node, "frametime", 0);

	AiNodeDeclare(node, "treshold", "constant FLOAT");
	AiNodeSetFlt(node, "treshold", treshold);

	AiNodeDeclare(node, "channel", "constant STRING");
	AiNodeSetStr(node, "channel", channel);

	AiNodeDeclare(node, "implicitname", "constant STRING");
	AiNodeSetStr(node, "implicitname", ndName);

	AiNodeDeclare(node, "naiad_distance_field", "constant STRING");
	AiNodeSetStr(node, "naiad_distance_field", implicitShaderPlugin);

	if (std::string(shader).size() > 0)
		AiNodeSetPtr(node, "shader", AiNodeLookUpByName(shader));

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
						, const float treshold
						, const char * bodies = "*"
						, const char * shader = ""
						)
{
	//Naiad Distance Node
	AtNode * nodeND = AiNode("naiad_distance_field");
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
	AiNodeSetFlt(node, "threshold", treshold);
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
