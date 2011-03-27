/*
 * cli_geo2emp.C
 *
 * command line frontend for converting BGEO to EMP files
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
#include "anyoption.h"
#include "pystring.h"

#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <CMD/CMD_Args.h>

using namespace geo2emp;

/**************************************************************************************************/

/*
 * Initialise the commandline option parser
 */
void processOpts(AnyOption& opt)
{
	
	opt.addUsage("Naiad Buddy for Houdini toolkit - command line geo2emp converter");
	opt.addUsage("");
	opt.addUsage("Usage: geo2emp [options] input.bgeo output.emp");
	opt.addUsage("Usage: geo2emp [options] -s startframe -e endframe input.#.bgeo output.#.emp");
	opt.addUsage("");
  opt.addUsage("  This tool is used to convert BGEO files to EMP files. The input");
	opt.addUsage("  will be regarded as a BGEO file and the output will be regarded");
	opt.addUsage("  as an EMP file regardless of their extensions.");
	opt.addUsage("  Sequence conversion is also supported.");
	opt.addUsage("");
	opt.addUsage("\t-h\t--help\t\t Prints this help");
	opt.addUsage("\t-v\t--verbose [lvl]\t Verbosity level:");
	opt.addUsage("\t\t\t\t     0 - Stealth mode (supress output)");
	opt.addUsage("\t\t\t\t     1 - Informative (default)");
	opt.addUsage("\t\t\t\t     2 - Talkative");
	opt.addUsage("\t\t\t\t     3 - Debug");
	opt.addUsage("\t-p\t--particles\t Interpret bgeo data as particles");
	opt.addUsage("\t-m\t--mesh\t\t Interpret bgeo data as one or more meshes");
	opt.addUsage("\t-f\t--field\t\t Interpret bgeo data as volume data");
	opt.addUsage("\t-s\t--startframe\t Start frame of the sequence");
	opt.addUsage("\t-e\t--endframe\t End frame of the sequence");
	opt.addUsage("\t-i\t--initframe\t Frame that is considered zero time for Naiad sim (default: 0)");
	opt.addUsage("\t-r\t--framerate\t Frame rate for the file sequence (default: 24)");
	opt.addUsage("\t-d\t--pad\t\t Frame number padding (default: 4)");
	opt.addUsage("\t-t\t--time\t\t Timestep of the EMP file (only use this if you know what you are doing)");
	opt.addUsage("\t-b\t--bodyname\t Force to use this body name (default: trimesh)");
	opt.addUsage("");


	//opt->setOption( "verbose", 'v' );

	/* by default all  options  will be checked on the command line and from option/resource file */
	opt.setOption( "verbose", 'v' );
	opt.setOption( "startframe", 's' );
	opt.setOption( "endframe", 'e' );
	opt.setOption( "initframe", 'i' );
	opt.setOption( "framerate", 'r' );
	opt.setOption( "pad", 'd' );	
	opt.setOption( "time", 't' );
	opt.setOption( "bodyname", 'b' );        
  opt.setFlag( "help", 'h' );   /* a flag (takes no argument), supporting long and short form */ 
	opt.setFlag( "particles", 'p' ); 
	opt.setFlag( "mesh", 'm' ); 
	opt.setFlag( "field", 'f' );
}
/**************************************************************************************************/

#include <IMG/IMG_Format.h>
#include <IMG/IMG_File.h>
#include <PXL/PXL_Common.h>

