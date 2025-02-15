
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2024

#ifndef READ_MED_IN
#define READ_MED_IN

// Includes
#include "medlib_m12.h"

// Version (Read_MED package including read_MED)
#define READ_MED_VER_MAJOR	((ui1) 1)
#define READ_MED_VER_MINOR	((ui1) 1)

// Miscellaneous
#define MAX_CHANNELS		512

// Matrix Parameter Structure element indices
#define RPS_DATA_IDX			0
#define RPS_EXTENTS_MODE_IDX		1
#define RPS_START_IDX			2
#define RPS_END_IDX			3
#define RPS_PASSWORD_IDX		4
#define RPS_INDEX_CHANNEL_IDX		5
#define RPS_FORMAT_IDX			6
#define RPS_FILTER_IDX			7
#define RPS_LOW_CUTOFF_IDX		8
#define RPS_HIGH_CUTOFF_IDX		9
#define RPS_PERSISTENCE_IDX		10
#define RPS_METADATA_IDX		11
#define RPS_RECORDS_IDX			12
#define RPS_CONTIGUA_IDX		13

// Extents Modes
#define EXTENTS_MODE_TIME	0
#define EXTENTS_MODE_INDICES	1

// Format
#define	FORMAT_DOUBLE		0
#define	FORMAT_SINGLE		1
#define	FORMAT_INT32		2
#define	FORMAT_INT16		3

// Filter Types
#define FILT_NONE		0
#define FILT_LOWPASS		1
#define FILT_HIGHPASS		2
#define FILT_BANDPASS		3
#define FILT_BANDSTOP		4

// Persistence
#define PERSIST_NONE		((ui1) 0)	// read current session (& open if none exists), close after read
#define PERSIST_OPEN		((ui1) 1)	// close & free any open session, open new session, & return
#define PERSIST_CLOSE		((ui1) 2)	// close & free any open session & return
#define PERSIST_READ		((ui1) 4)	// read current session (& open if none exists), replace existing parameters with non-empty passed parameters
#define PERSIST_READ_NEW	(PERSIST_READ | PERSIST_OPEN)	// close & free any open session, open & read new session, leave open after read
#define PERSIST_READ_CLOSE	(PERSIST_READ | PERSIST_CLOSE)	// read current session (& open if none exists), close after read

// Matlab Session Structure
#define NUMBER_OF_SESSION_FIELDS_mat            5
#define SESSION_FIELD_NAMES_mat { \
        "metadata", \
        "channels", \
        "records", \
        "contigua", \
	"status" \
}
#define SESSION_FIELDS_METADATA_IDX_mat         0
#define SESSION_FIELDS_CHANNELS_IDX_mat         1
#define SESSION_FIELDS_RECORDS_IDX_mat          2
#define SESSION_FIELDS_CONTIGUA_IDX_mat         3
#define SESSION_FIELDS_STATUS_IDX_mat		4

