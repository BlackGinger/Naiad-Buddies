/*
 * geo2emp_utils.h
 *
 * Utility functions
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

#ifndef __GEO2EMP_UTILS_H__
#define __GEO2EMP_UTILS_H__

#include <string>

namespace geo2emp
{
	std::string intToString(int i, int padding=0);

	int stringToInt(const char* s);

	int stringToInt(const std::string& s);
	
	float stringToFloat(const char* s);
	
	float stringToFloat(const std::string& s);

	/**
	 * Test whether the specified file exists or not.
	 */
	bool fileExists(const std::string& file);

	/**
	 * Clamping functions. If the given value is outside the range, the value is set as the boundary value.
	 */
	template <class T>
	T clamp(T value, T min, T max)
	{
		if (value < min)
			return min;
		if (value > max)
			return max;
		return value;
	}

	/** Templated to-string conversion
	 */

	template <class T>
	std::string toString(const T& val)
	{
		std::ostringstream sstream;
		sstream << val;
		return sstream.str();
	}

}

#endif //__GEO2EMP_UTILS_H__

