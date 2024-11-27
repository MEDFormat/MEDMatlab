
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2024

#ifndef MATRIX_MED_IN
#define MATRIX_MED_IN

//Includes
#include "medlib_m12.h"

// Version (Read_MED package including matrix_MED)
#define READ_MED_VER_MAJOR	((ui1) 1)
#define READ_MED_VER_MINOR	((ui1) 1)

// Miscellaneous
#define MAX_CHANNELS		512

// Persistent Behaviors
#define PERSIST_OPEN		((ui1) 1)	// close & free any open session, open new session, & return
#define PERSIST_CLOSE		((ui1) 2)	// close & free any open session & return
#define PERSIST_READ		((ui1) 4)	// read current session (& open if none exists), replace existing parameters with non-empty passed parameters
#define PERSIST_READ_NEW	(PERSIST_READ | PERSIST_OPEN)	// close & free any open session, open & read new session, leave open after read
#define PERSIST_READ_CLOSE	(PERSIST_READ | PERSIST_CLOSE)	// close & free any open session, open & read new session, close after read
#define PERSIST_NONE		PERSIST_READ_CLOSE	// default

// Matlab Raw Page Structure
#define NUMBER_OF_MATRIX_FIELDS_mat			10
#define MATRIX_FIELD_NAMES_mat { \
        "slice_start_time", \
	"slice_start_time_string", \
        "slice_end_time", \
        "slice_end_time_string", \
	"sampling_frequencies", \
	"contigua", \
	"records", \
	"samples", \
	"minima", \
	"maxima" \
}
#define MATRIX_FIELDS_SLICE_START_TIME_UUTC_IDX_mat	0
#define MATRIX_FIELDS_SLICE_START_TIME_STRING_IDX_mat	1
#define MATRIX_FIELDS_SLICE_END_TIME_UUTC_IDX_mat	2
#define MATRIX_FIELDS_SLICE_END_TIME_STRING_IDX_mat	3
#define MATRIX_FIELDS_SAMPLING_FREQUENCIES_IDX_mat	4
#define MATRIX_FIELDS_CONTIGUA_IDX_mat			5
#define MATRIX_FIELDS_RECORDS_IDX_mat			6
#define MATRIX_SAMPLES_IDX_mat				7
#define MATRIX_MINIMA_IDX_mat				8
#define MATRIX_MAXIMA_IDX_mat				9

// Matlab Contiguon Structure (note indices here are relative to output page)
#define NUMBER_OF_CONTIGUON_FIELDS_mat          	6
#define CONTIGUON_FIELD_NAMES_mat { \
	"start_index", \
	"end_index", \
	"start_time", \
	"start_time_string", \
	"end_time", \
	"end_time_string" \
}
#define CONTIGUON_FIELDS_START_INDEX_IDX_mat		0
#define CONTIGUON_FIELDS_END_INDEX_IDX_mat		1
#define CONTIGUON_FIELDS_START_TIME_IDX_mat		2
#define CONTIGUON_FIELDS_START_TIME_STRING_IDX_mat	3
#define CONTIGUON_FIELDS_END_TIME_IDX_mat		4
#define CONTIGUON_FIELDS_END_TIME_STRING_IDX_mat	5

// Commnon Matlab Record Structure Element Indices
#define RECORD_FIELDS_START_INDEX_IDX_mat        	0  // NOTE: this field does not exist in the other mex record functions, it's for drawing lines
#define RECORD_FIELDS_START_TIME_IDX_mat        	1
#define RECORD_FIELDS_START_TIME_STRING_IDX_mat		2
#define RECORD_FIELDS_TYPE_STRING_IDX_mat       	3
#define RECORD_FIELDS_TYPE_CODE_IDX_mat         	4
#define RECORD_FIELDS_VERSION_STRING_IDX_mat    	5
#define RECORD_FIELDS_ENCRYPTION_IDX_mat 		6
#define RECORD_FIELDS_ENCRYPTION_STRING_IDX_mat 	7

// Matlab NlxP (v1.0)Record Structure
#define NUMBER_OF_NLXP_v10_RECORD_FIELDS_mat	10
#define NLXP_v10_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"subport", \
	"value" \
}
#define NLXP_v10_RECORD_FIELDS_SUBPORT_IDX_mat	8
#define NLXP_v10_RECORD_FIELDS_VALUE_IDX_mat	9

