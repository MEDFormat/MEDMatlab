
function matrix = matrix_MED(chan_list, n_out_samps, varargin)

    %
    %   matrix_MED() requires 2 to 9 inputs: 
    %   1)  chan_list
    %   2)  n_out_samps
    %   3)  [start_time]
    %   4)  [end_time]
    %   5)  [password]
    %   6)  [antialias ([true] / false)]
    %   7)  [detrend (true / [false])]
    %   8)  [trace_ranges (true / [false])] 
    %   9)  [persistence_mode (string or code: options shown below)] 
    %
    %   Prototype:
    %   matrix_struct = matrix_MED(chan_list, n_out_samps, [start_time], [end_time], [password], [antialias], [detrend], [trace_ranges], [persistence_mode]);
    %
    %   matrix_MED() returns a single Matlab matrix structure
    %
    %   Arguments in square brackets are optional => '[]' will substitute default values
    %
    %   Input Arguments:
    %   chan_list:  cell array of strings (strings can contain regexp)
    %   start_time:  if empty/absent, defaults to session/channel start
    %   end_time:  if empty/absent, defaults to session/channel end
    %   n_out_samps: the output matrix sample dimension
    %   password:  if empty/absent, proceeds as if unencrypted (but, may error out)
    %   antialias:  if empty/absent, defaults to true (options: true, false)
    %   detrend:  if empty/absent, defaults to false (options: true, false)
    %   trace_ranges:  if empty/absent, defaults to false (options: true, false)
    %   persistence_mode:  if empty/absent, defaults to 'none' (0)
    %   Persistence Modes:
    %       'none' (0):	 single read behavior (default: this is identical to 'read close' below)
    %       'open' (1):	 close & free any open session, open new session, & return
    %		'close' (2):  close & free any open session & return
    %       'read' (4):	read current session (& open if none exists), replace existing parameters with non-empty passed parameters
    %       'read new' (5):	 close any open session, open & read new session
    %       'read close' (6):  close any open session, open & read new session, close session & return
    %
    %   In MED, times are preferable to indices as they are independent of sampling frequencies
    %       a) times are natively in offset µUTC (oUTC), but unoffset times may be used
    %       b) negatives times are considered to be relative to the session start
    %
    %   Copyright Dark Horse Neuro, 2021

    
    % Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = [];  % put in single quotes to make it char array

    matrix = false;  % failure return value

    if nargin < 2 || nargin > 9 || nargout ~=  1
        help matrix_MED;
        return;
    end

    % chan_list
    if ischar(chan_list) == false
        if isstring(chan_list)
            chan_list = char(chan_list);
        elseif iscell(chan_list)
            for i = 1:numel(chan_list)
                if ischar(chan_list{i}) == false
                    if isstring(chan_list{i})
                        chan_list{i} = char(chan_list{i});
                    else
                        help matrix_MED;
                        return;
                   end
                end
            end
        else
            help matrix_MED;
            return;
        end
    end

    % start_time
    if nargin >= 3
        start_time = varargin{1};
        if isscalar(start_time) == false
            if isempty(start_time) == false
                if ischar(start_time) == false
                    if isstring(start_time)  % mex functions only take strings as char arrays
                        start_time = char(start_time);
                    else
                        help matrix_MED;
                        return;
                    end
                end
            end
        end
    else
        start_time = [];
    end

    % end_time
    if nargin >= 4
        end_time = varargin{2};
        if isscalar(end_time) == false
            if isempty(end_time) == false
                if ischar(end_time) == false
                    if isstring(end_time)  % mex functions only take strings as char arrays
                        end_time = char(end_time);
                    else
                        help matrix_MED;
                        return;
                    end
                end
            end
        end
    else
        end_time = [];
    end

    % password
    if nargin >= 5
        password = varargin{3};
        if isempty(password) == false
            if ischar(password) == false
                if isstring(password)  % mex functions only take strings as char arrays
                    password = char(password);
                else
                    help matrix_MED;
                    return;
                end
            end
        end
    else
        password = DEFAULT_PASSWORD;
    end

    % antialias
    if nargin >= 6
        antialias = varargin{4};
        if isstring(antialias)  % mex functions only take strings as char arrays
            antialias = char(antialias);
        end
    else
        antialias = [];
    end

    % detrend
    if nargin >= 7
        detrend = varargin{5};
        if isstring(detrend)  % mex functions only take strings as char arrays
            detrend = char(detrend);
        end
    else
        detrend = [];
    end

    % trace_ranges
    if nargin >= 8
        trace_ranges = varargin{6};
        if isstring(trace_ranges)  % mex functions only take strings as char arrays
            trace_ranges = char(trace_ranges);
        end
    else
        trace_ranges = [];
    end

    % persistence_mode
    if nargin >= 9
        persistence_mode = varargin{7};
        if isempty(persistence_mode) == false
            if ischar(persistence_mode) == false
                if isstring(persistence_mode)
                    persistence_mode = char(persistence_mode);
                elseif isscalar(persistence_mode) == false
                    help matrix_MED;
                    return;
                end
            end
        end
    else
        persistence_mode = [];
    end

    % mex function
    try
        chan_list = get_full_paths(chan_list);
        matrix = matrix_MED_exec(chan_list, n_out_samps, start_time, end_time, password, antialias, detrend, trace_ranges, persistence_mode);
        if islogical(matrix)  % can be true, false, or structure
            if matrix == false
                errordlg('matrix_MED() error', 'Read MED');
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
                matrix = matrix_MED_exec(chan_list, n_out_samps, start_time, end_time, password, antialias, detrend, trace_ranges, persistence_mode);
                if islogical(matrix)  % can be true, false, or structure
                    if matrix == false
                        errordlg('matrix_MED() error', 'Read MED');
                    end
                    return;
                end
            otherwise
                rethrow(ME);
        end
    end
    
end

