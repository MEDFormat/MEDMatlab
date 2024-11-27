
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2023


//******************************************** Mex Compile Line *****************************************//
//****  mex COMPFLAGS='$COMPFLAGS -Wall -O3' read_MED_exec.c medlib_m12.c medrec_m12.c dhnlib_m12.c  ****//
//*******************************************************************************************************//


// session = get_session_stats(session_name, [password], [session_records])
// returns Matlab session structure


#include "MED_session_stats_exec.h"


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
        TERN_m12                return_channels, return_contigua, return_records;
        si1                     password[PASSWORD_BYTES_m12 + 1], **file_list_p, temp_str[16];
        si4                     i, n_files, len, max_len;
	void			*file_list, *temp_ptr;
        mxArray                 *sess, *mx_cell_p;

	
	PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(MAX_CHANNELS, 1), FALSE_m12);
	PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);

	// check for proper number of arguments
	if (nlhs != 1)
		mexErrMsgTxt("One output required: MED session structure\n");
	plhs[0] = mxCreateLogicalScalar((mxLogical) 0);  // set "false" return value for any subsequent errors
	if (nrhs < 1 || nrhs > 5)
		mexErrMsgTxt("One to five inputs required: session or channel name, [return_channels], [return_records], [return_contigua], [password]\n");

	// get the input file name(s) (argument 1)
	n_files = max_len = 0;
	if (mxIsEmpty(prhs[0]) == 1)
		mexErrMsgTxt("No input files specified\n");
	if (mxGetClassID(prhs[0]) == mxCHAR_CLASS) {
		max_len = mxGetNumberOfElements(prhs[0]) + 1; // Get the length of the input string
		if (max_len > FULL_FILE_NAME_BYTES_m12)
			mexErrMsgTxt("'file_list' (input 1) is too long\n");
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

	// password
	*password = 0;
	if (nrhs > 1) {
		if (mxIsEmpty(prhs[1]) == 0) {
			if (mxGetClassID(prhs[1]) == mxCHAR_CLASS) {
				len = mxGetNumberOfElements(prhs[1]); // Get the length of the input string
				if (len > (PASSWORD_BYTES_m12))  // allow full 16 bytes for password
					mexErrMsgTxt("'password' (input 2) is too long\n");
				else
					mxGetString(prhs[1], password, len + 1);
			} else {
				mexErrMsgTxt("'password' (input 2) must be a string\n");
			}
		}
	}
	
	// return channels
	return_channels = FALSE_m12;
	if (nrhs > 2) {
		if (mxIsEmpty(prhs[2]) == 0) {
			return_channels = UNKNOWN_m12;
			if (mxGetClassID(prhs[2]) == mxCHAR_CLASS) {
				mxGetString(prhs[2], temp_str, 16);
				if (*temp_str == 't' || *temp_str == 'T' || *temp_str == 'y' || *temp_str == 'Y' || *temp_str == '1')
					return_channels = TRUE_m12;
				else if (*temp_str == 'f' || *temp_str == 'F' || *temp_str == 'n' || *temp_str == 'N' || *temp_str == '0')
					return_channels = FALSE_m12;
			} else if (mxIsLogicalScalar(prhs[2])) {
				if (mxIsLogicalScalarTrue(prhs[2]) == 1)
					return_channels = TRUE_m12;
				else
					return_channels = FALSE_m12;
			} else if (mxIsScalar(prhs[2])) {
				if (mxGetScalar(prhs[2]) == 1)
					return_channels = TRUE_m12;
				else
					return_channels = FALSE_m12;
			}
			if (return_channels == UNKNOWN_m12)
				mexErrMsgTxt("'return_channels' (input 3) can be either true or false (default) only\n");
		}
	}
	
	// return contigua
	return_contigua = FALSE_m12;
	if (nrhs > 3) {
		if (mxIsEmpty(prhs[3]) == 0) {
			return_contigua = UNKNOWN_m12;
			if (mxGetClassID(prhs[3]) == mxCHAR_CLASS) {
				mxGetString(prhs[3], temp_str, 16);
				if (*temp_str == 't' || *temp_str == 'T' || *temp_str == 'y' || *temp_str == 'Y' || *temp_str == '1')
					return_contigua = TRUE_m12;
				else if (*temp_str == 'f' || *temp_str == 'F' || *temp_str == 'n' || *temp_str == 'N' || *temp_str == '0')
					return_contigua = FALSE_m12;
			} else if (mxIsLogicalScalar(prhs[3])) {
				if (mxIsLogicalScalarTrue(prhs[3]) == 1)
					return_contigua = TRUE_m12;
				else
					return_contigua = FALSE_m12;
			} else if (mxIsScalar(prhs[3])) {
				if (mxGetScalar(prhs[3]) == 1)
					return_contigua = TRUE_m12;
				else if (mxGetScalar(prhs[3]) == 0)
					return_contigua = FALSE_m12;
			}
			if (return_contigua == UNKNOWN_m12)
				mexErrMsgTxt("'return_contigua' (input 4) can be either true or false (default) only\n");
		}
	}

	// return records
	return_records = FALSE_m12;
	if (nrhs > 4) {
		if (mxIsEmpty(prhs[4]) == 0) {
			return_records = UNKNOWN_m12;
			if (mxGetClassID(prhs[4]) == mxCHAR_CLASS) {
				mxGetString(prhs[4], temp_str, 16);
				if (*temp_str == 't' || *temp_str == 'T' || *temp_str == 'y' || *temp_str == 'Y' || *temp_str == '1')
					return_records = TRUE_m12;
				else if (*temp_str == 'f' || *temp_str == 'F' || *temp_str == 'n' || *temp_str == 'N' || *temp_str == '0')
					return_records = FALSE_m12;
			} else if (mxIsLogicalScalar(prhs[4])) {
				if (mxIsLogicalScalarTrue(prhs[4]) == 1)
					return_records = TRUE_m12;
				else
					return_records = FALSE_m12;
			} else if (mxIsScalar(prhs[4])) {
				if (mxGetScalar(prhs[4]) == 1)
					return_records = TRUE_m12;
				else if (mxGetScalar(prhs[4]) == 0)
					return_records = FALSE_m12;
			}
			if (return_records == UNKNOWN_m12)
				mexErrMsgTxt("'return_records' (input 5) can be either true or false (default) only\n");
		}
	}
		
	// initialize MED library
	G_initialize_medlib_m12(FALSE_m12, FALSE_m12);
                
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
	if (return_channels == FALSE_m12 && n_files) {  // just read first channel to get session data
		temp_ptr = calloc_m12((size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
		strcpy((si1 *) temp_ptr, ((si1 **) file_list)[0]);
		free_m12(file_list, __FUNCTION__);
		file_list = temp_ptr;
		n_files = 0;
	}
		
       // get out of here
	sess = MED_session_stats(file_list, n_files, return_channels, return_contigua, return_records, password);
	if (sess != NULL) {
		mxDestroyArray(plhs[0]);
		plhs[0] = sess;
	}

        // clean up
	free_m12(file_list, __FUNCTION__);
	G_free_globals_m12(TRUE_m12);

        return;
}


mxArray     *MED_session_stats(void *file_list, si4 n_files, TERN_m12 return_channels, TERN_m12 return_contigua, TERN_m12 return_records, si1 *password)
{
	si4			n_channels;
	ui8			flags;
        SESSION_m12		*sess;
        TIME_SLICE_m12		slice;
        mxArray			*mat_session, *mat_channels;
        const si4		n_mat_session_fields = NUMBER_OF_SESSION_FIELDS_mat;
        const si1		*mat_session_field_names[] = SESSION_FIELD_NAMES_mat;
	const si4		n_mat_channel_fields = NUMBER_OF_CHANNEL_FIELDS_mat;
	const si1		*mat_channel_field_names[] = CHANNEL_FIELD_NAMES_mat;
        	
	
        // open session
        G_initialize_time_slice_m12(&slice);
	slice.start_time = BEGINNING_OF_TIME_m12;
	slice.end_time = END_OF_TIME_m12;
	flags = (LH_READ_SLICE_SEGMENT_DATA_m12 | LH_MAP_ALL_SEGMENTS_m12 | LH_THREAD_SEGMENT_READS_m12);  // LH_THREAD_SEGMENT_READS_m12 doesn't change speed noticably in most cases
	if (return_records == TRUE_m12)
		flags |= (LH_READ_FULL_SESSION_RECORDS_m12 | LH_READ_FULL_SEGMENTED_SESS_RECS_m12);
	sess = G_open_session_m12(NULL, &slice, file_list, n_files, flags, password);  // threaded version
	if (sess == NULL) {
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("\nMED_session_stats():\nCannot read session => session not found\n");
		} else {
			G_warning_message_m12("\nMED_session_stats():\nCannot read session => Check that the password is correct, and that metadata files exist.\n");
			G_show_password_hints_m12(NULL);
		}
		putchar_m12('\n');
		return(NULL);
	}
	
	// get variable frequency (not done on open)
	G_frequencies_vary_m12(sess);

       	/* ****************************************** */
        /* ********  Create Matlab structure  ******* */
        /* ****************************************** */

        // Create session output structure
        mat_session = mxCreateStructMatrix(1, 1, n_mat_session_fields, mat_session_field_names);

	// Create channel output structures
	if (return_channels == TRUE_m12) {
		n_channels = sess->number_of_time_series_channels;
		mat_channels = mxCreateStructMatrix(n_channels, 1, n_mat_channel_fields, mat_channel_field_names);
		mxSetFieldByNumber(mat_session, 0, SESSION_FIELDS_CHANNELS_IDX_mat, mat_channels);
		
		// build channel names (duplicated in metadata, but convenient for viewing
		build_channel_names(sess, mat_session);
	}

	// Build metadata
	build_metadata(sess, mat_session, return_channels);

	// Build contigua
	if (return_contigua == TRUE_m12)
		build_contigua(sess, mat_session, return_channels);
	
       	// Build session records
	if (return_records == TRUE_m12)
        	build_session_records(sess, mat_session);
	
	// clean up
	G_free_session_m12(sess, TRUE_m12);

        return(mat_session);
}


void	build_channel_names(SESSION_m12 *sess, mxArray *mat_sess)
{
	si8                             i, n_chans;
	CHANNEL_m12                     *chan;
	FILE_PROCESSING_STRUCT_m12	*metadata_fps;
	UNIVERSAL_HEADER_m12		*uh;
	mxArray                         *tmp_mxa, *mat_chans;
	
	
	// create name cell strings array
	n_chans = sess->number_of_time_series_channels;
	mat_chans = mxGetFieldByNumber(mat_sess, 0, SESSION_FIELDS_CHANNELS_IDX_mat);
	for (i = 0; i < n_chans; ++i) {
		chan = sess->time_series_channels[i];
		metadata_fps = chan->segments[0]->metadata_fps;
		uh = metadata_fps->universal_header;
		tmp_mxa = mxCreateString(uh->channel_name);
		mxSetFieldByNumber(mat_chans, i, CHANNEL_FIELDS_NAME_IDX_mat, tmp_mxa);
	}
	
	return;
}


// NOTE: this function assumes all discontinuities are session wide, which is not required by MED
void	build_contigua(SESSION_m12 *sess, mxArray *mat_session, TERN_m12 return_channels)
{
	TERN_m12			var_freq, relative_days;
	si1				time_str[TIME_STRING_BYTES_m12];
	si8                             i, j, n_chans, n_contigs, samp_num;
	CHANNEL_m12                     *chan;
	CONTIGUON_m12			*contigua;
	mxArray                         *mat_sess_contigua, *mat_chan_contigua, *mat_channels, *tmp_mxa;
	mwSize				n_dims, dims[2];
	const si4                       n_mat_contiguon_fields = NUMBER_OF_CONTIGUON_FIELDS_mat;
	const si1                       *mat_contiguon_field_names[] = CONTIGUON_FIELD_NAMES_mat;
	
	
	// build session contigua
	n_contigs = G_build_contigua_m12((LEVEL_HEADER_m12 *) sess);
	if (n_contigs <= 0)
		return;
	
	if (globals_m12->RTO_known == TRUE_m12)
		relative_days = FALSE_m12;
	else
		relative_days = TRUE_m12;
	
	mat_sess_contigua = mxCreateStructMatrix(n_contigs, 1, n_mat_contiguon_fields, mat_contiguon_field_names);
	dims[0] = dims[1] = 1; n_dims = 2;
	contigua = sess->contigua;
	
	for (i = 0; i < n_contigs; ++i) {
		// start index
		if (globals_m12->time_series_frequencies_vary == TRUE_m12)
			samp_num = -1;
		else if (contigua[i].start_sample_number > 0)
			samp_num = contigua[i].start_sample_number + 1;
		else
			samp_num = G_sample_number_for_uutc_m12((LEVEL_HEADER_m12 *) sess, contigua[i].start_time, FIND_CURRENT_m12) + 1;
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = samp_num;
		mxSetFieldByNumber(mat_sess_contigua, i, CONTIGUON_FIELDS_START_INDEX_IDX_mat, tmp_mxa);
		// end index
		if (globals_m12->time_series_frequencies_vary == TRUE_m12)
			samp_num = -1;
		else if (contigua[i].end_sample_number > 0)
			samp_num = contigua[i].end_sample_number + 1;
		else
			samp_num = G_sample_number_for_uutc_m12((LEVEL_HEADER_m12 *) sess, contigua[i].end_time, FIND_CURRENT_m12) + 1;
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = samp_num;
		mxSetFieldByNumber(mat_sess_contigua, i, CONTIGUON_FIELDS_END_INDEX_IDX_mat, tmp_mxa);
		// start time
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = contigua[i].start_time;
		mxSetFieldByNumber(mat_sess_contigua, i, CONTIGUON_FIELDS_START_TIME_IDX_mat, tmp_mxa);
		// start time string
		STR_time_string_m12(contigua[i].start_time, time_str, TRUE_m12, relative_days, FALSE_m12);
		tmp_mxa = mxCreateString(time_str);
		mxSetFieldByNumber(mat_sess_contigua, i, CONTIGUON_FIELDS_START_TIME_STRING_IDX_mat, tmp_mxa);
		// end time
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = contigua[i].end_time;
		mxSetFieldByNumber(mat_sess_contigua, i, CONTIGUON_FIELDS_END_TIME_IDX_mat, tmp_mxa);
		// end time string
		STR_time_string_m12(contigua[i].end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
		tmp_mxa = mxCreateString(time_str);
		mxSetFieldByNumber(mat_sess_contigua, i, CONTIGUON_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);
	}
	mxSetFieldByNumber(mat_session, 0, SESSION_FIELDS_CONTIGUA_IDX_mat, mat_sess_contigua);

	if (return_channels == FALSE_m12)
		return;
	
	// copy session contigua into channels
	n_chans = sess->number_of_time_series_channels;
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
		var_freq = TRUE_m12;
	else
		var_freq = FALSE_m12;
	mat_channels = mxGetFieldByNumber(mat_session, 0, SESSION_FIELDS_CHANNELS_IDX_mat);
	n_chans = sess->number_of_time_series_channels;
	for (i = 0; i < n_chans; ++i) {
		// copy session contigua
		mat_chan_contigua = mxDuplicateArray(mat_sess_contigua);
		if (var_freq == TRUE_m12) {
			chan = sess->time_series_channels[i];
			for (j = 0; j < n_contigs; ++j) {
				// start index
				samp_num = G_sample_number_for_uutc_m12((LEVEL_HEADER_m12 *) chan, contigua[j].start_time, FIND_CURRENT_m12) + 1;
				tmp_mxa = mxGetFieldByNumber(mat_chan_contigua, j, CONTIGUON_FIELDS_START_INDEX_IDX_mat);
				*((si8 *) mxGetPr(tmp_mxa)) = samp_num;
				// end index
				samp_num = G_sample_number_for_uutc_m12((LEVEL_HEADER_m12 *) chan, contigua[j].end_time, FIND_CURRENT_m12) + 1;
				tmp_mxa = mxGetFieldByNumber(mat_chan_contigua, j, CONTIGUON_FIELDS_END_INDEX_IDX_mat);
				*((si8 *) mxGetPr(tmp_mxa)) = samp_num;
			}
		}
		mxSetFieldByNumber(mat_channels, i, CHANNEL_FIELDS_CONTIGUA_IDX_mat, mat_chan_contigua);
	}
 
	return;
}


void    build_metadata(SESSION_m12 *sess, mxArray *mat_session, TERN_m12 return_channels)
{
	TERN_m12				relative_days;
	si1                                     time_str[TIME_STRING_BYTES_m12];
	si4                                     i, seg_idx, n_chans;
	mwSize					n_dims, dims[2];
	CHANNEL_m12				*chan;
	TIME_SLICE_m12				*slice;
	FILE_PROCESSING_STRUCT_m12		*metadata_fps;
	UNIVERSAL_HEADER_m12                    *uh;
	TIME_SERIES_METADATA_SECTION_2_m12      *tmd2;
	METADATA_SECTION_3_m12                  *md3;
	mxArray                                 *tmp_mxa, *mat_sess_metadata, *mat_chan_metadata, *mat_channels;
	const si4                               n_mat_metadata_fields = NUMBER_OF_METADATA_FIELDS_mat;
	const si1                               *mat_metadata_field_names[] = METADATA_FIELD_NAMES_mat;
	
	
	slice = &sess->time_slice;
	seg_idx = G_get_segment_index_m12(slice->start_segment_number);
	metadata_fps = globals_m12->reference_channel->segments[seg_idx]->metadata_fps;  // reference channel first segment (more efficient)
	uh = metadata_fps->universal_header;
	if (uh->type_code != TIME_SERIES_METADATA_FILE_TYPE_CODE_m12)
		printf_m12("%s(): reference channel is not a time series channel\n", __FUNCTION__);  // trigger to update code
	tmd2 = &metadata_fps->metadata->time_series_section_2;
	md3 = &metadata_fps->metadata->section_3;
	dims[0] = dims[1] = 1; n_dims = 2;
	if (globals_m12->RTO_known == TRUE_m12)
		relative_days = FALSE_m12;
	else
		relative_days = TRUE_m12;
	
	mat_sess_metadata = mxCreateStructMatrix(1, 1, n_mat_metadata_fields, mat_metadata_field_names);
	mxSetFieldByNumber(mat_session, 0, SESSION_FIELDS_METADATA_IDX_mat, mat_sess_metadata);

	// path
	tmp_mxa = mxCreateString(sess->path);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_PATH_IDX_mat, tmp_mxa);

	// session start time uutc
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = globals_m12->session_start_time;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_START_TIME_UUTC_IDX_mat, tmp_mxa);

	// session end time uutc
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = globals_m12->session_end_time;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_END_TIME_UUTC_IDX_mat, tmp_mxa);
	
	// session start time string
	STR_time_string_m12(globals_m12->session_start_time, time_str, TRUE_m12, relative_days, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_START_TIME_STRING_IDX_mat, tmp_mxa);
	
	// session end time string
	STR_time_string_m12(globals_m12->session_end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_END_TIME_STRING_IDX_mat, tmp_mxa);

	// absolute start sample number
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
		*((si8 *) mxGetPr(tmp_mxa)) = -1;
	else
		*((si8 *) mxGetPr(tmp_mxa)) = 1;  // one-based indexing
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_START_SAMPLE_NUMBER_IDX_mat, tmp_mxa);

	// absolute end sample number
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
		*((si8 *) mxGetPr(tmp_mxa)) = -1;
	else
		*((si8 *) mxGetPr(tmp_mxa)) = slice->end_sample_number + 1;  // convert to one-based indexing
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_END_SAMPLE_NUMBER_IDX_mat, tmp_mxa);

	// session name
	tmp_mxa = mxCreateString(globals_m12->fs_session_name);  // use file system name in case subset
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_NAME_IDX_mat, tmp_mxa);
	
	// channel name
	if (sess->number_of_time_series_channels == 1)
		tmp_mxa = mxCreateString(uh->channel_name);
	else
		tmp_mxa = mxCreateString("");
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_CHANNEL_NAME_IDX_mat, tmp_mxa);

	// anonymized subject ID
	tmp_mxa = mxCreateString(uh->anonymized_subject_ID);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_ANONYMIZED_SUBJECT_ID_IDX_mat, tmp_mxa);
	 
	// session UID
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxUINT64_CLASS, mxREAL);
	*((ui8 *) mxGetPr(tmp_mxa)) = uh->session_UID;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_UID_IDX_mat, tmp_mxa);

	// channel UID
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxUINT64_CLASS, mxREAL);
	if (sess->number_of_time_series_channels == 1)
		*((ui8 *) mxGetPr(tmp_mxa)) = uh->channel_UID;
	else
		*((ui8 *) mxGetPr(tmp_mxa)) = 0;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_CHANNEL_UID_IDX_mat, tmp_mxa);

	// session description
	tmp_mxa = mxCreateString(tmd2->session_description);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_DESCRIPTION_IDX_mat, tmp_mxa);

	// channel description
	if (sess->number_of_time_series_channels == 1)
		tmp_mxa = mxCreateString(tmd2->channel_description);
	else
		tmp_mxa = mxCreateString("");
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_CHANNEL_DESCRIPTION_IDX_mat, tmp_mxa);

	// equipment description
	tmp_mxa = mxCreateString(tmd2->equipment_description);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_EQUIPMENT_DESCRIPTION_IDX_mat, tmp_mxa);

	// acquisition channel number
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
	if (sess->number_of_time_series_channels == 1)
		*((si4 *) mxGetPr(tmp_mxa)) = tmd2->acquisition_channel_number;
	else
		*((si4 *) mxGetPr(tmp_mxa)) = -1;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat, tmp_mxa);

	// reference description
	tmp_mxa = mxCreateString(tmd2->reference_description);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_REFERENCE_DESCRIPTION_IDX_mat, tmp_mxa);

	// sampling frequency
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
		*((sf8 *) mxGetPr(tmp_mxa)) = (sf8) -1.0;
	else
		*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->sampling_frequency;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SAMPLING_FREQUENCY_IDX_mat, tmp_mxa);

	// low frequency filter setting
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
		*((sf8 *) mxGetPr(tmp_mxa)) = (sf8) -1.0;
	else
		*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->low_frequency_filter_setting;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_LOW_FREQUENCY_FILTER_SETTING_IDX_mat, tmp_mxa);

	// high frequency filter setting
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
		*((sf8 *) mxGetPr(tmp_mxa)) = (sf8) -1.0;
	else
		*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->high_frequency_filter_setting;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_HIGH_FREQUENCY_FILTER_SETTING_IDX_mat, tmp_mxa);

	// notch filter frequency setting
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->notch_filter_frequency_setting;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_NOTCH_FILTER_FREQUENCY_SETTING_IDX_mat, tmp_mxa);

	// AC line frequency
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->AC_line_frequency;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_AC_LINE_FREQUENCY_IDX_mat, tmp_mxa);

	// amplitude units conversion factor
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->amplitude_units_conversion_factor;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_AMPLITUDE_UNITS_CONVERSION_FACTOR_IDX_mat, tmp_mxa);

	// amplitude units description
	tmp_mxa = mxCreateString(tmd2->amplitude_units_description);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_AMPLITUDE_UNITS_DESCRIPTION_IDX_mat, tmp_mxa);

	// time base units conversion factor
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->time_base_units_conversion_factor;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_TIME_BASE_UNITS_CONVERSION_FACTOR_IDX_mat, tmp_mxa);

	// time base units description
	tmp_mxa = mxCreateString(tmd2->time_base_units_description);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_TIME_BASE_UNITS_DESCRIPTION_IDX_mat, tmp_mxa);

	if (metadata_fps->metadata->section_1.section_3_encryption_level <= 0) {

		// recording time offset
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = md3->recording_time_offset;
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_RECORDING_TIME_OFFSET_IDX_mat, tmp_mxa);

		// standard UTC offset
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
		*((si4 *) mxGetPr(tmp_mxa)) = md3->standard_UTC_offset;
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_STANDARD_UTC_OFFSET_IDX_mat, tmp_mxa);

		// standard timezone string
		tmp_mxa = mxCreateString(md3->standard_timezone_string);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_STANDARD_TIMEZONE_STRING_IDX_mat, tmp_mxa);

		// standard timezone acronym
		tmp_mxa = mxCreateString(md3->standard_timezone_acronym);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_STANDARD_TIMEZONE_ACRONYM_IDX_mat, tmp_mxa);

		// daylight timezone string
		tmp_mxa = mxCreateString(md3->daylight_timezone_string);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_DAYLIGHT_TIMEZONE_STRING_IDX_mat, tmp_mxa);

		// daylight timezone acronym
		tmp_mxa = mxCreateString(md3->daylight_timezone_acronym);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_DAYLIGHT_TIMEZONE_ACRONYM_IDX_mat, tmp_mxa);

		// subject name 1
		tmp_mxa = mxCreateString(md3->subject_name_1);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SUBJECT_NAME_1_IDX_mat, tmp_mxa);

		// subject name 2
		tmp_mxa = mxCreateString(md3->subject_name_2);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SUBJECT_NAME_2_IDX_mat, tmp_mxa);

		// subject name 3
		tmp_mxa = mxCreateString(md3->subject_name_3);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SUBJECT_NAME_3_IDX_mat, tmp_mxa);

		// subject ID
		tmp_mxa = mxCreateString(md3->subject_ID);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SUBJECT_ID_IDX_mat, tmp_mxa);

		// recording country
		tmp_mxa = mxCreateString(md3->recording_country);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_RECORDING_COUNTRY_IDX_mat, tmp_mxa);

		// recording territory
		tmp_mxa = mxCreateString(md3->recording_territory);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_RECORDING_TERRITORY_IDX_mat, tmp_mxa);

		// recording city
		tmp_mxa = mxCreateString(md3->recording_locality);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_RECORDING_LOCALITY_IDX_mat, tmp_mxa);

		// recording institution
		tmp_mxa = mxCreateString(md3->recording_institution);
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_RECORDING_INSTITUTION_IDX_mat, tmp_mxa);
       } else {
		// recording time offset
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = 0;
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_RECORDING_TIME_OFFSET_IDX_mat, tmp_mxa);

		// standard UTC offset
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
		*((si4 *) mxGetPr(tmp_mxa)) = 0;
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_STANDARD_UTC_OFFSET_IDX_mat, tmp_mxa);

		// standard timezone string
		tmp_mxa = mxCreateString("offset Coordinated Universal Time");
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_STANDARD_TIMEZONE_STRING_IDX_mat, tmp_mxa);

		// standard timezone acronym
		tmp_mxa = mxCreateString("oUTC");
		mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_STANDARD_TIMEZONE_ACRONYM_IDX_mat, tmp_mxa);

		for (i = METADATA_SECTION_3_NO_ACCESS_FIELDS_IDX_mat; i < NUMBER_OF_METADATA_FIELDS_mat; ++i) {
			tmp_mxa = mxCreateString("no access");
			mxSetFieldByNumber(mat_sess_metadata, 0, i, tmp_mxa);
		}
	}
	
	if (return_channels == FALSE_m12)
		return;
	
	// copy session metadata into channels & change channel specific fields
	n_chans = sess->number_of_time_series_channels;
	mat_channels = mxGetFieldByNumber(mat_session, 0, SESSION_FIELDS_CHANNELS_IDX_mat);
	n_chans = sess->number_of_time_series_channels;
	for (i = 0; i < n_chans; ++i) {
		chan = sess->time_series_channels[i];
		metadata_fps = chan->segments[seg_idx]->metadata_fps;
		tmd2 = &metadata_fps->metadata->time_series_section_2;
		slice = &chan->time_slice;
		uh = metadata_fps->universal_header;
		mat_chan_metadata = mxDuplicateArray(mat_sess_metadata);
		if (globals_m12->time_series_frequencies_vary == TRUE_m12) {
			// absolute start sample number
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_START_SAMPLE_NUMBER_IDX_mat);
			*((si8 *) mxGetPr(tmp_mxa)) = 1;  // one-based indexing
			// absolute end sample number
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_END_SAMPLE_NUMBER_IDX_mat);
			*((si8 *) mxGetPr(tmp_mxa)) = tmd2->absolute_start_sample_number + tmd2->number_of_samples;  // one-based indexing
			// sampling frequency
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_SAMPLING_FREQUENCY_IDX_mat);
			*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->sampling_frequency;
		}
		// channel_path
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_PATH_IDX_mat);
		mxDestroyArray(tmp_mxa);
		tmp_mxa = mxCreateString(chan->path);
		mxSetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_PATH_IDX_mat, tmp_mxa);
		// channel name
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_CHANNEL_NAME_IDX_mat);
		mxDestroyArray(tmp_mxa);
		tmp_mxa = mxCreateString(uh->channel_name);
		mxSetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_CHANNEL_NAME_IDX_mat, tmp_mxa);
		// channel UID
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_CHANNEL_UID_IDX_mat);
		*((si8 *) mxGetPr(tmp_mxa)) = uh->channel_UID;
		// channel description
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_CHANNEL_DESCRIPTION_IDX_mat);
		mxDestroyArray(tmp_mxa);
		tmp_mxa = mxCreateString(tmd2->channel_description);
		mxSetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_CHANNEL_DESCRIPTION_IDX_mat, tmp_mxa);
		// acquisition channel number
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat);
		*((si4 *) mxGetPr(tmp_mxa)) = tmd2->acquisition_channel_number;
		// low frequency filter setting
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_LOW_FREQUENCY_FILTER_SETTING_IDX_mat);
		*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->low_frequency_filter_setting;
		// high frequency filter setting
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_HIGH_FREQUENCY_FILTER_SETTING_IDX_mat);
		*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->high_frequency_filter_setting;
		// notch filter frequency setting
		tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_NOTCH_FILTER_FREQUENCY_SETTING_IDX_mat);
		*((sf8 *) mxGetPr(tmp_mxa)) = tmd2->notch_filter_frequency_setting;

		mxSetFieldByNumber(mat_channels, i, CHANNEL_FIELDS_METADATA_IDX_mat, mat_chan_metadata);
	}

	return;
}


