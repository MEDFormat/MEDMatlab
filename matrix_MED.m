function matrix = matrix_MED(chan_list, start_time, end_time, n_out_samps, varargin)

    %
    %   matrix_MED() requires 4 to 9 inputs: 
    %   1)  chan_list
    %   2)  start_time
    %   3)  end_time
    %   4)  n_out_samps
    %   5)  [password]
    %   6)  [antialias ([true] / false)]
    %   7)  [detrend (true / [false])]
    %   8)  [trace_ranges (true / [false])] 
    %   9)  [persistence_mode (string or code: options shown below)] 
    %
    %   Prototype:
    %   matrix_struct = matrix_MED(chan_list, start_time, end_time, n_out_samps, [password], [antialias], [detrend], [trace_ranges], [persistence_mode]);
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
    %   Options:
    %       'none' (0):	 single read behavior (default: this is identical to 'read close' below)
    %		'close' (1):  close & free any open session & return
    %       'open' (2):	 close & free any open session, open new session, & return
    %       'read' (4)	read current session (& open if none exists), replace existing parameters with non-empty passed parameters
    %       'read close' (5):  close any open session, open & read new session, close session & return
    %       'read new' (6):	 close any open session, open & read new session
    %
    %   In MED, times are preferable to indices as they are independent of sampling frequencies
    %       a) times are natively in offset µUTC (oUTC), but unoffset times may be used
    %       b) negatives times are considered to be relative to the session start
    %
    %   Copyright Dark Horse Neuro, 2021

    if nargin < 4 || nargin > 9 || nargout ~=  1
        help matrix_MED;
        return;
    end
   
    %   Enter DEFAULT_PASSWORD here for convenience, if doing does not violate your privacy requirements
    DEFAULT_PASSWORD = 'L2_password';  % default for example datasets
    if nargin >= 5
        password = varargin{1};
    else
        password = DEFAULT_PASSWORD;
    end
    if isstring(password)
        password = char(password);
    end

    if nargin >= 6
        antialias = varargin{2};
        if isstring(antialias)  % mex functions take strings as char arrays
            antialias = char(antialias);
        end
    else
        antialias = [];
    end

    if nargin >= 7
        detrend = varargin{3};
        if isstring(detrend)  % mex functions take strings as char arrays
            detrend = char(detrend);
        end
    else
        detrend = [];
    end

    if nargin >= 8
        trace_ranges = varargin{4};
        if isstring(trace_ranges)  % mex functions take strings as char arrays
            trace_ranges = char(trace_ranges);
        end
    else
        trace_ranges = [];
    end

    if nargin >= 9
        persistence_mode = varargin{5};
        if isstring(persistence_mode)
            persistence_mode = char(persistence_mode);
        end
    else
        persistence_mode = [];
    end

    try
        chan_list = get_full_paths(chan_list);
        matrix = matrix_MED_exec(chan_list, start_time, end_time, n_out_samps, password, antialias, detrend, trace_ranges, persistence_mode);
        if isa(matrix, 'logical')
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
                matrix = matrix_MED_exec(chan_list, start_time, end_time, n_out_samps, password, antialias, detrend, trace_ranges, persistence_mode);
                if isa(matrix, 'logical')
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

