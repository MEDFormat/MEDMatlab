
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2021


//******************************************** Mex Compile Line *****************************************//
//****  mex COMPFLAGS='$COMPFLAGS -Wall -O3' read_MED_exec.c medlib_m12.c medrec_m12.c dhnlib_m12.c  ****//
//*******************************************************************************************************//

// sample_numbers = MED_sample_for_time(times, MED_directory, [password])
// times: required, can be oUTC or µUTC
// times: required, can be oUTC or µUTC
// MED_directory: reference channel or session; if session default reference channel will be used
// password: if empty/absent, proceeds as if unencrypted (may error out)
// returns Matlab int64 value of sample number in absolute reference frame


#include "MED_sample_for_time_exec.h"


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
	si1                    	MED_directory[FULL_FILE_NAME_BYTES_m12];
        si1                     password[PASSWORD_BYTES_m12 + 1], temp_str[16];
        si4                     len, n_files;
        si8                     time;
        mxArray                 *times, *mx_cell_p, *tmp_mxa;
	mwSize			dims[2];

	
	PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(MAX_CHANNELS, 1), FALSE_m12);
	PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);

	//  check for proper number of arguments
	if (nlhs != 1)
		mexErrMsgTxt("One output required: sample_number\n");
	plhs[0] = mxCreateLogicalScalar((mxLogical) 0);  // set "false" return value for any subsequent errors
	if (nrhs < 2 || nrhs > 3)
		mexErrMsgTxt("Two to 3 inputs required: time(s), MED_directory, [password]\n");

	// time
	if (mxIsEmpty(prhs[0]) == 1)
		mexErrMsgTxt("'time(s)' (input 1) must be specified\n");
	if (mxGetClassID(prhs[0]) == mxCHAR_CLASS) {
		mxGetString(prhs[0], temp_str, 16);
		if (strcmp(temp_str, "start") == 0) {
			time = BEGINNING_OF_TIME_m12;
		} else if (strcmp(temp_str, "end") == 0) {
			time = END_OF_TIME_m12;
		} else {
			mexErrMsgTxt("'time(s)' (input 1) can be specified as 'start', 'end', or integer(s)\n");
			return;  // unnecessary - just to silence compiler warning
		}
		dims[0] = dims[1] = (mwSize) 1;
		times = mxCreateNumericArray((mwSize) 2, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(times)) = time;
	} else {
		times = get_si8_array(prhs[0]);
	}
	
        // get the MED directory
	len = 0;  // initialized to avoid bogus compiler warning
	if (mxIsEmpty(prhs[1]) == 1)
		mexErrMsgTxt("'MED_directory' (input 2) must be specified\n");
        if (mxGetClassID(prhs[1]) == mxCHAR_CLASS) {
                len = mxGetNumberOfElements(prhs[1]) + TYPE_BYTES_m12; // get max length of the input string
		if (len > FULL_FILE_NAME_BYTES_m12)
			mexErrMsgTxt("'MED_directory' (input 2) is too long\n");
        } else if (mxGetClassID(prhs[1]) == mxCELL_CLASS) {
                n_files = mxGetNumberOfElements(prhs[1]);
		if (n_files == 0)
			mexErrMsgTxt("'MED_directory' (input 2) cell array contains no entries\n");
		if (n_files > 1)
			mexErrMsgTxt("'MED_directory' (input 2) cell array contains multiple entries\n");
		mx_cell_p = mxGetCell(prhs[1], 0);
		if (mxGetClassID(mx_cell_p) != mxCHAR_CLASS)
			mexErrMsgTxt("Elements of 'MED_directory' (input 2) cell array must be char arrays\n");
		len = mxGetNumberOfElements(mx_cell_p) + TYPE_BYTES_m12; // get max length of the input string
		if (len > FULL_FILE_NAME_BYTES_m12)
			mexErrMsgTxt("'MED_directory' (input 2) is too long\n");
        } else {
		mexErrMsgTxt("'MED_directory' (input 2) must be a string or cell array\n");
        }
	mxGetString(prhs[1], MED_directory, len);
	
        // password
        *password = 0;
        if (nrhs == 3) {
                if (mxIsEmpty(prhs[2]) == 0) {
                        if (mxGetClassID(prhs[2]) == mxCHAR_CLASS) {
                                len = mxGetNumberOfElements(prhs[2]); // Get the length of the input string
                                if (len > (PASSWORD_BYTES_m12))  // allow full 16 bytes for password
					mexErrMsgTxt("'password' (input 3) is too long\n");
                                else
                                        mxGetString(prhs[2], password, len + 1);
                        } else {
				mexErrMsgTxt("'password' (input 3) must be a string\n");
                        }
                }
        }
         		
	// initialize MED library
	G_initialize_medlib_m12(FALSE_m12, FALSE_m12);
                		
        // get out of here
	tmp_mxa = MED_sample_for_time(times, MED_directory, password);
	if (tmp_mxa != NULL) {
		mxDestroyArray(plhs[0]);  // destroy default "false" return
		plhs[0] = tmp_mxa;
	}

        // clean up
	G_free_globals_m12(TRUE_m12);

        return;
}


