/**
 * geo2emp
 *
 */

#ifndef __GEO2EMP_H__
#define __GEO2EMP_H__

// To fix issues with gcc and GB_GenericData.h with inline templated function specialisation storage
// This is needed with GCC 4.1, but not with GCC 4.3
#define NEED_SPECIALIZATION_STORAGE

#include <iostream>

#include <GU/GU_Detail.h>


/**
 * Converts EMP data to BGEO data
 */

int loadEmpBodies(std::string empfile, GU_Detail& gdp);

/**
 * Converts BGEO data to EMP data.
 */
int saveEmpBodies(std::string empfile, GU_Detail& gdp);


// Code below this is line is being written for the more Object Orient command line tool

#define LOG(loglevel) if ( (loglevel) <= _logLevel ) _out
#define VERBOSE LOG(LL_VERBOSE)
#define DEBUG LOG(LL_DEBUG)

/**
 * Converts between Houdini BGEO and Naiad EMP format
 */
class Geo2Emp
{
	public:

		enum LogLevel
		{
			LL_SILENCE = -1,
			LL_INFO = 0,
			LL_VERBOSE,
			LL_DEBUG,
		};

		/**
		 * Default constructor
		 */
		Geo2Emp();
		/** 
		 * Default destructor
		 */
		~Geo2Emp();

		void setLogLevel(LogLevel level) { _logLevel = level; }

		/** 
		 * Direct output to specified ostream. Default is std::cout.
		 */
		void redirect(ostream &os);


	protected:

		int _logLevel;
		/** This is the internal logging stream for the class */
		std::ostream& _out;
};


#endif //geo2emp
