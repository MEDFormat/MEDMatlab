
function slice = read_MED(file_list, varargin)

    %
    %   read_MED() requires 1 to 12 inputs
    %
    %   Prototype:
    %   slice = read_MED(file_list, [start_time], [end_time], [start_index], [end_index], [password], [index_channel], [samples_as_singles], [persistence_mode], [metadata], [records], [contigua]);
    %
    %   read_MED() returns a single Matlab slice structure, or a boolean indicating success or failure
    %
    %   Arguments in square brackets are optional => '[]' will substitute default values
    %
    %   Input Arguments:
    %   file_list:  string array, strings can contain regexp
    %   start_time:  if empty/absent, defaults to session/channel start (unless indices are specified)
    %   end_time:  if empty/absent, defaults to session/channel end (unless indices are specified)
    %   start_index:  if empty/absent, defaults to session/channel start (unless times are specified)
    %   end_index:  if empty/absent, defaults to session/channel end (unless times are specified)
    %   password:  if empty/absent, proceeds as if unencrypted (but, may error out)
    %   index_channel:  if empty/absent, and necessary, defaults to first channel in set
    %   samples_as_singles:  if empty/absent, defaults to false (options: true, false)
    %   persistence_mode: if empty/absent, defaults to 'none' (string or code)
    %   metadata: if empty/absent, defaults to true (options: true, false)
    %   records: if empty/absent, defaults to true (options: true, false)
    %   contigua: if empty/absent, defaults to true (options: true, false)
    %
    %   Persistence Modes:
    %       'none' (0):	 single read behavior (default: this is identical to 'read close' below)
    %       'open' (1):	 close & free any open session, open new session, & return
    %       'close' (2):  close & free any open session & return
    %       'read' (4)	read current session (& open if none exists), replace existing parameters with non-empty passed parameters
    %       'read new' (5):	 close any open session, open & read new session
    %       'read close' (6):  close any open session, open & read new session, close session & return
    %
    %   If samples_as_singles is set to 'true', sample values are returned as singles (32-bit floating 
    %   point numbers), rather than doubles (64-bit floating point numbers, the Matlab default type).
    %   Singles have adequate precision to exactly represent integers up to 24-bits.
    %   Exercising this option doubles the amount of data that can be stored in memory by Matlab.
    %
    %   If metadata is set to 'false', slice metadata are not returned
    %
    %   If records is set to 'false', slice records are not returned
    %
    %   If contigua is set to 'false', slice contigua are not returned
    %
    %   In MED, times are preferable to indices as they are independent of sampling frequencies
    %       a) times are natively in offset µUTC (oUTC), but unoffset times may be used
    %       b) negatives times are considered to be relative to the session start
    %       c) if indices are used, index numbering begins at 1, per Matlab convention
    %
    %   In sessions with varying sampling frequencies, the indices channel is used to
    %   determine the import extents on all channels when delimited by index values
    %
    %   e.g. to get samples 1001:2000 from 'channel_1', and all the corresponding samples, in time, 
    %   from the other channels, regardless of their sampling frequencies: specify 1001 as the start 
    %   index, 2000 as the end index, and 'channel_1' as the index_channel
         
    %   Copyright Dark Horse Neuro, 2021


    % Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = [];  % put in single quotes to make it char array

    slice = false;  % failure return value

    if nargin == 0 || nargin > 12 || nargout ~=  1
        help read_MED;
        return;
    end

    % file_list
    if ischar(file_list) == false
        if isstring(file_list)
            file_list = char(file_list);
        elseif iscell(file_list)
            for i = 1:numel(file_list)
                if ischar(file_list{i}) == false
                    if isstring(file_list{i})
                        file_list{i} = char(file_list{i});
                    else
                        help read_MED;
                        return;
                   end
                end
            end
        else
            help read_MED;
            return;
        end
    end
    
    % start_time
    if nargin > 1
        start_time = varargin{1};
        if isscalar(start_time) == false
            if isempty(start_time) == false
                if ischar(start_time) == false
                    if isstring(start_time)  % mex functions only take strings as char arrays
                        start_time = char(start_time);
                    else
                        help read_MED;
                        return;
                    end
                end
            end
        end
    else
        start_time = [];
    end
  
    % end_time
    if nargin > 2
        end_time = varargin{2};
        if isscalar(end_time) == false
            if isempty(end_time) == false
                if ischar(end_time) == false
                    if isstring(end_time)  % mex functions only take strings as char arrays
                        end_time = char(end_time);
                    else
                        help read_MED;
                        return;
                    end
                end
            end
        end
    else
        end_time = [];
    end

    % start_index
    if nargin > 3
        start_index = varargin{3};
        if isscalar(start_index) == false
            if isempty(start_index) == false
                if ischar(start_index) == false
                    if isstring(start_index)  % mex functions only take strings as char arrays
                        start_index = char(start_index);
                    else
                        help read_MED;
                        return;
                    end
                end
            end
        end
    else
        start_index = [];
    end
  
    % end_index
    if nargin > 4
        end_index = varargin{4};
        if isscalar(end_index) == false
            if isempty(end_index) == false
                if ischar(end_index) == false
                    if isstring(end_index)  % mex functions only take strings as char arrays
                        end_index = char(end_index);
                    else
                        help read_MED;
                        return;
                    end
                end
            end
        end
    else
        end_index = [];
    end

    % password
    if nargin > 5
        password = varargin{5};
        if isempty(password) == false
            if ischar(password) == false
                if isstring(password)  % mex functions only take strings as char arrays
                    password = char(password);
                else
                    help read_MED;
                    return;
                end
            end
        end
    else
        password = DEFAULT_PASSWORD;
    end

    % index_channel
    if nargin > 6
        index_channel = varargin{6};
        if isempty(index_channel) == false
            if isstring(index_channel)  % mex functions only take strings as char arrays
                index_channel = char(index_channel);
            else
                help read_MED;
                return;
            end
        end
    else
        index_channel = [];
    end

    % samples_as_singles
    if nargin > 7
        samples_as_singles = varargin{7};
        if isempty(samples_as_singles) == false
            if isstring(samples_as_singles)  % mex functions only take strings as char arrays
                samples_as_singles = char(samples_as_singles);
            end
        end
    else
        samples_as_singles = [];
    end

    % persistence_mode
    if nargin > 8
        persistence_mode = varargin{8};
        if isempty(persistence_mode) == false
            if ischar(persistence_mode) == false
                if isstring(persistence_mode)
                    persistence_mode = char(persistence_mode);
                elseif isscalar(persistence_mode) == false
                    help read_MED;
                    return;
                end
            end
        end
    else
        persistence_mode = [];
    end

    % metadata
    if nargin > 9
        metadata = varargin{9};
        if isempty(metadata) == true
            if isstring(metadata)  % mex functions only take strings as char arrays
                metadata = char(metadata);
            end
        end
    else
        metadata = [];
    end

    % records
    if nargin > 10
        records = varargin{10};
        if isempty(records) == true
            if isstring(records)  % mex functions only take strings as char arrays
                records = char(records);
            end
        end
    else
        records = [];
    end

    % contigua
    if nargin > 11
        contigua = varargin{11};
        if isempty(contigua) == true
            if isstring(contigua)  % mex functions only take strings as char arrays
                contigua = char(contigua);
            end
        end
    else
        contigua = [];
    end

    % mex function
    try
        file_list = get_full_paths(file_list);
        slice = read_MED_exec(file_list, start_time, end_time, start_index, end_index, password, index_channel, samples_as_singles, persistence_mode, metadata, records, contigua);
        if islogical(slice)  % can be true, false, or structure
            if slice == false
                errordlg('read_MED() error', 'Read MED');
            end
            return;
        end
    catch ME
        OS = computer;
        if (strcmp(OS, 'PCWIN64') == 1)
            DIR_DELIM = '\';
        else
            DIR_DELIM = '/';
        end
        switch ME.identifier
            case 'MATLAB:UndefinedFunction'
                [READ_MED_PATH, ~, ~] = fileparts(which('read_MED'));
                RESOURCES = [READ_MED_PATH DIR_DELIM 'Resources'];
                addpath(RESOURCES, READ_MED_PATH, '-begin');
                savepath;
                msg = ['Added ', RESOURCES, ' to your search path.' newline];
                beep
                fprintf(2, '%s', msg);  % 2 == stderr, so red in command window
                file_list = get_full_paths(file_list);
                slice = read_MED_exec(file_list, start_time, end_time, start_index, end_index, password, index_channel, samples_as_singles, persistence_mode, metadata, records, contigua);
                if islogical(slice)  % can be true, false, or structure
                    if slice == false
                        errordlg('read_MED() error', 'Read MED');
                    end
                    return;
                end
           otherwise
                rethrow(ME);
        end
    end
    
end

