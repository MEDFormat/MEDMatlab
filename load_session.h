
// Written by Matt Stead
// Copyright Dark Horse Neuro Inc, 2020

#ifndef LOAD_SESSION_IN
#define LOAD_SESSION_IN

// Includes
#include "medlib_m12.h"

// Defines

// Version
#define LS_READ_MED_VER_MAJOR			((ui1) 1)
#define LS_READ_MED_VER_MINOR			((ui1) 1)

// Miscellaneous
#define MAX_CHANNELS                        	512

// Matlab Session Structure
#define NUMBER_OF_SESSION_FIELDS_mat            2
#define SESSION_FIELD_NAMES_mat { \
        "metadata", \
        "channels", \
}
#define SESSION_FIELDS_METADATA_IDX_mat         0
#define SESSION_FIELDS_CHANNELS_IDX_mat         1

// Matlab Metadata Structure
#define NUMBER_OF_METADATA_FIELDS_mat           45
#define METADATA_FIELD_NAMES_mat { \
	"path", \
	"start_time", \
	"end_time", \
	"start_time_string", \
	"end_time_string", \
	"session_start_time", \
	"session_end_time", \
	"session_start_time_string", \
	"session_end_time_string", \
	"start_sample_number", \
	"end_sample_number", \
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
#define METADATA_FIELDS_START_TIME_UUTC_IDX_mat                         1
#define METADATA_FIELDS_END_TIME_UUTC_IDX_mat                           2
#define METADATA_FIELDS_START_TIME_STRING_IDX_mat                       3
#define METADATA_FIELDS_END_TIME_STRING_IDX_mat                         4
#define METADATA_FIELDS_SESSION_START_TIME_UUTC_IDX_mat                 5
#define METADATA_FIELDS_SESSION_END_TIME_UUTC_IDX_mat                   6
#define METADATA_FIELDS_SESSION_START_TIME_STRING_IDX_mat               7
#define METADATA_FIELDS_SESSION_END_TIME_STRING_IDX_mat                 8
#define METADATA_FIELDS_START_SAMPLE_NUMBER_IDX_mat			9
#define METADATA_FIELDS_END_SAMPLE_NUMBER_IDX_mat			10
#define METADATA_FIELDS_SESSION_NAME_IDX_mat                            11
#define METADATA_FIELDS_CHANNEL_NAME_IDX_mat                            12
#define METADATA_FIELDS_INDEX_CHANNEL_NAME_mat				13
#define METADATA_FIELDS_ANONYMIZED_SUBJECT_ID_IDX_mat                 	14
#define METADATA_FIELDS_SESSION_UID_IDX_mat                             15
#define METADATA_FIELDS_CHANNEL_UID_IDX_mat                             16
#define METADATA_FIELDS_SESSION_DESCRIPTION_IDX_mat                     17
#define METADATA_FIELDS_CHANNEL_DESCRIPTION_IDX_mat                     18
#define METADATA_FIELDS_EQUIPMENT_DESCRIPTION_IDX_mat                   19
#define METADATA_FIELDS_ACQUISITION_CHANNEL_NUMBER_IDX_mat              20
#define METADATA_FIELDS_REFERENCE_DESCRIPTION_IDX_mat                   21
#define METADATA_FIELDS_SAMPLING_FREQUENCY_IDX_mat                      22
#define METADATA_FIELDS_LOW_FREQUENCY_FILTER_SETTING_IDX_mat            23
#define METADATA_FIELDS_HIGH_FREQUENCY_FILTER_SETTING_IDX_mat           24
#define METADATA_FIELDS_NOTCH_FILTER_FREQUENCY_SETTING_IDX_mat          25
#define METADATA_FIELDS_AC_LINE_FREQUENCY_IDX_mat                       26
#define METADATA_FIELDS_AMPLITUDE_UNITS_CONVERSION_FACTOR_IDX_mat       27
#define METADATA_FIELDS_AMPLITUDE_UNITS_DESCRIPTION_IDX_mat             28
#define METADATA_FIELDS_TIME_BASE_UNITS_CONVERSION_FACTOR_IDX_mat       29
#define METADATA_FIELDS_TIME_BASE_UNITS_DESCRIPTION_IDX_mat             30
#define METADATA_SECTION_3_FIELDS_IDX_mat                               31
#define METADATA_FIELDS_RECORDING_TIME_OFFSET_IDX_mat                   31
#define METADATA_FIELDS_STANDARD_UTC_OFFSET_IDX_mat                     32
#define METADATA_FIELDS_STANDARD_TIMEZONE_STRING_IDX_mat                33
#define METADATA_FIELDS_STANDARD_TIMEZONE_ACRONYM_IDX_mat               34
#define METADATA_SECTION_3_NO_ACCESS_FIELDS_IDX_mat			35
#define METADATA_FIELDS_DAYLIGHT_TIMEZONE_STRING_IDX_mat                35
#define METADATA_FIELDS_DAYLIGHT_TIMEZONE_ACRONYM_IDX_mat               36
#define METADATA_FIELDS_SUBJECT_NAME_1_IDX_mat                          37
#define METADATA_FIELDS_SUBJECT_NAME_2_IDX_mat                          38
#define METADATA_FIELDS_SUBJECT_NAME_3_IDX_mat                          39
#define METADATA_FIELDS_SUBJECT_ID_IDX_mat                              40
#define METADATA_FIELDS_RECORDING_COUNTRY_IDX_mat                       41
#define METADATA_FIELDS_RECORDING_TERRITORY_IDX_mat                     42
#define METADATA_FIELDS_RECORDING_LOCALITY_IDX_mat                      43
#define METADATA_FIELDS_RECORDING_INSTITUTION_IDX_mat                   44

// Matlab Channel Structure
#define NUMBER_OF_CHANNEL_FIELDS_mat            1
#define CHANNEL_FIELD_NAMES_mat { \
        "metadata", \
}
#define CHANNEL_FIELDS_METADATA_IDX_mat         0
#define CHANNEL_FIELDS_DATA_IDX_mat             1

// Matlab Discontiguon Structure
#define NUMBER_OF_DISCONTIGUON_FIELDS_mat	4
#define DISCONTIGUON_FIELD_NAMES_mat { \
	"start_time", \
	"end_time", \
	"start_proportion", \
	"end_proportion" \
}
#define DISCONTIGUON_FIELDS_START_TIME_IDX_mat	0
#define DISCONTIGUON_FIELDS_END_TIME_IDX_mat	1
#define DISCONTIGUON_FIELDS_START_PROP_IDX_mat	2
#define DISCONTIGUON_FIELDS_END_PROP_IDX_mat	3


// Prototypes
void            mexFunction(si4 nlhs, mxArray *plhs[], si4 nrhs, const mxArray *prhs[]);
si4     	load_session(void *file_list, si4 n_files, si1 *password, mxArray *plhs[]);
mxArray    	*build_discontigua(SESSION_m12 *sess);
void		build_metadata(SESSION_m12 *sess, mxArray *mat_session);
mxArray     	*get_sess_rec_times(SESSION_m12 *sess);


#endif /* LOAD_SESSION_IN */
