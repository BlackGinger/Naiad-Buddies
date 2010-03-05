// -----------------------------------------------------------------------------
//
// bgeo2emp.cc
//
// .bgeo to .emp converter
//
// Copyright (c) 2009 BlackGinger.  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of BlackGinger nor its contributors may be used to
//   endorse or promote products derived from this software without specific 
//   prior written permission. 
// 
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT 
//    LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
//    FOR  A  PARTICULAR  PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL THE
//    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//    BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS  OR  SERVICES; 
//    LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER
//    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT
//    LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN
//    ANY  WAY OUT OF THE USE OF  THIS SOFTWARE,  EVEN IF ADVISED OF  THE
//    POSSIBILITY OF SUCH DAMAGE.
//
//
// -----------------------------------------------------------------------------


#include "geo2emp.h"

#include <stdio.h>
#include <iostream>

#include <CMD/CMD_Args.h>

static void
usage(const char *program)
{
    cerr << "Usage: " << program << " sourcefile dstfile\n";
    cerr << "The extension of the source/dest will be used to determine" << endl;
    cerr << "how the conversion is done.  Supported extensions are .emp" << endl;
    cerr << "and .bgeo" << endl;
}

/**************************************************************************************************/

int main(int argc, char *argv[])
{
	std::cout << "creating new object and redirecting output..." << std::endl;
	Geo2Emp geo2emp;

	//geo2emp._out << "loggin throught default stream..." << std::endl;
	//geo2emp.redirect( std::cerr );
	//geo2emp._out << "loggin throught redirected stream..." << std::endl;

	return 0;
}

/**************************************************************************************************/

int main_old(int argc, char *argv[])
{
	CMD_Args		 args;
	GU_Detail		 gdp;
	
	args.initialize(argc, argv);

	if (args.argc() != 3)
	{
		usage(argv[0]);
		return 1;
	}

	UT_String	inputname, outputname;

	inputname.harden(argv[1]);
	outputname.harden(argv[2]);
	
// 	std::cout << "args: " << inputname << "," << outputname << std::endl;

	if (!strcmp(inputname.fileExtension(), ".emp"))
	{
		//std::cout << "Loading emp: " << inputname << std::endl;
		// Convert from emp
		loadEmpBodies(inputname.toStdString(), gdp);
		
		//std::cout << "Writing file: " << outputname << std::endl;
		// Save our result.
		gdp.save((const char *) outputname, 0, 0);
  }
  else
  {
		// Convert to emp.
		gdp.load((const char *) inputname, 0);

		//std::cout << "Writing file: " << outputname << std::endl;
		saveEmpBodies(outputname.toStdString(), gdp);
   }
   return 0;
}
