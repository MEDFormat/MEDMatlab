
function [slice, rps] = read_MED(varargin)

    %   read_MED() requires 1-2 outputs:
    %
    %   [slice, rps] = read_MED([rps], [parameter pairs], [numeric]);
    %
    %   slice:                  output data structure
    %   rps:                    read_MED parameter structure (both input and output, optional in some conditions)
    %   parameter pairs:        key/value pairs for setting elements of parameter structure
    %   numeric ('numeric'):    return parameter structure with numeric values (default behavior defined by NUMERIC_VALES below)
    %
    %   Example Usage:
    %
    %       %% return slice with specified parameters 
    %       slice = read_MED('Data', {'chan_0001', 'chan_0002'}, 'SampleDimension', 2000, 'Start' -10000001, 'End' -20000000);
    %
    %       %% return slice and parameter structure with specified parameters
    %       [slice, rps] = read_MED('Data', {'chan_0001', 'chan_0002'}, 'SampleDimension', 2000, 'Start', -10000001, 'End', -20000000);
    %       ( equivalently: [slice, rps] = read_MED([], 'Data', {'chan_0001', 'chan_0002'}, 'SampleDimension', 2000, 'Start', -10000001, 'End', -20000000); )
    %
    %       %% return slice using parameters in passed parameter structure
    %       slice = read_MED(rps);
    %
    %       %% return matrix using parameters in passed parameter structure, unless explicitly specified in subsequent arguments
    %       slice = read_MED(rps, 'Start', -20000001, 'End', -30000000);
    %
    %       %% return empty parameter structure with values set to defaults 
    %       rps = read_MED;   ( equivalently:  [~, rps] = read_MED(); )
    %
    %
    %   Parameter Structure Elements:
    %   (defaults are the first choice in all applicable fields)
    %
    %   Data:  char array or string, or cell array of char arrays or strings (entries can contain regexp)
    %   ExtMode:  how matrix slice extents are specified; specified as ['time'] or 'indices'
    %   Start:  slice start limit (time or index, set by ExtMode
    %   End:  slice end limit (time or index, set by ExtMode
    %   Pass:  data password (or empty if not encrypted)
    %   IdxChan:  applies when limits defined by indices; if empty and necessary, defaults to first channel in set
    %   Format (output sample size & type) specified as:  
    %       ['double']:  8-byte signed float
    %       'single':  4-byte signed float
    %       'int32':  4-byte signed integer
    %       'int16':  2-byte signed integer
    %   Filt specified as:
    %       ['none']:  no filtering
    %       'lowpass':  cutoff passed in HighCutOff (Hz)
    %       'highpass':  cutoff passed in LowCutOff (Hz)
    %       'bandpass':  low cutoff passed in LowCut, high cutoff passed in HighCut (Hz)
    %       'bandstop':  low cutoff passed in LowCut, high cutoff passed in HighCut (Hz)
    %   LowCut: required for highpass, bandpass, & bandstop filters
    %   HighCut: required for lowpass, bandpass, & bandstop filters
    %   Persist specified as:
	%       ['none']:  single read behavior (identical to 'read close' below)
	%       'open':  close any open session, open new session, & return
	%       'close':  close & free any open session & return
	%       'read':  read current session (& open if not)
	%       'read_new':  close any open session, open & read new session
	%       'read_close':  read current session (& open if not), close on return
    %   Metadata:  return slice session & channel metadata; specified as [true] or false
    %   Records:  return slice records; specified as [true] or false
    %   Contigua:  return slice contigua; specified as [true] or false
    %
    %
    %   NOTES:
    %
    %   Slice Extents:
    %       a) times are generally preferable to indices as they are independent of sampling frequencies
    %       b) times are natively in offset µUTC (oUTC), but unoffset (true) times may be used
    %       c) negative times are considered to be relative to the session start
    %       d) if indices are used, index numbering begins at 1, per Matlab convention
    %       e) if the slice is defined by both time & indicies, time is used
    %
    %
    %   Copyright Dark Horse Neuro, 2021


    % Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = [];  % put in single quotes to make it char array

    % Set NUMERIC_VALUES to false for readability, or true for efficiency
    NUMERIC_VALUES = false;

    % no arguments - give instructions
    if (nargin == 0 && nargout == 0)
        help read_MED;
        return;
    end

    % slice output is logical false on failure, true or data structure on success
    slice = false;  % failure return value

    if (nargout <  1 || nargout >  2)
        errordlg('One or two output arguments required', 'Read MED');
        return;
    end

    % check for 'numeric' argument
    nargin_copy = nargin;  % if modify nargin, treated as local variable
    if (nargin_copy)
        numeric_str = varargin{nargin_copy};
        if (isstring(numeric_str))
            numeric_str = char(numeric_str);
        end
        if (ischar(numeric_str))
            if (strcmp(numeric_str, 'numeric'))
                NUMERIC_VALUES = true;  % override default
                nargin_copy = nargin_copy - 1;
            end
        end
    end

    % check if need to build structure
    build_struct = false;
    if (nargin_copy == 0)
        build_struct = true;
    else
        rps = varargin{1};
        arg_start = 2;
        if (isempty(rps))
            build_struct = true;
        elseif (isstruct(rps) == false)
            build_struct = true;
            arg_start = 1;
        end
        unpaired_arg = false;
        if (arg_start == 1)
            if (mod(nargin_copy, 2) == 1)
                unpaired_arg = true;
            end
        else
            if (mod(nargin_copy, 2) == 0)
                unpaired_arg = true;
            end
        end
        if (unpaired_arg == true)
            errordlg('Labelled arguments must be paired with a value', 'Read MED');
            return;
        end
    end

    if (build_struct == true)
        rps = struct;
        if (NUMERIC_VALUES == true)
            rps.Data = [];  % required (MED session directory, or channel directories as cell array)
            rps.ExtMode = 0;  % slice extents mode: ['time' (0)] or 'indices' (1)
            rps.Start = [];  % slice start: ['start'], time, or index
            rps.End = [];  % slice end: ['end'], time, or index
            rps.Pass = DEFAULT_PASSWORD;  % password
            rps.IdxChan = [];  % name only, no path or extension
            rps.Format = 0;  % matrix sample format: ['double' (0)], 'single' (1), 'int32' (2), or 'int16' (3)
            rps.Filt = 0;  % filter type: ['none' (0)], 'lowpass' (1), 'highpass' (2), 'bandpass' (3), or 'bandstop' (4)
            rps.LowCut = [];  % low cutoff filter frequency: required for highpass, bandpass, & bandstop filters
            rps.HighCut = [];  % high cutoff filter frequency: required for lowpass, bandpass, & bandstop filters
            rps.Persist = 0;  % perisistence mode: ['none' (0)], 'open' (1), 'close' (2), 'read' (3), 'read_new' (4), 'read_close' (5)
            rps.Metadata = 1;  % return slice session & channel metadata: [true (1)] or false (0)
            rps.Records = 1;  % return slice records: [true (1)] or false (0)
            rps.Contigua = 1;  % return slice contigua: [true (1)] or false (0)
        else
            rps.Data = [];  % required (MED session directory, or channel directories as cell array)
            rps.ExtMode = 'time';  % slice extents mode: ['time'] or 'indices'
            rps.Start = [];  % slice start: ['start'], time, or index
            rps.End = [];  % slice end: ['end'], time, or index
            rps.Pass = DEFAULT_PASSWORD;  % password
            rps.IdxChan = [];  % name only, no path or extension
            rps.Format = 'double';  % matrix sample format: ['double'], 'single', 'int32', or 'int16'
            rps.Filt = 'none';  % filter type: ['none'], 'lowpass', 'highpass', 'bandpass', or 'bandstop'
            rps.LowCut = [];  % low cutoff filter frequency: required for highpass, bandpass, & bandstop filters
            rps.HighCut = [];  % high cutoff filter frequency: required for lowpass, bandpass, & bandstop filters
            rps.Persist = 'none';  % perisistence mode: ['none'], 'open', 'close', 'read', 'read_new', 'read_close'
            rps.Metadata = true;  % return slice session & channel metadata: [true] or false
            rps.Records = true;  % return slice records: [true] or false
            rps.Contigua = true;  % return slice contigua: [true] or false
        end
    end

    if (nargin_copy == 0)
        if (nargout == 1)
            slice = rps;  % return rps as slice for "rps = read_MED" (rps in slice output position)
            rps = [];
        end
        return;
    end    

    % get named arguments
    for i = arg_start:2:nargin_copy
        label = varargin{i};
        if (ischar(label) == false)
            if (isstring(label) == true)
                label = char(label);
            else
                errordlg('Parameter labels must be a strings or char arrays', 'Read MED');
                return;
            end
        end

        value = varargin{i + 1};
        switch (label)
            case 'Data'
                rps.Data = value;
            case 'ExtMode'
                rps.ExtMode = value;
            case 'Start'
                rps.Start = value;
            case 'End'
                rps.End = value;
            case 'Pass'
                rps.Pass = value;
            case 'IdxChan'
                rps.IdxChan = value;
            case 'Format'
                rps.Format = value;
            case 'Filt'
                rps.Filt = value;
            case 'LowCut'
                rps.LowCut = value;
            case 'HighCut'
                rps.HighCut = value;
            case 'Persist'
                rps.Persist = value;
            case 'Metadata'
                rps.Metadata = value;
            case 'Records'
                rps.Records = value;
            case 'Contigua'
                rps.Contigua = value;
        end
    end

    % Check structure elements

    % Data
    if (ischar(rps.Data) == false)
        if (isempty(rps.Data))
            errordlg('''Data'' parameter is required', 'Read MED');  % empty not OK
            return;
        end
        if (isstring(rps.Data))
            rps.Data = char(rps.Data);
        elseif (iscell(rps.Data))
            for i = 1:numel(rps.Data)
                if (ischar(rps.Data{i}) == false)
                    if (isstring(rps.Data{i}))
                        rps.Data{i} = char(rps.Data{i});
                    else
                        errordlg('''Data'' must be a cell array of strings or char arrays', 'Read MED');
                        return;
                   end
                end
            end
        else
            errordlg('''Data'' must be a string or char array', 'Read MED');
            return;
        end
    end

    % Get full paths for Data
    rps.Data = get_full_paths(rps.Data);

    % ExtMode
    rps.ExtMode = condition_named_string(rps.ExtMode, 'time', 2);
    if (isnan(rps.ExtMode))
        errordlg('''ExtMode'' must be a string, char array, index, or empty', 'Read MED');  % empty OK
        return;
    end
    if (ischar(rps.ExtMode))
        switch (rps.ExtMode)
            case 'time'
            case 'indices'
            otherwise
                errordlg('''ExtMode'' options: time, indices', 'Read MED');
                return;
        end
    end
    
    % Start
    rps.Start = condition_named_num(rps.Start);
    if (isnan(rps.Start))
        errordlg('''Start'' must be ''start'', a number, or empty', 'Read MED');  % empty OK
        return;
    end
    if (ischar(rps.Start))
        if (strcmp(rps.Start, 'start') == false)
            errordlg('''Start'' string options: start', 'Read MED');
            return;
        end
    end

    % End
    rps.End = condition_named_num(rps.End);
    if (isnan(rps.End))
        errordlg('''End'' must be ''end'', a number, or empty', 'Read MED');  % empty OK
        return;
    end
    if (ischar(rps.End))
        if (strcmp(rps.End, 'end') == false)
            errordlg('''End'' string options: end', 'Read MED');
            return;
        end
    end

    % Pass
    if (isempty(rps.Pass))
        rps.Pass = DEFAULT_PASSWORD;
    elseif (ischar(rps.Pass) == false)
        if (isstring(rps.Pass))
            rps.Pass = char(rps.Pass);
        else
            errordlg('''Pass'' must be a string, char array, or empty', 'Read MED');  % empty OK
            return;
        end
    end

    % IdxChan
    if (isempty(rps.IdxChan) == false)
        if (ischar(rps.IdxChan) == false)
            if (isstring(rps.IdxChan))
                rps.IdxChan = char(rps.IdxChan);
            else
                errordlg('''IdxChan'' must be a string, char array, or empty', 'Read MED');  % empty OK
                return;
            end
        end
    end

    % Format
    rps.Format = condition_named_string(rps.Format, 'double', 4);
    if (isnan(rps.Format))
        errordlg('''Format'' must be a string, char array, index, or empty', 'Read MED');  % empty OK
        return;
    end
    if (ischar(rps.Format))
        switch (rps.Format)
            case {'double', 0}
            case {'single', 1}
            case {'int32', 2}
            case {'int16', 3}
            otherwise
                errordlg('''Format'' options: double, single, int32, int16', 'Read MED');
                return;
        end
    end

    % Filt
    rps.Filt = condition_named_string(rps.Filt, 'none', 5);
    if (isnan(rps.Persist))
        errordlg('''Persist'' must be a string, char array, index, or empty', 'Read MED');  % empty OK
        return;
    end
    switch (rps.Filt)
        case {'none', 0}
        case {'lowpass', 1}
        case {'highpass', 2}
        case {'bandpass', 3}
        case {'bandstop', 4}
        otherwise
            errordlg('''Filt'' options: none, lowpass, highpass, bandpass, bandstop', 'Read MED');
            return;
    end
       
    % LowCut
    if (isempty(rps.LowCut) == false)  % empty OK
        if (isscalar(rps.LowCut) == false)
            errordlg('''LowCut'' must be a number, or empty', 'Read MED');
            return;
        elseif (rps.LowCut <= 0)
            errordlg('''LowCut'' must be positive', 'Read MED');
            return;
        end
    end

    % HighCut
    if (isempty(rps.HighCut) == false)  % empty OK
        if (isscalar(rps.HighCut) == false)
            errordlg('''HighCut'' must be a number, or empty', 'Read MED');
            return;
        elseif (rps.HighCut <= 0)
            errordlg('''HighCut'' must be positive', 'Read MED');
            return;
        end
    end

    % Check Filter
    need_low_cutoff = false;
    need_high_cutoff = false;
    if (isscalar(rps.Filt))
        switch (rps.Filt)
            case 1
                need_high_cutoff = true;
            case 2
                need_low_cutoff = true;
            case 3
                need_low_cutoff = true;
                need_high_cutoff = true;
            case 4
                need_low_cutoff = true;
                need_high_cutoff = true;
        end
    elseif (ischar(rps.Filt))
        switch (rps.Filt)
            case 'lowpass'
                need_high_cutoff = true;
            case 'highpass'
                need_low_cutoff = true;
            case 'bandpass'
                need_low_cutoff = true;
                need_high_cutoff = true;
            case 'bandstop'
                need_low_cutoff = true;
                need_high_cutoff = true;
        end
    end
    if (need_low_cutoff == true)
        if (isempty(rps.LowCut))
            errordlg('''LowCut'' must be specified for the selected filter type', 'Read MED');
            return;
        end
    end
    if (need_high_cutoff == true)
        if (isempty(rps.HighCut))
            errordlg('''HighCut'' must be specified for the selected filter type', 'Read MED');
            return;
        end
    end

    % Persist
    rps.Persist = condition_named_string(rps.Persist, 'none', 6);
    if (isnan(rps.Persist))
        errordlg('''Persist'' must be a string, char array, index, or empty', 'Read MED');  % empty OK
        return;
    end
    switch (rps.Persist)
        case {'none', 0}
        case {'open', 1}
        case {'close', 2}
        case {'read', 4}
        case {'read_new', 5}
        case {'read_close', 6}
        otherwise
            if (isscalar(rps.Persist) == true)
                errordlg('''Persist'' numeric options: 0, 1, 2, 4, 5, 6 (note there is no 3)', 'Matrix MED');
            else
                errordlg('''Persist'' options: none, open, close, read, read_new, read_close', 'Matrix MED');
            end
            return;
    end

    % Metadata
    rps.Metadata = condition_logical(rps.Metadata, false);
    if (isnan(rps.Metadata))
        errordlg('''Metadata'' options: true, false', 'Read MED');
        return;
    end

    % Records
    rps.Records = condition_logical(rps.Records, false);
    if (isnan(rps.Records))
        errordlg('''Records'' options: true, false', 'Read MED');
        return;
    end

    % Contigua
    rps.Contigua = condition_logical(rps.Contigua, false);
    if (isnan(rps.Contigua))
        errordlg('''Contigua'' options: true, false', 'Read MED');
        return;
    end

    % convert to numerical values where applicable
    if (NUMERIC_VALUES == true)

        % ExtMode
        if (ischar(rps.ExtMode))
            switch (rps.ExtMode)
                case 'time'
                    rps.ExtMode = 0;
                case 'indices'
                    rps.ExtMode = 1;
            end
        end

        % Format
        if (ischar(rps.Format))
            switch (rps.Format)
                case 'double'
                    rps.Format = 0;
                case 'single'
                    rps.Format = 1;
                case 'int32'
                    rps.Format = 2;
                 case 'int16'
                    rps.Format = 3;
            end
        end

        % Filt
        if (ischar(rps.Filt))
            switch (rps.Filt)
                case 'none'
                    rps.Filt = 0;
                case 'lowpass'
                    rps.Filt = 1;
                 case 'highpass'
                    rps.Filt = 2;
                case 'bandpass'
                    rps.Filt = 3;
                case 'bandstop'
                    rps.Filt = 4;
            end
        end

        % Persistence (NOTE: there is no 3, these are OR'd values (3 would be open & close)
        if (ischar(rps.Persist))
            switch (rps.Persist)
                case 'none'
                    rps.Persist = 0;
                case 'open'
                    rps.Persist = 1;
                case 'close'
                    rps.Persist = 2;
                case 'read'
                    rps.Persist = 4;
                case 'read_new'
                    rps.Persist = 5;
                case 'read_close'
                    rps.Persist = 6;
            end
        end
    end


    % Call mex function
    try
        slice = read_MED_exec(rps);
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
                [MATRIX_MED_PATH, ~, ~] = fileparts(which('matrix_MED'));
                RESOURCES = [MATRIX_MED_PATH DIR_DELIM 'Resources'];
                addpath(RESOURCES, MATRIX_MED_PATH, '-begin');
                savepath;
                msg = ['Added ', RESOURCES, ' to your search path.' newline];
                beep
                fprintf(2, '%s', msg);  % 2 == stderr, so red in command window
                slice = read_MED_exec(rps);
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


function e = condition_logical(e, default_val)
    if (isempty(e) == true)
        e = default_val;
    else
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
            switch (e)
                case 0
                    e = false;
                case 1
                    e = true;
                otherwise
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
                if (isstring(e))
                    e = char(e);
                else
                    e = NaN;
                    return;
                end
            end
        end
    end
end


% returns char array, integer, or NaN on error
function e = condition_named_string(e, default_str, n_values)
    if (isempty(e) == true)
        e = default_str;
    elseif (ischar(e) == false)
        if (isstring(e))
            e = char(e);
        elseif (isnumeric(e))
            e = round(e);
            if (e < 0 || e >= n_values)
                e = NaN;
            end
        else
            e = NaN;
        end
    end
end
