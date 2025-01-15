
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2021


#include "matrix_MED_exec.h"

// Globals
static TERN_m12			loaded = FALSE_m12;
static SESSION_m12		*med_session = NULL;
static DATA_MATRIX_m12		*med_matrix = NULL;


// Mex exit function
void	mexExitFunction(void)
{
	// free session
	if (med_session != NULL) {
		G_free_session_m12(med_session, TRUE_m12);
		med_session = NULL;
	}
		
	// free matrix
	if (med_matrix != NULL) {
		DM_free_matrix_m12(med_matrix, TRUE_m12);
		med_matrix = NULL;
	}

	G_free_globals_m12(TRUE_m12);
	
	return;
}


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
        si1				**MED_paths_p, temp_str[16];
        si4				i, len, max_len;
        si8				tmp_si8;
        mxArray				*tmp_mxa, *mx_cell_p, *mat_matrix;
	const mxArray			*mps;
	C_MPS				cmps;

	
	// mex function status
	if (loaded == FALSE_m12) {
		// register exit function
		mexAtExit(mexExitFunction);
		
		// adjust process limits (called this way, these functions do not require medlib to be initialized)
		PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(MAX_CHANNELS, 1), FALSE_m12);
		PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);
		
		// initialize medlib
		G_initialize_medlib_m12(FALSE_m12, FALSE_m12);
		
		loaded = TRUE_m12;
	}
	
	//  check for proper number of arguments
	if (nlhs != 1) {
		if (nrhs != 1)
			mexErrMsgTxt("One input: matrix_MED parameter structure\nOne output: matrix_MED data structure\n");
		else
			mexErrMsgTxt("One output: matrix_MED data structure\n");
	}
	plhs[0] = mxCreateLogicalScalar((mxLogical) 0);  // set "false" return value for any subsequent errors
	if (nrhs != 1)
		mexErrMsgTxt("One input: matrix_MED parameter structure\n");
	mps = prhs[0];
	if (mxIsStruct(mps) == 0)
		mexErrMsgTxt("Input must be a matrix_MED parameter structure\n");

	// get persistence mode if passed
	cmps.persist_mode = PERSIST_READ_CLOSE;  // default
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_PERSISTENCE_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {  // passed char array
			mxGetString(tmp_mxa, temp_str, 16);
			switch (*temp_str) {
				case 'c':  // "close"
				case 'C':
					cmps.persist_mode = PERSIST_CLOSE;
					break;
				case 'o':  // "open"
				case 'O':
					cmps.persist_mode = PERSIST_OPEN;
					break;
				case 'r':  // "read"
				case 'R':
					if (*(temp_str + 4) == 0)  // "read" only
						cmps.persist_mode = PERSIST_READ;
					else if (*(temp_str + 5) == 'n' || *(temp_str + 5) == 'N')  // "read new"
						cmps.persist_mode = PERSIST_READ_NEW;
					// else leave as "read close" (default)
					break;
				default:  // includes "none" (default)
					break;
			}
		} else {  // passed number
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 == PERSIST_NONE)  // none == read_close
				tmp_si8 = PERSIST_READ_CLOSE;
			if (tmp_si8 < PERSIST_OPEN || tmp_si8 > PERSIST_READ_CLOSE || tmp_si8 == 3)  // "3" not valid
				mexErrMsgTxt("Invalid 'Persist' mode\n");
			cmps.persist_mode = (ui1) tmp_si8;
		}
	}

	if (cmps.persist_mode & PERSIST_OPEN || cmps.persist_mode == PERSIST_CLOSE) {
		if (med_session != NULL) {  // free session
			G_free_session_m12(med_session, TRUE_m12);
			med_session = NULL;
			if (cmps.persist_mode == PERSIST_CLOSE) {  // set return to "true" for session closed
				mxDestroyArray(plhs[0]);  // no mex "set" function for logicals
				plhs[0] = mxCreateLogicalScalar((mxLogical) 1);
			}
		}
		if (med_matrix != NULL) {
			DM_free_matrix_m12(med_matrix, TRUE_m12);
			med_matrix = NULL;
		}
		if (cmps.persist_mode == PERSIST_CLOSE)
			return;
	}
	
	// check data parameter
	cmps.n_files = max_len = 0;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_DATA_IDX);
	if (mxIsEmpty(tmp_mxa) == 1)
		mexErrMsgTxt("'Data' element is not specified\n");
	if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
		max_len = mxGetNumberOfElements(tmp_mxa); // get the length of the input string
		if (max_len > FULL_FILE_NAME_BYTES_m12)
			mexErrMsgTxt("'Data' path is too long\n");
	} else if (mxGetClassID(tmp_mxa) == mxCELL_CLASS) {
		cmps.n_files = mxGetNumberOfElements(tmp_mxa);
		if (cmps.n_files == 0)
			mexErrMsgTxt("'Data' cell array contains no entries\n");
		for (i = max_len = 0; i < cmps.n_files; ++i) {
			mx_cell_p = mxGetCell(tmp_mxa, i);
			if (mxGetClassID(mx_cell_p) != mxCHAR_CLASS)
				mexErrMsgTxt("Elements of 'Data' cell array must be char arrays\n");
			len = mxGetNumberOfElements(mx_cell_p); // get the length of the input string
			if (len > FULL_FILE_NAME_BYTES_m12)
				mexErrMsgTxt("One of the 'Data' paths is too long\n");
			if (len > max_len)
				max_len = len;
		}
	} else {
		mexErrMsgTxt("'Data' must be a char array or cell array of char arrays\Elements may include regular expressions (regex)\n");
	}
	max_len += TYPE_BYTES_m12;  // add room for med type extension, in case not included

	// get the sample dimension mode
	cmps.sample_dimension_mode = SAMPLE_DIMENSION_MODE_COUNT;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_SAMPLE_DIMENSION_MODE_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {  // passed char array
			mxGetString(tmp_mxa, temp_str, 16);
			switch (*temp_str) {
				case 'c':  // "count"
				case 'C':
					cmps.sample_dimension_mode = SAMPLE_DIMENSION_MODE_COUNT;
					break;
				case 'r':  // "rate"
				case 'R':
					cmps.sample_dimension_mode = SAMPLE_DIMENSION_MODE_RATE;
					break;
				default:
					mexErrMsgTxt("Invalid 'SampDimMode' type\n");
					break;
			}
		} else {  // passed number
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < SAMPLE_DIMENSION_MODE_COUNT || tmp_si8 > SAMPLE_DIMENSION_MODE_RATE)
				mexErrMsgTxt("Invalid 'SampDimMode' type\n");
			cmps.sample_dimension_mode = tmp_si8;
		}
	}

	// get the sample dimension
	cmps.n_out_samps = 0;
	cmps.out_freq = (sf8) 0.0;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_SAMPLE_DIMENSION_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (cmps.sample_dimension_mode == SAMPLE_DIMENSION_MODE_COUNT)
			cmps.n_out_samps = get_si8_scalar(tmp_mxa);
		else
			cmps.out_freq = *((sf8 *) mxGetPr(tmp_mxa));
	}

	// output samples or sampling frequency required
	if (cmps.n_out_samps == 0 && cmps.out_freq == (sf8) 0.0) {
		mexErrMsgTxt("No output samples or sampling frequency specified\n");
	} else if (cmps.n_out_samps && cmps.out_freq) {
		printf_m12("Both output sample count and sampling frequency are defined => using sample count\n");
		cmps.out_freq = (sf8) 0.0;
	}
	
	// get limit mode
	cmps.extents_mode = EXTENTS_MODE_TIME;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_EXTENTS_MODE_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {  // passed char array
			mxGetString(tmp_mxa, temp_str, 16);
			switch (*temp_str) {
				case 't':  // "time"
				case 'T':
					cmps.extents_mode = EXTENTS_MODE_TIME;
					break;
				case 'i':  // "indices"
				case 'I':
					cmps.extents_mode = EXTENTS_MODE_INDICES;
					break;
				default:
					mexErrMsgTxt("Invalid 'ExtentsMode' type\n");
					break;
			}
		} else {  // passed number
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < EXTENTS_MODE_TIME || tmp_si8 > EXTENTS_MODE_INDICES)
				mexErrMsgTxt("Invalid 'ExtentsMode' type\n");
			cmps.extents_mode = tmp_si8;
		}
	}

	// get the start limit
	cmps.start_time = UUTC_NO_ENTRY_m12;
	cmps.start_index = SAMPLE_NUMBER_NO_ENTRY_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_START_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("'Start' can be specified as 'start', or an integer\n");
			if (strcmp(temp_str, "start") == 0) {
				if (cmps.extents_mode == EXTENTS_MODE_TIME)
					cmps.start_time = BEGINNING_OF_TIME_m12;
				else
					cmps.start_index = BEGINNING_OF_SAMPLE_NUMBERS_m12;
			} else {
				mexErrMsgTxt("'Start' can be specified as 'start', or an integer\n");
			}
		} else {
			if (cmps.extents_mode == EXTENTS_MODE_TIME)
				cmps.start_time = get_si8_scalar(tmp_mxa);
			else
				cmps.start_index = get_si8_scalar(tmp_mxa) - 1;  // convert to c indexing;
		}
	}

	// get the end limit
	cmps.end_time = UUTC_NO_ENTRY_m12;
	cmps.end_index = SAMPLE_NUMBER_NO_ENTRY_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_END_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("'End' can be specified as 'end', or an integer\n");
			if (strcmp(temp_str, "end") == 0) {
				if (cmps.extents_mode == EXTENTS_MODE_TIME)
					cmps.end_time = END_OF_TIME_m12;
				else
					cmps.end_index = END_OF_SAMPLE_NUMBERS_m12;
			} else {
				mexErrMsgTxt("'End' can be specified as 'end', or an integer\n");
			}
		} else {
			if (cmps.extents_mode == EXTENTS_MODE_TIME)
				cmps.end_time = get_si8_scalar(tmp_mxa);
			else
				cmps.end_index = get_si8_scalar(tmp_mxa) - 1;  // convert to c indexing;
		}
	}

	// get time mode
	cmps.time_mode = TIME_MODE_DURATION;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_TIME_MODE_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {  // passed char array
			mxGetString(tmp_mxa, temp_str, 16);
			switch (*temp_str) {
				case 'd':  // "duration"
				case 'D':
					cmps.time_mode = TIME_MODE_DURATION;
					break;
				case 'e':  // "cmps.end_time"
				case 'E':
					cmps.time_mode = TIME_MODE_END_TIME;
					break;
				default:
					mexErrMsgTxt("Invalid 'TimeMode' type\n");
					break;
			}
		} else {  // passed number
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < TIME_MODE_DURATION || tmp_si8 > TIME_MODE_END_TIME)
				mexErrMsgTxt("Invalid 'TimeMode' type\n");
			cmps.time_mode = tmp_si8;
		}
	}

       	// get password
        *cmps.password = 0;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_PASSWORD_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len > PASSWORD_BYTES_m12)
				mexErrMsgTxt("'Pass' is too long\n");
			else
				mxGetString(tmp_mxa, cmps.password, len);
		} else {
			mexErrMsgTxt("'Pass' must be a char array\n");
		}
	}
        
	// get index_channel
	*cmps.index_channel = 0;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_INDEX_CHANNEL_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len > FULL_FILE_NAME_BYTES_m12)
				mexErrMsgTxt("'IdxChan' is too long\n");
			else
				mxGetString(tmp_mxa, cmps.index_channel, len);
		} else {
			mexErrMsgTxt("'IdxChan' must be a string\n");
		}
	}

	// get filter
	cmps.filter = FILT_ANTIALIAS;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_FILTER_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("Invalid 'Filt' type\n");
			if (strcmp(temp_str, "antialias") == 0)
				cmps.filter = FILT_ANTIALIAS;
			else if (strcmp(temp_str, "none") == 0)
				cmps.filter = FILT_NONE;
			else if (strcmp(temp_str, "lowpass") == 0)
				cmps.filter = FILT_LOWPASS;
			else if (strcmp(temp_str, "highpass") == 0)
				cmps.filter = FILT_HIGHPASS;
			else if (strcmp(temp_str, "bandpass") == 0)
				cmps.filter = FILT_BANDPASS;
			else if (strcmp(temp_str, "bandstop") == 0)
				cmps.filter = FILT_BANDSTOP;
			else
				mexErrMsgTxt("Invalid 'Filt' type\n");
		} else {
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < FILT_ANTIALIAS || tmp_si8 > FILT_BANDSTOP)
				mexErrMsgTxt("Invalid 'Filt' type\n");
			cmps.filter = tmp_si8;
		}
	}

	// get low cutoff
	cmps.low_cutoff = (sf8) -1.0;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_LOW_CUTOFF_IDX);
	if (mxIsEmpty(tmp_mxa) == 0)
		cmps.low_cutoff = *((sf8 *) mxGetPr(tmp_mxa));

	// get high cutoff
	cmps.high_cutoff = (sf8) -1.0;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_HIGH_CUTOFF_IDX);
	if (mxIsEmpty(tmp_mxa) == 0)
		cmps.high_cutoff = *((sf8 *) mxGetPr(tmp_mxa));
	
	// check filter cutoffs
	if (cmps.filter >= FILT_LOWPASS) {
		if (cmps.filter == FILT_HIGHPASS || cmps.filter == FILT_BANDPASS || cmps.filter == FILT_BANDSTOP)
			if (cmps.high_cutoff == (sf8) 0.0)
				mexErrMsgTxt("'LowCut' is required for the specified filter type\n");
		if (cmps.filter == FILT_LOWPASS || cmps.filter == FILT_BANDPASS || cmps.filter == FILT_BANDSTOP)
			if (cmps.high_cutoff == (sf8) 0.0)
				mexErrMsgTxt("'HighCut' is required for the specified filter type\n");
	}

	// get scale
	cmps.scale = (sf8) 1.0;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_SCALE_IDX);
	if (mxIsEmpty(tmp_mxa) == 0)
		cmps.scale = *((sf8 *) mxGetPr(tmp_mxa));

	// get format
	cmps.format = FORMAT_DOUBLE;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_FORMAT_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("Invalid 'Format' type\n");
			if (strcmp(temp_str, "double") == 0)
				cmps.format = FORMAT_DOUBLE;
			else if (strcmp(temp_str, "single") == 0)
				cmps.format = FORMAT_SINGLE;
			else if (strcmp(temp_str, "int32") == 0)
				cmps.format = FORMAT_INT32;
			else if (strcmp(temp_str, "int16") == 0)
				cmps.format = FORMAT_INT16;
			else
				mexErrMsgTxt("Invalid 'Format' type\n");
		} else {
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < FORMAT_DOUBLE || tmp_si8 > FORMAT_INT16)
				mexErrMsgTxt("Invalid 'Format' type\n");
			cmps.format = tmp_si8;
		}
	}
	
	// get padding
	cmps.padding = PAD_NONE;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_PADDING_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("Invalid 'Padding' type\n");
			if (strcmp(temp_str, "none") == 0)
				cmps.padding = PAD_NONE;
			else if (strcmp(temp_str, "zero") == 0)
				cmps.padding = PAD_ZERO;
			else if (strcmp(temp_str, "nan") == 0)
				cmps.padding = PAD_NAN;
			else
				mexErrMsgTxt("Invalid 'Padding' type\n");
		} else {
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < PAD_NONE || tmp_si8 > PAD_NAN)
				mexErrMsgTxt("Invalid 'Padding' type\n");
			cmps.padding = tmp_si8;
		}
	}
	
	// get interpolation
	cmps.interpolation = INTERP_LINEAR_MAKIMA;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_INTERP_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("Invalid 'Interp' type\n");
			if (strcmp(temp_str, "linear_makima") == 0)
				cmps.interpolation = INTERP_LINEAR_MAKIMA;
			else if (strcmp(temp_str, "linear_spline") == 0)
				cmps.interpolation = INTERP_LINEAR_SPLINE;
			else if (strcmp(temp_str, "linear") == 0)
				cmps.interpolation = INTERP_LINEAR;
			else if (strcmp(temp_str, "spline") == 0)
				cmps.interpolation = INTERP_SPLINE;
			else if (strcmp(temp_str, "makima") == 0)
				cmps.interpolation = INTERP_MAKIMA;
			else if (strcmp(temp_str, "binterp") == 0)
				cmps.interpolation = INTERP_BINTERP;
			else
				mexErrMsgTxt("Invalid 'Interp' type\n");
		} else {
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < INTERP_LINEAR_MAKIMA || tmp_si8 > INTERP_BINTERP)
				mexErrMsgTxt("Invalid 'Interp' type\n");
			cmps.interpolation = tmp_si8;
		}
	}

	// get bin interpolation
	cmps.bin_interpolation = BINTERP_MEAN;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_BINTERP_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("Invalid 'Binterp' type\n");
			if (strcmp(temp_str, "mean") == 0)
				cmps.bin_interpolation = BINTERP_MEAN;
			else if (strcmp(temp_str, "median") == 0)
				cmps.bin_interpolation = BINTERP_MEDIAN;
			else if (strcmp(temp_str, "center") == 0)
				cmps.bin_interpolation = BINTERP_CENTER;
			else if (strcmp(temp_str, "fast") == 0)
				cmps.bin_interpolation = BINTERP_FAST;
			else
				mexErrMsgTxt("Invalid 'Binterp' type\n");
		} else {
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < BINTERP_MEAN || tmp_si8 > BINTERP_FAST)
				mexErrMsgTxt("Invalid 'Binterp' type\n");
			cmps.bin_interpolation = tmp_si8;
		}
	}

	// get detrend
	cmps.detrend = FALSE_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_DETREND_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		cmps.detrend = get_logical(tmp_mxa);
		if (cmps.detrend == UNKNOWN_m12)
			mexErrMsgTxt("'Detrend' can be either true or false\n");
	}

	// get ranges
	cmps.ranges = FALSE_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_RANGES_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		cmps.ranges = get_logical(tmp_mxa);
		if (cmps.ranges == UNKNOWN_m12)
			mexErrMsgTxt("'Ranges' can be either true or false\n");
	}

	// get extrema
	cmps.extrema = FALSE_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_EXTREMA_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		cmps.extrema = get_logical(tmp_mxa);
		if (cmps.extrema == UNKNOWN_m12)
			mexErrMsgTxt("'Extrema' can be either true or false\n");
	}
	
	// get records
	cmps.records = FALSE_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_RECORDS_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		cmps.records = get_logical(tmp_mxa);
		if (cmps.records == UNKNOWN_m12)
			mexErrMsgTxt("'Records' can be either true or false\n");
	}

	// get contigua
	cmps.contigua = FALSE_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_CONTIGUA_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		cmps.contigua = get_logical(tmp_mxa);
		if (cmps.contigua == UNKNOWN_m12)
			mexErrMsgTxt("'Contigua' can be either true or false\n");
	}

	// get channel names
	cmps.chan_names = FALSE_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_CHANNEL_NAMES_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		cmps.chan_names = get_logical(tmp_mxa);
		if (cmps.chan_names == UNKNOWN_m12)
			mexErrMsgTxt("'ChanNames' can be either true or false\n");
	}

	// get channel frequencies
	cmps.chan_freqs = FALSE_m12;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_CHANNEL_FREQUENCIES_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		cmps.chan_freqs = get_logical(tmp_mxa);
		if (cmps.chan_freqs == UNKNOWN_m12)
			mexErrMsgTxt("'ChanFreqs' can be either true or false\n");
	}

	// create input file list
	cmps.MED_paths = NULL;
	tmp_mxa = mxGetFieldByNumber(mps, 0, MPS_DATA_IDX);
	switch (cmps.n_files) {
		case 0:  // single string passed
			cmps.MED_paths = calloc_m12((size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			mxGetString(tmp_mxa, (si1 *) cmps.MED_paths, max_len);
			break;
		case 1:   // single string passed in cell array
			cmps.MED_paths = calloc_m12((size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			mx_cell_p = mxGetCell(tmp_mxa, 0);
			mxGetString(mx_cell_p, (si1 *) cmps.MED_paths, max_len);
			cmps.n_files = 0;  // (indicates single string)
			break;
		default:  // multiple strings in cell array
			cmps.MED_paths = (void *) calloc_2D_m12((size_t) cmps.n_files, (size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			MED_paths_p = (si1 **) cmps.MED_paths;
			for (i = 0; i < cmps.n_files; ++i) {
				mx_cell_p = mxGetCell(tmp_mxa, i);
				mxGetString(mx_cell_p, MED_paths_p[i], max_len);
			}
			break;
	}

       	// build matrix
        mat_matrix = matrix_MED(&cmps);
	if (mat_matrix != NULL) {
		mxDestroyArray(plhs[0]);  // get rid of logical return value
		// set status
		if (cmps.persist_mode == PERSIST_READ_CLOSE)  // "close" handled on entry, returns logical true
			tmp_mxa = mxCreateString("closed");
		else
			tmp_mxa = mxCreateString("open");
		mxSetFieldByNumber(mat_matrix, 0, MATRIX_STATUS_IDX_mat, tmp_mxa);
		plhs[0] = mat_matrix;
	}

        // clean up
        free_m12((void *) cmps.MED_paths, __FUNCTION__);
	if (med_matrix != NULL)
		med_matrix->data = med_matrix->range_minima = med_matrix->range_maxima = NULL;  // this memory belongs Matlab structure, must be allocated with each call
	if (cmps.persist_mode & PERSIST_CLOSE) {
		if (med_session != NULL) {
			G_free_session_m12(med_session, TRUE_m12);  // resets session globals (no not need to free until function unloaded)
			med_session = NULL;
		}
		if (med_matrix != NULL) {
			DM_free_matrix_m12(med_matrix, TRUE_m12);
			med_matrix = NULL;
		}
	}

        return;
}


mxArray	*matrix_MED(C_MPS *cmps)
{
	si1			*action_str, time_str[TIME_STRING_BYTES_m12];
	si4			n_chans, seg_idx;
	ui8			read_flags, matrix_flags;
	si8			i, n_out_samps, el_size;
	sf8			*in_samp_freqs, out_secs;
	TIME_SLICE_m12		*slice, local_slice;
	SESSION_m12		*sess;
	mxArray			*mat_matrix, *tmp_mxa;
	mwSize			n_dims, dims[2];
	mxClassID 		classid;
	DATA_MATRIX_m12		*dm;
	const si4		n_mat_matrix_fields = NUMBER_OF_MATRIX_FIELDS_mat;
	const si1		*mat_matrix_field_names[] = MATRIX_FIELD_NAMES_mat;
	
	
	// set limit pairs
	if (cmps->start_time == UUTC_NO_ENTRY_m12 && cmps->end_time == UUTC_NO_ENTRY_m12) {
		if (cmps->start_index == SAMPLE_NUMBER_NO_ENTRY_m12 && cmps->end_index == SAMPLE_NUMBER_NO_ENTRY_m12) {  // nothing passed, default to time
			cmps->start_time = BEGINNING_OF_TIME_m12;
			cmps->end_time = END_OF_TIME_m12;
		} else {
			if (cmps->start_index == SAMPLE_NUMBER_NO_ENTRY_m12)
				cmps->start_index = BEGINNING_OF_SAMPLE_NUMBERS_m12;
			else if (cmps->end_index == SAMPLE_NUMBER_NO_ENTRY_m12)
				cmps->end_index = END_OF_SAMPLE_NUMBERS_m12;
		}
	} else {  // at least one time passed
		if (cmps->start_time == UUTC_NO_ENTRY_m12)
			cmps->start_time = BEGINNING_OF_TIME_m12;
		else if (cmps->end_time == UUTC_NO_ENTRY_m12)
			cmps->end_time = END_OF_TIME_m12;
		cmps->start_index = cmps->end_index = SAMPLE_NUMBER_NO_ENTRY_m12;  // time supersedes indices
	}

	// set slice
	slice = &local_slice;
	G_initialize_time_slice_m12(slice);
	slice->start_time = cmps->start_time;
	slice->end_time = cmps->end_time;
	slice->start_sample_number = cmps->start_index;
	slice->end_sample_number = cmps->end_index;

	// set matrix flags
	n_out_samps = cmps->n_out_samps;
	matrix_flags = DM_FMT_CHANNEL_MAJOR_m12;
	if (n_out_samps)
		matrix_flags |= DM_EXTMD_SAMP_COUNT_m12;
	else
		matrix_flags |= DM_EXTMD_SAMP_FREQ_m12;
	switch (cmps->time_mode) {
		case TIME_MODE_DURATION:
			matrix_flags |= DM_EXTMD_RELATIVE_LIMITS_m12;
			break;
		case TIME_MODE_END_TIME:
			matrix_flags |= DM_EXTMD_ABSOLUTE_LIMITS_m12;
			break;
	}
	switch (cmps->filter) {
		case FILT_ANTIALIAS:
			matrix_flags |= DM_FILT_ANTIALIAS_m12;
			break;
		case FILT_NONE:
			break;
		case FILT_LOWPASS:
			matrix_flags |= DM_FILT_LOWPASS_m12;
			break;
		case FILT_HIGHPASS:
			matrix_flags |= DM_FILT_HIGHPASS_m12;
			break;
		case FILT_BANDPASS:
			matrix_flags |= DM_FILT_BANDPASS_m12;
			break;
		case FILT_BANDSTOP:
			matrix_flags |= DM_FILT_BANDSTOP_m12;
			break;
	}
	if (cmps->scale != (sf8) 1.0)
		matrix_flags |= DM_SCALE_m12;
	if (cmps->detrend == TRUE_m12)
		matrix_flags |= DM_DETREND_m12;
	if (cmps->ranges == TRUE_m12)
		matrix_flags |= DM_TRACE_RANGES_m12;
	if (cmps->extrema == TRUE_m12)
		matrix_flags |= DM_TRACE_EXTREMA_m12;
	switch (cmps->format) {
		case FORMAT_DOUBLE:
			matrix_flags |= DM_TYPE_SF8_m12;
			break;
		case FORMAT_SINGLE:
			matrix_flags |= DM_TYPE_SF4_m12;
			break;
		case FORMAT_INT32:
			matrix_flags |= DM_TYPE_SI4_m12;
			break;
		case FORMAT_INT16:
			matrix_flags |= DM_TYPE_SI2_m12;
			break;
	}
	if (cmps->contigua == TRUE_m12)
		matrix_flags |= DM_DSCNT_CONTIG_m12;
	switch (cmps->padding) {
		case PAD_NONE:
			break;
		case PAD_ZERO:
			matrix_flags |= DM_DSCNT_ZERO_m12;
			break;
		case PAD_NAN:
			matrix_flags |= DM_DSCNT_NAN_m12;
			break;
	}
	switch (cmps->interpolation) {
		case INTERP_LINEAR_MAKIMA:
			matrix_flags |= DM_INTRP_UP_MAKIMA_DN_LINEAR_m12;
			break;
		case INTERP_LINEAR_SPLINE:
			matrix_flags |= DM_INTRP_UP_SPLINE_DN_LINEAR_m12;
			break;
		case INTERP_LINEAR:
			matrix_flags |= DM_INTRP_LINEAR_m12;
			break;
		case INTERP_SPLINE:
			matrix_flags |= DM_INTRP_SPLINE_m12;
			break;
		case INTERP_MAKIMA:
			matrix_flags |= DM_INTRP_MAKIMA_m12;
			break;
		case INTERP_BINTERP:
			switch (cmps->bin_interpolation) {
				case BINTERP_MEAN:
					matrix_flags |= DM_INTRP_BINTRP_MEAN_m12;
					break;
				case BINTERP_MEDIAN:
					matrix_flags |= DM_INTRP_BINTRP_MEDN_m12;
					break;
				case BINTERP_CENTER:
					matrix_flags |= DM_INTRP_BINTRP_MDPT_m12;
					break;
				case BINTERP_FAST:
					matrix_flags |= DM_INTRP_BINTRP_FAST_m12;
					break;
			}
			break;
	}
	
	// copy globals
	sess = med_session;
	dm = med_matrix;
	
	// open / read session
	read_flags = LH_READ_SLICE_SEGMENT_DATA_m12;
	if (cmps->persist_mode & PERSIST_CLOSE) {
		if (sess == NULL)
			read_flags |= LH_NO_CPS_CACHING_m12;  // caching not efficient for single reads
	} else {
		read_flags |= LH_MAP_ALL_SEGMENTS_m12;  // more efficient for sequential reads
	}
	if (cmps->persist_mode == PERSIST_OPEN) {
		sess = G_open_session_m12(NULL, slice, cmps->MED_paths, cmps->n_files, read_flags, cmps->password);
		if (sess != NULL) {
			med_session = sess;  // save session
			return(mxCreateLogicalScalar((mxLogical) 1));
		}
		action_str = "open";
	} else if (sess == NULL) {  // PERSIST_READ, PERSIST_READ_CLOSE
		sess = G_open_session_m12(NULL, slice, cmps->MED_paths, cmps->n_files, read_flags, cmps->password);
		action_str = "read";
	}
	
	if (sess == NULL) {
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("%s(): cannot %s session => no matching input files\n", __FUNCTION__, action_str);
		} else {
			if (*globals_m12->password_data.level_1_password_hint || *globals_m12->password_data.level_2_password_hint)
				G_warning_message_m12("%s(): cannot %s session => check that the password is correct\n", __FUNCTION__, action_str);
			else
				G_warning_message_m12("%s(): cannot %s session => check that the password is correct, and that metadata files exist\n", __FUNCTION__, action_str);
		}
		if (med_session != NULL) {  // free session if exists
			G_free_session_m12(med_session, TRUE_m12);
			med_session = NULL;
		}
		if (med_matrix != NULL) {  // free matrix if exists
			DM_free_matrix_m12(med_matrix, TRUE_m12);
			med_matrix = NULL;
		}
		return(NULL);
	}

	if (sess->flags & LH_READ_SLICE_ALL_RECORDS_m12) {
		if (cmps->records == FALSE_m12) {
			read_flags &= ~(LH_READ_SLICE_SESSION_RECORDS_m12 | LH_READ_SLICE_SEGMENTED_SESS_RECS_m12);
			G_propogate_flags_m12((LEVEL_HEADER_m12 *) sess, read_flags);
		}
	} else if (cmps->records == TRUE_m12) {
		read_flags |= (LH_READ_SLICE_SESSION_RECORDS_m12 | LH_READ_SLICE_SEGMENTED_SESS_RECS_m12);
		G_propogate_flags_m12((LEVEL_HEADER_m12 *) sess, read_flags);
	}

	// Create matrix output structure
	n_chans = sess->number_of_time_series_channels;
	mat_matrix = mxCreateStructMatrix(1, 1, n_mat_matrix_fields, mat_matrix_field_names);
	switch (matrix_flags & DM_TYPE_MASK_m12) {
		case DM_TYPE_SF8_m12:
			classid = mxDOUBLE_CLASS;
			el_size = 8;
			break;
		case DM_TYPE_SF4_m12:
			classid = mxSINGLE_CLASS;
			el_size = 4;
			break;
		case DM_TYPE_SI4_m12:
			classid = mxINT32_CLASS;
			el_size = 4;
			break;
		case DM_TYPE_SI2_m12:
			classid = mxINT16_CLASS;
			el_size = 2;
			break;
	}

	// Create DM matrix structure
	if (dm == NULL) {
		dm = (DATA_MATRIX_m12 *) calloc_m12((size_t) 1, sizeof(DATA_MATRIX_m12), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
		dm->el_size = el_size;
	}

	// allocate Matlab output
	if (n_out_samps == 0) {  // sample dimension specified by frequency
		if (G_get_search_mode_m12(slice) == TIME_SEARCH_m12) {
			out_secs = (sf8) TIME_SLICE_DURATION_m12(slice) / (sf8) 1000000.0;  // requested time in seconds
			n_out_samps = (si8) ceil(cmps->out_freq * out_secs);
		} else {  // SAMPLE_SEARCH_m12
			n_out_samps = TIME_SLICE_SAMPLE_COUNT_m12(slice);
		}
	}
	dims[0] = n_out_samps; dims[1] = n_chans; n_dims = 2;
	tmp_mxa = mxCreateNumericArray(n_dims, dims, classid, mxREAL);
	mxSetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_SAMPLES_IDX_mat, tmp_mxa);
	dm->data = (void *) mxGetPr(tmp_mxa);
	if (matrix_flags & DM_TRACE_RANGES_m12) {
		dims[0] = n_out_samps; dims[1] = n_chans; n_dims = 2;
		tmp_mxa = mxCreateNumericArray(n_dims, dims, classid, mxREAL);
		mxSetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_RANGE_MINIMA_IDX_mat, tmp_mxa);
		dm->range_minima = (void *) mxGetPr(tmp_mxa);
		tmp_mxa = mxCreateNumericArray(n_dims, dims, classid, mxREAL);
		mxSetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_RANGE_MAXIMA_IDX_mat, tmp_mxa);
		dm->range_maxima = (void *) mxGetPr(tmp_mxa);
	}
	if (matrix_flags & DM_TRACE_EXTREMA_m12) {
		dims[0] = n_chans; dims[1] = 1; n_dims = 2;
	 	tmp_mxa = mxCreateNumericArray(n_dims, dims, classid, mxREAL);
		mxSetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_TRACE_MINIMA_IDX_mat, tmp_mxa);
		dm->trace_minima = (void *) mxGetPr(tmp_mxa);
		tmp_mxa = mxCreateNumericArray(n_dims, dims, classid, mxREAL);
		mxSetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_TRACE_MAXIMA_IDX_mat, tmp_mxa);
		dm->trace_maxima = (void *) mxGetPr(tmp_mxa);
	}

	dm->channel_count = n_chans;
	dm->sample_count = n_out_samps;
	dm->sampling_frequency = cmps->out_freq;
	dm->scale_factor = cmps->scale;
	dm->data_bytes = (n_out_samps * n_chans) << 3;
	dm->flags = matrix_flags;
	if (matrix_flags & DM_FILT_CUTOFFS_MASK_m12) {
		switch (matrix_flags & DM_FILT_CUTOFFS_MASK_m12) {
			case DM_FILT_LOWPASS_m12:
				dm->filter_high_fc = cmps->high_cutoff;
				break;
			case DM_FILT_HIGHPASS_m12:
				dm->filter_low_fc = cmps->low_cutoff;
				break;
			case DM_FILT_BANDPASS_m12:
			case DM_FILT_BANDSTOP_m12:
				dm->filter_low_fc = cmps->low_cutoff;
				dm->filter_high_fc = cmps->high_cutoff;
				break;
		}
	}

	// Build matrix
	dm = DM_get_matrix_m12(dm, sess, slice, FALSE_m12);
	if (dm == NULL) {
		G_warning_message_m12("\n%s():\nError generating matrix.\n", __FUNCTION__);
		mexExitFunction();
		return(NULL);
	}

	// Adjust output Matlab array sizes, if necessary
	if (dm->sample_count != n_out_samps) {
		n_out_samps = dm->sample_count;
		tmp_mxa = mxGetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_SAMPLES_IDX_mat);
		mxSetM(tmp_mxa, (mwSize) n_out_samps);
		if (matrix_flags & DM_TRACE_RANGES_m12) {
			tmp_mxa = mxGetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_RANGE_MINIMA_IDX_mat);
			mxSetM(tmp_mxa, (mwSize) n_out_samps);
			tmp_mxa = mxGetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_RANGE_MAXIMA_IDX_mat);
			mxSetM(tmp_mxa, (mwSize) n_out_samps);
		}
		if (matrix_flags & DM_TRACE_EXTREMA_m12) {
			tmp_mxa = mxGetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_TRACE_MINIMA_IDX_mat);
			mxSetM(tmp_mxa, (mwSize) n_out_samps);
			tmp_mxa = mxGetFieldByNumber(mat_matrix, (mwIndex) 0, (si4) MATRIX_TRACE_MAXIMA_IDX_mat);
			mxSetM(tmp_mxa, (mwSize) n_out_samps);
		}
	}

	// Switch to returned slice
	slice = &sess->time_slice;

	// Build channel names (duplicated in metadata, but convenient for viewing
	if (cmps->chan_names == TRUE_m12)
		build_channel_names(sess, mat_matrix);
	
	// Build contigua
	if (matrix_flags & DM_DSCNT_CONTIG_m12)
		build_contigua(dm, mat_matrix);
	
	// Build session records
	if (cmps->records == TRUE_m12)
		build_session_records(sess, dm, mat_matrix);

	// Fill in filter cutoffs
	dims[0] = 1; dims[1] = 1; n_dims = 2;
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	if (isnan(dm->filter_low_fc))
		*((sf8 *) mxGetPr(tmp_mxa)) = (sf8) -1.0;
	else
		*((sf8 *) mxGetPr(tmp_mxa)) = dm->filter_low_fc;
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_FILTER_LOW_CUTOFF_IDX_mat, tmp_mxa);
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	if (isnan(dm->filter_high_fc))
		*((sf8 *) mxGetPr(tmp_mxa)) = (sf8) -1.0;
	else
		*((sf8 *) mxGetPr(tmp_mxa)) = dm->filter_high_fc;
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_FILTER_HIGH_CUTOFF_IDX_mat, tmp_mxa);

	// Fill in sampling frequencies
	dims[0] = dims[1] = 1; n_dims = 2;
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	*((sf8 *) mxGetPr(tmp_mxa)) = dm->sampling_frequency;
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_MATRIX_SAMPLING_FREQUENCY_IDX_mat, tmp_mxa);
	if (cmps->chan_freqs == TRUE_m12) {
		dims[0] = n_chans; dims[1] = 1; n_dims = 2;
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
		in_samp_freqs = (sf8 *) mxGetPr(tmp_mxa);
		seg_idx = G_get_segment_index_m12(slice->start_segment_number);
		for (i = 0; i < n_chans; ++i)
			in_samp_freqs[i] = sess->time_series_channels[i]->segments[seg_idx]->metadata_fps->metadata->time_series_section_2.sampling_frequency;
		mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_CHANNEL_SAMPLING_FREQUENCIES_IDX_mat, tmp_mxa);
	}

	// Fill page times
	dims[0] = 1; dims[1] = 1; n_dims = 2;
	
	// slice start time
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = slice->start_time;
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_SLICE_START_TIME_IDX_mat, tmp_mxa);
	// slice start time string
	STR_time_string_m12(slice->start_time, time_str, TRUE_m12, FALSE_m12, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_SLICE_START_TIME_STRING_IDX_mat, tmp_mxa);
	// slice end time
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = slice->end_time;
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_SLICE_END_TIME_IDX_mat, tmp_mxa);
	// slice end time string
	STR_time_string_m12(slice->end_time, time_str, TRUE_m12, FALSE_m12, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_SLICE_END_TIME_STRING_IDX_mat, tmp_mxa);
	
	// set globals
	med_session = sess;
	med_matrix = dm;

       	return(mat_matrix);
}