mxArray     *MED_sample_for_time(mxArray *times, si1 *MED_directory, si1 *password)
{
	si1			tmp_str[FULL_FILE_NAME_BYTES_m12], extension[TYPE_BYTES_m12], **channel_list;
        si4			i, len, n_channels;
	ui8			flags;
	si8			min_time, max_time, tmp_time, *times_p, sess_start, offset;
        CHANNEL_m12		*chan;
        TIME_SLICE_m12		slice;

	
	// get full MED directory name
	G_path_from_root_m12(MED_directory, MED_directory);
	G_extract_path_parts_m12(MED_directory, NULL, NULL, extension);
	if (*extension == 0) {
		// see if time series channel with this name exists
		sprintf_m12(tmp_str, "%s.%s", MED_directory, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12);
		if (G_exists_m12(tmp_str) == DIR_EXISTS_m12) {
			strcpy(MED_directory, tmp_str);
			strcpy(extension, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12);
		} else {
			// see if session with this name exists
			sprintf_m12(MED_directory, "%s.%s", MED_directory, SESSION_DIRECTORY_TYPE_STRING_m12);
			if (G_exists_m12(MED_directory) == DIR_EXISTS_m12)
				strcpy(extension, SESSION_DIRECTORY_TYPE_STRING_m12);
			else
				return(NULL);
		}
	}

	// get a first channel from session
	if (strcmp(extension, SESSION_DIRECTORY_TYPE_STRING_m12) == 0) {
		channel_list = G_generate_file_list_m12(NULL, &n_channels, MED_directory, NULL, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12, GFL_FULL_PATH_m12);
		if (channel_list == NULL) {
			G_warning_message_m12("No time series channels in session directory\n");
			return(NULL);
		}
		strcpy(MED_directory, channel_list[0]);
		free_m12((void *) channel_list, __FUNCTION__);
	} else if (strcmp(extension, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12)) {
		G_warning_message_m12("'MED_directory' must be an existing MED channel or session\n");
		return(NULL);
	}
		
	// get times info
	len = (si4) mxGetNumberOfElements(times);
	times_p = (si8 *) mxGetPr(times);
	max_time = min_time = times_p[0];
	for (i = 1; i < len; ++i) {
		if (max_time < times_p[i])
			max_time = times_p[i];
		else if (min_time > times_p[i])
			min_time = times_p[i];
	}
	// handle relative times for slice
	if (max_time < 0 || min_time < 0) {
		tmp_time = max_time;
		max_time = min_time;
		min_time = tmp_time;
	}
		
        // open channel
        G_initialize_time_slice_m12(&slice);
	slice.start_time = min_time;
	slice.end_time = max_time;
	flags = (LH_READ_SLICE_SEGMENT_DATA_m12 | LH_MAP_ALL_SEGMENTS_m12);  // read in time series indices (this could be made more efficient)
	chan = G_open_channel_m12(NULL, &slice, MED_directory, flags, password);  // threaded version
	if (chan == NULL) {
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("%s(): cannot open channel => no matching input files\n", __FUNCTION__);
		} else {
			if (*globals_m12->password_data.level_1_password_hint || *globals_m12->password_data.level_2_password_hint)
				G_warning_message_m12("%s(): cannot open channel => check that the password is correct\n", __FUNCTION__);
			else
				G_warning_message_m12("%s(): cannot open channel => check that the password is correct, and that metadata files exist\n", __FUNCTION__);
		}
		return(NULL);
	}

	// convert to oUTC
	// user probably used only one method of specifying times, but you never know so check every time

	// relative times
	sess_start = globals_m12->session_start_time;
	for (i = 0; i < len; ++i)
		if (times_p[i] < 0)  // time is relative
			times_p[i] = sess_start - times_p[i];
		
	// absolute times
	offset = globals_m12->recording_time_offset;
	for (i = 0; i < len; ++i) {
		if ((times_p[i] - offset) > 0)  // time is absolute
			times_p[i] -= offset;
	}

	// get samples (put in times array)
	for (i = 0; i < len; ++i)
		times_p[i] = G_sample_number_for_uutc_m12((LEVEL_HEADER_m12 *) chan, times_p[i], (FIND_ABSOLUTE_m12 | FIND_CURRENT_m12));
	
       	// clean up
	G_free_channel_m12(chan, TRUE_m12);

        return(times);
}


