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

#ifndef __GEO2EMP_H__
#define __GEO2EMP_H__

// To fix issues with gcc and GB_GenericData.h with inline templated function specialisation storage
// This is needed with GCC 4.1, but not with GCC 4.3
#define NEED_SPECIALIZATION_STORAGE
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 3 ) 
	#undef NEED_SPECIALIZATION_STORAGE
#endif

#include <iostream>
#include <map>
#include <list>

#include <GU/GU_Detail.h>

#include <Ni.h>
#include <NgBody.h>

#include "nullstream.h"

namespace geo2emp
{


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
			LL_SILENCE = 0,
			LL_INFO = 1,
			LL_VERBOSE = 2,
			LL_DEBUG = 3,
		};

		enum ErrorCode
		{
			EC_SUCCESS = 0,
			EC_NULL_WRITE_GDP,
			EC_NULL_READ_GDP,
			EC_NAIAD_EXCEPTION,

			EC_NO_TRIANGLE_SHAPE,
			EC_NO_PARTICLE_SHAPE,
			EC_NO_FIELD_SHAPE,

			EC_NOT_YET_IMPLEMENTED,
		};

		enum EmpBodyTypes
		{
			BT_PARTICLE = 1,
			BT_MESH = 2,
			BT_FIELD = 4,
		};


		/**
		 * Default constructor
		 */
		Geo2Emp();

		/**
		 * Construct the object with in/out gdp contexts.
		 *
		 * It is safe to make any or either of the NULL. The loading/saving functions will
		 * handle this appropriately.
		 */
		Geo2Emp(GU_Detail* in, const GU_Detail* out);

		/** 
		 * Default destructor
		 */
		~Geo2Emp();

		void setLogLevel(LogLevel level) { _logLevel = level; }
		LogLevel getLogLevel() { return _logLevel; }

		/** 
		 * Get string representation of the error code
		 */
		std::string getErrorString(ErrorCode);

		/** 
		 * Direct output to specified ostream. Default is std::cout.
		 */
		void redirect(ostream &os);

		/** 
		 * Generic Logging function
		 */
		ostream& Log(LogLevel level) 
		{ 
			if ( (level) <= _logLevel ) 
				return _out; 
			else 
				return _outnull; 
		}
		
		ostream& LogInfo() { return Log(LL_INFO); }
		ostream& LogVerbose() { return Log(LL_VERBOSE); }
		ostream& LogDebug() { return Log(LL_DEBUG); }

		/**
		 * Set the GU_Detail object for reading/writing EMP data
		 */
		void setGdp(GU_Detail* gdp) { _gdp = gdp; }

		void setBodyName(std::string bname) { _bodyName = bname; }

		/**
		 * Set the input filename or sequence name
		 */
		void setInputFilename(const std::string input) { _inputFile = input; }

		/**
		 * Set the output filename or sequence name
		 */
		void setOutputFilename(const std::string output) { _outputFile = output; }

		void setStartFrame(int sf) { _startFrame = sf; }
		void setEndFrame(int ef) { _endFrame = ef; }
		void setFrameRate(float fps) { _fps = fps; }
		void setInitialFrame(int iframe) { _initFrame = iframe; }
		void setFramePadding(int padding) { _framepadding = padding; }
		void setTypeMask( unsigned int typeMask ) { _typeMask = typeMask; }

		/**
		 * Override sequence conversion timestep calculation. USE WITH CARE.
		 * 
		 * It should be noted that by overriding the time value for a sequence will result in all the
		 * EMP files having the same timestep value and will not simulate / interpolate correctly in Naiad.
		 * This function should only be used if you have a very good reason for explicitly setting the EMP time value.
		 */
		void setEmpTime(float t, bool overrideTime=true) { _time = t; _overrideEmpTime = overrideTime; }

		
		/**
		 * This function wraps the normal EMP save so that possible sequence conversion can take place.
		 */
		ErrorCode saveEmp();

		/**
		 * Load bodies from the EMP file. 
		 *
		 * @param filen The filename of the EMP file.
		 * @param types A bitset of EMP body type from enum EmpBodyTypes to define which bodies should be loaded.
		 *
		 */
		ErrorCode loadEmpBodies(std::string filen, int frame, int pad);

		/**
		 * Save the current GDP to an EMP file. 
		 *
		 * Saves a single EMP file with the given timestep. Rather use the saveEmp() that wraps this function.
		 *
		 * @param filen The filename of the EMP file.
		 * @param time The timestep to write into the EMP file.
		 * @param types A bitset of EMP body type from enum EmpBodyTypes to define which bodies should be loaded.
		 *
		 */
		ErrorCode saveEmpBodies(std::string filen, int frame, float time);

	protected:
		/** 
		 * Load shapes from EMP files. 
		 */
		ErrorCode loadMeshShape( const Ng::Body* pBody );
		ErrorCode loadParticleShape( const Ng::Body* pBody ); 
		ErrorCode loadFieldShape( const Ng::Body* pBody );

		/**
		 * Save shapes to EMP files.
		 */
		ErrorCode saveMeshShape_old( Ng::Body*& pBody );
		ErrorCode saveMeshShape( std::list<Ng::Body*>& pBody );
		ErrorCode saveParticleShape( Ng::Body*& pBody );
		ErrorCode saveFieldShape( Ng::Body*& pBody )  { return EC_NOT_YET_IMPLEMENTED; };

		/** 
		 * Utility functions
		 */
		typedef std::list<int> IntList;
		typedef std::list<const GEO_Primitive*> PrimPolyList;
		typedef std::map<std::string, PrimPolyList> StringToPrimPolyListMap;
		void buildTriPrimNamesMap(StringToPrimPolyListMap& namesMap);

		LogLevel _logLevel;
		/** This is the internal logging stream for the class */
		std::ostream _out;
		nullstream _outnull; 
		/** GDP used for reading from / writing to EMP files */
		GU_Detail* _gdp;

		std::string _inputFile;
		std::string _outputFile;
		int _framepadding;

		int _startFrame;
		int _endFrame;
		float _fps;
		int _initFrame;
	
		/** This is the body types to extract from the GDP */
		unsigned int _typeMask;

		/** Time value to write into EMP file(s). */
		float _time;
		/** This flags has to be set to true in order to override the calculated sequence timestep with the _time value */
		bool _overrideEmpTime;
		/** Use this as the bodyName, for now. This mechanism will be refined in the future. */
		std::string _bodyName;
		/** Storage of the last exception generated by Naiad. */
		std::string _niException;
};

} // namespace geo2emp

#endif //geo2emp

