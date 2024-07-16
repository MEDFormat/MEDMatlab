
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2021


#include "matrix_MED_exec.h"

// Saved Session & Medlib Globals
static SESSION_m12		*session_ptr = NULL;
static DATA_MATRIX_m12		*matrix_ptr = NULL;
static si4			globals_list_len = 0;
static GLOBALS_m12		**globals_list_ptr = NULL;
static pthread_mutex_t_m12	globals_list_mutex;
static GLOBAL_TABLES_m12	*global_tables_ptr = NULL;


// Mex Exit function
void	mexExitFunction(void) {
	
	// session cannot be cleared without clearing globals at this time
	if (session_ptr != NULL) {
		
		extern si4			globals_list_len_m12;
		extern GLOBALS_m12		**globals_list_m12;
		extern pthread_mutex_t_m12	globals_list_mutex_m12;
		extern GLOBAL_TABLES_m12	*global_tables_m12;
		
		// free session
		G_free_session_m12(session_ptr, TRUE_m12);
		
		// free matrix
		if (matrix_ptr != NULL) {
			matrix_ptr->data = matrix_ptr->range_minima = matrix_ptr->range_maxima = NULL;  // keep workspace allocated memory
			DM_free_matrix_m12(matrix_ptr, TRUE_m12);
		}

		// free globals
		globals_list_len_m12 = 1;  // this is not set up for parallel mex calls right now, so should only be one entry
		globals_list_mutex_m12 = globals_list_mutex;
		globals_list_m12 = globals_list_ptr;
		// pid is preserved between mex calls
		global_tables_m12 = global_tables_ptr;
		G_free_globals_m12(TRUE_m12);
		
		// not necessary when this is called because function cleared, but also used for reset
		session_ptr = NULL;
		matrix_ptr = NULL;
		globals_list_len = 0;
		globals_list_ptr = NULL;
		global_tables_ptr = NULL;
	}
	
	return;
}


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
	extern si4			globals_list_len_m12;
	extern GLOBALS_m12		**globals_list_m12;
	extern pthread_mutex_t_m12	globals_list_mutex_m12;
	extern GLOBAL_TABLES_m12	*global_tables_m12;
        TERN_m12			antialias, detrend, trace_ranges;
        void				*file_list;
	ui1				persist_mode;
        si1				**file_list_p, password[PASSWORD_BYTES_m12], temp_str[16];
        si4				i, n_files, len, max_len;
        si8				start_time, end_time, n_out_samps, tmp_si8;
        mxArray				*mx_cell_p, *matrix;

	
	//  check for proper number of arguments
	if (nlhs != 1)
		mexErrMsgTxt("One output: matrix_MED structure\n");
	plhs[0] = mxCreateLogicalScalar((mxLogical) 0);  // set "false" return value for subsequent errors
	if (nrhs < 4 || nrhs > 9)
		mexErrMsgTxt("Four to 9 inputs: chan_list, start_time, end_time, n_out_samps, [password], [antialias ([true] / false)], [detrend (true / [false])], [trace_ranges (true / [false])], [persistence_mode]\n");

	// get persistence mode if passed
	persist_mode = PERSIST_READ_CLOSE;  // default
	if (nrhs == 9) {
		if (mxIsEmpty(prhs[8]) == 0) {  // passed char array
			if (mxGetClassID(prhs[8]) == mxCHAR_CLASS) {
				mxGetString(prhs[8], temp_str, 16);
				switch (*temp_str) {
					case 'c':  // "close"
					case 'C':
						persist_mode = PERSIST_CLOSE;
						break;
					case 'o':  // "open"
					case 'O':
						persist_mode = PERSIST_OPEN;
						break;
					case 'r':  // "read"
					case 'R':
						if (*(temp_str + 4) == 0)  // "read"
							persist_mode = PERSIST_READ;
						else if (*(temp_str + 5) == 'n' || *(temp_str + 5) == 'N')  // "read new"
							persist_mode = PERSIST_READ_NEW;
						break;
					default:  // includes "none" & "read close" (default)
						break;
				}
			} else {
				tmp_si8 = get_si8_scalar(prhs[8]);
				if (tmp_si8 >= 1 && tmp_si8 <= 7)
					persist_mode = (ui1) tmp_si8;
			}
		}
	}

	if (persist_mode & (PERSIST_CLOSE | PERSIST_OPEN)) {
		mexExitFunction();
		if (persist_mode == PERSIST_CLOSE) {  // "read" flag not set
			mxDestroyArray(plhs[0]);
			plhs[0] = mxCreateLogicalScalar((mxLogical) 1);  // return "true"
			return;
		}
	}
	
	// get the input file name(s) (argument 1)
	n_files = max_len = 0;
	if (mxIsEmpty(prhs[0]) == 1)
		mexErrMsgTxt("No input files specified\n");
	if (mxGetClassID(prhs[0]) == mxCHAR_CLASS) {
		max_len = mxGetNumberOfElements(prhs[0]) + 1; // Get the length of the input string
		if (max_len > FULL_FILE_NAME_BYTES_m12)
			mexErrMsgTxt("Input File Name (input 1) is too long\n");
	} else if (mxGetClassID(prhs[0]) == mxCELL_CLASS) {
		n_files = mxGetNumberOfElements(prhs[0]);
		if (n_files == 0)
			mexErrMsgTxt("'file_list' (input 1) cell array contains no entries\n");
		for (i = max_len = 0; i < n_files; ++i) {
			mx_cell_p = mxGetCell(prhs[0], i);
			if (mxGetClassID(mx_cell_p) != mxCHAR_CLASS)
				mexErrMsgTxt("Elements of file_list cell array must be char arrays\n");
			len = mxGetNumberOfElements(mx_cell_p) + 1; // Get the length of the input string
			if (len > FULL_FILE_NAME_BYTES_m12)
				mexErrMsgTxt("'file_list' (input 1) is too long\n");
			if (len > max_len)
				max_len = len;
		}
	} else {
		mexErrMsgTxt("'file_list' (input 1) must be a string or cell array\nStrings may include regular expressions (regex)\n");
	}

        // start_time
        start_time = UUTC_NO_ENTRY_m12;
	if (mxIsEmpty(prhs[1]) == 1)
		mexErrMsgTxt("No start_time specified (input 2)\n");
	if (mxGetClassID(prhs[1]) == mxCHAR_CLASS) {
		len = mxGetNumberOfElements(prhs[1]) + 1; // Get the length of the input string
		if (len <= 16)
			mxGetString(prhs[1], temp_str, len);
		else
			mexErrMsgTxt("'start_time' (input 2) can be specified as 'start' (default), or an integer\n");
		if (strcmp(temp_str, "start") == 0)
			start_time = BEGINNING_OF_TIME_m12;
		else
			mexErrMsgTxt("'start_time' (input 2) can be specified as 'start' (default), or an integer\n");
	} else {
		start_time = get_si8_scalar(prhs[1]);
	}
        
        // end_time
        end_time = UUTC_NO_ENTRY_m12;
	if (mxIsEmpty(prhs[2]) == 1)
		mexErrMsgTxt("No end_time specified (input 3)\n");
	if (mxGetClassID(prhs[2]) == mxCHAR_CLASS) {
		len = mxGetNumberOfElements(prhs[2]) + 1; // Get the length of the input string
		if (len > 16)
			mexErrMsgTxt("'end_time' (input 3) can be specified as 'end' (default), or an integer\n");
		else
			mxGetString(prhs[2], temp_str, len);
		if (strcmp(temp_str, "end") == 0)
			end_time = END_OF_TIME_m12;
		else
			mexErrMsgTxt("'end_time' (input 3) can be specified as 'end' (default), or an integer\n");
	} else {
		end_time = get_si8_scalar(prhs[2]);
	}

        // n_out_samps
	n_out_samps = 0;
	if (mxIsEmpty(prhs[3]) == 1)
		mexErrMsgTxt("No output_samps specified (input 4)\n");
	n_out_samps = get_si8_scalar(prhs[3]);

       // password
        *password = 0;
	if (nrhs > 4) {
		if (mxIsEmpty(prhs[4]) == 0) {
			if (mxGetClassID(prhs[4]) == mxCHAR_CLASS) {
				len = mxGetNumberOfElements(prhs[4]) + 1; // Get the length of the input string
				if (len > PASSWORD_BYTES_m12)
					mexErrMsgTxt("'password' (input 5) is too long\n");
				else
					mxGetString(prhs[4], password, len);
			} else {
				mexErrMsgTxt("'password' (input 5) must be a string\n");
			}
		}
	}
        
	// antialias
	antialias = TRUE_m12;
	if (nrhs > 5) {
		if (mxIsEmpty(prhs[5]) == 0) {
			antialias = UNKNOWN_m12;
			if (mxGetClassID(prhs[5]) == mxCHAR_CLASS) {
				mxGetString(prhs[5], temp_str, 16);
				if (*temp_str == 't' || *temp_str == 'T' || *temp_str == 'y' || *temp_str == 'Y' || *temp_str == '1')
					antialias = TRUE_m12;
				else if (*temp_str == 'f' || *temp_str == 'F' || *temp_str == 'n' || *temp_str == 'N' || *temp_str == '0')
					antialias = FALSE_m12;
			} else if (mxIsLogicalScalar(prhs[5])) {
				if (mxIsLogicalScalarTrue(prhs[5]) == 1)
					antialias = TRUE_m12;
				else
					antialias = FALSE_m12;
			} else if (mxIsScalar(prhs[5])) {
				if (mxGetScalar(prhs[5]) == 1)
					antialias = TRUE_m12;
				else if (mxGetScalar(prhs[5]) == 0)
					antialias = FALSE_m12;
			}
			if (antialias == UNKNOWN_m12)
				mexErrMsgTxt("'antialias' (input 6) can be either true (default) or false only\n");
		}
	}

	// detrend
	detrend = FALSE_m12;
	if (nrhs > 6) {
		if (mxIsEmpty(prhs[6]) == 0) {
			detrend = UNKNOWN_m12;
			if (mxGetClassID(prhs[6]) == mxCHAR_CLASS) {
				mxGetString(prhs[6], temp_str, 16);
				if (*temp_str == 't' || *temp_str == 'T' || *temp_str == 'y' || *temp_str == 'Y' || *temp_str == '1')
					detrend = TRUE_m12;
				else if (*temp_str == 'f' || *temp_str == 'F' || *temp_str == 'n' || *temp_str == 'N' || *temp_str == '0')
					detrend = FALSE_m12;
			} else if (mxIsLogicalScalar(prhs[6])) {
				if (mxIsLogicalScalarTrue(prhs[6]) == 1)
					detrend = TRUE_m12;
				else
					detrend = FALSE_m12;
			} else if (mxIsScalar(prhs[6])) {
				if (mxGetScalar(prhs[6]) == 1)
					detrend = TRUE_m12;
				else if (mxGetScalar(prhs[6]) == 0)
					detrend = FALSE_m12;
			}
			if (detrend == UNKNOWN_m12)
				mexErrMsgTxt("'detrend' (input 7) can be either true or false (default) only\n");
		}
	}

	// trace_ranges
	trace_ranges = FALSE_m12;
	if (nrhs > 7) {
		if (mxIsEmpty(prhs[7]) == 0) {
			trace_ranges = UNKNOWN_m12;
			if (mxGetClassID(prhs[7]) == mxCHAR_CLASS) {
				mxGetString(prhs[7], temp_str, 16);
				if (*temp_str == 't' || *temp_str == 'T' || *temp_str == 'y' || *temp_str == 'Y' || *temp_str == '1')
					trace_ranges = TRUE_m12;
				else if (*temp_str == 'f' || *temp_str == 'F' || *temp_str == 'n' || *temp_str == 'N' || *temp_str == '0')
					trace_ranges = FALSE_m12;
			} else if (mxIsLogicalScalar(prhs[7])) {
				if (mxIsLogicalScalarTrue(prhs[7]) == 1)
					trace_ranges = TRUE_m12;
				else
					trace_ranges = FALSE_m12;
			} else if (mxIsScalar(prhs[7])) {
				if (mxGetScalar(prhs[7]) == 1)
					trace_ranges = TRUE_m12;
				else if (mxGetScalar(prhs[7]) == 0)
					trace_ranges = FALSE_m12;
			}
			if (trace_ranges == UNKNOWN_m12)
				mexErrMsgTxt("'trace_ranges' (input 8) can be either true or false (default) only\n");
		}
	}

	// initialize MED library
	if (globals_list_len) {
		globals_list_len_m12 = 1;  // this is not set up for parallel mex calls right now, so should only be one entry
		globals_list_mutex_m12 = globals_list_mutex;
		globals_list_m12 = globals_list_ptr;
		// pid is preserved between mex calls
		global_tables_m12 = global_tables_ptr;
	} else {
		mexAtExit(mexExitFunction);
		PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(MAX_CHANNELS, 1), FALSE_m12);
		PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);

		G_initialize_medlib_m12(FALSE_m12, FALSE_m12);
		globals_list_len = 1;  // this is not set up for parallel mex calls right now, so should only be one entry
		globals_list_mutex = globals_list_mutex_m12;
		globals_list_ptr = globals_list_m12;
		global_tables_ptr = global_tables_m12;
	}
	
	// create input file list
	file_list = NULL;
	switch (n_files) {
		case 0:  // single string passed
			file_list = calloc_m12((size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			mxGetString(prhs[0], (si1 *) file_list, max_len);
			break;
		case 1:   // single string passed in cell array
			file_list = calloc_m12((size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			mx_cell_p = mxGetCell(prhs[0], 0);
			mxGetString(mx_cell_p, (si1 *) file_list, max_len);
			n_files = 0;  // (indicates single string)
			break;
		default:  // multiple strings in cell array
			file_list = (void *) calloc_2D_m12((size_t) n_files, (size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			file_list_p = (si1 **) file_list;
			for (i = 0; i < n_files; ++i) {
				mx_cell_p = mxGetCell(prhs[0], i);
				mxGetString(mx_cell_p, file_list_p[i], max_len);
			}
			break;
	}

       	// get out of here
        matrix = matrix_MED(file_list, n_files, start_time, end_time, n_out_samps, password, antialias, detrend, trace_ranges, persist_mode);
	if (matrix != NULL) {
		mxDestroyArray(plhs[0]);
		plhs[0] = matrix;
	}

        // clean up
        free_m12((void *) file_list, __FUNCTION__);
	
	matrix_ptr->data = matrix_ptr->range_minima = matrix_ptr->range_maxima = NULL;  // this memory belongs return variable, must be allocated with each call
	if (persist_mode & PERSIST_CLOSE)
		mexExitFunction();
	
        return;
}


mxArray	*matrix_MED(void *file_list, si4 n_files, si8 start_time, si8 end_time, si8 n_out_samps, si1 *password, TERN_m12 antialias, TERN_m12 detrend, TERN_m12 trace_ranges, ui1 persist_mode)
{
	si1			time_str[TIME_STRING_BYTES_m12];
	si4			n_chans, seg_idx;
	ui8			flags;
        si8			i;
        sf8			*in_samp_freqs;
	void			*samps, *mins, *maxs;
	SESSION_m12		*sess;
        TIME_SLICE_m12		slice;
        mxArray			*mat_raw_page, *tmp_mxa;
	mwSize			n_dims, dims[2];
	DATA_MATRIX_m12		*dm;
        const si4		n_mat_raw_page_fields = NUMBER_OF_MATRIX_FIELDS_mat;
        const si1		*mat_raw_page_field_names[] = MATRIX_FIELD_NAMES_mat;
	

	// copy globals
	sess = session_ptr;
	dm = matrix_ptr;

	// open session
	G_initialize_time_slice_m12(&slice);
	slice.start_time = start_time;
	slice.end_time = end_time;
	flags = (LH_INCLUDE_TIME_SERIES_CHANNELS_m12 | LH_READ_SLICE_SEGMENT_DATA_m12 | LH_READ_SLICE_SESSION_RECORDS_m12 | LH_READ_SLICE_SEGMENTED_SESS_RECS_m12);
	if (persist_mode & PERSIST_CLOSE)
		flags |= LH_NO_CPS_CACHING_m12;  // not efficient for single reads
	else
		flags |= LH_MAP_ALL_SEGMENTS_m12;  // more efficient for sequential reads
	
	sess = G_open_session_m12(sess, &slice, file_list, n_files, flags, password);
	if (persist_mode == PERSIST_OPEN) {
		if (sess == NULL) {
			return(NULL);
		} else {
			session_ptr = sess;  // save session
			return(mxCreateLogicalScalar((mxLogical) 1));
		}
	}

	// Create raw page output structure
	n_chans = sess->number_of_time_series_channels;
	mat_raw_page = mxCreateStructMatrix(1, 1, n_mat_raw_page_fields, mat_raw_page_field_names);
	dims[0] = n_out_samps; dims[1] = n_chans; n_dims = 2;
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_SAMPLES_IDX_mat, tmp_mxa);
	samps = (void *) mxGetPr(tmp_mxa);
	if (trace_ranges == TRUE_m12) {
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
		mxSetFieldByNumber(mat_raw_page, 0, MATRIX_MINIMA_IDX_mat, tmp_mxa);
		mins = (void *) mxGetPr(tmp_mxa);
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
		mxSetFieldByNumber(mat_raw_page, 0, MATRIX_MAXIMA_IDX_mat, tmp_mxa);
		maxs = (void *) mxGetPr(tmp_mxa);
	}

	// Create DM matrix structure
	if (dm == NULL)
		dm = (DATA_MATRIX_m12 *) calloc_m12((size_t) 1, sizeof(DATA_MATRIX_m12), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
	dm->channel_count = n_chans;
	dm->sample_count = n_out_samps;
	dm->data_bytes = (n_out_samps * n_chans) << 3;
	dm->data = samps;  // Matlab allocated memory
	if (trace_ranges == TRUE_m12) {
		dm->range_minima = mins;  // Matlab allocated pointer
		dm->range_maxima = maxs;  // Matlab allocated pointer
	}
	dm->flags = ( DM_TYPE_SF8_m12 | DM_FMT_CHANNEL_MAJOR_m12 | DM_EXTMD_SAMP_COUNT_m12 | DM_EXTMD_RELATIVE_LIMITS_m12 | DM_DSCNT_CONTIG_m12 | DM_INTRP_UP_MAKIMA_DN_LINEAR_m12 );
	if (antialias == TRUE_m12)
		dm->flags |= DM_FILT_ANTIALIAS_m12;
	if (detrend == TRUE_m12)
		dm->flags |= DM_DETREND_m12;
	if (trace_ranges == TRUE_m12)
		dm->flags |= DM_TRACE_RANGES_m12;

	// Build matrix
	dm = DM_get_matrix_m12(dm, sess, NULL, FALSE_m12);
	if (dm == NULL)
		return(NULL);

	// Build contigua
	build_contigua(dm, mat_raw_page);
	
	// Build session records
	build_session_records(sess, dm, mat_raw_page);
	
	// Fill in channel sampling frequencies
	dims[0] = n_chans; dims[1] = 1; n_dims = 2;
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_FIELDS_SAMPLING_FREQUENCIES_IDX_mat, tmp_mxa);
	in_samp_freqs = (sf8 *) mxGetPr(tmp_mxa);
	slice = sess->time_slice;
	seg_idx = G_get_segment_index_m12(slice.start_segment_number);
	for (i = 0; i < n_chans; ++i)
		in_samp_freqs[i] = sess->time_series_channels[i]->segments[seg_idx]->metadata_fps->metadata->time_series_section_2.sampling_frequency;

	// fill page times
	dims[0] = 1; dims[1] = 1; n_dims = 2;
	
	// start time
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = slice.start_time;
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_FIELDS_START_TIME_UUTC_IDX_mat, tmp_mxa);
	// start time string
	STR_time_string_m12(slice.start_time, time_str, TRUE_m12, FALSE_m12, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_FIELDS_START_TIME_STRING_IDX_mat, tmp_mxa);
	// end time
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = slice.end_time;
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_FIELDS_END_TIME_UUTC_IDX_mat, tmp_mxa);
	// end time string
	STR_time_string_m12(slice.end_time, time_str, TRUE_m12, FALSE_m12, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
	
	// set globals
	session_ptr = sess;
	matrix_ptr = dm;

       	return(mat_raw_page);
}


void	build_contigua(DATA_MATRIX_m12 *dm, mxArray *mat_raw_page)
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
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_FIELDS_CONTIGUA_IDX_mat, mat_contigua);

	return;
}


void    build_session_records(SESSION_m12 *sess, DATA_MATRIX_m12 *dm, mxArray *mat_raw_page)
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
	mxSetFieldByNumber(mat_raw_page, 0, MATRIX_FIELDS_RECORDS_IDX_mat, mat_records);
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
	si8			offset_samps, start_idx;
	sf8                     ver, offset_secs;
	mwSize			dims[2];
	mxArray                 *mat_record, *tmp_mxa;
	const si4               n_mat_NlxP_v10_record_fields = NUMBER_OF_NLXP_v10_RECORD_FIELDS_mat;
	const si1               *mat_NlxP_v10_record_field_names[] = NLXP_v10_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Note_v10_record_fields = NUMBER_OF_NOTE_v10_RECORD_FIELDS_mat;
	const si1               *mat_Note_v10_record_field_names[] = NOTE_v10_RECORD_FIELD_NAMES_mat;
	const si4               n_mat_Note_v11_record_fields = NUMBER_OF_NOTE_v11_RECORD_FIELDS_mat;
	const si1               *mat_Note_v11_record_field_names[] = NOTE_v11_RECORD_FIELD_NAMES_mat;
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
	contigua = dm->contigua;
	for (i = 0; i < dm->number_of_contigua; ++i)
		if (rh->start_time <= contigua[i].end_time)
			break;
	offset_secs = (sf8) (rh->start_time - contigua[i].start_time) / (sf8) 1e6;
	offset_samps = (si8) round(offset_secs * dm->sampling_frequency);
	start_idx = contigua[i].start_sample_number + offset_samps;
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = start_idx;
	mxSetFieldByNumber(mat_record, 0, RECORD_FIELDS_START_INDEX_IDX_mat, tmp_mxa);
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
					// sampling frequency
					if (Sgmt_v10->sampling_frequency == REC_Sgmt_v10_SAMPLING_FREQUENCY_VARIABLE_m12) {
						tmp_mxa = mxCreateString("variable");
					} else {
						tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
						*((sf8 *) mxGetPr(tmp_mxa)) = Sgmt_v10->sampling_frequency;
					}
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_SAMPLING_FREQUENCY_IDX_mat, tmp_mxa);
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
					// description
					text = Sgmt_v11->description;
					if (*text)
						tmp_mxa = mxCreateString(text);
					else
						tmp_mxa = mxCreateString("<no description>");
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
					// end time
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
					// text
					mxSetFieldByNumber(mat_record, 0, NOTE_v11_RECORD_FIELDS_TEXT_IDX_mat, tmp_mxa);
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
					// sampling frequency
					mxSetFieldByNumber(mat_record, 0, SGMT_v10_RECORD_FIELDS_SAMPLING_FREQUENCY_IDX_mat, tmp_mxa);
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
                case mxINT8_CLASS:
                case mxUINT8_CLASS:
                case mxINT16_CLASS:
                case mxUINT16_CLASS:
                case mxINT32_CLASS:
                case mxUINT32_CLASS:
                case mxINT64_CLASS:
                case mxUINT64_CLASS:
			return((si8) mxGetScalar(mx_arr));
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

