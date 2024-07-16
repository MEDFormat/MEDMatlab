function session = read_MED(file_list, varargin)

    %
    %   read_MED() requires 1 to 9 inputs
    %
    %   Prototype:
    %   session = read_MED(file_list, [start_time], [end_time], [start_index], [end_index], [password], [indices_reference_channel], [samples_as_singles], [persistence_mode]);
    %
    %   read_MED() returns a single Matlab session structure, or a boolean indicating success or failure
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
    %   indices_reference_channel:  if empty/absent, and necessary, defaults to first channel in set
    %   samples_as_singles:  if empty/absent, defaults to false (options: true, false)
    %   persistence_mode: (string or code)
    %       'none' (0):	 single read behavior (default: this is identical to 'read close' below)
    %		'close' (1):  close & free any open session & return
    %       'open' (2):	 close & free any open session, open new session, & return
    %       'read' (4)	read current session (& open if none exists), replace existing parameters with non-empty passed parameters
    %       'read close' (5):  close any open session, open & read new session, close session & return
    %       'read new' (6):	 close any open session, open & read new session
    %
    %   If samples_as_singles is set to 'true', sample values are returned as singles (32-bit floating 
    %   point numbers), rather than doubles (64-bit floating point numbers, the Matlab default type).
    %   Singles have adequate precision to exactly represent integers up to 24-bits.
    %   Exercising this option doubles the amount of data that can be stored in memory by Matlab.
    %
    %   In MED, times are preferable to indices as they are independent of sampling frequencies
    %       a) times are natively in offset µUTC (oUTC), but unoffset times may be used
    %       b) negatives times are considered to be relative to the session start
    %       c) if indices are used, index numbering begins at 1, per Matlab convention
    %
    %   In sessions with varying sampling frequencies, the indices reference channel is used to
    %   determine the import extents on all channels when delimited by index values
    %
    %   e.g. to get samples 1001:2000 from 'channel_1', and all the corresponding samples, in time, 
    %   from the other channels, regardless of their sampling frequencies: specify 1001 as the start 
    %   index, 2000 as the end index, and 'channel_1' as the indices_reference_channel
         
    %   Copyright Dark Horse Neuro, 2021


    if nargin == 0 || nargin > 9 || nargout ~=  1
        help read_MED;
        return;
    end
    
    if nargin > 1
        start_time = varargin{1};
        if isstring(start_time)  % mex functions take strings as char arrays
            start_time = char(start_time);
        end
    else
        start_time = [];
    end
  
    if nargin > 2
        end_time = varargin{2};
        if isstring(end_time)  % mex functions take strings as char arrays
            end_time = char(end_time);
        end
    else
        end_time = [];
    end

    if nargin > 3
        start_index = varargin{3};
        if isstring(start_index)  % mex functions take strings as char arrays
            start_index = char(start_index);
        end
    else
        start_index = [];
    end
  
    if nargin > 4
        end_index = varargin{4};
        if isstring(end_index)  % mex functions take strings as char arrays
            end_index = char(end_index);
        end
    else
        end_index = [];
    end

    %   Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = [];
    if nargin > 5
        password = varargin{5};
    else
        password = DEFAULT_PASSWORD;
    end
    if isstring(password)  % mex functions take strings as char arrays
        password = char(password);
    end

    if nargin > 6
        reference_channel = varargin{6};
        if isstring(reference_channel)  % mex functions take strings as char arrays
            reference_channel = char(reference_channel);
        end
    else
        reference_channel = [];
    end

    if nargin > 7
        samples_as_singles = varargin{7};
        if isstring(samples_as_singles)  % mex functions take strings as char arrays
            samples_as_singles = char(samples_as_singles);
        end
    else
        samples_as_singles = [];
    end

    if nargin > 8
        persistence_mode = varargin{8};
        if isstring(persistence_mode)
            persistence_mode = char(persistence_mode);
        end
    else
        persistence_mode = [];
    end


    % launch mex function
    try
        file_list = get_full_paths(file_list);
        session = read_MED_exec(file_list, start_time, end_time, start_index, end_index, password, reference_channel, samples_as_singles, persistence_mode);
        if isa(session, 'logical')
            if session == false
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
                session = read_MED_exec(file_list, start_time, end_time, start_index, end_index, password, reference_channel, samples_as_singles, persistence_mode);
                if isa(session, 'logical')
                    if session == false
                        errordlg('read_MED() error', 'Read MED');
                    end
                    return;
                end
           otherwise
                rethrow(ME);
        end
    end
    
end

