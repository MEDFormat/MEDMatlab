
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2020

#ifndef DELETE_RECORD_EXEC_IN
#define DELETE_RECORD_EXEC_IN

// Includes
#include "medlib_m12.h"


// Defines

// Version
#define LS_READ_MED_VER_MAJOR		((ui1) 1)
#define LS_READ_MED_VER_MINOR		((ui1) 1)


// Prototypes
void	mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
si8	get_si8_scalar(const mxArray *mx_arr);
sf8	delete_record(si1 *chan_dir, si1 *password, si8 rec_time, ui4 rec_type);


#endif /* DELETE_RECORD_EXEC_IN */
