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

#endif //geo2emp
