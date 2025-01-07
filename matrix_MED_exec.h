
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

// Matrix Parameter Structure element indices
#define MPS_DATA_IDX			0
#define MPS_SAMPLE_DIMENSION_MODE_IDX	1
#define MPS_SAMPLE_DIMENSION_IDX	2
#define MPS_EXTENTS_MODE_IDX		3
#define MPS_START_IDX			4
#define MPS_END_IDX			5
#define MPS_TIME_MODE_IDX		6
#define MPS_PASSWORD_IDX		7
#define MPS_INDEX_CHANNEL_IDX		8
#define MPS_FILTER_IDX			9
#define MPS_LOW_CUTOFF_IDX		10
#define MPS_HIGH_CUTOFF_IDX		11
#define MPS_SCALE_IDX			12
#define MPS_FORMAT_IDX			13
#define MPS_PADDING_IDX			14
#define MPS_INTERP_IDX			15
#define MPS_BINTERP_IDX			16
#define MPS_PERSISTENCE_IDX		17
#define MPS_DETREND_IDX			18
#define MPS_RANGES_IDX			19
#define MPS_EXTREMA_IDX			20
#define MPS_RECORDS_IDX			21
#define MPS_CONTIGUA_IDX		22
#define MPS_CHANNEL_NAMES_IDX		23
#define MPS_CHANNEL_FREQUENCIES_IDX	24

// Sample Dimension Modes
#define SAMPLE_DIMENSION_MODE_COUNT		0
#define SAMPLE_DIMENSION_MODE_RATE		1

// Extents Modes
#define EXTENTS_MODE_TIME	0
#define EXTENTS_MODE_INDICES	1

// Time Modes
#define TIME_MODE_DURATION	0
#define TIME_MODE_END_TIME	1

// Filter Types
#define FILT_ANTIALIAS		0
#define FILT_NONE		1
#define FILT_LOWPASS		2
#define FILT_HIGHPASS		3
#define FILT_BANDPASS		4
#define FILT_BANDSTOP		5

// Format
#define	FORMAT_DOUBLE		0
#define	FORMAT_SINGLE		1
#define	FORMAT_INT32		2
#define	FORMAT_INT16		3

// Padding
#define PAD_NONE		0
#define PAD_ZERO		1
#define PAD_NAN			2

// Interpolation
#define INTERP_LINEAR_MAKIMA	0
#define INTERP_LINEAR_SPLINE	1
#define INTERP_LINEAR		2
#define INTERP_SPLINE		3
#define INTERP_MAKIMA		4
#define INTERP_BINTERP		5

// Binterpolation Modes
#define BINTERP_MEAN		0
#define BINTERP_MEDIAN		1
#define BINTERP_CENTER		2
#define BINTERP_FAST		3

// Persistence
#define PERSIST_NONE		((ui1) 0)	// read current session (& open if none exists), close after read
#define PERSIST_OPEN		((ui1) 1)	// close & free any open session, open new session, & return
#define PERSIST_CLOSE		((ui1) 2)	// close & free any open session & return
#define PERSIST_READ		((ui1) 4)	// read current session (& open if none exists), replace existing parameters with non-empty passed parameters
#define PERSIST_READ_NEW	(PERSIST_READ | PERSIST_OPEN)	// close & free any open session, open & read new session, leave open after read
#define PERSIST_READ_CLOSE	(PERSIST_READ | PERSIST_CLOSE)	// read current session (& open if none exists), close after read

