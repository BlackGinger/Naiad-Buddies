/*
 * geo2emp.C
 *
 * .bgeo to .emp converter
 *
 * Copyright (c) 2010 Van Aarde Krynauw.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "geo2emp.h"
#include "geo2emp_utils.h"

#include <stdio.h>
#include <iostream>

//Naiad headers
#include <NbBody.h>
#include <NbFilename.h>
#include <NbEmpReader.h>
#include <NbEmpWriter.h>
#include <NbString.h>

//Houdini headers
#include <GEO/GEO_AttributeHandle.h>
#include <GEO/GEO_PrimPoly.h>

//Thirdparty headers
#include "pystring.h"

using namespace geo2emp;

/**************************************************************************************************/

std::string Geo2Emp::getErrorString(ErrorCode error)
{
	switch (error)
	{
		case EC_SUCCESS:
			return "Success";
			break;
		case EC_NULL_READ_GDP:
			return "Null GDP found for loading EMP data. Set this using Geo2Emp::setGdpIn().";
			break;
		case EC_NULL_WRITE_GDP:
			return "Null GDP found for writing EMP data. Set this using Geo2Emp::setGdpOut().";
			break;
		case EC_NAIAD_EXCEPTION:
			return _niException;
			break;
		case EC_NO_TRIANGLE_SHAPE:
			return "The EMP body does not have a triangle shape.";
			break;
		case EC_NO_PARTICLE_SHAPE:
			return "The EMP body does not have a particle shape.";
			break;
		case EC_NO_FIELD_SHAPE:
			return "The EMP body does not have a field shape.";
			break;
		case EC_NOT_YET_IMPLEMENTED:
			return "Not yet implemented";
			break;
		default:
			return "Unknown error.";
	}

	return "Unknown error.";
}

/**************************************************************************************************/

Geo2Emp::Geo2Emp() :
	_out( _outnull.rdbuf() ),
	_logLevel(LL_INFO),
	_gdp(0),
	_bodyName("unnamed_body"),
	_startFrame(0),
	_endFrame(0),
	_initFrame(0),
	_fps(24),
	_decimation(0),
	_time(0),
	_overrideEmpTime(false),
	_framepadding(4)
{
	_geoAttribMangle["P"] = "position";
	_geoAttribMangle["N"] = "normal";
	_geoAttribMangle["v"] = "velocity";

	_empAttribMangle["position"] = "P";
	_empAttribMangle["normal"] = "N";
	_empAttribMangle["velocity"] = "v";
}

/**************************************************************************************************/

Geo2Emp::~Geo2Emp()
{

}

/**************************************************************************************************/

