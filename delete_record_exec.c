
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2023


#include "delete_record_exec.h"


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
	si1	chan_dir[FULL_FILE_NAME_BYTES_m12], password[PASSWORD_BYTES_m12];
	ui4	rec_type;
	si8	rec_time, len;
	sf8	success;
	
	
	PROC_increase_process_priority_m12(FALSE_m12, FALSE_m12);
	
	//  check for proper number of output arguments
	if (nlhs != 1) {
		mexPrintf("One output required: success (0), or unspecified failure (-1), insufficient access (-2)\n");
		return;
	}
	
	// set unspecified fail return value for subsequent errors
	plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
	*((sf8 *) mxGetPr(plhs[0])) = -1.0;
	
	//  check for proper number of input arguments
	if (nrhs != 4) {
		mexPrintf("Four inputs required: chan_dir, password, rec_time, rec_type\n");
		return;
	}
	
	// session directory
	if (mxIsEmpty(prhs[0]) == 1) {
		mexPrintf("No channel directory (input 1) specified\n");
		return;
	}
	if (mxGetClassID(prhs[0]) != mxCHAR_CLASS) {
		mexPrintf("Channel directory (input 1) must be a string\n");
		return;
	}
	mxGetString(prhs[0], chan_dir, FULL_FILE_NAME_BYTES_m12);
	
	// password
	*password = 0;
	if (mxIsEmpty(prhs[1]) == 0) {
		if (mxGetClassID(prhs[1]) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(prhs[1]) + 1; // Get the length of the input string
			if (len > (PASSWORD_BYTES_m12 + 1)) {  // allow full 16 bytes for password
				mexPrintf("Password (input 2) is too long\n");
				return;
			} else {
				mxGetString(prhs[1], password, len);
			}
		} else {
			mexPrintf("Password (input 2) must be a string\n");
			return;
		}
	}
	
	// rec time
	if (mxIsEmpty(prhs[2]) == 1) {
		mexPrintf("No record time (input 3) specified\n");
		return;
	}
	rec_time = get_si8_scalar(prhs[2]);
	
	// rec time
	if (mxIsEmpty(prhs[3]) == 1) {
		mexPrintf("No record type (input 4) specified\n");
		return;
	}
	rec_type = (ui4) get_si8_scalar(prhs[3]);
	
	// initialize MED library
	G_initialize_medlib_m12(FALSE_m12, FALSE_m12);
	
	// return codes: 0.0 == ok, -1.0 == unspecified error, -2.0 == insufficient access
	success = delete_record(chan_dir, password, rec_time, rec_type);
	*((sf8 *) mxGetPr(plhs[0])) = success;
	
	// clean up
	G_free_globals_m12(TRUE_m12);
	
	return;
}


