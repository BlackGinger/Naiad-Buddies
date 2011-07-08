#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <NbFilename.h>
#include <iostream>
#include <iterator>
#include <../common/NbAi.cc>
using namespace std;

int opaque, mBlur, fps = 0, padding;
vector<int> frames;
float radius = 0;
string input, output, shader, ASS(""), lib, pMode("sphere"), bodies("*"), dfo("");
bool dfoExists = false;

int main(int ac, char * av[])
{
	try {

		// Declare a group of options that will be
		// allowed only on command line


		po::options_description desc("Generic options");
		desc.add_options()
		    ("help", "produce help message")
		    ("input,i",po::value< string >(),"The input EMP-sequence. Use # for frame number. (works without -input). Required")
		    ("frames,f", po::value< vector<int> >()->multitoken(), "The frames that will be exported. Required. Either enter one single frame or start and end frame" )
		    ("padding,p", po::value<int>(&padding)->default_value(4), "The size of the frame number padding. Default 4, i.e 0001 for frame 1")
		    ("bodies,b", po::value<string>(&bodies), "Which bodies in the EMP that should be rendered(use white spacing as in Naiad). Default * (all)")
		    ("output,o", po::value<string>(), "Output of the .ASS file. Default is the EMP name but with the .ASS file extension")
		    ("ASS-file,a", po::value<string>(), "Arnold ASS file where information about camera, shaders etc can be found. Required.")
		    ("shader,s", po::value< string>(), "Name of the shader. Required")
		    ("opaque,q", po::value<int>(&opaque)->default_value(0), "Opaque (0 or 1). Default 0")
		    ("mblur,m", po::value<int>(&mBlur)->default_value(0), "Motion Blur (0 or 1). Default 0. If the points do not have velocites, it will use the EMP on the next frame to create motion keys. If not available, motion blur is not possible. Also it requires the fps.")
		    ("lib,l", po::value<string>(), "Path to the procedural dso. Default is $NAIAD_PATH/buddies/arnold/plug-ins/TODOoo")
		    ("fps,F", po::value<int>(), "Frames per second. By default it will check the nearby EMP files in sequence to determine the FPS")
		    ("dfo,d", po::value<string>(), "Distance Field override. If an implicit field should be rendered, enter the name of it here and it will override any Mesh or Particle shape.")
		    ("radius,r", po::value<float>(), "Only for bodies with Particle Shape. Radius of particles.")
		    ("particle-mode,M", po::value<string>(), "Only for bodies with Particle Shape. Points render mode.")
		 ;
		cerr << "emps2ass: \n";
		po::positional_options_description p;
		p.add("input", -1);

		po::variables_map vm;
		po::store(po::command_line_parser(ac, av).
		          options(desc).positional(p).run(), vm);
		po::notify(vm);

		//If help, print help and quit
		if (vm.count("help")) {
			cerr << desc << "\n";
		    return 1;
		}

		//Check input. If no input, return error.
		if (vm.count("input"))		{
			input = vm["input"].as< string >();
			cerr << "Input file is: "  << input<< "\n";
		}
		else{
			cerr << "emp2ass error: No input .EMP file\n";
			return 0;
		}

		if (vm.count("frames")){
			frames = vm["frames"].as<vector <int> >();
			cerr << "Frame: " << frames[0];
			if (frames.size() > 1)
				cerr << " - " << frames[1];
			else
				frames[1] = frames[0];
			cerr << "\n";
			if (frames[0] > frames[1]){
				cerr << "emp2ass error: No valid frame range given.";
				return 0;
			}
		}
		else{
			cerr << "emp2ass error: No frame or frame range given.";
			return 0;
		}

		if (vm.count("ASS-file")){
			ASS = vm["ASS-file"].as< string >();
			cerr << "Will use: " << ASS << " as Arnold .ASS file\n";
		}
		else
		{
			cerr << "emp2ass error: No Arnold .ASS file detected. Use -a or -ASS \n";
			return 0;
		}

		//Check input. If no input, return error.
		if (vm.count("shader"))		{
			shader = vm["shader"].as< string >();
			cerr << "Shader: "  << shader<< "\n";
		}
		else{
			cerr << "emp2ass error: No shader detected\n";
			return 0;
		}

		if (vm.count("output"))
			output = vm["output"].as< string >();
		else {
			output = input;
			size_t pos= input.find(".emp");
			if (pos == string::npos)
				pos = input.find(".EMP");
			if (pos == string::npos){
				cerr << "emp2ass error: Did not find .emp or .EMP in filename. Enter output manually with --output\n";
				return 0;
			}
			output.replace(pos,4,string(".ASS"));
		}
		cerr << "Output file is: "  << output<< "\n";

		if (bodies != string("bodies"))
			cerr << "Bodies: " << bodies << "\n";
		else
			cerr << "All bodies\n";

		if (opaque)
			cerr << "Object is opaque\n";
		else
			cerr << "Object is not opaque\n";

		if (mBlur)
			cerr << "Motion Blur: "<< mBlur << "  \n";
		else
			cerr << "No Motion Blur\n";

		if (vm.count("dfo")){
			dfo = vm["dfo"].as< string >();
			cerr << "Distance Field: " << dfo << "\n";
			dfoExists = true;
		}

		if (vm.count("lib")){
			lib = vm["lib"].as< string >();
			cerr << "Path to dso: " << lib<< "\n";
		}
		else
			lib = string("$NAIAD_PATH/buddies/arnold/plug-ins/blabla.so"); //todo fix this

		if (vm.count("fps")){
			fps = vm["fps"].as< int >();
			cerr << "FPS: " << fps << "\n";
		}

		if (vm.count("radius")){
			radius = vm["radius"].as< float >();
			cerr << "Particle radius:: " << radius << "\n";
			if (radius <= 0)
				cerr << "emp2ass error: illegal radius " << radius << "\n";
		}

		if (vm.count("particle-mode")){
			pMode = vm["particle-mode"].as< string >();
			cerr << "Particle mode:: " << pMode << "\n";
		}

		//Init Naiad base api
		Nb::begin();

		for (unsigned int frame = frames[0]; frame <= frames[1]; ++frame){
			cerr << "Frame is: " << frame << "\n";

			// Calc min max bounds
			cerr << "Computing total bounding box for bodies in frame " << frame << "...\n";
			Nb::Vec3f min,max;


			if (dfoExists){
		        Nb::String expandedFilename = Nb::sequenceToFilename("", input,frame, 0, padding );
		        cerr << "Input is: " << expandedFilename << "\n";
		        string bodyName;
				Nb::EmpReader* myEmpReader = new Nb::EmpReader(expandedFilename, bodies);
				for(int i = 0; i < myEmpReader->bodyCount(); ++i) {
					const Nb::Body* body = myEmpReader->constBody(i);
					if (body->constFieldShape().hasChannels1f(dfo)){
						body->bounds(min,max);
						bodyName = body->name();
						break;
					}
				}

				AiBegin();
				AiLoadPlugin("/home/per/Arnold/buddies/Arnold/procedural-dso/nd.so");
				AiASSLoad(ASS.c_str());

				//todo this is broken. fix or skip cmd?
				/*NbAi::createImplicitASS( "distance-field1" //todo add more fields
										, expandedFilename.c_str()
										, frame
										, dfo.c_str()
										, bodyName.c_str()
										, 5
										, min
										, max
										, .01f
										, shader.c_str()
										, bodies.c_str());*/
				string outputASS = Nb::sequenceToFilename("", output,frame, 0, padding );
				AiASSWrite(outputASS.c_str(), AI_NODE_ALL, FALSE);
				cerr << "Done creating " << outputASS << "\n\n";
				AiEnd();
				delete myEmpReader;
			}
			else {
				min[0] = min[1] = min[2] = std::numeric_limits<float>::max();
				max[0] = max[1] = max[2] = std::numeric_limits<float>::min();
				// read the EMP file
		        Nb::String expandedFilename = Nb::sequenceToFilename("", input,frame, 0, padding );
		        cerr << "Input is: " << expandedFilename << "\n";
				Nb::EmpReader* myEmpReader = new Nb::EmpReader(expandedFilename, bodies);
				for(int i = 0; i < myEmpReader->bodyCount(); ++i) {
					Nb::Vec3f minBody,maxBody;
					  const Nb::Body* body = myEmpReader->constBody(i);
					  NbAi::computeMinMax(body,minBody,maxBody);
					  NbAi::minMaxLocal(minBody, min, max);
					  NbAi::minMaxLocal(maxBody, min, max);
				}
				cerr << "Min bounds: " << min[0] << ", " << min[1] << ", " << min[2] << "\n";
				cerr << "Max bounds: " << max[0] << ", " << max[1] << ", " << max[2] << "\n";

				//Calc Frametime
				float frametime = 0.0f;
				if (mBlur == 1 && fps == 0){
					//Check if frame +- 1 is available
					string filename1p = Nb::sequenceToFilename("", input,frame + 1, 0, padding );
					string filename1m = Nb::sequenceToFilename("", input,frame - 1, 0, padding );

					Nb::EmpReader* myEmpReaderNext;
					if (NbAi::empExists(filename1p))
						myEmpReaderNext = new Nb::EmpReader(filename1p, bodies);
					else if (NbAi::empExists(filename1m))
						myEmpReaderNext = new Nb::EmpReader(filename1m, bodies);
					else {
						cerr << "emp2ass error: Was not able to compute FPS. Enter manually with --fps or -F\n";
						return 0;
					}
					frametime = myEmpReaderNext->time() - myEmpReader->time();
					delete myEmpReaderNext;
				}
				else if (mBlur == 1 && fps != 0)
					frametime = 1.f / fps;

				AiBegin();
				AiASSLoad(ASS.c_str());

				//particle info or not
				bool particleInfo = false;
				if (radius != 0 || pMode != "")
					particleInfo = true;

				//broken, needs fix
				/*NbAi::createProceduralASS( lib.c_str()
										  , (input + string(" ") + bodies).c_str()
										  , min
										  , max
										  , frametime
										  , particleInfo
										  , radius
										  , pMode.c_str());*/

				string outputASS = Nb::sequenceToFilename("", output,frame, 0, padding );
				AiASSWrite(outputASS.c_str(), AI_NODE_ALL, FALSE);
				cerr << "Done creating " << outputASS << "\n\n";
				AiEnd();
				delete myEmpReader;
			}
		}
	    Nb::end();
	}
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }
}