void Geo2Emp::redirect(ostream &os)
{
 _out.rdbuf( os.rdbuf() );
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadEmpBodies(std::string filen, int frame)
{
	Nb::EmpReader* empReader = NULL;	

	unsigned int numBodies = 0;
	try
	{
		Nb::String ngfilen = Nb::String(filen);
                const Nb::String empFilename = 
                    Nb::sequenceToFilename(
                        "", ngfilen, frame, 0, _framepadding
                        );
		empReader = new Nb::EmpReader(empFilename, "*");
		numBodies = empReader->bodyCount();
	}
	catch( std::exception& e )
	{
		//TODO: generate a proper error / exit code for Houdini. 
		// return exit code?
		_niException = e.what();

		return EC_NAIAD_EXCEPTION;
	}

	//Run through each body and process them according to their shape type.
	const Nb::Body* pBody = 0;
	Nb::Body::ConstShapeMapIter shapeIt;
	for (int i = 0; i < numBodies; i++)
	{
		pBody = empReader->cloneBody(i);
		//std::cout << i << ":" << pBody->name() << std::endl;
		//std::cout << "number of shapes: " << pBody->shape_count() << std::endl;
		/*	
		std::cout << "body Mesh: " << pBody->matches("Mesh") << std::endl;
		std::cout << "body Particle: " << pBody->matches("Particle") << std::endl;
		std::cout << "body Particle-Liquid: " << pBody->matches("Particle-Liquid") << std::endl;
		std::cout << "body Field: " << pBody->matches("Field") << std::endl;
		std::cout << "body Volume: " << pBody->matches("Volume") << std::endl;
		*/

		//Inspect the body singature and call the appropriate loader.
		
		if ( (_typeMask & BT_PARTICLE) && pBody->matches("Particle") )
		{
			loadParticleShape(pBody);
		}
		if ((_typeMask & BT_FIELD) && pBody->matches("Field") )
		{
			loadFieldShape(pBody);
		}
		if ( (_typeMask & BT_MESH) && pBody->matches("Mesh") )
		{
			loadMeshShape(pBody);
		}
	}

	if ( empReader )
		delete empReader;

	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::saveEmp()
{
	std::string frameinput, frameoutput;

	LogDebug() << "Saving EMP with data: " << std::endl;
	LogDebug() << "Startframe: " << _startFrame << std::endl;
	LogDebug() << "Endframe: " << _endFrame << std::endl;
	LogDebug() << "FPS: " << _fps << std::endl;
	LogDebug() << "initframe: " << _initFrame << std::endl;
	LogDebug() << "framepadding: " << _framepadding << std::endl;

        bool error=false;

  // Initialize the Naiad Base library only (we don't need the full
  // Naiad Interface here - besides, it could consume an expensive
  // Naiad software license! :-)
  Nb::begin();

	//Perform sequence conversion evaluation
	if ( pystring::find( _inputFile, "#" ) != -1 || pystring::find( _outputFile, "#" ) != -1 )
	{
		int sf = _startFrame;
		int ef = _endFrame;
		float time;
		bool stdinput = false;
		//Do a sequence conversion
		if (sf > ef)
		{
			ef = _startFrame;
			sf = _endFrame;
		}

		for (int i = sf; i <= ef; i++)
		{
			GU_Detail* pGdp = new GU_Detail();
			//Create and a set a local gdp object
			setGdp( pGdp );

			frameinput = pystring::replace(_inputFile, "#", intToString(i, _framepadding));
			frameoutput = pystring::replace(_outputFile, "#", intToString(i, _framepadding));
			//Make sure the input file exists
			stdinput = frameinput.compare("stdin.bgeo") == 0;
			if ( stdinput || (!stdinput) && fileExists(frameinput) )
			{
				LogInfo() << "Converting frame " << i << " from range " << sf << "-" << ef << std::endl;
				LogVerbose() << " converting : " << frameinput << " to " << frameoutput << std::endl;	

				//Load the bgeo data
				_gdp->load( frameinput.c_str(), 0 );

				//time = (i - (_startFrame-_initFrame-1))/_fps; 
				time = ( i - _initFrame ) / _fps; 
				LogDebug() << " frame: " << i << " time: " << time << std::endl;

				saveEmpBodies( frameoutput, i, time );
			}
			else
			{
				//The input file could not be located on disk.
                                LogInfo() << "Error: Input file was not found - " << frameinput << std::endl;                                
                                error = true;
			}
			setGdp(0);
			delete pGdp;
		}
	}
	else
	{
		LogDebug() << "straight convert : " << _inputFile << " to " << _outputFile << std::endl;			
		bool stdinput = _inputFile.compare("stdin.bgeo") == 0;

		if ( stdinput || (!stdinput) && fileExists(_inputFile) )
		{
			GU_Detail* pGdp = new GU_Detail();
			
			LogVerbose() << " converting : " << _inputFile << " to " << _outputFile << std::endl;	

			setGdp( pGdp );
			LogVerbose() << " Gdp has been set. " << std::endl;	
			_gdp->clearAndDestroy();
			LogVerbose() << " Gdp is being loaded. " << std::endl;	
			//Load the bgeo data
			_gdp->load( _inputFile.c_str(), 0 );
			LogVerbose() << " Saving Emp Bodies. " << std::endl;	
			//Just pass zero as the frame since it is not a sequence.
			saveEmpBodies( _outputFile, 0, _time );

			setGdp( 0 );
			delete pGdp;
		}
		else
		{
			//The input file could not be located on disk.
			LogInfo() << "Error: Input file was not found - " << frameinput << std::endl;
                        error = true;
		}
		//Do a straight conversion
	}

  Nb::end();

        if(error)
            return EC_GENERAL_ERROR;

	LogVerbose() << "Conversion completed..." << std::endl;

	return EC_SUCCESS;
}


/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::saveGeo()
{
	std::string frameinput, frameoutput;

	LogDebug() << "Saving EMP with data: " << std::endl;
	LogDebug() << "Startframe: " << _startFrame << std::endl;
	LogDebug() << "Endframe: " << _endFrame << std::endl;
	LogDebug() << "FPS: " << _fps << std::endl;
	LogDebug() << "initframe: " << _initFrame << std::endl;
	LogDebug() << "framepadding: " << _framepadding << std::endl;

        bool error=false;

  // Initialize the Naiad Base library only (we don't need the full
  // Naiad Interface here - besides, it could consume an expensive
  // Naiad software license! :-)
  Nb::begin();

	//Perform sequence conversion evaluation
	if ( pystring::find( _inputFile, "#" ) != -1 || pystring::find( _outputFile, "#" ) != -1 )
	{
		int sf = _startFrame;
		int ef = _endFrame;
		bool stdinput = false;
		//Do a sequence conversion
		if (sf > ef)
		{
			ef = _startFrame;
			sf = _endFrame;
		}

		//Create and a set a local gdp object


		for (int i = sf; i <= ef; i++)
		{
			GU_Detail* pGdp = new GU_Detail();
			setGdp( pGdp );

			frameinput = pystring::replace(_inputFile, "#", intToString(i, _framepadding));
			frameoutput = pystring::replace(_outputFile, "#", intToString(i, _framepadding));
			//Make sure the input file exists
			stdinput = frameinput.compare("stdin.bgeo") == 0;
			if ( stdinput || (!stdinput) && fileExists(frameinput) )
			{
				LogInfo() << "Converting frame " << i << " from range " << sf << "-" << ef << std::endl;
				LogVerbose() << " converting : " << frameinput << " to " << frameoutput << std::endl;	

				LogDebug() << " frame: " << i << std::endl;

				//Load emp data into the current gdp
				_gdp->clearAndDestroy();
				loadEmpBodies( frameinput, i );
				//Save out the gdp
				//TODO: add a variable to Geo2Emp to allow controlling this binary flag
				_gdp->save( frameoutput.c_str(), 1, 0 );

				setGdp(0);
				delete pGdp;
			}
			else
			{
				//The input file could not be located on disk.
				LogInfo() << "Error: Input file was not found - " << frameinput << std::endl;
                                error = true;
			}
		}

	}
	else
	{
		//Perform a single file conversion. No sequence.
		LogInfo() << "straight convert : " << _inputFile << " to " << _outputFile << std::endl;			
		bool stdinput = _inputFile.compare("stdin.bgeo") == 0;
		LogInfo() << "stdinput? " << stdinput << std::endl;			

		if ( stdinput || (!stdinput) && fileExists(_inputFile) )
		{
			GU_Detail* pGdp = new GU_Detail();
			
			LogVerbose() << " converting : " << _inputFile << " to " << _outputFile << std::endl;	

			setGdp( pGdp );

			//Load the EMP data into the current gdp
			//Just pass zero as the frame since it is not a sequence.
			loadEmpBodies( _inputFile, 0);

			//now save the gdp to disk
			_gdp->save( _outputFile.c_str(), 1, 0 );

			setGdp( 0 );
			delete pGdp;
		}
		else
		{
			//The input file could not be located on disk.
			LogInfo() << "Error: Input file was not found - " << frameinput << std::endl;
                        error = true;
		}
		//Do a straight conversion
	}

  Nb::end();

        if(error)
            return EC_GENERAL_ERROR;

	return EC_SUCCESS;
}


/**************************************************************************************************/
Geo2Emp::ErrorCode Geo2Emp::saveEmpBodies(std::string empfile, int frame, float time)
{
	LogInfo() << "Saving EMP Bodies" << std::endl;

	LogDebug() << "particle count: " << _gdp->particleCount() << std::endl;
	LogDebug() << "paste count: " << _gdp->pasteCount() << std::endl;
	LogDebug() << "quadric count: " << _gdp->quadricCount() << std::endl;

	std::cout << "creating EmpWriter..." << empfile << std::endl;

	//Construct the EMP Writer and add bodies as they get processed.
	Nb::EmpWriter empWriter("", empfile, frame, (1.0f/_fps), _framepadding, time);

	std::cout << "Done with emp writer." << std::endl;

	if (_typeMask & BT_MESH)
	{
		LogVerbose() << "Writing Mesh to EMP!" << std::endl;	
		//Attempt to extract meshes from the current GDP
		//If we have more than just particle primitives it means that we most likely have a mesh that we're dealing with
		Nb::Body* pBody = 0;
		std::list<Nb::Body*> bodyList;
		saveMeshShape( bodyList );
		for (std::list<Nb::Body*>::iterator bodyIt = bodyList.begin(); bodyIt != bodyList.end(); bodyIt++)
		{
			if (*bodyIt)
			{
				//Make sure all the emp particles are sorted into the correct blocks
                                (*bodyIt)->update(Nb::ZeroTimeBundle);
				empWriter.write( (*bodyIt), Nb::String("*.*"));

				//Question: Can the bodies be deleted directly after a write, or does the writer need to be closed first?
				delete (*bodyIt);
			}
		}
	}

	if ( _typeMask & BT_PARTICLE )
	{	
		LogVerbose() << "Writing Particles to EMP!" << std::endl;	
		//Attempt to extract particles from the current GDP
		//Note that all points in the GDP will be copied into an EMP as particles, regardless of whether they 
		// are Houdini particles or not. 		
		Nb::Body* pBody = 0;
		saveParticleShape( pBody );
		if (pBody)
		{
			//Make sure all the emp particles are sorted into the correct blocks
                        pBody->update(Nb::ZeroTimeBundle);
			empWriter.write(pBody, Nb::String("*.*"));

			//Question: Can the bodies be deleted directly after a write, or does the writer need to be closed first?
			delete pBody;
			pBody = 0;
		}
	}

	//All done
	empWriter.close();
	
	if ( _typeMask & BT_FIELD )
	{
		LogInfo() << "Unsupported shape type!" << std::endl;	
                return EC_NOT_YET_IMPLEMENTED;
	}

	//Return success
	return EC_SUCCESS;
}

/**************************************************************************************************/

void Geo2Emp::buildTriPrimNamesMap(StringToPrimPolyListMap& namesMap)
{
	//Iterate over the gdp and map geometry (triangle) primitives to their name attributes
	
	const GEO_PrimList& prims = _gdp->primitives();
	const GEO_Primitive* pprim;
	GEO_AttributeHandle attrName;
	bool hasName = true;
	UT_String primName;

	attrName = _gdp->getPrimAttribute("name");
	if ( !attrName.isAttributeValid() )
		hasName = false;

	int numprims = prims.entries();

	for (int i = 0; i < numprims; i++)
	{
		pprim = prims[i];
		if (! dynamic_cast<const GEO_PrimPoly*>( pprim ) )
			//If we don't have a GEO_PrimPoly, then bail
			continue;

		//If we have a GEO_PrimPoly, get its name attribute
		if (hasName)
		{
			attrName.setElement( pprim );
			attrName.getString( primName );
			namesMap[ primName.toStdString() ].push_back( pprim );
		}
		else
		{
			namesMap[ _bodyName ].push_back( pprim );
		}		
	}
}

/**************************************************************************************************/

