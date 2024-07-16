
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2021


//******************************************** Mex Compile Line *****************************************//
//****  mex COMPFLAGS='$COMPFLAGS -Wall -O3' read_MED_exec.c medlib_m12.c medrec_m12.c dhnlib_m12.c  ****//
//*******************************************************************************************************//


// [session, record_times, discontigua] = read_MED(file_list, [password])
// file_list: string array, strings can contain regexp
// password: if empty/absent, proceeds as if unencrypted (may error out)
// session: Matlab session structure with metadata & no data
// record times: times as proportion of session duration
// discontigua: Matlab discontigua structur array


#include "load_session.h"


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
        void                    *file_list;
        si1                     password[PASSWORD_BYTES_m12], **file_list_p;
	si4                     i, len, max_len, n_files;
        mxArray                 *mx_cell_p, *outputs[3];

	
	PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(MAX_CHANNELS, 1), FALSE_m12);
	PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);

	//  check for proper number of arguments
	if (nlhs != 3)
		mexErrMsgTxt("Three outputs required: MED_session, record_times, discontigua\n");
	for (i = 0; i < 3; ++i)
		plhs[i] = mxCreateDoubleMatrix(0, 0, mxREAL);
	if (nrhs == 0 || nrhs > 2)
		mexErrMsgTxt("One to 2 inputs required: file_list, [password]\n");
	
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
			mexErrMsgTxt("File List (input 1) cell array contains no entries\n");
                for (i = max_len = 0; i < n_files; ++i) {
                        mx_cell_p = mxGetCell(prhs[0], i);
			if (mxGetClassID(mx_cell_p) != mxCHAR_CLASS)
				mexErrMsgTxt("Elements of file_list cell array must be char arrays\n");
                        len = mxGetNumberOfElements(mx_cell_p) + 1; // Get the length of the input string
                        if (len > FULL_FILE_NAME_BYTES_m12)
				mexErrMsgTxt("Input File Name (input 1) is too long\n");
                        if (len > max_len)
                                max_len = len;
                }
        } else {
		mexErrMsgTxt("File List (input 1) must be a string or cell array\nStrings may include regular expressions (regex)\n");
        }
	
        // password
        *password = 0;
        if (nrhs == 2) {
                if (mxIsEmpty(prhs[1]) == 0) {
                        if (mxGetClassID(prhs[1]) == mxCHAR_CLASS) {
                                len = mxGetNumberOfElements(prhs[1]); // Get the length of the input string
                                if (len > PASSWORD_BYTES_m12)
					mexErrMsgTxt("Password (input 6) is too long\n");
                                else
                                        mxGetString(prhs[1], password, len + 1);  // allow for terminal zero
                        } else {
				mexErrMsgTxt("Password (input 6) must be a string\n");
                        }
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
		default:
			file_list = (void *) calloc_2D_m12((size_t) n_files, (size_t) max_len, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
			file_list_p = (si1 **) file_list;
			for (i = 0; i < n_files; ++i) {
				mx_cell_p = mxGetCell(prhs[0], i);
				mxGetString(mx_cell_p, file_list_p[i], max_len);
			}
			break;
	}
		
        // get out of here
	for (i = 0; i < 3; ++i)
		outputs[i] = NULL;
	load_session(file_list, n_files, password, outputs);
	
	// set return values
	for (i = 0; i < 3; ++i) {
		if (outputs[i] != NULL) {  // session
			mxDestroyArray(plhs[i]);
			plhs[i] = outputs[i];
		}
	}

        // clean up
        free_m12((void *) file_list, __FUNCTION__);
	G_free_globals_m12(TRUE_m12);

        return;
}


si4     load_session(void *file_list, si4 n_files, si1 *password, mxArray *plhs[])
{
        si4                                     n_channels;
	ui8                                     flags;
        SESSION_m12                             *sess;
        TIME_SLICE_m12                          local_sess_slice, *sess_slice;
        mxArray                                 *mat_session, *mat_channels;
	const si4                               n_mat_session_fields = NUMBER_OF_SESSION_FIELDS_mat;
	const si1                               *mat_session_field_names[] = SESSION_FIELD_NAMES_mat;
	const si4                               n_mat_channel_fields = NUMBER_OF_CHANNEL_FIELDS_mat;
	const si1                               *mat_channel_field_names[] = CHANNEL_FIELD_NAMES_mat;

	
        // read session
	sess_slice = &local_sess_slice;
        G_initialize_time_slice_m12(sess_slice);
	flags = (LH_INCLUDE_TIME_SERIES_CHANNELS_m12 | LH_READ_SEGMENT_METADATA_m12 | LH_READ_SLICE_SESSION_RECORDS_m12 | LH_READ_SLICE_SEGMENTED_SESS_RECS_m12);
//	printf_m12("%s(%d): switch back to threaded\n", __FUNCTION__, __LINE__);
//	sess = G_open_session_nt_m12(NULL, sess_slice, file_list, n_files, flags, password);
	sess = G_open_session_m12(NULL, sess_slice, file_list, n_files, flags, password);
	if (sess == NULL) {
		G_push_behavior_m12(RETURN_ON_FAIL_m12);
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("%s():\nCannot read session => no matching, or damaged input files.\n", __FUNCTION__);
		} else {
			G_warning_message_m12("%s():\nCannot read session => Check that the password is correct, and that metadata files exist.\n", __FUNCTION__);
			G_show_password_hints_m12(NULL);
		}
		putchar_m12('\n');
		G_pop_behavior_m12();
		return(-1);
	}
	
	// get variable frequency (not done on open)
	G_frequencies_vary_m12(sess);

	// use slice from open_session_m12()
	sess_slice = &sess->time_slice;

        /* ****************************************** */
        /* ********  Create Matlab structure  ******* */
        /* ****************************************** */

	// Create session output structure
        mat_session = mxCreateStructMatrix(1, 1, n_mat_session_fields, mat_session_field_names);
	plhs[0] = mat_session;

	// Create channel output structures
        n_channels = sess->number_of_time_series_channels;
        mat_channels = mxCreateStructMatrix(n_channels, 1, n_mat_channel_fields, mat_channel_field_names);
        mxSetFieldByNumber(mat_session, 0, SESSION_FIELDS_CHANNELS_IDX_mat, mat_channels);

  	// Create session record times output array
	plhs[1] = get_sess_rec_times(sess);
	
	// Create discontigua output structure
	plhs[2] = build_discontigua(sess);
	
	// Build metadata
	build_metadata(sess, mat_session);

	// clean up
	G_free_session_m12(sess, TRUE_m12);

        return(0);
}