void	build_channel_names(SESSION_m12 *sess, mxArray *mat_matrix)
{
	si4				i, seg_idx, n_chans;
	CHANNEL_m12                     *chan;
	FILE_PROCESSING_STRUCT_m12	*metadata_fps;
	UNIVERSAL_HEADER_m12		*uh;
	mwSize				n_dims, dims[2];
	mxArray                         *mat_chans, *tmp_mxa;
	
	
	// create channel output array
	n_chans = sess->number_of_time_series_channels;
	dims[0] = n_chans; dims[1] = 1; n_dims = 2;
	mat_chans = mxCreateCellArray(n_dims, dims);

	// build name strings array
	seg_idx = G_get_segment_index_m12(sess->time_slice.start_segment_number);
	for (i = 0; i < n_chans; ++i) {
		chan = sess->time_series_channels[i];
		metadata_fps = chan->segments[seg_idx]->metadata_fps;
		uh = metadata_fps->universal_header;
		tmp_mxa = mxCreateString(uh->channel_name);
		mxSetCell(mat_chans, i, tmp_mxa);
	}
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_CHANNEL_NAMES_IDX_mat, mat_chans);

	return;
}


void	build_contigua(DATA_MATRIX_m12 *dm, mxArray *mat_matrix)
{
	TERN_m12			relative_days;
	si1                             time_str[TIME_STRING_BYTES_m12];
	si8                             i, n_contigs;
	CONTIGUON_m12			*contigua;
	mxArray                         *mat_contigua, *tmp_mxa;
	mwSize				n_dims, dims[2];
	const si4                       n_mat_contiguon_fields = NUMBER_OF_CONTIGUON_FIELDS_mat;
	const si1                       *mat_contiguon_field_names[] = CONTIGUON_FIELD_NAMES_mat;
	
	
	n_contigs = dm->number_of_contigua;
	if (n_contigs <= 0)
		return;
	
	if (globals_m12->RTO_known == TRUE_m12)
		relative_days = FALSE_m12;
	else
		relative_days = TRUE_m12;
	
	mat_contigua = mxCreateStructMatrix(n_contigs, 1, n_mat_contiguon_fields, mat_contiguon_field_names);
	dims[0] = dims[1] = 1; n_dims = 2;
	contigua = dm->contigua;
	for (i = 0; i < n_contigs; ++i) {
		// start index
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = contigua[i].start_sample_number + 1;
		mxSetFieldByNumber(mat_contigua, i, CONTIGUON_FIELDS_START_INDEX_IDX_mat, tmp_mxa);
		// end index
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = contigua[i].end_sample_number + 1;
		mxSetFieldByNumber(mat_contigua, i, CONTIGUON_FIELDS_END_INDEX_IDX_mat, tmp_mxa);
		// start time
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = contigua[i].start_time;
		mxSetFieldByNumber(mat_contigua, i, CONTIGUON_FIELDS_START_TIME_IDX_mat, tmp_mxa);
		// start time string
		STR_time_string_m12(contigua[i].start_time, time_str, TRUE_m12, relative_days, FALSE_m12);
		tmp_mxa = mxCreateString(time_str);
		mxSetFieldByNumber(mat_contigua, i, CONTIGUON_FIELDS_START_TIME_STRING_IDX_mat, tmp_mxa);
		// end time
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = contigua[i].end_time;
		mxSetFieldByNumber(mat_contigua, i, CONTIGUON_FIELDS_END_TIME_IDX_mat, tmp_mxa);
		// end time string
		STR_time_string_m12(contigua[i].end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
		tmp_mxa = mxCreateString(time_str);
		mxSetFieldByNumber(mat_contigua, i, CONTIGUON_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
	}
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_CONTIGUA_IDX_mat, mat_contigua);

	return;
}


void    build_session_records(SESSION_m12 *sess, DATA_MATRIX_m12 *dm, mxArray *mat_matrix)
{
	si4				n_segs, seg_idx;
	si8                     	i, j, k, n_items, tot_recs, n_recs;
	ui1                     	*rd;
	mxArray                 	*mat_records, *mat_record;
	FILE_PROCESSING_STRUCT_m12	*rd_fps;
	RECORD_HEADER_m12       	**rec_ptrs, *rh;

	
	n_segs = sess->time_slice.number_of_segments;

	// set up sorted records array
	tot_recs = 0;
	if (sess->record_data_fps != NULL && sess->record_indices_fps != NULL)
		tot_recs = sess->record_data_fps->number_of_items;
	if (sess->segmented_sess_recs != NULL) {
		seg_idx = G_get_segment_index_m12(sess->time_slice.start_segment_number);
		for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
			rd_fps = sess->segmented_sess_recs->record_data_fps[j];
			if (rd_fps != NULL)
				tot_recs += rd_fps->number_of_items;
		}
	}
	if (tot_recs == 0)
		return;

	rec_ptrs = (RECORD_HEADER_m12 **) malloc((size_t) tot_recs * sizeof(RECORD_HEADER_m12 *));
	if (rec_ptrs == NULL)
		return;
	n_recs = 0;
	if (sess->record_data_fps != NULL) {
		n_items = sess->record_data_fps->number_of_items;
		rd = sess->record_data_fps->record_data;
		for (i = 0; i < n_items; ++i) {
			rh = (RECORD_HEADER_m12 *) rd;
			switch (rh->type_code) {
				// excluded types
				case REC_Term_TYPE_CODE_m12:
				case REC_SyLg_TYPE_CODE_m12:
					break;
				default:  // include all other record types
					rec_ptrs[n_recs++] = rh;
					break;
			}
			rd += rh->total_record_bytes;
		}
	}
	if (sess->segmented_sess_recs != NULL) {
		for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
			rd_fps = sess->segmented_sess_recs->record_data_fps[j];
			if (rd_fps == NULL)
				continue;
			n_items = rd_fps->number_of_items;
			rd = rd_fps->record_data;
			for (k = 0; k < n_items; ++k) {
				rh = (RECORD_HEADER_m12 *) rd;
				switch (rh->type_code) {
					// excluded types
					case REC_Term_TYPE_CODE_m12:
					case REC_SyLg_TYPE_CODE_m12:
						break;
					default:  // include all other record types
						rec_ptrs[n_recs++] = rh;
						break;
				}
				rd += rh->total_record_bytes;
			}
		}
	}
	if (n_recs == 0) {
		free((void *) rec_ptrs);
		return;
	}
	qsort((void *) rec_ptrs, n_recs, sizeof(RECORD_HEADER_m12 *), rec_compare);
	
	// create matlab records
	mat_records = mxCreateCellMatrix(n_recs, 1);
	mxSetFieldByNumber(mat_matrix, 0, MATRIX_FIELDS_RECORDS_IDX_mat, mat_records);
	for (i = 0; i < n_recs; ++i) {
		mat_record = fill_record(rec_ptrs[i], dm);
		mxSetCell(mat_records, i, mat_record);
	}

	// clean up
	free((void *) rec_ptrs);

	return;
}


