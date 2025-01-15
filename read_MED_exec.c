
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2021


//******************************************** Mex Compile Line *****************************************//
//****  mex COMPFLAGS='$COMPFLAGS -Wall -O3' read_MED_exec.c medlib_m12.c medrec_m12.c dhnlib_m12.c  ****//
//*******************************************************************************************************//


#include "read_MED_exec.h"

// Globals
static TERN_m12			loaded = FALSE_m12;
static SESSION_m12		*med_sess = NULL;


// Mex exit function
void	mexExitFunction(void)
{
	// free session
	if (med_sess != NULL) {
		G_free_session_m12(med_sess, TRUE_m12);
		med_sess = NULL;
	}
	
	// free globals (pid is preserved between mex calls)
	G_free_globals_m12(TRUE_m12);
	
	return;
}


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
        si1                     	temp_str[16], **MED_paths_p;
        si4                     	i, len, max_len;
        si8                     	tmp_si8;
	const mxArray			*rps;
	C_RPS				crps;
        mxArray                 	*tmp_mxa, *mx_cell_p, *mat_sess;

	
	// function loaded
	if (loaded == FALSE_m12) {
		// register exit function
		mexAtExit(mexExitFunction);
		
		// adjust process limits
		PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(MAX_CHANNELS, 1), FALSE_m12);
		PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);
		
		// initialze medlib
		G_initialize_medlib_m12(FALSE_m12, FALSE_m12);
		
		loaded = TRUE_m12;
	}
	
	//  check for proper number of arguments
	if (nlhs != 1) {
		if (nrhs != 1)
			mexErrMsgTxt("One input: read_MED parameter structure\nOne output: read_MED data structure\n");
		else
			mexErrMsgTxt("One output: read_MED data structure\n");
	}
	plhs[0] = mxCreateLogicalScalar((mxLogical) 0);  // set "false" return value for any subsequent errors
	if (nrhs != 1)
		mexErrMsgTxt("One input: read_MED parameter structure\n");
	rps = prhs[0];
	if (mxIsStruct(rps) == 0)
		mexErrMsgTxt("Input must be a read_MED parameter structure\n");

	// get persistence mode if passed
	crps.persist_mode = PERSIST_READ_CLOSE;  // default
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_PERSISTENCE_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {  // passed char array
			mxGetString(tmp_mxa, temp_str, 16);
			switch (*temp_str) {
				case 'c':  // "close"
				case 'C':
					crps.persist_mode = PERSIST_CLOSE;
					break;
				case 'o':  // "open"
				case 'O':
					crps.persist_mode = PERSIST_OPEN;
					break;
				case 'r':  // "read"
				case 'R':
					if (*(temp_str + 4) == 0)  // "read" only
						crps.persist_mode = PERSIST_READ;
					else if (*(temp_str + 5) == 'n' || *(temp_str + 5) == 'N')  // "read new"
						crps.persist_mode = PERSIST_READ_NEW;
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
			crps.persist_mode = (ui1) tmp_si8;
		}
	}

	if (crps.persist_mode & PERSIST_OPEN || crps.persist_mode == PERSIST_CLOSE) {
		if (med_sess != NULL) {  // free session
			G_free_session_m12(med_sess, TRUE_m12);
			med_sess = NULL;
			if (crps.persist_mode == PERSIST_CLOSE) {  // set return to "true" for session closed
				mxDestroyArray(plhs[0]);
				plhs[0] = mxCreateLogicalScalar((mxLogical) 1);
			}
		}
		if (crps.persist_mode == PERSIST_CLOSE)
			return;
	}
	
	// check data parameter
	crps.n_files = max_len = 0;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_DATA_IDX);
	if (mxIsEmpty(tmp_mxa) == 1)
		mexErrMsgTxt("'Data' element is not specified\n");
	if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
		max_len = mxGetNumberOfElements(tmp_mxa); // get the length of the input string
		if (max_len > FULL_FILE_NAME_BYTES_m12)
			mexErrMsgTxt("'Data' path is too long\n");
	} else if (mxGetClassID(tmp_mxa) == mxCELL_CLASS) {
		crps.n_files = mxGetNumberOfElements(tmp_mxa);
		if (crps.n_files == 0)
			mexErrMsgTxt("'Data' cell array contains no entries\n");
		for (i = max_len = 0; i < crps.n_files; ++i) {
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

	// get extents mode
	crps.extents_mode = EXTENTS_MODE_TIME;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_EXTENTS_MODE_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {  // passed char array
			mxGetString(tmp_mxa, temp_str, 16);
			switch (*temp_str) {
				case 't':  // "time"
				case 'T':
					crps.extents_mode = EXTENTS_MODE_TIME;
					break;
				case 'i':  // "indices"
				case 'I':
					crps.extents_mode = EXTENTS_MODE_INDICES;
					break;
				default:
					mexErrMsgTxt("Invalid 'ExtentsMode' type\n");
					break;
			}
		} else {  // passed number
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < EXTENTS_MODE_TIME || tmp_si8 > EXTENTS_MODE_INDICES)
				mexErrMsgTxt("Invalid 'ExtentsMode' type\n");
			crps.extents_mode = tmp_si8;
		}
	}

	// get the start limit
	crps.start_time = UUTC_NO_ENTRY_m12;
	crps.start_index = SAMPLE_NUMBER_NO_ENTRY_m12;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_START_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1; // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("'Start' can be specified as 'start', or an integer\n");
			if (strcmp(temp_str, "start") == 0) {
				if (crps.extents_mode == EXTENTS_MODE_TIME)
					crps.start_time = BEGINNING_OF_TIME_m12;
				else
					crps.start_index = BEGINNING_OF_SAMPLE_NUMBERS_m12;
			} else {
				mexErrMsgTxt("'Start' can be specified as 'start', or an integer\n");
			}
		} else {
			if (crps.extents_mode == EXTENTS_MODE_TIME)
				crps.start_time = get_si8_scalar(tmp_mxa);
			else
				crps.start_index = get_si8_scalar(tmp_mxa) - 1;  // convert to c indexing
		}
	}

	// get the end limit
	crps.end_time = UUTC_NO_ENTRY_m12;
	crps.end_index = SAMPLE_NUMBER_NO_ENTRY_m12;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_END_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1; // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("'End' can be specified as 'end', or an integer\n");
			if (strcmp(temp_str, "end") == 0) {
				if (crps.extents_mode == EXTENTS_MODE_TIME)
					crps.end_time = END_OF_TIME_m12;
				else
					crps.end_index = END_OF_SAMPLE_NUMBERS_m12;
			} else {
				mexErrMsgTxt("'End' can be specified as 'end', or an integer\n");
			}
		} else {
			if (crps.extents_mode == EXTENTS_MODE_TIME)
				crps.end_time = get_si8_scalar(tmp_mxa);
			else
				crps.end_index = get_si8_scalar(tmp_mxa) - 1;  // convert to c indexing;
		}
	}

	// get password
	*crps.password = 0;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_PASSWORD_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1; // get the length of the input string
			if (len > PASSWORD_BYTES_m12)
				mexErrMsgTxt("'Pass' is too long\n");
			else
				mxGetString(tmp_mxa, crps.password, len);
		} else {
			mexErrMsgTxt("'Passw' must be a char array\n");
		}
	}

	// get index_channel
	*crps.index_channel = 0;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_INDEX_CHANNEL_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1; // get the length of the input string
			if (len > FULL_FILE_NAME_BYTES_m12)
				mexErrMsgTxt("'IdxChan' is too long\n");
			else
				mxGetString(tmp_mxa, crps.index_channel, len);
		} else {
			mexErrMsgTxt("'IdxChan' must be a string\n");
		}
	}

	// get format
	crps.format = FORMAT_DOUBLE;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_FORMAT_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("Invalid 'Format' type\n");
			if (strcmp(temp_str, "double") == 0)
				crps.format = FORMAT_DOUBLE;
			else if (strcmp(temp_str, "single") == 0)
				crps.format = FORMAT_SINGLE;
			else if (strcmp(temp_str, "int32") == 0)
				crps.format = FORMAT_INT32;
			else if (strcmp(temp_str, "int16") == 0)
				crps.format = FORMAT_INT16;
			else
				mexErrMsgTxt("Invalid 'Format' type\n");
		} else {
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < FORMAT_DOUBLE || tmp_si8 > FORMAT_INT16)
				mexErrMsgTxt("Invalid 'Format' type\n");
			crps.format = tmp_si8;
		}
	}
	
	// get filter
	crps.filter = FILT_NONE;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_FILTER_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		if (mxGetClassID(tmp_mxa) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(tmp_mxa) + 1;  // get the length of the input string
			if (len <= 16)
				mxGetString(tmp_mxa, temp_str, len);
			else
				mexErrMsgTxt("Invalid 'Filt' type\n");
			if (strcmp(temp_str, "none") == 0)
				crps.filter = FILT_NONE;
			else if (strcmp(temp_str, "lowpass") == 0)
				crps.filter = FILT_LOWPASS;
			else if (strcmp(temp_str, "highpass") == 0)
				crps.filter = FILT_HIGHPASS;
			else if (strcmp(temp_str, "bandpass") == 0)
				crps.filter = FILT_BANDPASS;
			else if (strcmp(temp_str, "bandstop") == 0)
				crps.filter = FILT_BANDSTOP;
			else
				mexErrMsgTxt("Invalid 'Filt' type\n");
		} else {
			tmp_si8 = get_si8_scalar(tmp_mxa);
			if (tmp_si8 < FILT_NONE || tmp_si8 > FILT_BANDSTOP)
				mexErrMsgTxt("Invalid 'Filt' type\n");
			crps.filter = tmp_si8;
		}
	}

	// get low cutoff
	crps.low_cutoff = (sf8) -1.0;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_LOW_CUTOFF_IDX);
	if (mxIsEmpty(tmp_mxa) == 0)
		crps.low_cutoff = *((sf8 *) mxGetPr(tmp_mxa));

	// get high cutoff
	crps.high_cutoff = (sf8) -1.0;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_HIGH_CUTOFF_IDX);
	if (mxIsEmpty(tmp_mxa) == 0)
		crps.high_cutoff = *((sf8 *) mxGetPr(tmp_mxa));
	
	// check filter cutoffs
	if (crps.filter >= FILT_LOWPASS) {
		if (crps.filter == FILT_HIGHPASS || crps.filter == FILT_BANDPASS || crps.filter == FILT_BANDSTOP)
			if (crps.high_cutoff == (sf8) 0.0)
				mexErrMsgTxt("'LowCut' is required for the specified filter type\n");
		if (crps.filter == FILT_LOWPASS || crps.filter == FILT_BANDPASS || crps.filter == FILT_BANDSTOP)
			if (crps.high_cutoff == (sf8) 0.0)
				mexErrMsgTxt("'HighCut' is required for the specified filter type\n");
	}

	// get metadata
	crps.metadata = TRUE_m12;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_METADATA_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		crps.metadata = get_logical(tmp_mxa);
		if (crps.metadata == UNKNOWN_m12)
			mexErrMsgTxt("'Metadata' can be either true or false\n");
	}

	// records
	crps.records = TRUE_m12;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_RECORDS_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		crps.records = get_logical(tmp_mxa);
		if (crps.records == UNKNOWN_m12)
			mexErrMsgTxt("'Records' can be either true or false\n");
	}

	// contigua
	crps.contigua = TRUE_m12;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_CONTIGUA_IDX);
	if (mxIsEmpty(tmp_mxa) == 0) {
		crps.contigua = get_logical(tmp_mxa);
		if (crps.contigua == UNKNOWN_m12)
			mexErrMsgTxt("'Contigua' can be either true or false\n");
	}

	// create input file list
	crps.MED_paths = NULL;
	tmp_mxa = mxGetFieldByNumber(rps, 0, RPS_DATA_IDX);
	switch (crps.n_files) {
		case 0:  // single string passed
			crps.MED_paths = calloc_m12((size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			mxGetString(tmp_mxa, (si1 *) crps.MED_paths, max_len);
			break;
		case 1:   // single string passed in cell array
			crps.MED_paths = calloc_m12((size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			mx_cell_p = mxGetCell(tmp_mxa, 0);
			mxGetString(mx_cell_p, (si1 *) crps.MED_paths, max_len);
			crps.n_files = 0;  // (indicates single string)
			break;
		default:  // multiple strings in cell array
			crps.MED_paths = (void *) calloc_2D_m12((size_t) crps.n_files, (size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			MED_paths_p = (si1 **) crps.MED_paths;
			for (i = 0; i < crps.n_files; ++i) {
				mx_cell_p = mxGetCell(tmp_mxa, i);
				mxGetString(mx_cell_p, MED_paths_p[i], max_len);
			}
			break;
	}

        // read MED
	mat_sess = read_MED(&crps);
	if (mat_sess != NULL) {
		mxDestroyArray(plhs[0]);
		// set  status
		if (crps.persist_mode == PERSIST_READ_CLOSE)  // "close" handled on entry, returns logical true
			tmp_mxa = mxCreateString("closed");
		else
			tmp_mxa = mxCreateString("open");
		mxSetFieldByNumber(mat_sess, 0, SESSION_FIELDS_STATUS_IDX_mat, tmp_mxa);
		plhs[0] = mat_sess;
	}

        // clean up
	free_m12(crps.MED_paths, __FUNCTION__);
	
	if (crps.persist_mode & PERSIST_CLOSE) {
		if (med_sess != NULL) {
			G_free_session_m12(med_sess, TRUE_m12);  // resets session globals (no not need to free until function unloaded)
			med_sess = NULL;
		}
	}
	
        return;
}


mxArray     *read_MED(C_RPS *crps)
{
	si1					*action_str;
        si4                                     n_channels, n_active_channels;
	ui8                                     flags;
        si8                                     i, j;
	TIME_SLICE_m12				slice;
        SESSION_m12                             *sess;
        CHANNEL_m12                             *chan;
	JOB_INFO				*jobs;
	PROC_THREAD_INFO_m12			*proc_thread_infos;
        mxArray                                 *mat_sess, *mat_chans;
        const si4                               n_mat_sess_fields = NUMBER_OF_SESSION_FIELDS_mat;
        const si1                               *mat_sess_field_names[] = SESSION_FIELD_NAMES_mat;
        const si4                               n_mat_channel_fields = NUMBER_OF_CHANNEL_FIELDS_mat;
        const si1                               *mat_channel_field_names[] = CHANNEL_FIELD_NAMES_mat;


	// set limit pairs
	if (crps->start_time == UUTC_NO_ENTRY_m12 && crps->end_time == UUTC_NO_ENTRY_m12) {
		if (crps->start_index == SAMPLE_NUMBER_NO_ENTRY_m12 && crps->end_index == SAMPLE_NUMBER_NO_ENTRY_m12) {  // nothing passed, default to time
			crps->start_time = BEGINNING_OF_TIME_m12;
			crps->end_time = END_OF_TIME_m12;
		} else {
			if (crps->start_index == SAMPLE_NUMBER_NO_ENTRY_m12)
				crps->start_index = BEGINNING_OF_SAMPLE_NUMBERS_m12;
			if (crps->end_index == SAMPLE_NUMBER_NO_ENTRY_m12)
				crps->end_index = END_OF_SAMPLE_NUMBERS_m12;
		}
	} else {  // at least one time passed
		if (crps->start_time == UUTC_NO_ENTRY_m12)
			crps->start_time = BEGINNING_OF_TIME_m12;
		else if (crps->end_time == UUTC_NO_ENTRY_m12)
			crps->end_time = END_OF_TIME_m12;
		crps->start_index = crps->end_index = SAMPLE_NUMBER_NO_ENTRY_m12;  // time supersedes indices
	}

	// copy global
	sess = med_sess;
	
        // read session
        G_initialize_time_slice_m12(&slice);
	slice.start_time = crps->start_time;
	slice.end_time = crps->end_time;
	slice.start_sample_number = crps->start_index;
	slice.end_sample_number = crps->end_index;
	if (*crps->index_channel)
		strcpy(globals_m12->reference_channel_name, crps->index_channel);
	flags = (LH_READ_SLICE_SEGMENT_DATA_m12 | LH_READ_SLICE_SESSION_RECORDS_m12 | LH_READ_SLICE_SEGMENTED_SESS_RECS_m12);
	if (crps->persist_mode & PERSIST_CLOSE) {
		if (med_sess == NULL)
			flags |= LH_NO_CPS_CACHING_m12;  // not efficient for single reads
	} else {
		flags |= LH_MAP_ALL_SEGMENTS_m12;  // more efficient for sequential reads
	}
	    
	if (crps->persist_mode == PERSIST_OPEN) {
		sess = G_open_session_m12(NULL, &slice, crps->MED_paths, crps->n_files, flags, crps->password);
		if (sess != NULL) {
			med_sess = sess;  // save session
			return(mxCreateLogicalScalar((mxLogical) 1));
		}
		action_str = "open";
	} else {
		sess = G_read_session_m12(sess, &slice, crps->MED_paths, crps->n_files, flags, crps->password);
		action_str = "read";
	}
	if (sess == NULL) {
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("%s(): Cannot %s session => no matching input files\n", __FUNCTION__, action_str);
		} else {
			if (*globals_m12->password_data.level_1_password_hint || *globals_m12->password_data.level_2_password_hint)
				G_warning_message_m12("%s(): Cannot %s session => check that the password is correct\n", __FUNCTION__, action_str);
			else
				G_warning_message_m12("%s():Cannot %s session => check that the password is correct, and that crps.metadata files exist\n", __FUNCTION__, action_str);
		}
		if (med_sess != NULL) {  // free session if exists
			G_free_session_m12(med_sess, TRUE_m12);
			med_sess = NULL;
		}
		return(NULL);
	}
	
       	/* ****************************************** */
        /* ********  Create Matlab structure  ******* */
        /* ****************************************** */

        // Create session output structure
        mat_sess = mxCreateStructMatrix(1, 1, n_mat_sess_fields, mat_sess_field_names);

        // Create channel output structures
        n_channels = sess->number_of_time_series_channels;
	for (i = n_active_channels = 0; i < n_channels; ++i) {
		chan = sess->time_series_channels[i];
		if (chan->flags & LH_CHANNEL_ACTIVE_m12)
			++n_active_channels;
	}
        mat_chans = mxCreateStructMatrix(n_active_channels, 1, n_mat_channel_fields, mat_channel_field_names);
        mxSetFieldByNumber(mat_sess, 0, SESSION_FIELDS_CHANNELS_IDX_mat, mat_chans);

	// build channel names (duplicated in crps.metadata, but convenient for viewing
	build_channel_names(sess, mat_sess);
	
	// Build crps.metadata
	if (crps->metadata == TRUE_m12)
		build_metadata(sess, mat_sess);

	// Build contigua
	if (crps->contigua == TRUE_m12)
		build_contigua(sess, mat_sess);
	
	// Build session records
	if (crps->records == TRUE_m12)
        	build_session_records(sess, mat_sess);
	
	// set up distribution & filtering jobs
	jobs = (JOB_INFO *) malloc((size_t) n_active_channels * sizeof(JOB_INFO));
	proc_thread_infos = (PROC_THREAD_INFO_m12 *) calloc((size_t) n_active_channels, sizeof(PROC_THREAD_INFO_m12));
	for (i = j = 0; i < n_channels; ++i) {
		chan = sess->time_series_channels[i];
		if ((chan->flags & LH_CHANNEL_ACTIVE_m12) == 0)
			continue;
		jobs[j].channel = chan;
		jobs[j].crps = crps;
		jobs[j].samples = NULL;
		proc_thread_infos[j].thread_f = distribute_and_filter;
		proc_thread_infos[j].thread_label = "distribute_and_filter";
		proc_thread_infos[j].priority = PROC_HIGH_PRIORITY_m12;
		proc_thread_infos[j].arg = (void *) (jobs + j);
		++j;
	}

	// thread out distrution & filtering
	PROC_distribute_jobs_m12(proc_thread_infos, n_active_channels, 0, TRUE_m12);  // no reserved cores, wait for completion

	// assign data
	for (i = j = 0; i < n_channels; ++i) {
		chan = sess->time_series_channels[i];
		if ((chan->flags & LH_CHANNEL_ACTIVE_m12) == 0)
			continue;
		mxSetFieldByNumber(mat_chans, j, CHANNEL_FIELDS_DATA_IDX_mat, jobs[j].samples);
		++j;
	}
	
	// set global
	med_sess = sess;

        return(mat_sess);
}


// NOTE: this function assumes all discontinuities are session wide, which is not required by MED
void	build_contigua(SESSION_m12 *sess, mxArray *mat_sess)
{
	TERN_m12			relative_days;
	si1				time_str[TIME_STRING_BYTES_m12];
        si8                             i, j, k, n_chans, n_contigs, samp_num, slice_start_sample_number, val;
        CHANNEL_m12                     *chan;
	CONTIGUON_m12			*contigua;
	mxArray                         *mat_sess_contigua, *mat_chan_contigua, *mat_chans, *tmp_mxa;
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
	slice_start_sample_number = sess->time_slice.start_sample_number;
	dims[0] = dims[1] = 1; n_dims = 2;
	contigua = sess->contigua;
	for (i = 0; i < n_contigs; ++i) {
		// start index
		if (contigua[i].start_sample_number == SAMPLE_NUMBER_NO_ENTRY_m12)
			val = -1;
		else
			val = (contigua[i].start_sample_number - slice_start_sample_number) + 1;
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = val;
		mxSetFieldByNumber(mat_sess_contigua, i, CONTIGUON_FIELDS_START_INDEX_IDX_mat, tmp_mxa);
		// end index
		if (contigua[i].start_sample_number == SAMPLE_NUMBER_NO_ENTRY_m12)
			val = -1;
		else
			val = (contigua[i].end_sample_number - slice_start_sample_number) + 1;
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		*((si8 *) mxGetPr(tmp_mxa)) = val;
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
	mxSetFieldByNumber(mat_sess, 0, SESSION_FIELDS_CONTIGUA_IDX_mat, mat_sess_contigua);

	// copy session contigua into channels
	mat_chans = mxGetFieldByNumber(mat_sess, 0, SESSION_FIELDS_CHANNELS_IDX_mat);
	n_chans = sess->number_of_time_series_channels;
	for (i = j = 0; i < n_chans; ++i) {
		chan = sess->time_series_channels[i];
		if ((chan->flags & LH_CHANNEL_ACTIVE_m12) == 0)
			continue;
		
		// copy session contigua
		mat_chan_contigua = mxDuplicateArray(mat_sess_contigua);
		if (globals_m12->time_series_frequencies_vary == TRUE_m12) {
			slice_start_sample_number = chan->time_slice.start_sample_number;
			for (k = 0; k < n_contigs; ++k) {
				// start index
				tmp_mxa = mxGetFieldByNumber(mat_chan_contigua, k, CONTIGUON_FIELDS_START_INDEX_IDX_mat);
				samp_num = G_sample_number_for_uutc_m12((LEVEL_HEADER_m12 *) chan, contigua[k].start_time, FIND_CURRENT_m12);
				*((si8 *) mxGetPr(tmp_mxa)) = (samp_num - slice_start_sample_number) + 1;
				// end index
				tmp_mxa = mxGetFieldByNumber(mat_chan_contigua, k, CONTIGUON_FIELDS_END_INDEX_IDX_mat);
				samp_num = G_sample_number_for_uutc_m12((LEVEL_HEADER_m12 *) chan, contigua[k].end_time, FIND_CURRENT_m12);
				*((si8 *) mxGetPr(tmp_mxa)) = (samp_num - slice_start_sample_number) + 1;
			}
		}
		mxSetFieldByNumber(mat_chans, j, CHANNEL_FIELDS_CONTIGUA_IDX_mat, mat_chan_contigua);
		++j;
	}
 
        return;
}


void	build_channel_names(SESSION_m12 *sess, mxArray *mat_sess)
{
	si4				seg_idx;
	si8                             i, j, n_chans;
	TIME_SLICE_m12			*slice;
	CHANNEL_m12                     *chan;
	FILE_PROCESSING_STRUCT_m12	*metadata_fps;
	UNIVERSAL_HEADER_m12		*uh;
	mxArray                         *tmp_mxa, *mat_chans;
	
	
	// create name cell strings array
	n_chans = sess->number_of_time_series_channels;
	slice = &sess->time_slice;
	seg_idx = G_get_segment_index_m12(slice->start_segment_number);
	mat_chans = mxGetFieldByNumber(mat_sess, 0, SESSION_FIELDS_CHANNELS_IDX_mat);
	for (i = j = 0; i < n_chans; ++i) {
		chan = sess->time_series_channels[i];
		if ((chan->flags & LH_CHANNEL_ACTIVE_m12) == 0)
			continue;
		metadata_fps = chan->segments[seg_idx]->metadata_fps;
		uh = metadata_fps->universal_header;
		tmp_mxa = mxCreateString(uh->channel_name);
		mxSetFieldByNumber(mat_chans, j, CHANNEL_FIELDS_NAME_IDX_mat, tmp_mxa);
		++j;
	}
	
	return;
}


void    build_metadata(SESSION_m12 *sess, mxArray *mat_sess)
{
	TERN_m12				relative_days;
        si1                                     time_str[TIME_STRING_BYTES_m12];
        si4                                     i, j, seg_idx, n_chans, n_sess_segs;
	mwSize					n_dims, dims[2];
	CHANNEL_m12				*chan;
	TIME_SLICE_m12				*slice;
	FILE_PROCESSING_STRUCT_m12		*metadata_fps;
        UNIVERSAL_HEADER_m12                    *uh;
        TIME_SERIES_METADATA_SECTION_2_m12      *tmd2;
        METADATA_SECTION_3_m12                  *md3;
        mxArray                                 *tmp_mxa, *mat_sess_metadata, *mat_chan_metadata, *mat_chans;
	const si4                               n_mat_metadata_fields = NUMBER_OF_METADATA_FIELDS_mat;
	const si1                               *mat_metadata_field_names[] = METADATA_FIELD_NAMES_mat;
	
	
	slice = &sess->time_slice;
	seg_idx = G_get_segment_index_m12(slice->start_segment_number);
	metadata_fps = globals_m12->reference_channel->segments[seg_idx]->metadata_fps;  // reference channel first segment (more efficient)
        uh = metadata_fps->universal_header;
	if (uh->type_code != TIME_SERIES_METADATA_FILE_TYPE_CODE_m12)
		G_warning_message_m12("%s(): reference channel is not a time series channel => update code\n", __FUNCTION__);  // trigger to update code
        tmd2 = &metadata_fps->metadata->time_series_section_2;
        md3 = &metadata_fps->metadata->section_3;
        dims[0] = dims[1] = 1; n_dims = 2;
	if (globals_m12->RTO_known == TRUE_m12)
		relative_days = FALSE_m12;
	else
		relative_days = TRUE_m12;
	
	mat_sess_metadata = mxCreateStructMatrix(1, 1, n_mat_metadata_fields, mat_metadata_field_names);
	mxSetFieldByNumber(mat_sess, 0, SESSION_FIELDS_METADATA_IDX_mat, mat_sess_metadata);

	// path
        tmp_mxa = mxCreateString(sess->path);
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_PATH_IDX_mat, tmp_mxa);

        // slice start time uutc
        tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
        *((si8 *) mxGetPr(tmp_mxa)) = slice->start_time;
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SLICE_START_TIME_UUTC_IDX_mat, tmp_mxa);

        // slice end time uutc
        tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
        *((si8 *) mxGetPr(tmp_mxa)) = slice->end_time;
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SLICE_END_TIME_UUTC_IDX_mat, tmp_mxa);
        
        // slice start time string
	STR_time_string_m12(slice->start_time, time_str, TRUE_m12, relative_days, FALSE_m12);
        tmp_mxa = mxCreateString(time_str);
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SLICE_START_TIME_STRING_IDX_mat, tmp_mxa);
        
        // slice end time string
	STR_time_string_m12(slice->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
        tmp_mxa = mxCreateString(time_str);
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SLICE_END_TIME_STRING_IDX_mat, tmp_mxa);

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

	// slice start sample number
        tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
                *((si8 *) mxGetPr(tmp_mxa)) = -1;
        else
                *((si8 *) mxGetPr(tmp_mxa)) = slice->start_sample_number + 1;  // convert to one-based indexing
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SLICE_START_SAMPLE_NUMBER_IDX_mat, tmp_mxa);

       	// slice end sample number
        tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
        if (globals_m12->time_series_frequencies_vary == TRUE_m12)
                *((si8 *) mxGetPr(tmp_mxa)) = -1;
	else
		*((si8 *) mxGetPr(tmp_mxa)) = slice->end_sample_number + 1;
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SLICE_END_SAMPLE_NUMBER_IDX_mat, tmp_mxa);

	// session number of samples
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	if (slice->start_sample_number == SAMPLE_NUMBER_NO_ENTRY_m12)
		*((si8 *) mxGetPr(tmp_mxa)) = -1;
	else
		*((si8 *) mxGetPr(tmp_mxa)) = globals_m12->number_of_session_samples;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_NUMBER_OF_SAMPLES_IDX_mat, tmp_mxa);

       	// session name
        tmp_mxa = mxCreateString(globals_m12->fs_session_name);  // use file system name in case subset
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_SESSION_NAME_IDX_mat, tmp_mxa);
        
        // channel name
	if (sess->number_of_time_series_channels == 1)
		tmp_mxa = mxCreateString(uh->channel_name);
	else
		tmp_mxa = mxCreateString("");
        mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_CHANNEL_NAME_IDX_mat, tmp_mxa);

	// indices reference channel name
	tmp_mxa = mxCreateString(globals_m12->reference_channel_name);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_INDEX_CHANNEL_NAME_mat, tmp_mxa);

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
                *((sf8 *) mxGetPr(tmp_mxa)) = -1.0;
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
	
	// copy session metadata into channels & change channel specific fields
	n_chans = sess->number_of_time_series_channels;
	mat_chans = mxGetFieldByNumber(mat_sess, 0, SESSION_FIELDS_CHANNELS_IDX_mat);
	n_chans = sess->number_of_time_series_channels;
	n_sess_segs = globals_m12->number_of_session_segments;
	for (i = j = 0; i < n_chans; ++i) {
		chan = sess->time_series_channels[i];
		if ((chan->flags & LH_CHANNEL_ACTIVE_m12) == 0)
			continue;
		metadata_fps = chan->segments[seg_idx]->metadata_fps;
		tmd2 = &metadata_fps->metadata->time_series_section_2;
		slice = &chan->time_slice;
		uh = metadata_fps->universal_header;
		mat_chan_metadata = mxDuplicateArray(mat_sess_metadata);
		if (globals_m12->time_series_frequencies_vary == TRUE_m12) {
			// slice start sample number
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_SLICE_START_SAMPLE_NUMBER_IDX_mat);
			*((si8 *) mxGetPr(tmp_mxa)) = slice->start_sample_number + 1;  // convert to one-based indexing
			// slice end sample number
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_SLICE_END_SAMPLE_NUMBER_IDX_mat);
			*((si8 *) mxGetPr(tmp_mxa)) = slice->end_sample_number + 1;  // convert to one-based indexing
			// session number of samples
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_SESSION_NUMBER_OF_SAMPLES_IDX_mat);
			if (chan->Sgmt_records == NULL)
				chan->Sgmt_records = G_build_Sgmt_records_array_m12(NULL, NULL, chan);
			*((si8 *) mxGetPr(tmp_mxa)) = chan->Sgmt_records[n_sess_segs - 1].end_sample_number + 1;  // convert to count
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

		mxSetFieldByNumber(mat_chans, j, CHANNEL_FIELDS_METADATA_IDX_mat, mat_chan_metadata);
		++j;
	}

        return;
}


