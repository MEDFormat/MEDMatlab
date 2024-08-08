
function sample_numbers = MED_sample_for_time(times, MED_directory, varargin)

    %
    %   MED_sample_for_time() requires 2 to 3 inputs
    %
    %   Prototype:
    %   sample_number(s) = MED_sample_for_time(time(s), MED_directory, [password]);
    %
    %   MED_sample_for_time() returns sample number(s) for specified time(s),
    %   in Matlab index schema: (1:n) rather than 0:(n-1)
    %
    %   Arguments in square brackets are optional => '[]' will substitute default values
    %
    %   Input Arguments:
    %   time(s):  scalar or array of scalars specifying time(s) ('start' & 'end' are also accepted)
    %   MED_directory:  string specifying channel or session
    %   password:  if empty/absent, proceeds as if unencrypted (but, may error out)
    %         
    %   Copyright Dark Horse Neuro, 2024


    %   Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = [];  % put in single quotes to make it char array

    sample_numbers = false;  % failure return value

    if nargin < 2 || nargin > 3 || nargout ~=  1
        help MED_sample_for_time;
        return;
    end

    % times
    if isscalar(times) == false
        if numel(times) > 1
            if isscalar(times(1)) == false
                help MED_sample_for_time;
                return;
            end      
        elseif ischar(times) == false  % allow for 'start' & 'end'
            if isstring(times)
                times = char(times);
            end
        else
            help MED_sample_for_time;
            return;
        end
    end

    % MED_directory
    if ischar(MED_directory) == false
        if isstring(MED_directory)
            MED_directory = char(MED_directory);
        else
            help MED_sample_for_time;
            return;
        end
    end

    % password
    if nargin == 3
        password = varargin{1};
        if isempty(password) == false
            if ischar(password) == false
                if isstring(password)  % mex functions only take strings as char arrays
                    password = char(password);
                else
                    help MED_sample_for_time;
                    return;
                end
            end
        end
    else
        password = DEFAULT_PASSWORD;
    end
  
    % mex function
    try
        MED_directory = get_full_paths(MED_directory);
        sample_numbers = MED_sample_for_time_exec(times, MED_directory, password);
        if islogical(sample_numbers)  % false or value - don't need to check if true
            errordlg('MED_sample_for_time() error', 'Read MED');
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
                MED_directory = get_full_paths(MED_directory);
                sample_numbers = MED_sample_for_time_exec(times, MED_directory, password);
                if islogical(sample_numbers)  % false or value - don't need to check if true
                    errordlg('MED_sample_for_time() error', 'Read MED');
                    return;
                end
            otherwise
                rethrow(ME);
        end
    end
    
end

