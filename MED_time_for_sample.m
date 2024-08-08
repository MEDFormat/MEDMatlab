
function times = MED_time_for_sample(sample_numbers, MED_directory, varargin)

    %
    %   MED_time_for_sample() requires 2 to 3 inputs
    %
    %   Prototype:
    %   time(s) = MED_time_for_sample(sample_number(s), MED_directory, [password]);
    %
    %   MED_time_for_sample() returns time(s) for a specified sample number(s)
    %
    %   Arguments in square brackets are optional => '[]' will substitute default values
    %
    %   Input Arguments:
    %   sample_number(s):  scalar or array of scalars specifying sample number(s) ('start' & 'end' are also accepted)
    %   sample_number(s) are in Matlab index schema: (1:n) rather than 0:(n-1)
    %   MED_directory:  string specifying channel or session
    %   password:  if empty/absent, proceeds as if unencrypted (but, may error out)
    %         
    %   Copyright Dark Horse Neuro, 2024


    %   Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = [];  % put in single quotes to make it char array

    times = false;  % failure return value

    if nargin < 2 || nargin > 3 || nargout ~=  1
        help MED_time_for_sample;
        return;
    end

    % sample_numbers
    if isscalar(sample_numbers) == false
        if numel(sample_numbers) > 1
            if isscalar(sample_numbers(1)) == false
                help MED_time_for_sample;
                return;
            end      
        elseif ischar(sample_numbers) == false  % allow for 'start' & 'end'
           if isstring(sample_numbers)
                sample_numbers = char(sample_numbers);
            end
        else
            help MED_time_for_sample;
            return;
        end
    end

    % MED_directory
    if ischar(MED_directory) == false
        if isstring(MED_directory)
            MED_directory = char(MED_directory);
        else
            help MED_time_for_sample;
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
        times = MED_time_for_sample_exec(sample_numbers, MED_directory, password);
        if islogical(times)  % false or value - don't need to check if true
            errordlg('MED_time_for_sample() error', 'Read MED');
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
                times = MED_time_for_sample_exec(sample_numbers, MED_directory, password);
                if islogical(times)  % false or value - don't need to check if true
                    errordlg('MED_time_for_sample() error', 'Read MED');
                    return;
                end
            otherwise
                rethrow(ME);
        end
    end
    
end