void    build_session_records(SESSION_m12 *sess, mxArray *mat_sess)
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
        mxSetFieldByNumber(mat_sess, 0, SESSION_FIELDS_RECORDS_IDX_mat, mat_records);
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
			case REC_Seiz_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 0) {
					Seiz_v10 = (REC_Seiz_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
					// end time
					tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
					*((si8 *) mxGetPr(tmp_mxa)) = Seiz_v10->end_time;
					mxSetFieldByNumber(mat_record, 0, SEIZ_v10_RECORD_FIELDS_END_TIME_IDX_mat, tmp_mxa);
					// end time string
					if (Seiz_v10->end_time > 0)
						STR_time_string_m12(Seiz_v10->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
					else
						tmp_mxa = mxCreateString("<no entry>");
					tmp_mxa = mxCreateString(time_str);
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
					// acquisition channel number
					if (Sgmt_v11->acquisition_channel_number == REC_Sgmt_v11_ACQUISITION_CHANNEL_NUMBER_ALL_CHANNELS_m12) {
						tmp_mxa = mxCreateString("all channels");
					} else {
						tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT32_CLASS, mxREAL);
						*((si4 *) mxGetPr(tmp_mxa)) = Sgmt_v11->acquisition_channel_number;
					}
					mxSetFieldByNumber(mat_record, 0, SGMT_v11_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat, tmp_mxa);
					// description
					if (rh->total_record_bytes > (RECORD_HEADER_BYTES_m12 + REC_Sgmt_v11_BYTES_m12)) {
						text = (si1 *) rh + RECORD_HEADER_BYTES_m12 + REC_Sgmt_v11_BYTES_m12;
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
			case REC_Seiz_TYPE_CODE_m12:
				if (rh->version_major == 1 && rh->version_minor == 1) {
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
		case mxLOGICAL_CLASS:
			if (mxIsLogicalScalarTrue(mx_arr) == 1)
				return((si8) 1);
			else
				return((si8) 0);
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


pthread_rval_m12	distribute_and_filter(void *ptr)
{
	si2				*mat_si2_samps;
	si4				*mat_si4_samps, seg_idx, n_segs, *seg_samps, filt_type, val, pos_inf, neg_inf;
	sf4				*mat_sf4_samps;
	si8				i, j, k, data_len;
	sf8				*mat_sf8_samps, samp_freq, cut_1, cut_2;
	PROC_THREAD_INFO_m12		*pi;
	JOB_INFO 			*job;
	TIME_SLICE_m12			*slice;
	CHANNEL_m12			*chan;
	SEGMENT_m12			*seg;
	CMP_PROCESSING_STRUCT_m12	*cps;
	FILT_PROCESSING_STRUCT_m12	*filtps;
	C_RPS				*crps;
	mwSize				n_dims, dims[2], pad_samps, el_size, element_multiplier;
	mxArray				*samps;
	mxClassID			mat_class;

	
	pi = (PROC_THREAD_INFO_m12 *) ptr;
	pi->status = PROC_THREAD_RUNNING_m12;  // volatile
	
	job = (JOB_INFO *) (pi->arg);
	chan = job->channel;
	slice = &chan->time_slice;
	data_len = TIME_SLICE_SAMPLE_COUNT_m12(slice);
	n_segs = TIME_SLICE_SEGMENT_COUNT_m12(slice);
	seg_idx = G_get_segment_index_m12(slice->start_segment_number);
	crps = job->crps;

	// set up for format / filtering
	if (crps->filter > FILT_NONE) {
		switch (crps->filter) {
			case FILT_LOWPASS:
				filt_type = FILT_LOWPASS_TYPE_m12;
				cut_1 = crps->high_cutoff;
				cut_2 = (sf8) -1.0;  // assauge compiler
				break;
			case FILT_HIGHPASS:
				filt_type = FILT_HIGHPASS_TYPE_m12;
				cut_1 = crps->low_cutoff;
				cut_2 = (sf8) -1.0;  // assauge compiler
				break;
			case FILT_BANDPASS:
				filt_type = FILT_BANDPASS_TYPE_m12;
				cut_1 = crps->low_cutoff;
				cut_2 = crps->high_cutoff;
			case FILT_BANDSTOP:
				filt_type = FILT_BANDSTOP_TYPE_m12;
				cut_1 = crps->low_cutoff;
				cut_2 = crps->high_cutoff;
				break;
		}
		samp_freq = chan->segments[seg_idx]->metadata_fps->metadata->time_series_section_2.sampling_frequency;
		filtps = FILT_initialize_processing_struct_m12(FILTER_ORDER, filt_type, samp_freq, data_len, FALSE_m12, FALSE_m12, TRUE_m12, RETURN_ON_FAIL_m12 | SUPPRESS_OUTPUT_m12, cut_1, cut_2);
		pad_samps = (mwSize) FILT_FILT_PAD_SAMPLES_m12(filtps->n_poles);
	} else {
		pad_samps = 0;
	}

	switch (crps->format) {
		case FORMAT_DOUBLE:
			el_size = (mwSize) 8;
			mat_class = mxDOUBLE_CLASS;
			break;
		case FORMAT_SINGLE:
			el_size = (mwSize) 4;
			mat_class = mxSINGLE_CLASS;
			break;
		case FORMAT_INT32:
			el_size = (mwSize) 4;
			mat_class = mxINT32_CLASS;
			break;
		case FORMAT_INT16:
			el_size = (mwSize) 2;
			mat_class = mxINT16_CLASS;
			break;
	}
	if (crps->filter > FILT_NONE)
		element_multiplier = (mwSize) 8 / el_size;  // need sf8s to filter
	else
		element_multiplier = (mwSize) 1;

	// allocate Matlab array
	n_dims = 2; dims[1] = 1;
	dims[0] = ((mwSize) data_len * element_multiplier) + pad_samps;
	samps = mxCreateNumericArray(n_dims, dims, mat_class, mxREAL);

	// Fill in channel data
	mat_sf8_samps = (sf8 *) mxGetPr(samps);
	switch (crps->format) {
		case FORMAT_SINGLE:
			mat_sf4_samps = (sf4 *) mat_sf8_samps;
			break;
		case FORMAT_INT32:
			mat_si4_samps = (si4 *) mat_sf8_samps;
			break;
		case FORMAT_INT16:
			mat_si2_samps = (si2 *) mat_sf8_samps;
			break;
	}

	
	// no filtering
	if (crps->filter == FILT_NONE) {
		for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
			seg = chan->segments[j];
			cps = seg->time_series_data_fps->parameters.cps;
			seg_samps = cps->decompressed_data;
			k = TIME_SLICE_SAMPLE_COUNT_S_m12(seg->time_slice);
			switch (crps->format) {
				case FORMAT_DOUBLE:
					while (k--)
						*mat_sf8_samps++ = (sf8) *seg_samps++;
					break;
				case FORMAT_SINGLE:
					while (k--)
						*mat_sf4_samps++ = (sf4) *seg_samps++;
					break;
				case FORMAT_INT32:
					while (k--)
						*mat_si4_samps++ = (si4) *seg_samps++;
					break;
				case FORMAT_INT16:
					pos_inf = (si4) POS_INF_SI2_m12;
					neg_inf = (si4) NEG_INF_SI2_m12;
					while (k--) {
						val = *seg_samps++;  // curtail overflow
						if (val > pos_inf)
							val = POS_INF_SI2_m12;
						else if (val < neg_inf)
							val = NEG_INF_SI2_m12;
						*mat_si2_samps++ = (si2) val;
					}
					break;
			}
		}
	} else {  // filter
		
		// fill original data as sf8s
		filtps->filt_data = mat_sf8_samps;
		filtps->orig_data = FILT_OFFSET_ORIG_DATA_m12(filtps);  // offset to skip intial copy, filter in place
		mat_sf8_samps = filtps->orig_data;  // offset position
		for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
			seg = chan->segments[j];
			cps = seg->time_series_data_fps->parameters.cps;
			seg_samps = cps->decompressed_data;
			k = TIME_SLICE_SAMPLE_COUNT_S_m12(seg->time_slice);
			while (k--)
				*mat_sf8_samps++ = (sf8) *seg_samps++;
		}
		
		// filter
		FILT_filtfilt_m12(filtps);
		
		// convert to output size (& round)
		mat_sf8_samps = filtps->filt_data;  // base position
		switch (crps->format) {
			case FORMAT_DOUBLE:
				break;
			case FORMAT_SINGLE:
				CMP_sf8_to_sf4_m12(mat_sf8_samps, mat_sf4_samps, data_len, TRUE_m12);
				break;
			case FORMAT_INT32:
				CMP_sf8_to_si4_m12(mat_sf8_samps, mat_si4_samps, data_len, TRUE_m12);
				break;
			case FORMAT_INT16:
				CMP_sf8_to_si2_m12(mat_sf8_samps, mat_si2_samps, data_len, TRUE_m12);
				break;
		}

		// clean up
		FILT_free_processing_struct_m12(filtps, FALSE_m12, FALSE_m12, TRUE_m12, FALSE_m12);

		// resize
		mxSetM(samps, (mwSize) data_len);
		mxRealloc((void *) mat_sf8_samps, (mwSize) data_len * el_size);
	}
	
	job->samples = samps;
	
	pi->status = PROC_THREAD_FINISHED_m12;  // volatile

	return((pthread_rval_m12) 0);
}
