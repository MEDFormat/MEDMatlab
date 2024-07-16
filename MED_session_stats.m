function session = MED_session_stats(file_list, varargin)

    %
    %   MED_session_stats() requires 1 to 5 inputs
    %
    %   Prototype:
    %   session = MED_session_stats(file_list, [return_channels], [return_contigua], [return_records], [password]);
    %
    %   MED_session_stats returns a single Matlab session structure
    %
    %   Arguments in square brackets are optional => '[]' will substitute default values
    %
    %   Input Arguments:
    %   file_list:  string array, strings can contain regexp
    %   return_channels:  if empty/absent, defaults to false (options: true, false)
    %   return_contigua:  if empty/absent, defaults to false (options: true, false)
    %   return_records:  if empty/absent, defaults to false (options: true, false)
    %   password:  if empty/absent, proceeds as if unencrypted (but, will return an error if necessary data are encrypted)
    %         
    %   Copyright Dark Horse Neuro, 2023


    session = [];
    if nargin == 0 || nargin > 5 || nargout ~=  1
        help MED_session_stats;
        return;
    end
    
    %   Enter DEFAULT_PASSWORD here for convenience, if doing does not violate your privacy requirements
    DEFAULT_PASSWORD = [];

    if nargin > 1
        return_channels = varargin{1};
    else
        return_channels = [];
    end

    if nargin > 2
        return_contigua = varargin{2};
    else
        return_contigua = [];
    end

    if nargin > 3
        return_records = varargin{3};
    else
        return_records = [];
    end

    if nargin > 4
        password = varargin{4};
    else
        password = DEFAULT_PASSWORD;
    end
    if isstring(password)
        password = char(password);
    end

    file_list = get_full_paths(file_list);
    
    try
        session = MED_session_stats_exec(file_list, return_channels, return_contigua, return_records, password);
        if (isempty(session))
            errordlg('MED_session_stats() error', 'Read MED');
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
                [MED_STATS_PATH, ~, ~] = fileparts(which('MED_session_stats'));
                RESOURCES = [MED_STATS_PATH DIR_DELIM 'Resources'];
                addpath(RESOURCES, MED_STATS_PATH, '-begin');
                savepath;
                msg = ['Added ', RESOURCES, ' to your search path.' newline];
                beep
                fprintf(2, '%s', msg);  % 2 == stderr, so red in command window
                session = MED_session_stats_exec(file_list, return_channels, return_contigua, return_records, password);
            otherwise
                rethrow(ME);
        end
    end
    
end

