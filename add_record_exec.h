
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2020

#ifndef ADD_RECORD_EXEC_IN
#define ADD_RECORD_EXEC_IN


// Defines

// Version
#define LS_READ_MED_VER_MAJOR		((ui1) 1)
#define LS_READ_MED_VER_MINOR		((ui1) 1)


// Prototypes
void	mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
si8	get_si8_scalar(const mxArray *mx_arr);
sf8	add_record(si1 *chan_dir, si1 *password, si8 rec_time, si1 *note_text, si1 enc_level);


#endif /* ADD_RECORD_EXEC_IN */
