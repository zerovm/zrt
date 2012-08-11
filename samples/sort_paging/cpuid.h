/*
 * cpuid.h
 *
 *  Created on: 29.05.2012
 *      Author: YaroslavLitvinov
 */

#ifndef CPUID_H_
#define CPUID_H_

#include <stdint.h>

/*@return 0 if not available*/
int test_sse41_CPU();
/*@return 0 if not available*/
int test_sse4A_CPU();

#endif /* CPUID_H_ */
