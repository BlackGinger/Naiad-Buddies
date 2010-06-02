
#include "geo2emp_utils.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

/**************************************************************************************************/

std::string geo2emp::intToString(int i, int padding)
{
	std::ostringstream sstream;
	sstream << std::setw(padding) << std::setfill('0') << i;
	return sstream.str();
}

/**************************************************************************************************/

int geo2emp::stringToInt(const char* s)
{
	int result;
	std::istringstream timestream( s );
	timestream >> result;
	return result;
}

/**************************************************************************************************/

int geo2emp::stringToInt(const std::string& s)
{
	return geo2emp::stringToInt( s.c_str() );
}

/**************************************************************************************************/

float geo2emp::stringToFloat(const char* s)
{
	float result;
	std::istringstream timestream( s );
	timestream >> result;
	return result;
}

/**************************************************************************************************/

float geo2emp::stringToFloat(const std::string& s)
{
	return stringToFloat( s.c_str() );
}

/**************************************************************************************************/

bool geo2emp::fileExists(const std::string& filen)
{
	//Check whether the input filename actually exist
	bool result;
	std::ifstream inp;
	inp.open( filen.c_str(), std::ifstream::in );
	if ( inp.good() )
	{
		result = true;
	}
	else
	{
		result = false;
	}

	inp.close();
	return result;
}

