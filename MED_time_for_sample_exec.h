
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2020

#ifndef MED_TIME_FOR_SAMPLE_EXEC_IN
#define MED_TIME_FOR_SAMPLE_EXEC_IN

// Includes
#include "medlib_m12.h"

// Defines

// Version
#define MED_TIME_FOR_SAMPLE_VER_MAJOR		((ui1) 1)
#define MED_TIME_FOR_SAMPLE_VER_MINOR		((ui1) 0)

// Miscellaneous
#define MAX_CHANNELS                        	512


// Prototypes
void		mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
mxArray		*get_si8_array(const mxArray *mx_in_arr);
mxArray		*MED_time_for_sample(mxArray *samples, si1 *MED_directory, si1 *password);


#endif /* MED_TIME_FOR_SAMPLE_EXEC_IN */
