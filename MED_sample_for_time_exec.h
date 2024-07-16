
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2020

#ifndef MED_SAMPLE_FOR_TIME_EXEC_IN
#define MED_SAMPLE_FOR_TIME_EXEC_IN

// Includes
#include "medlib_m12.h"

// Defines

// Version
#define MED_SAMPLE_FOR_TIME_VER_MAJOR		((ui1) 1)
#define MED_SAMPLE_FOR_TIME_VER_MINOR		((ui1) 0)

// Miscellaneous
#define MAX_CHANNELS                        	512


// Prototypes
void	mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
si8	get_si8_scalar(const mxArray *mx_arr);
si8	MED_sample_for_time(si8 time, si1 *MED_directory, si1 *password);


#endif /* MED_SAMPLE_FOR_TIME_EXEC_IN */