mxArray	*fill_record(RECORD_HEADER_m12 *rh, DATA_MATRIX_m12 *dm)
{
	TERN_m12		relative_days;
	si1                     *text, *stage_str, ver_str[8], *enc_str, enc_level, time_str[TIME_STRING_BYTES_m12];
	si4			i;
	ui8                     n_dims;
	si8			offset_samps, start_idx, end_idx;
	sf8                     ver, offset_secs;
	mwSize			dims[2];
	mxArray                 *mat_record, *tmp_mxa;
	const si4               n_mat_NlxP_v10_record_fields = NUMBER_OF_NLXP_v10_RECORD_FIELDS_mat;
	const si1               *mat_NlxP_v10_record_field_names[] = NLXP_v10_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Note_v10_record_fields = NUMBER_OF_NOTE_v10_RECORD_FIELDS_mat;
	const si1               *mat_Note_v10_record_field_names[] = NOTE_v10_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Note_v11_record_fields = NUMBER_OF_NOTE_v11_RECORD_FIELDS_mat;
	const si1               *mat_Note_v11_record_field_names[] = NOTE_v11_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Seiz_v10_record_fields = NUMBER_OF_SEIZ_v10_RECORD_FIELDS_mat;
	const si1               *mat_Seiz_v10_record_field_names[] = SEIZ_v10_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Epoc_v20_record_fields = NUMBER_OF_EPOC_v20_RECORD_FIELDS_mat;
	const si1               *mat_Epoc_v20_record_field_names[] = EPOC_v20_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Sgmt_v10_record_fields = NUMBER_OF_SGMT_v10_RECORD_FIELDS_mat;
	const si1               *mat_Sgmt_v10_record_field_names[] = SGMT_v10_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Sgmt_v11_record_fields = NUMBER_OF_SGMT_v11_RECORD_FIELDS_mat;
	const si1               *mat_Sgmt_v11_record_field_names[] = SGMT_v11_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Unkn_record_fields = NUMBER_OF_UNKN_RECORD_FIELDS_mat;
	const si1               *mat_Unkn_record_field_names[] = UNKN_RECORD_FIELD_NAMES_mat;
	CONTIGUON_m12		*contigua;
	REC_Note_v11_m12        *Note_v11;
	REC_Seiz_v10_m12        *Seiz_v10;
	REC_NlxP_v10_m12        *NlxP_v10;
	REC_Epoc_v20_m12        *Epoc_v20;
	REC_Sgmt_v10_m12        *Sgmt_v10;
	REC_Sgmt_v11_m12        *Sgmt_v11;

			
	dims[0] = dims[1] = 1; n_dims = 2;
	mat_record = NULL;

	switch (rh->type_code) {
		case REC_NlxP_TYPE_CODE_m12:
			if (rh->version_major == 1 && rh->version_minor == 0) {
				mat_record = mxCreateStructMatrix(1, 1, n_mat_NlxP_v10_record_fields, mat_NlxP_v10_record_field_names);
				break;
			}
			rh->type_code = 0;
			break;
		case REC_Note_TYPE_CODE_m12:
			if (rh->version_major == 1 && rh->version_minor == 0) {
				mat_record = mxCreateStructMatrix(1, 1, n_mat_Note_v10_record_fields, mat_Note_v10_record_field_names);
				break;
			}
			if (rh->version_major == 1 && rh->version_minor == 1) {
				mat_record = mxCreateStructMatrix(1, 1, n_mat_Note_v11_record_fields, mat_Note_v11_record_field_names);
				break;
			}
			rh->type_code = 0;
			break;
		case REC_Seiz_TYPE_CODE_m12:
			if (rh->version_major == 1 && rh->version_minor == 0) {
				mat_record = mxCreateStructMatrix(1, 1, n_mat_Seiz_v10_record_fields, mat_Seiz_v10_record_field_names);
				break;
			}
			rh->type_code = 0;
			break;
		case REC_Epoc_TYPE_CODE_m12:
			if (rh->version_major == 2 && rh->version_minor == 0) {
				mat_record = mxCreateStructMatrix(1, 1, n_mat_Epoc_v20_record_fields, mat_Epoc_v20_record_field_names);
				break;
			}
			rh->type_code = 0;
			break;
		case REC_Sgmt_TYPE_CODE_m12:
			if (rh->version_major == 1 && rh->version_minor == 0) {
				mat_record = mxCreateStructMatrix(1, 1, n_mat_Sgmt_v10_record_fields, mat_Sgmt_v10_record_field_names);
				break;
			}
			if (rh->version_major == 1 && rh->version_minor == 1) {
				mat_record = mxCreateStructMatrix(1, 1, n_mat_Sgmt_v11_record_fields, mat_Sgmt_v11_record_field_names);
				break;
			}
			rh->type_code = 0;
			break;
		default:
			rh->type_code = 0;
			break;
	}
	if (rh->type_code == 0)
		mat_record = mxCreateStructMatrix(1, 1, n_mat_Unkn_record_fields, mat_Unkn_record_field_names);

	if (globals_m12->RTO_known == TRUE_m12)
		relative_days = FALSE_m12;
	else
		relative_days = TRUE_m12;
	
	// start index (in matrix reference frame)
	if (dm->flags & DM_DSCNT_CONTIG_m12) {
		contigua = dm->contigua;
		for (i = 0; i < dm->number_of_contigua; ++i)
			if (rh->start_time <= contigua[i].end_time)
				break;
		offset_secs = (sf8) (rh->start_time - contigua[i].start_time) / (sf8) 1e6;
		offset_samps = (si8) round(offset_secs * dm->sampling_frequency);
		start_idx = contigua[i].start_sample_number + offset_samps;
		if (start_idx <= dm->sample_count) {
			tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
			*((si8 *) mxGetPr(tmp_mxa)) = start_idx;
			mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_START_INDEX_IDX_mat, tmp_mxa);
		}
	}
	// start time
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = rh->start_time;
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_START_TIME_IDX_mat, tmp_mxa);
	// start time string
	STR_time_string_m12(rh->start_time, time_str, TRUE_m12, relative_days, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_START_TIME_STRING_IDX_mat, tmp_mxa);
	// type string
	tmp_mxa = mxCreateString(rh->type_string);
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_TYPE_STRING_IDX_mat, tmp_mxa);
	// type code
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxUINT32_CLASS, mxREAL);
	*((ui4 *) mxGetPr(tmp_mxa)) = rh->type_code;
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_TYPE_CODE_IDX_mat, tmp_mxa);
	// version string
	ver = (sf8) rh->version_major + ((sf8) rh->version_minor / (sf8) 1000.0);
	sprintf(ver_str, "%0.3lf", ver);
	tmp_mxa = mxCreateString(ver_str);
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_VERSION_STRING_IDX_mat, tmp_mxa);
	// encryption
	enc_level = rh->encryption_level;
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT8_CLASS, mxREAL);
	*((si1 *) mxGetPr(tmp_mxa)) = enc_level;
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_ENCRYPTION_IDX_mat, tmp_mxa);
	// encryption string
	switch (enc_level) {
		case NO_ENCRYPTION_m12:
			enc_str = "none";
			break;
		case LEVEL_1_ENCRYPTION_m12:
			enc_str = "level 1, encrypted";
			break;
		case LEVEL_2_ENCRYPTION_m12:
			enc_str = "level 2, encrypted";
			break;
		case LEVEL_1_ENCRYPTION_DECRYPTED_m12:
			enc_str = "level 1, decrypted";
			break;
		case LEVEL_2_ENCRYPTION_DECRYPTED_m12:
			enc_str = "level 2, decrypted";
			break;
		default:
			enc_str = "<unrecognized level>";
			enc_level = LEVEL_1_ENCRYPTION_m12;  // set to any encrypted level
			break;
	}
	tmp_mxa = mxCreateString(enc_str);
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_ENCRYPTION_STRING_IDX_mat, tmp_mxa);

	if (enc_level <= 0) {
		switch (rh->type_code) {
			case REC_NlxP_TYPE_CODE_m12:
				NlxP_v10 = (REC_NlxP_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
				// subport
				tmp_mxa = mxCreateNumericArray(n_dims, dims, mxUINT8_CLASS, mxREAL);
				*((ui1 *) mxGetPr(tmp_mxa)) = NlxP_v10->subport;
				mxSetFieldByNumber(mat_record, 0, NLXP_v10_RECORD_FIELDS_SUBPORT_IDX_mat, tmp_mxa);
				// value
				tmp_mxa = mxCreateNumericArray(n_dims, dims, mxUINT32_CLASS, mxREAL);
				*((ui4 *) mxGetPr(tmp_mxa)) = NlxP_v10->value;
				mxSetFieldByNumber(mat_record, 0, NLXP_v10_RECORD_FIELDS_VALUE_IDX_mat, tmp_mxa);
				break;
			case REC_Note_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 0) {
					// text
					if (rh->total_record_bytes > RECORD_HEADER_BYTES_m12) {
						text = (si1 *) rh + RECORD_HEADER_BYTES_m12;
						if (*text)
							tmp_mxa = mxCreateString(text);
						else
							tmp_mxa = mxCreateString("<empty note>");
					} else {
						tmp_mxa = mxCreateString("<empty note>");
					}
					mxSetFieldByNumber(mat_record, 0, NOTE_v10_RECORD_FIELDS_TEXT_IDX_mat, tmp_mxa);
				} else if (rh->version_major == 1 && rh->version_minor == 1) {
					Note_v11 = (REC_Note_v11_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
					if (dm->flags & DM_DSCNT_CONTIG_m12) {
						contigua = dm->contigua;
						for (i = 0; i < dm->number_of_contigua; ++i)
							if (Note_v11->end_time <= contigua[i].end_time)
								break;
						offset_secs = (sf8) (Note_v11->end_time - contigua[i].start_time) / (sf8) 1e6;
						offset_samps = (si8) round(offset_secs * dm->sampling_frequency);
						end_idx = contigua[i].start_sample_number + offset_samps;
						if (end_idx <= dm->sample_count) {
							tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
							*((si8 *) mxGetPr(tmp_mxa)) = end_idx;
							mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_INDEX_IDX_mat, tmp_mxa);
						}
					}
					// end time
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					*((si8 *) mxGetPr(tmp_mxa)) = Note_v11->end_time;
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					STR_time_string_m12(Note_v11->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
					tmp_mxa = mxCreateString(time_str);
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// text
					text = Note_v11->text;
					if (*text)
						tmp_mxa = mxCreateString(text);
					else
						tmp_mxa = mxCreateString("<empty note>");
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_TEXT_IDX_mat, tmp_mxa);
				}
				break;
			case REC_Seiz_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 0) {
					Seiz_v10 = (REC_Seiz_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
					if (dm->flags & DM_DSCNT_CONTIG_m12) {
						contigua = dm->contigua;
						for (i = 0; i < dm->number_of_contigua; ++i)
							if (Seiz_v10->end_time <= contigua[i].end_time)
								break;
						offset_secs = (sf8) (Seiz_v10->end_time - contigua[i].start_time) / (sf8) 1e6;
						offset_samps = (si8) round(offset_secs * dm->sampling_frequency);
						end_idx = contigua[i].start_sample_number + offset_samps;
						if (end_idx <= dm->sample_count) {
							tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
							*((si8 *) mxGetPr(tmp_mxa)) = end_idx;
							mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_END_INDEX_IDX_mat, tmp_mxa);
						}
					}
					// end time
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					// end time string
					if (Seiz_v10->end_time > 0) {
						*((si8 *) mxGetPr(tmp_mxa)) = Seiz_v10->end_time;
						mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
						STR_time_string_m12(Seiz_v10->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
						tmp_mxa = mxCreateString(time_str);
					} else {
						tmp_mxa = mxCreateString("<no entry>");
					}
					mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// description
					text = Seiz_v10->description;
					if (*text)
						tmp_mxa = mxCreateString(text);
					else
						tmp_mxa = mxCreateString("<no description>");
					mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat, tmp_mxa);
				}
				break;
			case REC_Epoc_TYPE_CODE_m12:
				Epoc_v20 = (REC_Epoc_v20_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
				// end time
				tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
				*((si8 *) mxGetPr(tmp_mxa)) = Epoc_v20->end_time;
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
				// end time string
				STR_time_string_m12(Epoc_v20->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
				tmp_mxa = mxCreateString(time_str);
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
				// stage code
				tmp_mxa = mxCreateNumericArray(n_dims, dims, mxUINT8_CLASS, mxREAL);
				*((ui1 *) mxGetPr(tmp_mxa)) = Epoc_v20->stage_code;
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_STAGE_CODE_IDX_mat, tmp_mxa);
				// stage string
				switch (Epoc_v20->stage_code) {
					case REC_Epoc_v20_STAGE_AWAKE_m12:
						stage_str = "awake";
						break;
					case REC_Epoc_v20_STAGE_NREM_1_m12:
						stage_str = "non-REM 1";
						break;
					case REC_Epoc_v20_STAGE_NREM_2_m12:
						stage_str = "non-REM 2";
						break;
					case REC_Epoc_v20_STAGE_NREM_3_m12:
						stage_str = "non-REM 3";
						break;
					case REC_Epoc_v20_STAGE_NREM_4_m12:
						stage_str = "non-REM 4";
						break;
					case REC_Epoc_v20_STAGE_REM_m12:
						stage_str = "REM";
						break;
					case REC_Epoc_v20_STAGE_UNKNOWN_m12:
						stage_str = "unknown";
						break;
					default:
						G_warning_message_m12("%s(): Unrecognized Epoc v2.0 stage code (%hhu)\n", __FUNCTION__, Epoc_v20->stage_code);
						break;
				}
				tmp_mxa = mxCreateString(stage_str);
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_STAGE_STRING_IDX_mat, tmp_mxa);
				// scorer ID
				tmp_mxa = mxCreateString(Epoc_v20->scorer_id);
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_SCORER_ID_IDX_mat, tmp_mxa);
				break;
			case REC_Sgmt_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 0) {
					Sgmt_v10 = (REC_Sgmt_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
					// end time
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					*((si8 *) mxGetPr(tmp_mxa)) = Sgmt_v10->end_time;
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					STR_time_string_m12(Sgmt_v10->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
					tmp_mxa = mxCreateString(time_str);
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// start sample number
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					if (Sgmt_v10->start_sample_number == SAMPLE_NUMBER_NO_ENTRY_m12)
						*((si8 *) mxGetPr(tmp_mxa)) = -1;  // no entry
					else
						*((si8 *) mxGetPr(tmp_mxa)) = Sgmt_v10->start_sample_number + 1;  // convert to Matlab indexing
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// end sample number
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					if (Sgmt_v10->end_sample_number == SAMPLE_NUMBER_NO_ENTRY_m12)
						*((si8 *) mxGetPr(tmp_mxa)) = -1;  // no entry
					else
						*((si8 *) mxGetPr(tmp_mxa)) = Sgmt_v10->end_sample_number + 1;  // convert to Matlab indexing
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// segment number
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
					*((si4 *) mxGetPr(tmp_mxa)) = Sgmt_v10->segment_number;
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat, tmp_mxa);
					// segment UID
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxUINT64_CLASS, mxREAL);
					*((ui8 *) mxGetPr(tmp_mxa)) = Sgmt_v10->segment_UID;
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_SEGMENT_UID_IDX_mat, tmp_mxa);
					// acquisition channel number
					if (Sgmt_v10->acquisition_channel_number == REC_Sgmt_v10_ACQUISITION_CHANNEL_NUMBER_ALL_CHANNELS_m12) {
						tmp_mxa = mxCreateString("all channels");
					} else {
						tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
						*((si4 *) mxGetPr(tmp_mxa)) = Sgmt_v10->acquisition_channel_number;
					}
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat, tmp_mxa);
					// description
					if (rh->total_record_bytes > (RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12)) {
						text = (si1 *) rh + RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12;
						if (*text)
							tmp_mxa = mxCreateString(text);
						else
							tmp_mxa = mxCreateString("<no description>");
					} else {
						tmp_mxa = mxCreateString("<no description>");
					}
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat, tmp_mxa);
				} else if (rh->version_major == 1 && rh->version_minor == 1) {
					Sgmt_v11 = (REC_Sgmt_v11_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
					// end time
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					*((si8 *) mxGetPr(tmp_mxa)) = Sgmt_v11->end_time;
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					STR_time_string_m12(Sgmt_v11->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
					tmp_mxa = mxCreateString(time_str);
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// start sample number
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					if (Sgmt_v11->start_sample_number == SAMPLE_NUMBER_NO_ENTRY_m12)
						*((si8 *) mxGetPr(tmp_mxa)) = -1;  // no entry
					else
						*((si8 *) mxGetPr(tmp_mxa)) = Sgmt_v11->start_sample_number + 1;  // convert to Matlab indexing
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// end sample number
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					if (Sgmt_v11->end_sample_number == SAMPLE_NUMBER_NO_ENTRY_m12)
						*((si8 *) mxGetPr(tmp_mxa)) = -1;  // no entry
					else
						*((si8 *) mxGetPr(tmp_mxa)) = Sgmt_v11->end_sample_number + 1;  // convert to Matlab indexing
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// segment number
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
					*((si4 *) mxGetPr(tmp_mxa)) = Sgmt_v11->segment_number;
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat, tmp_mxa);
					// acquisition channel number
					if (Sgmt_v11->acquisition_channel_number == REC_Sgmt_v11_ACQUISITION_CHANNEL_NUMBER_ALL_CHANNELS_m12) {
						tmp_mxa = mxCreateString("all channels");
					} else {
						tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
						*((si4 *) mxGetPr(tmp_mxa)) = Sgmt_v11->acquisition_channel_number;
					}
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat, tmp_mxa);
					// description
					if (rh->total_record_bytes > (RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12)) {
						text = (si1 *) rh + RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12;
						if (*text)
							tmp_mxa = mxCreateString(text);
						else
							tmp_mxa = mxCreateString("<no description>");
					} else {
						tmp_mxa = mxCreateString("<no description>");
					}
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_DESCRIPTION_IDX_mat, tmp_mxa);
					break;
				}
				break;
			default:  // Unknown record type
				tmp_mxa = mxCreateString("<unknown record type>");
				mxSetFieldByNumber(mat_record, 0, UNKN_RECORD_FIELDS_COMMENT_IDX_mat, tmp_mxa);
				break;
		}
	} else {  // no access
		tmp_mxa = mxCreateString("<no access>");
		switch (rh->type_code) {
			case REC_NlxP_TYPE_CODE_m12:
				// subport
				mxSetFieldByNumber(mat_record, 0, NLXP_v10_RECORD_FIELDS_SUBPORT_IDX_mat, tmp_mxa);
				// value
				mxSetFieldByNumber(mat_record, 0, NLXP_v10_RECORD_FIELDS_VALUE_IDX_mat, tmp_mxa);
				break;
			case REC_Note_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 0) {
					// text
					mxSetFieldByNumber(mat_record, 0, NOTE_v10_RECORD_FIELDS_TEXT_IDX_mat, tmp_mxa);
				}
				if (rh->version_major == 1 && rh->version_minor == 1) {
					// end index
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_INDEX_IDX_mat, tmp_mxa);
					// end time
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// text
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_TEXT_IDX_mat, tmp_mxa);
				}
				break;
			case REC_Seiz_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 1) {
					// end index
					mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_END_INDEX_IDX_mat, tmp_mxa);
					// end time
					mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// text
					mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat, tmp_mxa);
				}
				break;
			case REC_Epoc_TYPE_CODE_m12:
				// end time
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
				// end time string
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
				// stage code
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_STAGE_CODE_IDX_mat, tmp_mxa);
				// stage string
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_STAGE_STRING_IDX_mat, tmp_mxa);
				// scorer id string
				mxSetFieldByNumber(mat_record, 0, EPOC_v20_RECORD_FIELDS_SCORER_ID_IDX_mat, tmp_mxa);
				break;
			case REC_Sgmt_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 0) {
					// end time
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// start sample number
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// end sample number
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// segment number
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat, tmp_mxa);
					// segment UID
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_SEGMENT_UID_IDX_mat, tmp_mxa);
					// acquisition channel number
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat, tmp_mxa);
					// description
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat, tmp_mxa);
				} else if (rh->version_major == 1 && rh->version_minor == 1) {
					// end time
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// start sample number
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// end sample number
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat, tmp_mxa);
					// segment number
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat, tmp_mxa);
					// acquisition channel number
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat, tmp_mxa);
					// description
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_DESCRIPTION_IDX_mat, tmp_mxa);
				}
				break;
			default:  // Unknown record type
				mxSetFieldByNumber(mat_record, 0, UNKN_RECORD_FIELDS_COMMENT_IDX_mat, tmp_mxa);
				break;
		}

	}

	return(mat_record);
}


