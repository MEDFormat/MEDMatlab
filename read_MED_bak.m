
function slice = read_MED(MED_dirs, varargin)

    %
    %   read_MED() requires 1 to 12 inputs
    %
    %   Prototype:
    %   slice = read_MED(MED_dirs, [start_time], [end_time], [start_index], [end_index], [password], [index_channel], [persistence], [samples_as_singles], [metadata], [records], [contigua]);
    %
    %   read_MED() returns a single Matlab slice structure, or a boolean indicating success or failure
    %
    %   Arguments in square brackets are optional => '[]' will substitute default values
    %
    %   Input Arguments:
    %   MED_dirs:  string array, strings can contain regexp
    %   start_time:  if empty/absent, defaults to session/channel start (unless indices are specified)
    %   end_time:  if empty/absent, defaults to session/channel end (unless indices are specified)
    %   start_index:  if empty/absent, defaults to session/channel start (unless times are specified)
    %   end_index:  if empty/absent, defaults to session/channel end (unless times are specified)
    %   password:  if empty/absent, proceeds as if unencrypted (but, may error out)
    %   index_channel:  if empty/absent, and necessary, defaults to first channel in set
    %   persistence: if empty/absent, defaults to 'none' (string or code)
    %   samples_as_singles:  if empty/absent, defaults to false (options: true, false)
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
    %       b) negative times are considered to be relative to the session start
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

    if (nargin < 1 || nargin > 12)
        errordlg('Bad input number of arguments', 'Read MED');
        help read_MED;
        return;
    end

    if (nargout ~=  1)
        errordlg('One output argument required', 'Read MED');
        help read_MED;
        return;
    end

    % MED_dirs
    if (ischar(MED_dirs)) == false
        if (isstring(MED_dirs))
            MED_dirs = char(MED_dirs);
        elseif (iscell(MED_dirs))
            for i = 1:numel(MED_dirs)
                if (ischar(MED_dirs{i})) == false
                    if (isstring(MED_dirs{i}))  % mex functions only take strings as char arrays
                        MED_dirs{i} = char(MED_dirs{i});
                    else
                        errordlg('Bad MED directory list', 'Read MED');
                        help read_MED;
                        return;
                   end
                end
            end
        else
            errordlg('Bad MED directory list', 'Read MED');
            help read_MED;
            return;
        end
    end
    
    % start_time
    if (nargin > 1)
        start_time = condition_named_num(varargin{1});
        if (isnan(start_time))
            errordlg('Bad start time', 'Read MED');
            help read_MED;
            return;
        end
    else
        start_time = [];
    end
  
    % end_time
    if (nargin > 2)
        end_time = condition_named_num(varargin{2});
        if (isnan(end_time))
            errordlg('Bad end time', 'Read MED');
            help read_MED;
            return;
        end
    else
        end_time = [];
    end

    % start_index
    if (nargin > 3)
        start_index = condition_named_num(varargin{3});
        if (isnan(start_index))
            errordlg('Bad start index', 'Read MED');
            help read_MED;
            return;
        end
    else
        start_index = [];
    end
  
    % end_index
    if (nargin > 4)
        end_index = condition_named_num(varargin{4});
        if (isnan(end_index))
            errordlg('Bad end index', 'Read MED');
            help read_MED;
            return;
        end
    else
        end_index = [];
    end

    % password
    if (nargin > 5)
        password = varargin{5};
        if (isempty(password)) == false
            if (ischar(password)) == false
                if (isstring(password))  % mex functions only take strings as char arrays
                    password = char(password);
                else
                    errordlg('Bad password', 'Read MED');
                    help read_MED;
                    return;
                end
            end
        else
            password = DEFAULT_PASSWORD;
        end
    else
        password = DEFAULT_PASSWORD;
    end

    % index_channel
    if (nargin > 6)
        index_channel = varargin{6};
        if (isempty(index_channel)) == false
            if (ischar(index_channel)) == false
                if (isstring(index_channel))  % mex functions only take strings as char arrays
                    index_channel = char(index_channel);
                else
                    errordlg('Bad index channel', 'Read MED');
                    help read_MED;
                    return;
                end
            end
        end
    else
        index_channel = [];
    end

    % persistence
    if (nargin > 7)
        persistence = condition_named_num(varargin{7});
        if (isnan(persistence))
            errordlg('Bad persistence option', 'Read MED');
            help read_MED;
            return;
        end
    else
        persistence = [];
    end

    % samples_as_singles
    if (nargin > 8)
        samples_as_singles = condition_logical(varargin{8});
        if (isnan(samples_as_singles))
            errordlg('Bad samples as singles option', 'Read MED');
            help read_MED;
            return;
        end
    else
        samples_as_singles = [];
    end

    % metadata
    if (nargin > 9)
        metadata = condition_logical(varargin{9});
        if (isnan(metadata))
            errordlg('Bad metadata option', 'Read MED');
            help read_MED;
            return;
        end
    else
        metadata = [];
    end

    % records
    if (nargin > 10)
        records = condition_logical(varargin{10});
        if (isnan(records))
            errordlg('Bad records option', 'Read MED');
            help read_MED;
            return;
        end
    else
        records = [];
    end

    % contigua
    if (nargin > 11)
        contigua = condition_logical(varargin{11});
        if (isnan(contigua))
            errordlg('Bad contigua option', 'Read MED');
            help read_MED;
            return;
        end
    else
        contigua = [];
    end

    % mex function
    try
        MED_dirs = get_full_paths(MED_dirs);
        slice = read_MED_exec(MED_dirs, start_time, end_time, start_index, end_index, password, index_channel, persistence, samples_as_singles, metadata, records, contigua);
        if (islogical(slice))  % can be true, false, or structure
            if (slice == false)
                errordlg('Read error', 'Read MED');
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
                MED_dirs = get_full_paths(MED_dirs);
                slice = read_MED_exec(MED_dirs, start_time, end_time, start_index, end_index, password, index_channel, persistence, samples_as_singles, metadata, records, contigua);
                if (islogical(slice))  % can be true, false, or structure
                    if (slice == false)
                        errordlg('Read error', 'Read MED');
                    end
                    return;
                end
           otherwise
                rethrow(ME);
        end
    end
    
end


% returns logical, empty set, or NaN on error
function e = condition_logical(e)
    if (isempty(e) == false)
        if (isscalar(e) == false)
            if (ischar(e) == false)
                if (isstring(e))
                    e = char(e);
                else
                    e = NaN;
                end
                if (e(1) == 't' || e(1) == 'T' || e(1) == 'y' || e(1) == 'Y')
                    e = true;
                elseif (e(1) == 'f' || e(1) == 'F' || e(1) == 'n' || e(1) == 'N')
                    e = false;
                else
                    e = NaN;
                end
            end
        elseif (islogical(e) == false)
            if (e == 0)
                e = false;
            elseif (e == 1)
                e = true;
            else
                e = NaN;
            end
        end
    end
end


% returns char array, scalar, or NaN on error
function e = condition_named_num(e)
    if (isempty(e) == false)
        if (isscalar(e) == false)
            if (ischar(e) == false)
                if (isstring(e))  % mex functions only take strings as char arrays
                    e = char(e);
                else
                    e = NaN;
                    return;
                end
            end
        end
    end
end