// Matlab Metadata Structure
#define NUMBER_OF_METADATA_FIELDS_mat           46
#define METADATA_FIELD_NAMES_mat { \
        "path", \
        "slice_start_time", \
        "slice_end_time", \
        "slice_start_time_string", \
        "slice_end_time_string", \
        "session_start_time", \
        "session_end_time", \
        "session_start_time_string", \
        "session_end_time_string", \
        "slice_start_sample_number", \
        "slice_end_sample_number", \
	"session_number_of_samples", \
	"session_name", \
        "channel_name", \
	"index_channel_name", \
        "anonymized_subject_ID", \
        "session_UID", \
        "channel_UID", \
        "session_description", \
        "channel_description", \
        "equipment_description", \
        "acquisition_channel_number", \
        "reference_description", \
        "sampling_frequency", \
        "low_frequency_filter_setting", \
        "high_frequency_filter_setting", \
        "notch_filter_frequency_setting", \
        "AC_line_frequency", \
        "amplitude_units_conversion_factor", \
        "amplitude_units_description", \
        "time_base_units_conversion_factor", \
        "time_base_units_description", \
        "recording_time_offset", \
	"standard_UTC_offset", \
	"standard_timezone_string", \
	"standard_timezone_acronym", \
	"daylight_timezone_string", \
	"daylight_timezone_acronym", \
        "subject_name_1", \
        "subject_name_2", \
        "subject_name_3", \
        "subject_ID", \
        "recording_country", \
        "recording_territory", \
        "recording_locality", \
        "recording_institution" \
}
#define METADATA_FIELDS_PATH_IDX_mat                                    0
#define METADATA_FIELDS_SLICE_START_TIME_UUTC_IDX_mat			1
#define METADATA_FIELDS_SLICE_END_TIME_UUTC_IDX_mat			2
#define METADATA_FIELDS_SLICE_START_TIME_STRING_IDX_mat			3
#define METADATA_FIELDS_SLICE_END_TIME_STRING_IDX_mat			4
#define METADATA_FIELDS_SESSION_START_TIME_UUTC_IDX_mat                 5
#define METADATA_FIELDS_SESSION_END_TIME_UUTC_IDX_mat                   6
#define METADATA_FIELDS_SESSION_START_TIME_STRING_IDX_mat               7
#define METADATA_FIELDS_SESSION_END_TIME_STRING_IDX_mat                 8
#define METADATA_FIELDS_SLICE_START_SAMPLE_NUMBER_IDX_mat		9
#define METADATA_FIELDS_SLICE_END_SAMPLE_NUMBER_IDX_mat			10
#define METADATA_FIELDS_SESSION_NUMBER_OF_SAMPLES_IDX_mat		11
#define METADATA_FIELDS_SESSION_NAME_IDX_mat                            12
#define METADATA_FIELDS_CHANNEL_NAME_IDX_mat                            13
#define METADATA_FIELDS_INDEX_CHANNEL_NAME_mat				14
#define METADATA_FIELDS_ANONYMIZED_SUBJECT_ID_IDX_mat                 	15
#define METADATA_FIELDS_SESSION_UID_IDX_mat                             16
#define METADATA_FIELDS_CHANNEL_UID_IDX_mat                             17
#define METADATA_FIELDS_SESSION_DESCRIPTION_IDX_mat                     18
#define METADATA_FIELDS_CHANNEL_DESCRIPTION_IDX_mat                     19
#define METADATA_FIELDS_EQUIPMENT_DESCRIPTION_IDX_mat                   20
#define METADATA_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat              21
#define METADATA_FIELDS_REFERENCE_DESCRIPTION_IDX_mat                   22
#define METADATA_FIELDS_SAMPLING_FREQUENCY_IDX_mat                      23
#define METADATA_FIELDS_LOW_FREQUENCY_FILTER_SETTING_IDX_mat            24
#define METADATA_FIELDS_HIGH_FREQUENCY_FILTER_SETTING_IDX_mat           25
#define METADATA_FIELDS_NOTCH_FILTER_FREQUENCY_SETTING_IDX_mat          26
#define METADATA_FIELDS_AC_LINE_FREQUENCY_IDX_mat                       27
#define METADATA_FIELDS_AMPLITUDE_UNITS_CONVERSION_FACTOR_IDX_mat       28
#define METADATA_FIELDS_AMPLITUDE_UNITS_DESCRIPTION_IDX_mat             29
#define METADATA_FIELDS_TIME_BASE_UNITS_CONVERSION_FACTOR_IDX_mat       30
#define METADATA_FIELDS_TIME_BASE_UNITS_DESCRIPTION_IDX_mat             31
#define METADATA_SECTION_3_FIELDS_IDX_mat                               32
#define METADATA_FIELDS_RECORDING_TIME_OFFSET_IDX_mat                   32
#define METADATA_FIELDS_STANDARD_UTC_OFFSET_IDX_mat                     33
#define METADATA_FIELDS_STANDARD_TIMEZONE_STRING_IDX_mat                34
#define METADATA_FIELDS_STANDARD_TIMEZONE_ACRONYM_IDX_mat               35
#define METADATA_SECTION_3_NO_ACCESS_FIELDS_IDX_mat			36
#define METADATA_FIELDS_DAYLIGHT_TIMEZONE_STRING_IDX_mat                36
#define METADATA_FIELDS_DAYLIGHT_TIMEZONE_ACRONYM_IDX_mat               37
#define METADATA_FIELDS_SUBJECT_NAME_1_IDX_mat                          38
#define METADATA_FIELDS_SUBJECT_NAME_2_IDX_mat                          39
#define METADATA_FIELDS_SUBJECT_NAME_3_IDX_mat                          40
#define METADATA_FIELDS_SUBJECT_ID_IDX_mat                              41
#define METADATA_FIELDS_RECORDING_COUNTRY_IDX_mat                       42
#define METADATA_FIELDS_RECORDING_TERRITORY_IDX_mat                     43
#define METADATA_FIELDS_RECORDING_LOCALITY_IDX_mat                      44
#define METADATA_FIELDS_RECORDING_INSTITUTION_IDX_mat                   45