int main(int argc, char *argv[])
{
	Geo2Emp geo2emp;
	AnyOption opt;
	int shapeMask = 0;
	bool seqconv = false;
	char pwd[BUFSIZ];

	
	processOpts(opt);

	opt.processCommandArgs(argc, argv);
	
	//If we don't have exactly two args (input & output file name) then bug out.
	if (! opt.hasOptions() || opt.getArgc() != 2) 
	{ 
		/* print usage if no options */
 		opt.printUsage();
		return 0;
	}
	
	if( opt.getFlag( "help" ) || opt.getFlag( 'h' ) ) 
	{
		opt.printUsage();
		return 0;
	}

	if ( opt.getValue("verbose") != NULL || opt.getValue( 'v' ) != NULL )
	{
		istringstream intstream( opt.getValue('v') );
		int i;
		if (intstream >> i)
		{
			geo2emp.setLogLevel( (Geo2Emp::LogLevel)i );
		}
	}

	if ( opt.getValue("bodyname") != NULL || opt.getValue( 'b' ) != NULL )
	{
		geo2emp.setBodyName( opt.getValue('b') );
	}

	if ( opt.getValue("particle") != NULL || opt.getValue( 'p' ) != NULL )
	{
		shapeMask |= Geo2Emp::BT_PARTICLE;
	}

	if ( opt.getValue("mesh") != NULL || opt.getValue( 'm' ) != NULL )
	{
		shapeMask |= Geo2Emp::BT_MESH;
	}

	if ( opt.getValue("field") != NULL || opt.getValue( 'f' ) != NULL )
	{
		shapeMask |= Geo2Emp::BT_FIELD;
	}

	if ( opt.getValue("startframe") != NULL || opt.getValue( 's' ) != NULL )
	{
		geo2emp.setStartFrame( stringToInt( opt.getValue('s') ) );
	}
	if ( opt.getValue("endframe") != NULL || opt.getValue( 'e' ) != NULL )
	{
		geo2emp.setEndFrame( stringToInt( opt.getValue('e') ) );
	}
	if ( opt.getValue("framerate") != NULL || opt.getValue( 'r' ) != NULL )
	{
		geo2emp.setFrameRate( stringToFloat( opt.getValue('r') ) );
	}
	if ( opt.getValue("initframe") != NULL || opt.getValue( 'i' ) != NULL )
	{
		geo2emp.setInitialFrame( stringToInt( opt.getValue('i') ) );
	}
	if ( opt.getValue("time") != NULL || opt.getValue( 't' ) != NULL )
	{
		geo2emp.setEmpTime( stringToFloat( opt.getValue('t') ) );
	}
	if ( opt.getValue("pad") != NULL || opt.getValue( 'd' ) != NULL )
	{
		//istringstream padstream( opt.getValue('d') );
		//padstream >> pad;
		geo2emp.setFramePadding( stringToInt( opt.getValue('d') ) );
	}

	geo2emp.setTypeMask( shapeMask );

	geo2emp.redirect( std::cerr );

	UT_String	inputname, outputname, lowerin, lowerout;

	inputname.harden(opt.getArgv(0));
	outputname.harden(opt.getArgv(1));
	lowerout = outputname;
	lowerout.toLower();

	//If the input or ouput name has a hash, then we enable sequence conversion
	if ( pystring::find( inputname.toStdString(), "#" ) != -1 || pystring::find( outputname.toStdString(), "#" ) != -1 )
	{
		seqconv = true;
	}

	//Do some parm integrity checks
	if (shapeMask == 0)
	{
		//Require, for now, that a conversion needs to be specified.
		geo2emp.LogInfo() << "No conversion mode found. Please specify the conversion mode using -p, -m, or -f." << std::endl;
		return 1;
	}

	// Naiad's EmpReader and EmpWriter classes require absolute paths from 0.96. Check whether
	// lowerin and lowerout start with /, if not prefix it with the current working directory
	// TODO: test whether this is still the case with Naiad 0.2
	getcwd(pwd, BUFSIZ);
	if ( ( lowerin != "stdin.bgeo" ) && ( !inputname.startsWith("/") ) )
	{
		// Not an absolute path, prefix current working directory
		inputname.prepend("/");
		inputname.prepend(pwd);
	}
	if ( ( lowerout != "stdout.bgeo" ) && ( !outputname.startsWith("/") ) )
	{
		// Not an absolute path, prefix current working directory
		outputname.prepend("/");
		outputname.prepend(pwd);
	}

	geo2emp.LogInfo() << "Absolute input path: " << inputname << std::endl;
	geo2emp.LogInfo() << "Absolute output path: " << outputname << std::endl;

	lowerin = inputname;
	lowerin.toLower();
	lowerout = outputname;
	lowerout.toLower();

	//File existence checks will be done during sequence conversion
	/*
	if (not inputname.match("stdin.bgeo"))
	{
		//Check whether the input filename actually exist
		ifstream inp;
		inp.open( inputname );
		inp.close();
		if (inp.fail())
		{
			geo2emp.LogInfo() << "Error: The input file does not exist: " << inputname << std::endl;
		}

	}
	*/

	geo2emp.setInputFilename( inputname.toStdString() );
	geo2emp.setOutputFilename( outputname.toStdString() );
	//Save EMP file(s) using the current geo2emp state.
        Geo2Emp::ErrorCode ec = geo2emp.saveEmp();

		// Convert to emp.
		/*
		geo2emp.LogInfo() << "Loading GDP: " << inputname << std::endl;
		int result = gdp.load((const char *) inputname, 0);

		if (result >= 0)
		{
			geo2emp.LogInfo() << "Saving EMP: " << outputname << std::endl;
			//geo2emp.saveEmpBodies(outputname.toStdString(), time, frame, pad, shapeMask);
		}
		else
		{
			geo2emp.LogInfo() << "Error occured while loading BGEO. Does the file exist? " << inputname.toStdString() << std::endl;
			return 1;
		}
		*/

	return ec;
}

