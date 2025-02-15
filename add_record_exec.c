
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2021


#include "medlib_m12.h"
#include "add_record_exec.h"


// Mex gateway routine
void    mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[])
{
        si1	chan_dir[FULL_FILE_NAME_BYTES_m12], password[PASSWORD_BYTES_m12], rec_type[TYPE_BYTES_m12];
        si1	*rec_text, enc_level;
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
	if (nrhs != 6) {
		mexPrintf("Six inputs required: chan_dir, password, rec_type, rec_time, rec_text, encryption_level\n");
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
	
	// rec type
	*rec_type = 0;
	if (mxIsEmpty(prhs[2]) == 0) {
		if (mxGetClassID(prhs[2]) == mxCHAR_CLASS) {
			len = mxGetNumberOfElements(prhs[2]) + 1; // Get the length of the input string
			if (len > TYPE_BYTES_m12) {
				mexPrintf("Record type (input 3) is too long\n");
				return;
			} else {
				mxGetString(prhs[2], rec_type, len);
			}
		} else {
			mexPrintf("Record type (input 3) must be a string\n");
			return;
		}
	}

        // rec time
	if (mxIsEmpty(prhs[3]) == 1) {
		mexPrintf("No record time (input 4) specified\n");
		return;
	}
	rec_time = get_si8_scalar(prhs[3]);
        
        // rec text
	rec_text = NULL;
	if (mxIsEmpty(prhs[4]) == 1) {
		if (strcmp(rec_type, "Note") == 0)
			mexPrintf("No note text (input 5) specified\n");
		return;
	}
	if (mxGetClassID(prhs[4]) == mxCHAR_CLASS) {
		len = mxGetNumberOfElements(prhs[4]) + 1; // Get the length of the input string
		rec_text = calloc((size_t) len, sizeof(si1));
		mxGetString(prhs[4], rec_text, len);
	} else {
		mexPrintf("Record text (input 5) must be a string\n");
		return;
	}

	// enc level
	if (mxIsEmpty(prhs[5]) == 1) {
		mexPrintf("No encryption level (input 6) specified\n");
		return;
	}
	enc_level = (si1) get_si8_scalar(prhs[5]);
	
 	// initialize MED library
	G_initialize_medlib_m12(FALSE_m12, FALSE_m12);
                
	// return codes: 0.0 == ok, -1.0 == unspecified error, -2.0 == insufficient access
	success = add_record(chan_dir, password, rec_type, rec_time, rec_text, enc_level);
	*((sf8 *) mxGetPr(plhs[0])) = success;

        // clean up
	free((void *) rec_text);
	G_free_globals_m12(TRUE_m12);

        return;
}


sf8	add_record(si1 *chan_dir, si1 *password, si1 *rec_type, si8 rec_time, si1 *rec_text, si1 enc_level)
{
	ui1				*record_bytes;
	si1				*rec_str, number_str[FILE_NUMBERING_DIGITS_m12 + 1], *sess_name, *ssr_name;
	si1				ri_file[FULL_FILE_NAME_BYTES_m12], rd_file[FULL_FILE_NAME_BYTES_m12];
	si1				tmp_ri_file[FULL_FILE_NAME_BYTES_m12], tmp_rd_file[FULL_FILE_NAME_BYTES_m12];
	si1				ssr_path[FULL_FILE_NAME_BYTES_m12], command[(FULL_FILE_NAME_BYTES_m12 * 2) + 32];
	si4				seg_idx, fe;
	ui8				flags;
	si8				i, n_recs, text_len;
	ui1				*rd;
	FILE_PROCESSING_STRUCT_m12	*orig_ri_fps, *orig_rd_fps, *new_ri_fps, *new_rd_fps, *proto_fps;
	RECORD_HEADER_m12		*rh, *nrh;
	RECORD_INDEX_m12		*ri, *nri, new_ri;
	UNIVERSAL_HEADER_m12		*uh;
	SESSION_m12			*sess;
	TIME_SLICE_m12			slice;
	REC_Seiz_v10_m12		*Seiz_v10;
	
	
	// read session
	G_initialize_time_slice_m12(&slice);
	slice.start_time = slice.end_time = rec_time;
	flags = LH_READ_SEGMENT_METADATA_m12;
	sess = G_open_session_m12(NULL, &slice, chan_dir, 0, flags, password);   // limited open to get segment records, read segment metadata, & process password
	if (sess == NULL) {
		if (globals_m12->password_data.processed == 0) {
			G_warning_message_m12("%s():\nCannot read session\n", __FUNCTION__);
		} else {
			G_warning_message_m12("%s():\nCannot read session => Check that the password is correct, and that metadata files exist\n", __FUNCTION__);
			G_show_password_hints_m12(NULL, 0);
		}
		putchar_m12('\n');
		return(-1.0);
	}
	if (globals_m12->password_data.access_level < enc_level) {
		G_warning_message_m12("%s(): password not valid for this encryption level\n", __FUNCTION__);
		G_show_password_hints_m12(NULL, enc_level);
		G_free_session_m12(sess, TRUE_m12);
		putchar_m12('\n');
		return(-2.0);
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
	}
	if (fe == DOES_NOT_EXIST_m12) {
		ssr_name = sess_name = globals_m12->fs_session_name;
		sprintf_m12(ssr_path, "%s/%s.%s", sess->path, sess_name, RECORD_DIRECTORY_TYPE_STRING_m12);
		sprintf_m12(ri_file, "%s/%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_DATA_FILE_TYPE_STRING_m12);
		sprintf_m12(rd_file, "%s/%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_DATA_FILE_TYPE_STRING_m12);
		ssr_name = globals_m12->fs_session_name;
		n_recs = 0;
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
	memset((void *) uh->channel_name, 0, BASE_FILE_NAME_BYTES_m12);
	uh->channel_UID = UID_NO_ENTRY_m12;
	if (orig_ri_fps) {
		uh->file_UID = orig_ri_fps->universal_header->file_UID;  // keep original file UIDs since overwriting
		uh->provenance_UID = orig_ri_fps->universal_header->provenance_UID;  // keep original provenance UIDs since overwriting
	}
	G_write_file_m12(new_ri_fps, 0, UNIVERSAL_HEADER_BYTES_m12, FPS_UNIVERSAL_HEADER_ONLY_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);
	
	// create new segmented session record data fps
	sprintf_m12(tmp_rd_file, "%s/tmp_%s_s%s.%s", ssr_path, ssr_name, number_str, RECORD_DATA_FILE_TYPE_STRING_m12);
	new_rd_fps = FPS_allocate_processing_struct_m12(NULL, tmp_rd_file, RECORD_DATA_FILE_TYPE_CODE_m12, REC_LARGEST_RECORD_BYTES_m12, NULL, proto_fps, 0);
	if (new_rd_fps == NULL)
		return(-1.0);
	new_rd_fps->directives.open_mode = FPS_W_OPEN_MODE_m12;
	new_rd_fps->directives.close_file = FALSE_m12;
	uh = new_rd_fps->universal_header;
	memset((void *) uh->channel_name, 0, BASE_FILE_NAME_BYTES_m12);
	uh->channel_UID = UID_NO_ENTRY_m12;
	if (orig_rd_fps) {
		uh->file_UID = orig_rd_fps->universal_header->file_UID;  // keep original file UIDs since overwriting
		uh->provenance_UID = orig_rd_fps->universal_header->provenance_UID;  // keep original provenance UIDs since overwriting
	}
	G_write_file_m12(new_rd_fps, 0, UNIVERSAL_HEADER_BYTES_m12, FPS_UNIVERSAL_HEADER_ONLY_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);

	// build new record
	text_len = strlen(rec_text) + 1;  // account for terminal zero
	if (strcmp(rec_type, "Note") == 0) {
		record_bytes = (ui1 *) calloc((size_t) (RECORD_HEADER_BYTES_m12 + text_len + REC_RECORD_BODY_ALIGNMENT_m12), sizeof(ui1));  // leave room for padding
		nrh = (RECORD_HEADER_m12 *) record_bytes;
		nri = &new_ri;
		nrh->type_code = nri->type_code = REC_Note_TYPE_CODE_m12;
		nrh->version_major = nri->version_major = 1;
		nrh->version_minor = nri->version_minor = 0;
		nrh->start_time = nri->start_time = rec_time;
		nri->encryption_level = enc_level;
		nrh->encryption_level = -enc_level;
		rec_str = (si1 *) nrh + RECORD_HEADER_BYTES_m12;
		strcpy(rec_str, rec_text);
		nrh->total_record_bytes = (ui4) G_pad_m12((ui1 *) rec_str, text_len, REC_RECORD_BODY_ALIGNMENT_m12) + RECORD_HEADER_BYTES_m12;
	} else if (strcmp(rec_type, "Seiz") == 0) {
		text_len -= REC_Seiz_v10_PAD_BYTES_m12;  // first 8 bytes of string within structure
		if (text_len < 0)
			text_len = 0;
		record_bytes = (ui1 *) calloc((size_t) (RECORD_HEADER_BYTES_m12 + REC_Seiz_v10_DESCRIPTION_OFFSET_m12 + text_len + REC_RECORD_BODY_ALIGNMENT_m12), sizeof(ui1));  // leave room for padding
		nrh = (RECORD_HEADER_m12 *) record_bytes;
		nri = &new_ri;
		nrh->type_code = nri->type_code = REC_Seiz_TYPE_CODE_m12;
		nrh->version_major = nri->version_major = 1;
		nrh->version_minor = nri->version_minor = 0;
		nrh->start_time = nri->start_time = rec_time;
		nri->encryption_level = enc_level;
		nrh->encryption_level = -enc_level;
		Seiz_v10 = (REC_Seiz_v10_m12 *) (nrh + 1);
		Seiz_v10->end_time = UUTC_NO_ENTRY_m12;  // no option to enter seizure end time in this version
		strcpy(Seiz_v10->description, rec_text);
		nrh->total_record_bytes = (ui4) G_pad_m12((ui1 *) (nrh + 1), REC_Seiz_v10_BYTES_m12 + text_len, REC_RECORD_BODY_ALIGNMENT_m12) + RECORD_HEADER_BYTES_m12;
	} else {
		G_warning_message_m12("%s(): unrecognized record type\n", __FUNCTION__);
		FPS_free_processing_struct_m12(new_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(new_rd_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_ri_fps, TRUE_m12);
		FPS_free_processing_struct_m12(orig_rd_fps, TRUE_m12);
		G_free_session_m12(sess, TRUE_m12);
		return(-1.0);
	}

	// write preceding records
	for (i = 0; i < n_recs; ++i) {
		if (rec_time < ri[i].start_time)
			break;
		G_write_file_m12(new_ri_fps, FPS_APPEND_m12, (size_t) INDEX_BYTES_m12, (size_t) 1, (void *) (ri + i), USE_GLOBAL_BEHAVIOR_m12);
		rh = (RECORD_HEADER_m12 *) rd;
		G_write_file_m12(new_rd_fps, FPS_APPEND_m12, (size_t) rh->total_record_bytes, (size_t) 1, (void *) rd, USE_GLOBAL_BEHAVIOR_m12);
		rd += rh->total_record_bytes;
	}

	// write new record
	nri->file_offset = new_rd_fps->parameters.flen;
	G_write_file_m12(new_ri_fps, FPS_APPEND_m12, (size_t) INDEX_BYTES_m12, (size_t) 1, (void *) nri, USE_GLOBAL_BEHAVIOR_m12);
	G_write_file_m12(new_rd_fps, FPS_APPEND_m12, (size_t) nrh->total_record_bytes, (size_t) 1, (void *) nrh, USE_GLOBAL_BEHAVIOR_m12);
	
	// write subsequent records
	for (; i < n_recs; ++i) {
		ri[i].file_offset = new_rd_fps->parameters.flen;
		G_write_file_m12(new_ri_fps, FPS_APPEND_m12, (size_t) INDEX_BYTES_m12, (size_t) 1, (void *) (ri + i), USE_GLOBAL_BEHAVIOR_m12);
		rh = (RECORD_HEADER_m12 *) rd;
		G_write_file_m12(new_rd_fps, FPS_APPEND_m12, (size_t) rh->total_record_bytes, (size_t) 1, (void *) rd, USE_GLOBAL_BEHAVIOR_m12);
		rd += rh->total_record_bytes;
	}
	
	// write terminal index
	nri->file_offset = new_rd_fps->parameters.flen;
	nri->start_time = new_ri_fps->universal_header->segment_end_time + 1;
	nri->type_code = REC_Term_TYPE_CODE_m12;
	nri->version_major = 0xFF;
	nri->version_minor = 0xFF;
	nri->encryption_level = NO_ENCRYPTION_m12;
	G_write_file_m12(new_ri_fps, FPS_APPEND_m12, INDEX_BYTES_m12, 1, nri, USE_GLOBAL_BEHAVIOR_m12);

	// update headers & close
	G_write_file_m12(new_ri_fps, 0, 0, FPS_CLOSE_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);
	G_write_file_m12(new_rd_fps, 0, 0, FPS_CLOSE_m12, NULL, USE_GLOBAL_BEHAVIOR_m12);

	// close & free files
	free((void *) record_bytes);
	FPS_free_processing_struct_m12(new_ri_fps, TRUE_m12);
	FPS_free_processing_struct_m12(new_rd_fps, TRUE_m12);
	FPS_free_processing_struct_m12(orig_ri_fps, TRUE_m12);
	FPS_free_processing_struct_m12(orig_rd_fps, TRUE_m12);

	// move temp files into place
#if defined MACOS_m12 || defined LINUX_m12
	sprintf_m12(command, "mv -f %s %s 1> %s 2> %s", tmp_ri_file, ri_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
#ifdef WINDOWS_m12
	sprintf(command, "move /y %s %s 1> %s 2> %s", tmp_ri_file, ri_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
	system_m12(command, FALSE_m12, __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
	
#if defined MACOS_m12 || defined LINUX_m12
	sprintf_m12(command, "mv -f %s %s 1> %s 2> %s", tmp_rd_file, rd_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
#ifdef WINDOWS_m12
	sprintf(command, "move /y %s %s 1> %s 2> %s", tmp_rd_file, rd_file, NULL_DEVICE_m12, NULL_DEVICE_m12);
#endif
	system_m12(command, FALSE_m12, __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);

	// clean up
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