sf8	delete_record(si1 *chan_dir, si1 *password, si8 rec_time, ui4 rec_type)
{
	si1				ri_file[FULL_FILE_NAME_BYTES_m12], rd_file[FULL_FILE_NAME_BYTES_m12], **ssr_list;
	si1				tmp_ri_file[FULL_FILE_NAME_BYTES_m12], tmp_rd_file[FULL_FILE_NAME_BYTES_m12];
	si1				number_str[FILE_NUMBERING_DIGITS_m12 + 1], command[(FULL_FILE_NAME_BYTES_m12 * 2) + 32];
	si1				ssr_path[FULL_FILE_NAME_BYTES_m12], *sess_name, *ssr_name;
	si4				ssr_list_len, seg_idx, fe;
	ui8				flags;
	si8				i, n_recs;
	ui1				*rd;
	FILE_PROCESSING_STRUCT_m12	*orig_ri_fps, *orig_rd_fps, *new_ri_fps, *new_rd_fps, *proto_fps;
	RECORD_HEADER_m12		*rh;
	RECORD_INDEX_m12		*ri;
	SESSION_m12			*sess;
	UNIVERSAL_HEADER_m12		*uh;
	TIME_SLICE_m12			slice;
	
	
	// read session
	G_initialize_time_slice_m12(&slice);
	slice.start_time = slice.end_time = rec_time;
	flags = LH_READ_SEGMENT_METADATA_m12;
	sess = G_open_session_m12(NULL, &slice, chan_dir, 0, flags, password);  // limited open to get segment records, read segment metadata, & process password
	if (sess == NULL) {
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("%s(): cannot read session => no matching input files\n", __FUNCTION__);
		} else {
			if (*globals_m12->password_data.level_1_password_hint || *globals_m12->password_data.level_2_password_hint)
				G_warning_message_m12("%s(): cannot read session => check that the password is correct\n", __FUNCTION__);
			else
				G_warning_message_m12("%s(): cannot read session => check that the password is correct, and that metadata files exist\n", __FUNCTION__);
		}
		return(-1.0);
	}
	slice = sess->time_slice;
	seg_idx = G_get_segment_index_m12(slice.start_segment_number);
	G_numerical_fixed_width_string_m12(number_str, FILE_NUMBERING_DIGITS_m12, slice.start_segment_number);
		
	// get a segment prototype
	proto_fps = sess->time_series_channels[0]->segments[seg_idx]->metadata_fps;
	
	// read original records
	sess_name = globals_m12->fs_session_name;
	sprintf_m12(ssr_path, "%s/%s.%s", sess->path, sess_name, RECORD_DIRECTORY_TYPE_STRING_m12);
	fe = G_exists_m12(ssr_path);
	if (fe == DOES_NOT_EXIST_m12) {
		sess_name = globals_m12->uh_session_name;
		sprintf_m12(ssr_path, "%s/%s.%s", sess->path, sess_name, RECORD_DIRECTORY_TYPE_STRING_m12);
		fe = G_exists_m12(ssr_path);
	}
	if (fe == DIR_EXISTS_m12) {
		ssr_name = globals_m12->fs_session_name;
		sprintf_m12(ri_file, "%s/%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_INDICES_FILE_TYPE_STRING_m12);
		fe = G_exists_m12(ri_file);
		if (fe == DOES_NOT_EXIST_m12) {
			ssr_name = globals_m12->uh_session_name;
			sprintf_m12(ri_file, "%s/%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_INDICES_FILE_TYPE_STRING_m12);
			fe = G_exists_m12(ri_file);
		}
	} else {  // no ssr directory
		G_warning_message_m12("%s(): can only delete segmented session records at this time\n", __FUNCTION__);
		G_free_session_m12(sess, TRUE_m12);
		putchar_m12('\n');
		return(-1.0);
	}
	if (fe == DOES_NOT_EXIST_m12) {  // no records for this segment
		G_warning_message_m12("%s(): segmented records not found for this segment\n", __FUNCTION__);
		G_free_session_m12(sess, TRUE_m12);
		putchar_m12('\n');
		return(-1.0);
	} else {  // read in data
		orig_ri_fps = G_read_file_m12(NULL, ri_file, 0, 0, FPS_FULL_FILE_m12, NULL, NULL, USE_GLOBAL_BEHAVIOR_m12);
		sprintf_m12(rd_file, "%s/%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_DATA_FILE_TYPE_STRING_m12);
		orig_rd_fps = G_read_file_m12(NULL, rd_file, 0, 0, FPS_FULL_FILE_m12, NULL, NULL, USE_GLOBAL_BEHAVIOR_m12);
		n_recs = orig_rd_fps->number_of_items;
		ri = orig_ri_fps->record_indices;
		rd = orig_rd_fps->record_data;
	}
	
	// create new segmented session record indices fps
	sprintf_m12(tmp_ri_file, "%s/tmp_%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_INDICES_FILE_TYPE_STRING_m12);
	new_ri_fps = FPS_allocate_processing_struct_m12(NULL, tmp_ri_file, RECORD_INDICES_FILE_TYPE_CODE_m12, RECORD_INDEX_BYTES_m12, NULL, proto_fps, 0);
	if (new_ri_fps == NULL)
		return(-1.0);
	new_ri_fps->directives.open_mode = FPS_W_OPEN_MODE_m12;
	new_ri_fps->directives.close_file = FALSE_m12;
	uh = new_ri_fps->universal_header;
	uh->channel_UID = UID_NO_ENTRY_m12;
	memset((void *) uh->channel_name, 0, BASE_FILE_NAME_BYTES_m12);
	uh->file_UID = orig_ri_fps->universal_header->file_UID;  // keep original file UIDs since overwriting
	uh->provenance_UID = orig_ri_fps->universal_header->provenance_UID;  // keep original provenance UIDs since overwriting
	G_write_file_m12(new_ri_fps, 0, UNIVERSAL_HEADER_BYTES_m12, FPS_UNIVERSAL_HEADER_ONLY_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);
	
	// create new segmented session record data fps
	sprintf_m12(tmp_rd_file, "%s/tmp_%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_DATA_FILE_TYPE_STRING_m12);
	new_rd_fps = FPS_allocate_processing_struct_m12(NULL, tmp_rd_file, RECORD_INDICES_FILE_TYPE_CODE_m12, RECORD_INDEX_BYTES_m12, NULL, proto_fps, 0);
	if (new_ri_fps == NULL)
		return(-1.0);
	new_rd_fps->directives.open_mode = FPS_W_OPEN_MODE_m12;
	new_rd_fps->directives.close_file = FALSE_m12;
	uh = new_rd_fps->universal_header;
	uh->channel_UID = UID_NO_ENTRY_m12;
	memset((void *) uh->channel_name, 0, BASE_FILE_NAME_BYTES_m12);
	uh->file_UID = orig_rd_fps->universal_header->file_UID;  // keep original file UIDs since overwriting
	uh->provenance_UID = orig_rd_fps->universal_header->provenance_UID;  // keep original provenance UIDs since overwriting
	G_write_file_m12(new_rd_fps, 0, UNIVERSAL_HEADER_BYTES_m12, FPS_UNIVERSAL_HEADER_ONLY_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);
	
	// find selected preceding records
	for (i = 0; i < n_recs; ++i) {
		if (rec_time == ri[i].start_time)
			if (rec_type == ri[i].type_code)
				break;
		G_write_file_m12(new_ri_fps, FPS_APPEND_m12, INDEX_BYTES_m12, 1, ri + i, USE_GLOBAL_BEHAVIOR_m12);
		rh = (RECORD_HEADER_m12 *) rd;
		G_write_file_m12(new_rd_fps, FPS_APPEND_m12, rh->total_record_bytes, 1, rd, USE_GLOBAL_BEHAVIOR_m12);
		rd += rh->total_record_bytes;
	}
	if (i == n_recs) {  // record not found in segmented session records
		G_warning_message_m12("%s(): add_record_exec(): record not found\n", __FUNCTION__);
		FPS_free_processing_struct_m12(new_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(new_rd_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_rd_fps, TRUE_m12);
		G_free_session_m12(sess, TRUE_m12);
		return(-1.0);
	}
	rh = (RECORD_HEADER_m12 *) rd;  // insufficient access to delete
	if (globals_m12->password_data.access_level < rh->encryption_level) {
		G_warning_message_m12("%s(): insufficient access to delete this record\n", __FUNCTION__);
		G_show_password_hints_m12(NULL, rh->encryption_level);
		FPS_free_processing_struct_m12(new_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(new_rd_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_rd_fps, TRUE_m12);
		G_free_session_m12(sess, TRUE_m12);
		return(-2.0);
	}
	if (n_recs == 1) {  // last record in files - delete files
		// record index file
		FPS_close_m12(new_ri_fps);
#if defined MACOS_m12 || defined LINUX_m12
		sprintf_m12(command, "rm -f %s %s1> %s 2> %s", ri_file, tmp_ri_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
#ifdef WINDOWS_m12
		sprintf(command, "del /f %s 1> %s %s 2> %s", ri_file, tmp_ri_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
		system_m12(command, FALSE_m12, __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
		
		// record data file
		FPS_close_m12(new_rd_fps);
#if defined MACOS_m12 || defined LINUX_m12
		sprintf_m12(command, "mv -f %s %s 1> %s 2> %s", rd_file, tmp_rd_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
#ifdef WINDOWS_m12
		sprintf(command, "del /f %s %s 1> %s 2> %s", rd_file, tmp_rd_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
		system_m12(command, FALSE_m12, __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
		
		ssr_list = G_generate_file_list_m12(NULL, &ssr_list_len, ssr_path, NULL, "ridx", GFL_FULL_PATH_m12);
		if (ssr_list_len == 0) {  // no other ssr records - delete directory
			sprintf_m12(command, "rmdir %s 1> %s 2> %s", ssr_path, NULL_DEVICE_m12, NULL_DEVICE_m12);
			system_m12(command, FALSE_m12, __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
		}
		
		free_m12((void *) ssr_list, __FUNCTION__);
		FPS_free_processing_struct_m12(new_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(new_rd_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_rd_fps, TRUE_m12);
		G_free_session_m12(sess, TRUE_m12);
		return(0.0);
	}
	
	// skip record
	++i;
	rd += rh->total_record_bytes;
	
	// write subsequent records
	for (; i < n_recs; ++i) {
		ri[i].file_offset = new_rd_fps->parameters.flen;
		G_write_file_m12(new_ri_fps, FPS_APPEND_m12, INDEX_BYTES_m12, 1, ri + i, USE_GLOBAL_BEHAVIOR_m12);
		rh = (RECORD_HEADER_m12 *) rd;
		G_write_file_m12(new_rd_fps, FPS_APPEND_m12, rh->total_record_bytes, 1, rd, USE_GLOBAL_BEHAVIOR_m12);
		rd += rh->total_record_bytes;
	}
	
	// write terminal index
	--i;  // reuse last index
	ri[i].file_offset = new_rd_fps->parameters.flen;
	ri[i].start_time = new_ri_fps->universal_header->segment_end_time + 1;
	ri[i].type_code = REC_Term_TYPE_CODE_m12;
	ri[i].version_major = 0xFF;
	ri[i].version_minor = 0xFF;
	ri[i].encryption_level = NO_ENCRYPTION_m12;
	G_write_file_m12(new_ri_fps, FPS_APPEND_m12, INDEX_BYTES_m12, 1, ri + i, USE_GLOBAL_BEHAVIOR_m12);
	
	// close new files
	G_write_file_m12(new_ri_fps, 0, 0, FPS_CLOSE_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);  // flush header
	G_write_file_m12(new_rd_fps, 0, 0, FPS_CLOSE_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);  // flush header
	
	// move temp record index file into place
#if defined MACOS_m12 || defined LINUX_m12
	sprintf_m12(command, "mv -f %s %s 1> %s 2> %s", tmp_ri_file, ri_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
#ifdef WINDOWS_m12
	sprintf(command, "move /y %s %s 1> %s 2> %s", tmp_ri_file, ri_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
	system_m12(command, FALSE_m12, __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
	
	// move temp record data file into place
#if defined MACOS_m12 || defined LINUX_m12
	sprintf_m12(command, "mv -f %s %s 1> %s 2> %s", tmp_rd_file, rd_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
#ifdef WINDOWS_m12
	sprintf(command, "move /y %s %s 1> %s 2> %s", tmp_rd_file, rd_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
	system_m12(command, FALSE_m12, __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
	
	// clean up
	FPS_free_processing_struct_m12(new_ri_fps, TRUE_m12);
	FPS_free_processing_struct_m12(new_rd_fps, TRUE_m12);
	FPS_free_processing_struct_m12(orig_ri_fps, TRUE_m12);
	FPS_free_processing_struct_m12(orig_rd_fps, TRUE_m12);
	G_free_session_m12(sess, TRUE_m12);
	
	return(0.0);
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
