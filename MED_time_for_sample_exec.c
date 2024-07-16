
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2021


//******************************************** Mex Compile Line *****************************************//
//****  mex COMPFLAGS='$COMPFLAGS -Wall -O3' read_MED_exec.c medlib_m12.c medrec_m12.c dhnlib_m12.c  ****//
//*******************************************************************************************************//

// time = MED_time_for_sample(sample_number, MED_directory, [password])
// sample_number: required
// MED_directory: reference channel or session; if session default reference channel will be used
// password: if empty/absent, proceeds as if unencrypted (may error out)
// returns Matlab int64 value of sample number in absolute reference frame


#include "MED_time_for_sample_exec.h"


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
	si1                    	MED_directory[FULL_FILE_NAME_BYTES_m12];
        si1                     password[PASSWORD_BYTES_m12 + 1], temp_str[16];
        si4                     len, n_files;
	ui8			n_dims;
        si8                     time, sample;
        mxArray                 *mx_cell_p, *tmp_mxa;
	mwSize			dims[2];

	
	PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(MAX_CHANNELS, 1), FALSE_m12);
	PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);

	//  check for proper number of arguments
	if (nlhs != 1)
		mexErrMsgTxt("One output required: time\n");
	plhs[0] = mxCreateDoubleMatrix(0, 0, mxREAL);  // set empty return value for subsequent errors
	if (nrhs < 3 || nrhs > 8)
		mexErrMsgTxt("Two to 3 inputs required: sample_number, MED_directory, [password]\n");

	time = UUTC_NO_ENTRY_m12;
	sample = SAMPLE_NUMBER_NO_ENTRY_m12;

	// sample_number
	if (mxIsEmpty(prhs[0]) == 1)
		mexErrMsgTxt("'sample_number' (input 1) must be specified\n");
	if (mxGetClassID(prhs[0]) == mxCHAR_CLASS) {
		mxGetString(prhs[0], temp_str, 16);
		if (strcmp(temp_str, "start") == 0)
			sample = BEGINNING_OF_SAMPLE_NUMBERS_m12;
		else if (strcmp(temp_str, "end") == 0)
			sample = END_OF_SAMPLE_NUMBERS_m12;
		else
			mexErrMsgTxt("'sample_number' (input 1) can be specified as 'start', 'end', or an integer\n");
	} else {
		sample = get_si8_scalar(prhs[0]);
		if (sample > 0)  // convert to MED indexing
			--sample;
	}
	
        // get the MED directory
	len = 0;  // initialized to avoid bogus compiler warning
	if (mxIsEmpty(prhs[1]) == 1)
		mexErrMsgTxt("'MED_directory' (input 2) must be specified\n");
        if (mxGetClassID(prhs[1]) == mxCHAR_CLASS) {
                len = mxGetNumberOfElements(prhs[1]) + 1; // Get the length of the input string
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
		len = mxGetNumberOfElements(mx_cell_p) + 1; // Get the length of the input string
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
	time = MED_time_for_sample(sample, MED_directory, password);
	if (time != UUTC_NO_ENTRY_m12) {
		mxDestroyArray(plhs[0]);
		n_dims = 2; dims[0] = dims[1] = 1;
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = time;
		plhs[0] = tmp_mxa;
	}

        // clean up
	G_free_globals_m12(TRUE_m12);

        return;
}