mxArray     *get_si8_array(const mxArray *mx_in_arr)
{
	ui1		*ui1_p;
	si1		*si1_p;
	ui2		*ui2_p;
	si2		*si2_p;
	ui4		*ui4_p;
	si4		*si4_p, i, len;
	ui8		*ui8_p;
	si8		*si8_p, *out_p;
	sf4     	*sf4_p;
	sf8     	*sf8_p;
	mwSize		rows, cols, dims[2];
	mxArray		*mx_out_arr;
	mxClassID	class;
	
	
	class = mxGetClassID(mx_in_arr);
	switch (class) {
		case mxUINT8_CLASS:
		case mxINT8_CLASS:
		case mxUINT16_CLASS:
		case mxINT16_CLASS:
		case mxUINT32_CLASS:
		case mxINT32_CLASS:
		case mxUINT64_CLASS:
		case mxINT64_CLASS:
		case mxSINGLE_CLASS:
		case mxDOUBLE_CLASS:
			break;
		default:
			mexErrMsgTxt("Input array must be a numeric type\n");
	}

	// create new mx array of si8s
	rows = (mwSize) mxGetM(mx_in_arr);
	cols = (mwSize) mxGetN(mx_in_arr);
	if (rows > 1 && cols > 1)
		mexErrMsgTxt("Input array must be one-dimensional\n");
	len = (si4) (rows * cols);
	dims[0] = rows;
	dims[1] = cols;
	mx_out_arr = mxCreateNumericArray((mwSize) 2, dims, mxINT64_CLASS, mxREAL);
	out_p = (si8 *) mxGetPr(mx_out_arr);
	switch (class) {
		case mxUINT8_CLASS:
			ui1_p = (ui1 *) mxGetPr(mx_out_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *ui1_p++;
			break;
		case mxINT8_CLASS:
			si1_p = (si1 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *si1_p++;
			break;
		case mxUINT16_CLASS:
			ui2_p = (ui2 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *ui2_p++;
			break;
		case mxINT16_CLASS:
			si2_p = (si2 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *si2_p++;
			break;
		case mxUINT32_CLASS:
			ui4_p = (ui4 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *ui4_p++;
			break;
		case mxINT32_CLASS:
			si4_p = (si4 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *si4_p++;
			break;
		case mxUINT64_CLASS:
			ui8_p = (ui8 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *ui8_p++;
			break;
		case mxINT64_CLASS:
			si8_p = (si8 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) *si8_p++;
			break;
		case mxSINGLE_CLASS:
			sf4_p = (sf4 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) round(*sf4_p++);
			break;
		case mxDOUBLE_CLASS:
			sf8_p = (sf8 *) mxGetPr(mx_in_arr);
			for (i = len; i--;)
				*out_p++ = (si8) round(*sf8_p++);
			break;
		default:  // can't get here - just to silence compiler warning
			break;
	}

	return(mx_out_arr);
}