mxArray    *build_discontigua(SESSION_m12 *sess)
{
        mxArray                         *mat_discontigua, *tmp_mxa;
	mwSize				n_dims, dims[2];
        si8                             i, n_contigs, n_discontigs, sess_start_time, start_time, end_time;
        const si4                       n_mat_discontiguon_fields = NUMBER_OF_DISCONTIGUON_FIELDS_mat;
	sf8				sess_duration;
        const si1                       *mat_discontiguon_field_names[] = DISCONTIGUON_FIELD_NAMES_mat;
        
        
	n_contigs = G_build_contigua_m12((LEVEL_HEADER_m12 *) sess);
	n_discontigs = n_contigs - 1;
	if (n_discontigs <= 0)
		return(NULL);
	
	// build discontigua
	dims[0] = dims[1] = 1; n_dims = 2;
	mat_discontigua = mxCreateStructMatrix(n_discontigs, 1, n_mat_discontiguon_fields, mat_discontiguon_field_names);
	sess_start_time = globals_m12->session_start_time;
	sess_duration = (sf8) ((globals_m12->session_end_time - sess_start_time) + 1);
	for (i = 0; i < n_discontigs; ++i) {
		// start time
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		start_time = sess->contigua[i].end_time + 1;
		*((si8 *) mxGetPr(tmp_mxa)) = start_time;
		mxSetFieldByNumber(mat_discontigua, i, DISCONTIGUON_FIELDS_START_TIME_IDX_mat, tmp_mxa);
		// end time
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
		end_time = sess->contigua[i + 1].start_time - 1;
		*((si8 *) mxGetPr(tmp_mxa)) = end_time;
		mxSetFieldByNumber(mat_discontigua, i, DISCONTIGUON_FIELDS_END_TIME_IDX_mat, tmp_mxa);
		// start proportion
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
		*((sf8 *) mxGetPr(tmp_mxa)) = (sf8) (start_time - sess_start_time) / sess_duration;
		mxSetFieldByNumber(mat_discontigua, i, DISCONTIGUON_FIELDS_START_PROP_IDX_mat, tmp_mxa);
		// end proportion
		tmp_mxa = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
		*((sf8 *) mxGetPr(tmp_mxa)) = (sf8) (end_time - sess_start_time) / sess_duration;
		mxSetFieldByNumber(mat_discontigua, i, DISCONTIGUON_FIELDS_END_PROP_IDX_mat, tmp_mxa);
	}

	return(mat_discontigua);
}


