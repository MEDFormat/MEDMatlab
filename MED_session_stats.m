function session = MED_session_stats(file_list, varargin)

    %
    %   MED_session_stats() requires 1 to 5 inputs
    %
    %   Prototype:
    %   session = MED_session_stats(file_list, [password], [return_channels], [return_contigua], [return_records]);
    %
    %   MED_session_stats returns a single Matlab session structure
    %
    %   Arguments in square brackets are optional => '[]' will substitute default values
    %
    %   Input Arguments:
    %   file_list:  string array, strings can contain regexp
    %   password:  if empty/absent, proceeds as if unencrypted (but, will return an error if necessary data are encrypted)
    %   return_channels:  if empty/absent, defaults to false (options: true, false)
    %   return_contigua:  if empty/absent, defaults to false (options: true, false)
    %   return_records:  if empty/absent, defaults to false (options: true, false)
    %         
    %   Copyright Dark Horse Neuro, 2023


    % Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = [];  % put in single quotes to make it char array

    session = false;  % failure return value

    if nargin == 0 || nargin > 5 || nargout ~=  1
        help MED_session_stats;
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
                        help MED_session_stats;
                        return;
                   end
                end
            end
        else
            help MED_session_stats;
            return;
        end
    end

    % password
    if nargin > 1
        password = varargin{1};
        if isempty(password) == false
            if ischar(password) == false
                if isstring(password)  % mex functions only take strings as char arrays
                    password = char(password);
                else
                    help MED_session_stats;
                    return;
                end
            end
        end
    else
        password = DEFAULT_PASSWORD;
    end

    % return_channels
    if nargin > 2
        return_channels = varargin{2};
        if isempty(return_channels) == false
            if isstring(return_channels)  % mex functions only take strings as char arrays
                return_channels = char(return_channels);
            end
        end
    else
        return_channels = [];
    end

    % return_contigua
    if nargin > 3
        return_contigua = varargin{3};
        if isempty(return_contigua) == false
            if isstring(return_contigua)  % mex functions only take strings as char arrays
                return_contigua = char(return_contigua);
            end
        end
    else
        return_contigua = [];
    end

    % return_records
    if nargin > 4
        return_records = varargin{4};
        if isempty(return_records) == false
            if isstring(return_records)  % mex functions only take strings as char arrays
                return_records = char(return_records);
            end
        end
    else
        return_records = [];
    end

    % mex function
    try
        file_list = get_full_paths(file_list);
        session = MED_session_stats_exec(file_list, password, return_channels, return_contigua, return_records);
        if islogical(session)  % false or structure - don't need to check if true
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
                file_list = get_full_paths(file_list);
                session = MED_session_stats_exec(file_list, password, return_channels, return_contigua, return_records);
                if islogical(session)  % false or structure - don't need to check if true
                    errordlg('MED_session_stats() error', 'Read MED');
                    return;
                end
            otherwise
                rethrow(ME);
        end
    end
    
end

