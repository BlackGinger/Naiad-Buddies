/*
 * main.C
 *
 * command line frontend to the .bgeo to .emp converter
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
#include "anyoption.h"

#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include <CMD/CMD_Args.h>

/**************************************************************************************************/

/*
 * Initialise the commandline option parser
 */
void processOpts(AnyOption& opt)
{
	
	opt.addUsage("Naiad Buddy for Houdini toolkit - geo2emp converter");
	opt.addUsage("");
	opt.addUsage("Usage: geo2emp [options] source dest");
	opt.addUsage("");
  opt.addUsage("  The extension of the source/dest will be used to determine");
	opt.addUsage("  how the conversion is done.  Supported extensions are .emp");
	opt.addUsage("  and .bgeo");
	opt.addUsage("");
	opt.addUsage("\t-h\t--help\t\t Prints this help");
	opt.addUsage("\t-v\t--verbose [lvl]\t Verbosity level:");
	opt.addUsage("\t\t\t\t     0 - Stealth mode");
	opt.addUsage("\t\t\t\t     1 - Informative");
	opt.addUsage("\t\t\t\t     2 - Talkative");
	opt.addUsage("\t\t\t\t     3 - Debug");
	opt.addUsage("\t-p\t--particles\t Load particle shape");
	opt.addUsage("\t-m\t--mesh\t\t Load mesh shape");
	opt.addUsage("\t-f\t--field\t\t Load field shape");
	opt.addUsage("\t-t\t--time\t\t Timestep of file (only relevent for EMP files)");
	opt.addUsage("\t-n\t--frame\t\t Frame of file");
	opt.addUsage("\t-d\t--pad\t\t Frame number zero-padding of file");
	opt.addUsage("\t-b\t--bodyname\t\t Force to use this body name (default: trimesh)");
	opt.addUsage("");


	//opt->setOption( "verbose", 'v' );

	/* by default all  options  will be checked on the command line and from option/resource file */
	opt.setOption( "verbose", 'v' );
	opt.setOption( "time", 't' );
	opt.setOption( "frame", 'n' );
	opt.setOption( "pad", 'd' );	
	opt.setOption( "bodyname", 'b' );        
  opt.setFlag( "help", 'h' );   /* a flag (takes no argument), supporting long and short form */ 
	opt.setFlag( "particles", 'p' ); 
	opt.setFlag( "mesh", 'm' ); 
	opt.setFlag( "field", 'f' );
}
/**************************************************************************************************/

int main(int argc, char *argv[])
{
	Geo2Emp geo2emp;
	AnyOption opt;
	GU_Detail gdp;
	int shapeMask = 0;
	bool loadAllShapes = true;
	float time = 0;
	int frame = 1;
	int pad = 0;
	char pwd[BUFSIZ];

	geo2emp.setGdpIn(&gdp);
	geo2emp.setGdpOut(&gdp);
	
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
		loadAllShapes = false;
		shapeMask |= Geo2Emp::BT_PARTICLE;
	}


	if ( opt.getValue("mesh") != NULL || opt.getValue( 'm' ) != NULL )
	{
		loadAllShapes = false;
		shapeMask |= Geo2Emp::BT_MESH;
	}

	if ( opt.getValue("field") != NULL || opt.getValue( 'f' ) != NULL )
	{
		loadAllShapes = false;
		shapeMask |= Geo2Emp::BT_FIELD;
	}

	if ( opt.getValue("frame") != NULL || opt.getValue( 'n' ) != NULL )
	{
		istringstream framestream( opt.getValue('n') );
		framestream >> frame;
	}
	if ( opt.getValue("pad") != NULL || opt.getValue( 'd' ) != NULL )
	{
		istringstream padstream( opt.getValue('d') );
		padstream >> pad;
	}

	geo2emp.redirect( std::cerr );

	UT_String	inputname, outputname, lowerin, lowerout;

	inputname.harden(opt.getArgv(0));
	outputname.harden(opt.getArgv(1));

	if (loadAllShapes)
	{
		shapeMask = 0xFFFFFFFF;
	}

	lowerin = inputname;
	lowerin.toLower();
	lowerout = outputname;
	lowerout.toLower();

	// Naiad's EmpReader and EmpWriter classes require absoulte paths from 0.96. Check whether
	// lowerin and lowerout start with /, if not prefix it with the current working directory
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

	if ( ! (lowerin.endsWith(".emp") || lowerin.endsWith(".geo") || lowerin.endsWith(".bgeo") ) )
	{
		geo2emp.LogInfo() << "Unrecognized extension for source file: " << inputname << std::endl;
		return 1;
	}
	else if (not inputname.match("stdin.bgeo"))
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

	if ( ! (lowerout.endsWith(".emp") || lowerout.endsWith(".geo") || lowerout.endsWith(".bgeo") ) )
	{
		geo2emp.LogInfo() << "Unrecognized extension for destination file: " << outputname << std::endl;
		return 1;
	}

	if ( lowerin.endsWith(".emp") )
	{
		geo2emp.LogInfo() << "Loading EMP: " << inputname << std::endl;
		// Convert from emp (by default, convert all naiad body types to bgeo)
		geo2emp.loadEmpBodies( inputname.toStdString(), frame, pad, shapeMask );
	
		geo2emp.LogInfo() << "Saving GDP: " << outputname << std::endl;
		//std::cout << "Writing file: " << outputname << std::endl;
		// Save our result.
		gdp.save((const char *) outputname, 0, 0);
  }
  else if (lowerin.endsWith(".geo") || lowerin.endsWith(".bgeo") )
  {
		if ( opt.getValue("time") != NULL || opt.getValue( 't' ) != NULL )
		{
			istringstream timestream( opt.getValue('t') );
			timestream >> time;
			geo2emp.LogInfo() << "Time is set to: " << time << std::endl;
		}

		// Convert to emp.
		geo2emp.LogInfo() << "Loading GDP: " << inputname << std::endl;
		gdp.load((const char *) inputname, 0);

		geo2emp.LogInfo() << "Saving EMP: " << outputname << std::endl;
		geo2emp.saveEmpBodies(outputname.toStdString(), time, frame, pad, shapeMask);
   }
	else
	{
		geo2emp.LogInfo() << "Unrecognized extension for source file: " << inputname << std::endl;
		geo2emp.LogInfo() << "How did you get to this point anyways? You should've been kicked out at the extension integrity checks already!" << std::endl;
		return 1;
	}

	return 0;
}

