/**
 * geo2emp
 *
 */

#ifndef __GEO2EMP_H__
#define __GEO2EMP_H__

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