si8     MED_time_for_sample(si8 sample, si1 *MED_directory, si1 *password)
{
	si1			tmp_str[FULL_FILE_NAME_BYTES_m12], extension[TYPE_BYTES_m12], **channel_list;
        si4			n_channels;
	ui8			flags;
	si8			time;
        CHANNEL_m12		*chan;
        TIME_SLICE_m12		slice;

	
	// get full MED directory name
	G_path_from_root_m12(MED_directory, MED_directory);
	G_extract_path_parts_m12(MED_directory, NULL, NULL, extension);
	if (*extension == 0) {
		// see if time series channel with this name exists
		sprintf_m12(tmp_str, "%s.%s", MED_directory, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12);
		if (G_file_exists_m12(tmp_str) == DIR_EXISTS_m12) {
			strcpy(MED_directory, tmp_str);
			strcpy(extension, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12);
		} else {
			// see if session with this name exists
			sprintf_m12(MED_directory, "%s.%s", MED_directory, SESSION_DIRECTORY_TYPE_STRING_m12);
			if (G_file_exists_m12(MED_directory) == DIR_EXISTS_m12)
				strcpy(extension, SESSION_DIRECTORY_TYPE_STRING_m12);
			else {
				return(UUTC_NO_ENTRY_m12);
			}
		}
	}

	// get a first channel from session
	if (strcmp(extension, SESSION_DIRECTORY_TYPE_STRING_m12) == 0) {
		channel_list = G_generate_file_list_m12(NULL, &n_channels, MED_directory, NULL, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12, GFL_FULL_PATH_m12);
		if (channel_list == NULL) {
			G_warning_message_m12("No time series channels in session directory\n");
			return(UUTC_NO_ENTRY_m12);
		}
		strcpy(MED_directory, channel_list[0]);
		free_m12((void *) channel_list, __FUNCTION__);
	} else if (strcmp(extension, TIME_SERIES_CHANNEL_DIRECTORY_TYPE_STRING_m12)) {
		G_warning_message_m12("'MED_directory' must be an existing MED channel or session\n");
		return(UUTC_NO_ENTRY_m12);
	}
		
        // open channel
        G_initialize_time_slice_m12(&slice);
	slice.start_sample_number = slice.end_sample_number = sample;
	flags = LH_READ_SLICE_SEGMENT_DATA_m12;  // read in time series indices (this could be made more efficient)
	chan = G_open_channel_m12(NULL, &slice, MED_directory, flags, password);  // threaded version
	if (chan == NULL) {
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("\n%s():\nCannot read channel => no matching input files.\n", __FUNCTION__);
		} else {
			if (*globals_m12->password_data.level_1_password_hint || *globals_m12->password_data.level_2_password_hint) {
				G_warning_message_m12("\n%s():\nCannot read channel => Check that the password is correct.\n", __FUNCTION__);
				G_show_password_hints_m12(NULL);
			} else {
				G_warning_message_m12("\n%s():\nCannot read channel => Check that the password is correct, and that metadata files exist.\n", __FUNCTION__);
			}
		}
		putchar_m12('\n');
		return(UUTC_NO_ENTRY_m12);
	}

	// get sample number
	time = G_uutc_for_sample_number_m12((LEVEL_HEADER_m12 *) chan, sample, FIND_START_m12);
	
        // clean up
	G_free_channel_m12(chan, TRUE_m12);

        return(time);
}


si8     get_si8_scalar(const mxArray *mx_arr)
{
        sf8     tmp_sf8;
        
        
        if (mxGetNumberOfElements(mx_arr) != 1)
		G_error_message_m12("%s(): multiple element array\n", __FUNCTION__);
        
        switch (mxGetClassID(mx_arr)) {
                case mxDOUBLE_CLASS:
                case mxSINGLE_CLASS:
                        tmp_sf8 = (sf8) mxGetScalar(mx_arr);
                        tmp_sf8 = round(tmp_sf8);
                        return((si8) tmp_sf8);
                case mxCHAR_CLASS:
                case mxINT8_CLASS:
                case mxUINT8_CLASS:
                case mxINT16_CLASS:
                case mxUINT16_CLASS:
                case mxINT32_CLASS:
                case mxUINT32_CLASS:
                case mxINT64_CLASS:
                case mxUINT64_CLASS:
                        break;
                default:
                        return((si8) UUTC_NO_ENTRY_m12);
        }

        return((si8) mxGetScalar(mx_arr));
}