// Matlab Matrix Structure
#define NUMBER_OF_MATRIX_FIELDS_mat				17
#define MATRIX_FIELD_NAMES_mat { \
        "slice_start_time", \
	"slice_start_time_string", \
        "slice_end_time", \
        "slice_end_time_string", \
	"channel_names", \
	"matrix_sampling_frequency", \
	"channel_sampling_frequencies", \
	"filter_low_cutoff", \
	"filter_high_cutoff", \
	"contigua", \
	"records", \
	"samples", \
	"range_minima", \
	"range_maxima", \
	"trace_minima", \
	"trace_maxima", \
	"status" \
}
#define MATRIX_FIELDS_SLICE_START_TIME_IDX_mat			0
#define MATRIX_FIELDS_SLICE_START_TIME_STRING_IDX_mat		1
#define MATRIX_FIELDS_SLICE_END_TIME_IDX_mat			2
#define MATRIX_FIELDS_SLICE_END_TIME_STRING_IDX_mat		3
#define MATRIX_FIELDS_CHANNEL_NAMES_IDX_mat			4
#define MATRIX_FIELDS_MATRIX_SAMPLING_FREQUENCY_IDX_mat		5
#define MATRIX_FIELDS_CHANNEL_SAMPLING_FREQUENCIES_IDX_mat	6
#define MATRIX_FIELDS_FILTER_LOW_CUTOFF_IDX_mat			7
#define MATRIX_FIELDS_FILTER_HIGH_CUTOFF_IDX_mat		8
#define MATRIX_FIELDS_CONTIGUA_IDX_mat				9
#define MATRIX_FIELDS_RECORDS_IDX_mat				10
#define MATRIX_SAMPLES_IDX_mat					11
#define MATRIX_RANGE_MINIMA_IDX_mat				12
#define MATRIX_RANGE_MAXIMA_IDX_mat				13
#define MATRIX_TRACE_MINIMA_IDX_mat				14
#define MATRIX_TRACE_MAXIMA_IDX_mat				15
#define MATRIX_STATUS_IDX_mat					16

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

// Matlab NlxP (v1.0) Record Structure
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
#define NUMBER_OF_NOTE_v11_RECORD_FIELDS_mat		12
#define NOTE_v11_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"end_index", \
	"end_time", \
	"end_time_string", \
	"text" \
}
#define NOTE_v11_RECORD_FIELDS_END_INDEX_IDX_mat	8  // NOTE: this field does not exist in the other mex record functions, it's for drawing lines
#define NOTE_v11_RECORD_FIELDS_END_TIME_IDX_mat		9
#define NOTE_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat	10
#define NOTE_v11_RECORD_FIELDS_TEXT_IDX_mat		11

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

// Matlab Seiz (v1.0) Record Structure
#define NUMBER_OF_SEIZ_v10_RECORD_FIELDS_mat		12
#define SEIZ_v10_RECORD_FIELD_NAMES_mat { \
	"start_index", \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"end_index", \
	"end_time", \
	"end_time_string", \
	"description" \
}
#define SEIZ_v10_RECORD_FIELDS_END_INDEX_IDX_mat	8  // NOTE: this field does not exist in the other mex record functions, it's for drawing lines
#define SEIZ_v10_RECORD_FIELDS_END_TIME_IDX_mat		9
#define SEIZ_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat	10
#define SEIZ_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat	11

// Matlab Sgmt (v1.0) Record Structure
#define NUMBER_OF_SGMT_v10_RECORD_FIELDS_mat				16
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
	"description" \
}
#define SGMT_v10_RECORD_FIELDS_END_TIME_IDX_mat				8
#define SGMT_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat			9
#define SGMT_v10_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat		10
#define SGMT_v10_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat		11
#define SGMT_v10_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat			12
#define SGMT_v10_RECORD_FIELDS_SEGMENT_UID_IDX_mat			13
#define SGMT_v10_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat	14
#define SGMT_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat			15

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
	TERN_m12			detrend, ranges, extrema, records, contigua, chan_names, chan_freqs;
	ui1				persist_mode;
	void				*MED_paths;
	si1				password[PASSWORD_BYTES_m12], index_channel[BASE_FILE_NAME_BYTES_m12];
	si4				n_files, filter, format, padding, interpolation, bin_interpolation;
	si4				sample_dimension_mode, extents_mode, time_mode;
	si8				start_time, end_time, start_index, end_index, n_out_samps;
	sf8				out_freq, low_cutoff, high_cutoff, scale;
} C_MPS;

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
mxArray		*matrix_MED(C_MPS *cmps);
void		build_channel_names(SESSION_m12 *sess, mxArray *mat_matrix);
void		build_contigua(DATA_MATRIX_m12 *dm, mxArray *mat_raw_page);
void		build_session_records(SESSION_m12 *sess, DATA_MATRIX_m12 *dm, mxArray *mat_raw_page);
mxArray		*fill_record(RECORD_HEADER_m12 *rh, DATA_MATRIX_m12 *dm);
si4		rec_compare(const void *a, const void *b);
TERN_m12	get_logical(const mxArray *mx_arr);
si8		get_si8_scalar(const mxArray *mx_arr);


#endif /* MATRIX_MED_IN */