void    build_metadata(SESSION_m12 *sess, mxArray *mat_session)
{
	TERN_m12				relative_days;
	si1                                     time_str[TIME_STRING_BYTES_m12];
	si4                                     i, end_seg_idx, n_chans;
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
	metadata_fps = globals_m12->reference_channel->segments[0]->metadata_fps;  // reference channel first segment (more efficient)
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

	// start time uutc
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = slice->start_time;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_START_TIME_UUTC_IDX_mat, tmp_mxa);

	// end time uutc
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	*((si8 *) mxGetPr(tmp_mxa)) = slice->end_time;
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_END_TIME_UUTC_IDX_mat, tmp_mxa);
	
	// start time string
	STR_time_string_m12(slice->start_time, time_str, TRUE_m12, relative_days, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_START_TIME_STRING_IDX_mat, tmp_mxa);
	
	// end time string
	STR_time_string_m12(slice->end_time, time_str, TRUE_m12, relative_days, FALSE_m12);
	tmp_mxa = mxCreateString(time_str);
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_END_TIME_STRING_IDX_mat, tmp_mxa);

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
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_ABSOLUTE_START_SAMPLE_NUMBER_IDX_mat, tmp_mxa);

	// absolute end sample number
	tmp_mxa = mxCreateNumericArray(n_dims, dims, mxINT64_CLASS, mxREAL);
	if (globals_m12->time_series_frequencies_vary == TRUE_m12)
		*((si8 *) mxGetPr(tmp_mxa)) = -1;
	else
		*((si8 *) mxGetPr(tmp_mxa)) = slice->end_sample_number + 1;  // convert to one-based indexing
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_ABSOLUTE_END_SAMPLE_NUMBER_IDX_mat, tmp_mxa);

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
	mxSetFieldByNumber(mat_sess_metadata, 0, METADATA_FIELDS_INDICES_REFERENCE_CHANNEL_NAME_mat, tmp_mxa);

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
	
	// copy session metadata into channels & change channel specific fields
	n_chans = sess->number_of_time_series_channels;
	mat_channels = mxGetFieldByNumber(mat_session, 0, SESSION_FIELDS_CHANNELS_IDX_mat);
	n_chans = sess->number_of_time_series_channels;
	end_seg_idx = slice->end_segment_number - 1;
	for (i = 0; i < n_chans; ++i) {
		chan = sess->time_series_channels[i];
		metadata_fps = chan->segments[end_seg_idx]->metadata_fps;
		tmd2 = &metadata_fps->metadata->time_series_section_2;
		uh = metadata_fps->universal_header;
		mat_chan_metadata = mxDuplicateArray(mat_sess_metadata);
		if (globals_m12->time_series_frequencies_vary == TRUE_m12) {
			// absolute start sample number
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_ABSOLUTE_START_SAMPLE_NUMBER_IDX_mat);
			*((si8 *) mxGetPr(tmp_mxa)) = 1;  // one-based indexing
			// absolute end sample number
			tmp_mxa = mxGetFieldByNumber(mat_chan_metadata, 0, METADATA_FIELDS_ABSOLUTE_END_SAMPLE_NUMBER_IDX_mat);
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


mxArray     *get_sess_rec_times(SESSION_m12 *sess)
{
	si4				n_segs, seg_idx;
	si8                     	i, j, k, n_inds, tot_recs, n_recs, *rec_times, *rec_end, *si8_p1, *si8_p2;
	si8				sess_start_time, sess_end_time;
	sf8				*rec_props, sess_dur;
	FILE_PROCESSING_STRUCT_m12	*ri_fps;
	RECORD_INDEX_m12		*ri;
	mxArray                 	*mat_rec_props;
	mwSize				n_dims, dims[2];


	// count records
	tot_recs = 0;
	n_segs = globals_m12->number_of_session_segments;
	if (sess->record_indices_fps != NULL) {
		ri_fps = sess->record_indices_fps;
		tot_recs += ri_fps->universal_header->number_of_entries;
	}
	if (sess->segmented_sess_recs != NULL) {
		seg_idx = G_get_segment_index_m12(sess->time_slice.start_segment_number);
		for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
			ri_fps = sess->segmented_sess_recs->record_indices_fps[j];
			if (ri_fps != NULL)
				tot_recs += ri_fps->universal_header->number_of_entries;
		}
	}
	rec_times = (si8 *) calloc((size_t) tot_recs, sizeof(si8));
	if (rec_times == NULL)
		return(NULL);
	
	// get record times
	n_recs = 0;
	if (sess->record_indices_fps != NULL) {
		ri_fps = sess->record_indices_fps;
		ri = ri_fps->record_indices;
		n_inds = ri_fps->universal_header->number_of_entries;
		for (i = 0; i < n_inds; ++i) {
			switch (ri[i].type_code) {
				case REC_SyLg_TYPE_CODE_m12:
				case REC_Term_TYPE_CODE_m12:
					break;
				default:
					rec_times[n_recs] = ri[i].start_time;
					++n_recs;
					break;
			}
		}
	}
	
	if (sess->segmented_sess_recs != NULL) {
		for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
			ri_fps = sess->segmented_sess_recs->record_indices_fps[j];
			if (ri_fps == NULL)
				continue;
			ri = ri_fps->record_indices;
			n_inds = ri_fps->universal_header->number_of_entries;
			for (k = 0; k < n_inds; ++k) {
				switch (ri[j].type_code) {
					case REC_SyLg_TYPE_CODE_m12:
					case REC_Term_TYPE_CODE_m12:
						break;
					default:
						rec_times[n_recs] = ri[k].start_time;
						++n_recs;
						break;
				}
			}
		}
	}
	if (n_recs == 0) {
		free((void *) rec_times);
		return(NULL);
	}

	// sort record times
	qsort((void *) rec_times, n_recs, sizeof(si8), CMP_compare_si8_m12);
	
	// remove duplicates
	si8_p1 = rec_times;
	si8_p2 = si8_p1 + 1;
	rec_end = rec_times + n_recs;
	while (si8_p2 != rec_end) {
		if (*si8_p1 != *si8_p2)
			*++si8_p1 = *si8_p2++;
		else
			si8_p2++;
	}
	n_recs = (si8_p1 - rec_times) + 1;

	// create matlab output array
	n_dims = 2; dims[0] = n_recs; dims[1] = 1;
	mat_rec_props = mxCreateNumericArray(n_dims, dims, mxDOUBLE_CLASS, mxREAL);
	rec_props = (sf8 *) mxGetPr(mat_rec_props);
	
	// convert times to proportions of session
	sess_start_time = globals_m12->session_start_time;
	sess_end_time = globals_m12->session_end_time;
	sess_dur = (sf8) (sess_end_time - sess_start_time);
	
	for (i = 0; i < n_recs; ++i)
		rec_props[i] = (sf8) (rec_times[i] - sess_start_time) / sess_dur;
	
	// clean up
	free((void *) rec_times);

	return(mat_rec_props);
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