void    build_session_records(SESSION_m12 *sess, mxArray *mat_session)
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
					// excluded tyoes
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
        mxSetFieldByNumber(mat_session, 0, SESSION_FIELDS_RECORDS_IDX_mat, mat_records);
        for (i = 0; i < n_recs; ++i) {
                mat_record = fill_record(rec_ptrs[i]);
                mxSetCell(mat_records, i, mat_record);
        }

        // clean up
	free((void *) rec_ptrs);

        return;
}


mxArray	*fill_record(RECORD_HEADER_m12 *rh)
{
	TERN_m12		relative_days;
	si1                     *text, *stage_str, ver_str[8], *enc_str, enc_level, time_str[TIME_STRING_BYTES_m12];
	ui8                     n_dims;
	mwSize			dims[2];
	sf8                     ver;
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
					// acquisition channel number
					if (Sgmt_v11->acquisition_channel_number == REC_Sgmt_v11_ACQUISITION_CHANNEL_NUMBER_ALL_CHANNELS_m12) {
						tmp_mxa = mxCreateString("all channels");
					} else {
						tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
						*((si4 *) mxGetPr(tmp_mxa)) = Sgmt_v11->acquisition_channel_number;
					}
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat, tmp_mxa);
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


si4     rec_compare(const void *a, const void *b)
{
	si8	time_d;
	
	
	time_d = (*((RECORD_HEADER_m12 **) a))->start_time - (*((RECORD_HEADER_m12 **) b))->start_time;
	
	// sort by time
	if (time_d > 0)
		return(1);
	if (time_d < 0)
		return(-1);
	
	// if same time, sort by location in memory
	if ((ui8) *((RECORD_HEADER_m12 **) a) > (ui8) *((RECORD_HEADER_m12 **) a))
		return(1);

	return(-1);
}