// Matlab Channel Structure
#define NUMBER_OF_CHANNEL_FIELDS_mat            4
#define CHANNEL_FIELD_NAMES_mat { \
	"name", \
        "metadata", \
        "data", \
        "contigua" \
}
#define CHANNEL_FIELDS_NAME_IDX_mat         	0
#define CHANNEL_FIELDS_METADATA_IDX_mat         1
#define CHANNEL_FIELDS_DATA_IDX_mat             2
#define CHANNEL_FIELDS_CONTIGUA_IDX_mat         3

// Matlab Contiguon Structure (contiguous region - plural "contigua")
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
#define RECORD_FIELDS_START_TIME_IDX_mat        	0
#define RECORD_FIELDS_START_TIME_STRING_IDX_mat		1
#define RECORD_FIELDS_TYPE_STRING_IDX_mat       	2
#define RECORD_FIELDS_TYPE_CODE_IDX_mat         	3
#define RECORD_FIELDS_VERSION_STRING_IDX_mat    	4
#define RECORD_FIELDS_ENCRYPTION_IDX_mat 		5
#define RECORD_FIELDS_ENCRYPTION_STRING_IDX_mat 	6

// Matlab NlxP (v1.0) Record Structure
#define NUMBER_OF_NLXP_v10_RECORD_FIELDS_mat	9
#define NLXP_v10_RECORD_FIELD_NAMES_mat { \
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
#define NLXP_v10_RECORD_FIELDS_SUBPORT_IDX_mat	7
#define NLXP_v10_RECORD_FIELDS_VALUE_IDX_mat	8

// Matlab Note (v1.0) Record Structure
#define NUMBER_OF_NOTE_v10_RECORD_FIELDS_mat	8
#define NOTE_v10_RECORD_FIELD_NAMES_mat { \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"text" \
}
#define NOTE_v10_RECORD_FIELDS_TEXT_IDX_mat	7

// Matlab Note (v1.1) Record Structure
#define NUMBER_OF_NOTE_v11_RECORD_FIELDS_mat		10
#define NOTE_v11_RECORD_FIELD_NAMES_mat { \
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
#define NOTE_v11_RECORD_FIELDS_END_TIME_IDX_mat		7
#define NOTE_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat	8
#define NOTE_v11_RECORD_FIELDS_TEXT_IDX_mat		9

// Matlab Seiz (v1.0) Record Structure
#define NUMBER_OF_SEIZ_v10_RECORD_FIELDS_mat		10
#define SEIZ_v10_RECORD_FIELD_NAMES_mat { \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"end_time", \
	"end_time_string", \
	"description" \
}
#define SEIZ_v10_RECORD_FIELDS_END_TIME_IDX_mat		7
#define SEIZ_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat	8
#define SEIZ_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat	9