// Matlab Note (v1.0) Record Structure
#define NUMBER_OF_NOTE_v10_RECORD_FIELDS_mat	9
#define NOTE_v10_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"text" \
}
#define NOTE_v10_RECORD_FIELDS_TEXT_IDX_mat	8

// Matlab Note (v1.1) Record Structure
#define NUMBER_OF_NOTE_v11_RECORD_FIELDS_mat		11
#define NOTE_v11_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"end_time", \
	"end_time_string", \
	"text" \
}
#define NOTE_v11_RECORD_FIELDS_END_TIME_IDX_mat		8
#define NOTE_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat	9
#define NOTE_v11_RECORD_FIELDS_TEXT_IDX_mat		10

// Matlab Epoc (v2.0) Record Structure
#define NUMBER_OF_EPOC_v20_RECORD_FIELDS_mat        		13
#define EPOC_v20_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"end_time", \
	"end_time_string", \
	"stage_code", \
	"stage_string", \
	"scorer_id" \
}
#define EPOC_v20_RECORD_FIELDS_END_TIME_IDX_mat			8
#define EPOC_v20_RECORD_FIELDS_END_TIME_STRING_IDX_mat		9
#define EPOC_v20_RECORD_FIELDS_STAGE_CODE_IDX_mat		10
#define EPOC_v20_RECORD_FIELDS_STAGE_STRING_IDX_mat		11
#define EPOC_v20_RECORD_FIELDS_SCORER_ID_IDX_mat		12

// Matlab Sgmt (v1.0) Record Structure
#define NUMBER_OF_SGMT_v10_RECORD_FIELDS_mat				17
#define SGMT_v10_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"end_time", \
	"end_time_string", \
	"start_sample_number", \
	"end_sample_number", \
	"segment_number", \
	"segment_UID", \
	"acquistion_channel_number", \
	"sampling_frequency", \
	"description" \
}
#define SGMT_v10_RECORD_FIELDS_END_TIME_IDX_mat				8
#define SGMT_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat			9
#define SGMT_v10_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat		10
#define SGMT_v10_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat		11
#define SGMT_v10_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat			12
#define SGMT_v10_RECORD_FIELDS_SEGMENT_UID_IDX_mat			13
#define SGMT_v10_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat	14
#define SGMT_v10_RECORD_FIELDS_SAMPLING_FREQUENCY_IDX_mat		15
#define SGMT_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat			16

// Matlab Sgmt (v1.1) Record Structure
#define NUMBER_OF_SGMT_v11_RECORD_FIELDS_mat				15
#define SGMT_v11_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"end_time", \
	"end_time_string", \
	"start_sample_number", \
	"end_sample_number", \
	"segment_number", \
	"acquistion_channel_number", \
	"description" \
}
#define SGMT_v11_RECORD_FIELDS_END_TIME_IDX_mat				8
#define SGMT_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat			9
#define SGMT_v11_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat		10
#define SGMT_v11_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat		11
#define SGMT_v11_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat			12
#define SGMT_v11_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat	13
#define SGMT_v11_RECORD_FIELDS_DESCRIPTION_IDX_mat			14

// Matlab Unknown Record Structure
#define NUMBER_OF_UNKN_RECORD_FIELDS_mat	9
#define UNKN_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"comment" \
}
#define UNKN_RECORD_FIELDS_COMMENT_IDX_mat	8

typedef struct {
	pthread_t_m12	thread_id;
	si1		*chan_path;
	TERN_m12	detrend;
	TERN_m12	trace_ranges;
	TERN_m12	antialias;
	TIME_SLICE_m12	*slice;
	si8		n_out_samps;
	sf8		in_sf;
	sf8		out_sf;
	sf8		*samps;
	sf8		*mins;
	sf8		*maxs;
} MATRIX_THREAD_INFO;

// Prototypes
void		mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
si8		get_si8_scalar(const mxArray *mx_arr);
mxArray		*matrix_MED(void *chan_list, si4 n_files, si8 start_time, si8 end_time, si8 n_out_samps, si1 *password, TERN_m12 antialias, TERN_m12 detrend, TERN_m12 trace_ranges, ui1 persist_mode);
void		build_contigua(DATA_MATRIX_m12 *dm, mxArray *mat_raw_page);
void		build_session_records(SESSION_m12 *sess, DATA_MATRIX_m12 *dm, mxArray *mat_raw_page);
mxArray		*fill_record(RECORD_HEADER_m12 *rh, DATA_MATRIX_m12 *dm);
si4		rec_compare(const void *a, const void *b);


#endif /* MATRIX_MED_IN */