TERN_m12	get_logical(const mxArray *mx_arr)
{
	TERN_m12	val;
	si1		temp_str[16];
	
	
	val = UNKNOWN_m12;
	if (mxGetClassID(mx_arr) == mxCHAR_CLASS) {
		mxGetString(mx_arr, temp_str, 16);
		if (*temp_str == 't' || *temp_str == 'T' || *temp_str == 'y' || *temp_str == 'Y' || *temp_str == '1')
			val = TRUE_m12;
		else if (*temp_str == 'f' || *temp_str == 'F' || *temp_str == 'n' || *temp_str == 'N' || *temp_str == '0')
			val = FALSE_m12;
	} else if (mxIsLogicalScalar(mx_arr)) {
		if (mxIsLogicalScalarTrue(mx_arr) == 1)
			val = TRUE_m12;
		else
			val = FALSE_m12;
	} else if (mxIsScalar(mx_arr)) {
		if (mxGetScalar(mx_arr) == 1)
			val = TRUE_m12;
		else if (mxGetScalar(mx_arr) == 0)
			val = FALSE_m12;
	}
	
	return(val);
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
                        return((si8) round(tmp_sf8));
                case mxINT8_CLASS:
                case mxUINT8_CLASS:
                case mxINT16_CLASS:
                case mxUINT16_CLASS:
                case mxINT32_CLASS:
                case mxUINT32_CLASS:
                case mxINT64_CLASS:
                case mxUINT64_CLASS:
			return((si8) mxGetScalar(mx_arr));
		case mxLOGICAL_CLASS:
			if (mxIsLogicalScalarTrue(mx_arr) == 1)
				return((si8) 1);
			else
				return((si8) 0);
                default:
                        return((si8) UUTC_NO_ENTRY_m12);
        }
}


si4     rec_compare(const void *a, const void *b)
{
	si8			time_d;
	RECORD_HEADER_m12	*rha, *rhb;
	
	
	rha = *((RECORD_HEADER_m12 **) a);
	rhb = *((RECORD_HEADER_m12 **) b);
	time_d = rha->start_time - rhb->start_time;
	
	// sort by time
	if (time_d > 0)
		return(1);
	if (time_d < 0)
		return(-1);
	
	// if same time, sort by location in memory
	if ((ui8) rha >= (ui8) rhb)
		return(1);

	return(-1);
}