// Matlab Epoc (v2.0) Record Structure
#define NUMBER_OF_EPOC_v20_RECORD_FIELDS_mat        		12
#define EPOC_v20_RECORD_FIELD_NAMES_mat { \
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
#define EPOC_v20_RECORD_FIELDS_END_TIME_IDX_mat			7
#define EPOC_v20_RECORD_FIELDS_END_TIME_STRING_IDX_mat		8
#define EPOC_v20_RECORD_FIELDS_STAGE_CODE_IDX_mat		9
#define EPOC_v20_RECORD_FIELDS_STAGE_STRING_IDX_mat		10
#define EPOC_v20_RECORD_FIELDS_SCORER_ID_IDX_mat		11

// Matlab Sgmt (v1.0) Record Structure
#define NUMBER_OF_SGMT_v10_RECORD_FIELDS_mat				16
#define SGMT_v10_RECORD_FIELD_NAMES_mat { \
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
#define SGMT_v10_RECORD_FIELDS_END_TIME_IDX_mat				7
#define SGMT_v10_RECORD_FIELDS_END_TIME_STRING_IDX_mat			8
#define SGMT_v10_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat		9
#define SGMT_v10_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat		10
#define SGMT_v10_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat			11
#define SGMT_v10_RECORD_FIELDS_SEGMENT_UID_IDX_mat			12
#define SGMT_v10_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat	13
#define SGMT_v10_RECORD_FIELDS_SAMPLING_FREQUENCY_IDX_mat		14
#define SGMT_v10_RECORD_FIELDS_DESCRIPTION_IDX_mat			15

// Matlab Sgmt (v1.1) Record Structure
#define NUMBER_OF_SGMT_v11_RECORD_FIELDS_mat				14
#define SGMT_v11_RECORD_FIELD_NAMES_mat { \
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
#define SGMT_v11_RECORD_FIELDS_END_TIME_IDX_mat				7
#define SGMT_v11_RECORD_FIELDS_END_TIME_STRING_IDX_mat			8
#define SGMT_v11_RECORD_FIELDS_START_SAMPLE_NUMBER_IDX_mat		9
#define SGMT_v11_RECORD_FIELDS_END_SAMPLE_NUMBER_IDX_mat		10
#define SGMT_v11_RECORD_FIELDS_SEGMENT_NUMBER_IDX_mat			11
#define SGMT_v11_RECORD_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat	12
#define SGMT_v11_RECORD_FIELDS_DESCRIPTION_IDX_mat			13

// Matlab Unknown Record Structure
#define NUMBER_OF_UNKN_RECORD_FIELDS_mat	8
#define UNKN_RECORD_FIELD_NAMES_mat { \
	"start_time", \
	"start_time_string", \
	"type_string", \
	"type_code", \
	"version_string", \
	"encryption", \
	"encryption_string", \
	"comment" \
}
#define UNKN_RECORD_FIELDS_COMMENT_IDX_mat	7

#define FILTER_ORDER	4

typedef struct {
	TERN_m12                	metadata, records, contigua;
	void                    	*MED_paths;
	ui1				persist_mode;
	si1                     	password[PASSWORD_BYTES_m12 + 1];
	si1                     	index_channel[FULL_FILE_NAME_BYTES_m12];
	si4                     	extents_mode, n_files, filter, format;
	si8                     	start_time, end_time, start_index, end_index;
	sf8				low_cutoff, high_cutoff;
} C_RPS;

typedef struct {
	CHANNEL_m12	*channel;
	C_RPS		*crps;
	mxArray		*samples;
} JOB_INFO;


// Prototypes
void			mexExitFunction(void);
void            	mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
TERN_m12		get_logical(const mxArray *mx_arr);
si8             	get_si8_scalar(const mxArray *mx_arr);
mxArray     		*read_MED(C_RPS *crps);
void			build_channel_names(SESSION_m12 *sess, mxArray *mat_sess);
void    		build_metadata(SESSION_m12 *sess, mxArray *mat_session);
void			build_contigua(SESSION_m12 *sess, mxArray *mat_session);
void           		build_session_records(SESSION_m12 *sess, mxArray *mat_session);
mxArray         	*fill_record(RECORD_HEADER_m12 *rh);
si4             	rec_compare(const void *a, const void *b);
pthread_rval_m12	distribute_and_filter(void *ptr);


#endif /* READ_MED_IN */
