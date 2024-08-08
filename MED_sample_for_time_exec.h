
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2020

#ifndef MED_SAMPLE_FOR_TIME_EXEC_IN
#define MED_SAMPLE_FOR_TIME_EXEC_IN

// Includes
#include "medlib_m12.h"

// Defines

// Version
#define READ_MED_VER_MAJOR	((ui1) 1)
#define READ_MED_VER_MINOR	((ui1) 0)

// Miscellaneous
#define MAX_CHANNELS		512


// Prototypes
void		mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
mxArray		*get_si8_array(const mxArray *mx_in_arr);
mxArray		*MED_sample_for_time(mxArray *times, si1 *MED_directory, si1 *password);


#endif /* MED_SAMPLE_FOR_TIME_EXEC_IN */
