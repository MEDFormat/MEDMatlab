
function [mat, mps] = matrix_MED(varargin)

    %   matrix_MED() requires 1-2 outputs:
    %
    %   [mat, mps] = matrix_MED([mps], [parameter pairs], [numeric]);
    %
    %   mat:                    matrix output data structure
    %   mps:                    matrix parameter structure (both input and output, optional in some conditions)
    %   parameter pairs:        key/value pairs for setting elements of parameter structure
    %   numeric ('numeric'):    return parameter structure with numeric values (default behavior defined by NUMERIC_VALES below)
    %
    %   Example Usage:
    %
    %       %% return matrix with specified parameters 
    %       mat = matrix_MED('Data', {'chan_0001', 'chan_0002'}, 'SampleDimension', 2000, 'Start' -10000001, 'End' -20000000);
    %
    %       %% return matrix and parameter structure with specified parameters
    %       [mat, mps] = matrix_MED('Data', {'chan_0001', 'chan_0002'}, 'SampleDimension', 2000, 'Start', -10000001, 'End', -20000000);
    %       ( equivalently: [mat, mps] = matrix_MED([], 'Data', {'chan_0001', 'chan_0002'}, 'SampleDimension', 2000, 'Start', -10000001, 'End', -20000000); )
    %
    %       %% return matrix using parameters in passed parameter structure
    %       mat = matrix_MED(mps);
    %
    %       %% return matrix using parameters in passed parameter structure, unless explicitly specified in subsequent arguments
    %       mat = matrix_MED(mps, 'Start', -20000001, 'End', -30000000);
    %
    %       %% return empty parameter structure with values set to defaults 
    %       mps = matrix_MED;   ( equivalently:  [~, mps] = matrix_MED(); )
    %
    %
    %   Parameter Structure Elements:
    %   (defaults are the first choice in all applicable fields)
    %
    %   Data:  char array or string, or cell array of char arrays or strings (entries can contain regexp)
    %   SampDimMode:  how sample dimmension is specified; specified as ['count'] or 'rate'
    %   SampDim:  the matrix sample count or sampling frequency
    %   ExtMode:  how matrix slice extents are specified; specified as ['time'] or 'indices'
    %   Start:  slice start limit (time or index, set by ExtMode
    %   End:  slice end limit (time or index, set by ExtMode
    %   TimeMode:  applies only to time limits; specified as ['duration'] or 'end_time'
    %   Password:  data password (or empty if not encrypted)
    %   IdxChan:  applies when limits defined by indices; if empty and necessary, defaults to first channel in set
    %   Filt specified as:
    %       ['antialias']:  lowpass, rolloff begins at 4 samples / cycle (downsampling only)
    %       'none':  no filtering
    %       'lowpass':  cutoff passed in HighCut (Hz)
    %       'highpass':  cutoff passed in LowCut (Hz)
    %       'bandpass':  low cutoff passed in LowCut, high cutoff passed in HighCut (Hz)
    %       'bandstop':  low cutoff passed in LowCut, high cutoff passed in HighCut (Hz)
    %   LowCut: required for highpass, bandpass, & bandstop filters
    %   HighCut: required for lowpass, bandpass, & bandstop filters
    %   Scale:  factor by which to scale output; [1.0] for no scaling
    %   Format (output sample size & type) specified as:  
    %       ['double']:  8-byte signed float
    %       'single':  4-byte signed float
    %       'int32':  4-byte signed integer
    %       'int16':  2-byte signed integer
    %   Padding (pad discontinuities)  specified as:
    %       ['none']:  no padding between discontinuities (contigua specify breaks)
    %       'zero':  zero padding between discontinuities
    %		'nan':  NaN padding between discontinuities
    %   Interp specified as:
    %       ['linear_makima']:  downsample using linear, sample using makima (modified Akima)
    %       'linear_spline':  downsample using linear, sample using spline
    %       'linear':  up & downsample using linear interpolation
    %       'spline':  up & downsample using cubic spline interpolation
    %       'makima':  up & downsample using modified Akima interpolation
    %       'binterp':  downsample using bin interpolation (upsampling not defined for binterp; current version uses spline to upsample)
    %   Binterp (required mode for bin interpolation) specified as:
    %       ['mean']:  use bin mean (fastest without ranges)
    %       'median':  use bin median (slowest, but least sensitive to outliers)
    %       'center':  use bin center (fastest with ranges)
    %       'fast':  use bin mean or center, depending on whether ranges requested
    %   Persist specified as:
    %       ['none']:  single read behavior (default: this is identical to 'read close' below)
    %       'open':  close & free any open session, open new session, & return
    %		'close':  close & free any open session & return
    %       'read':  read current session (& open if none exists), replace existing parameters with non-empty passed parameters
    %       'read_new':  close any open session, open & read new session
    %       'read_close':  read current session (& open if none exists), close session on return
    %   Detrend:  subtract linear regression line (minimum absolute deviation) from each channel; specfied as [false] or true
    %   Ranges:  return minima & maxima traces of samples in each matrix column contributing to output samples; specfied as [false] or true
    %   Extrema:  return minima & maxima of matrix output channels; specfied as [false] or true
    %   Records:  return slice records; specfied as [false] or true
    %   Contigua:  return slice contigua; specfied as [false] or true
    %   ChanNames:  return array of channel names; specfied as [false] or true
    %   ChanFreqs:  return array of input channel sampling frequencies; specfied as [false] or true
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
    %   Matrix Sample Dimension: If define by both sample count & sampling frequency, count will be used
    %
    %   Time Mode: if padding is requested & discontinuit(ies) occur in the slice, limits are converted to absolute time for that read) 
    %
    %
    %   Copyright Dark Horse Neuro, 2021


    % Enter DEFAULT_PASSWORD here for convenience, if doing so does not violate your privacy requirements
    DEFAULT_PASSWORD = 'L2_password';  % put in single quotes to make it char array

    % Set NUMERIC_VALUES to false for readability, or true for efficiency
    NUMERIC_VALUES = false;

    % no arguments - give instructions
    if (nargin == 0 && nargout == 0)
        help matrix_MED;
        return;
    end

    % matrix output is logical false on failure, true or data structure on success
    mat = false;  

    if (nargout <  1 || nargout >  2)
        errordlg('One or two output arguments required', 'Matrix MED');
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
        mps = varargin{1};
        arg_start = 2;
        if (isempty(mps))
            build_struct = true;
        elseif (isstruct(mps) == false)
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
            errordlg('Labelled arguments must be paired with a value', 'Matrix MED');
            return;
        end
    end

    if (build_struct == true)
        mps = struct;
        if (NUMERIC_VALUES == true)
            mps.Data = [];  % required (MED session directory, or channel directories as cell array)
            mps.SampDimMode = 0;  % matrix sample dimension mode: ['count' (0)], or 'rate' (1)
            mps.SampDim = [];  % matrix sample dimension, required (matrix sample count or sampling frequency)
            mps.ExtMode = 0;  % slice extents mode: ['time' (0)] or 'indices' (1)
            mps.Start = [];  % slice start: ['start'], time, or index
            mps.End = [];  % slice end: ['end'], time, or index
            mps.TimeMode = 0;  % applicable when slice extents set by time: ['duration' (0)] or 'end_time' (1)  ('duration' delivers a quantity of sampled time, ignoring discontinuities) 
            mps.Pass = DEFAULT_PASSWORD;  % password
            mps.IdxChan = [];  % name only, no path or extension
            mps.Filt = 0;  % filter type: ['antialias' (0)], 'none' (1), 'lowpass' (2), 'highpass' (3), 'bandpass' (4), or 'bandstop' (5)
            mps.LowCut = [];  % low cutoff filter frequency: required for highpass, bandpass, & bandstop filters
            mps.HighCut = [];  % high cutoff filter frequency: required for lowpass, bandpass, & bandstop filters
            mps.Scale = 1;  % matrix scale factor: [1.0]
            mps.Format = 0;  % matrix sample format: ['double' (0)], 'single' (1), 'int32' (2), or 'int16' (3)
            mps.Padding = 0;  % matrix padding (if discontinuity in slice): ['none' (0)], 'zero' (1), or 'nan' (2)
            mps.Interp = 0;  % interpolation type: ['linear_makima' (0)], 'linear_spline' (1), 'linear' (2), 'spline' (3), 'makima' (4), 'binterp' (5), 
            mps.Binterp = 0;  % bin interpolation mode (required for binterp): ['mean' (0)], 'median' (1), 'center' (2), 'fast' (3)
            mps.Persist = 0;  % perisistence mode: ['none' (0)], 'open' (1), 'close' (2), 'read' (3), 'read_new' (4), 'read_close' (5)
            mps.Detrend = 0;  % detrend channel traces: [false (0)] or true (1)
            mps.Ranges = 0;  % return channnel trace ranges: [false (0)] or true (1)
            mps.Extrema = 0;  % return channnel extrema: [false (0)] or true (1)
            mps.Records = 0;  % return slice records: [false (0)] or true (1)
            mps.Contigua = 0;  % return slice contigua (in matrix frame): [false (0)] or true (1)
            mps.ChanNames = 0;  % return channnel names: [false (0)] or true (1)
            mps.ChanFreqs = 0;  % return channnel sampling frequencies: [false (0)] or true (1)
        else
            mps.Data = [];  % required (MED session directory, or channel directories as cell array)
            mps.SampDimMode = 'count';  % matrix sample dimension mode: ['count'], or 'rate'
            mps.SampDim = [];  % matrix sample dimension, required (matrix sample count or sampling frequency)
            mps.ExtMode = 'time';  % slice extents mode: ['time'] or 'indices'
            mps.Start = [];  % slice start: ['start'], time, or index
            mps.End = [];  % slice end: ['end'], time, or index
            mps.TimeMode = 'duration';  % applicable when slice extents set by time: ['duration'] or 'end_time'  ('duration' delivers a quantity of sampled time, ignoring discontinuities) 
            mps.Pass = DEFAULT_PASSWORD;  % password
            mps.IdxChan = [];  % name only, no path or extension
            mps.Filt = 'antialias';  % filter type: ['antialias'], 'none', 'lowpass', 'highpass)', 'bandpass', or 'bandstop'
            mps.LowCut = [];  % low cutoff filter frequency: required for highpass, bandpass, & bandstop filters
            mps.HighCut = [];  % high cutoff filter frequency: required for lowpass, bandpass, & bandstop filters
            mps.Scale = 1;  % matrix scale factor: [1.0]
            mps.Format = 'double';  % matrix sample format: ['double'], 'single', 'int32', or 'int16'
            mps.Padding = 'none';  % matrix padding (if discontinuity in slice): ['none'], 'zero', or 'nan'
            mps.Interp = 'linear_makima';  % interpolation type: ['linear_makima'], 'linear_spline', 'linear', 'spline', 'makima', 'binterp', 
            mps.Binterp = 'mean';  % bin interpolation mode (required for binterp): ['mean'], 'median', 'center', 'fast'
            mps.Persist = 'none';  % perisistence mode: ['none'], 'open', 'close', 'read', 'read_new', 'read_close'
            mps.Detrend = false;  % detrend channel traces: [false] or true
            mps.Ranges = false;  % return channnel trace ranges: [false] or true
            mps.Extrema = false;  % return channnel extrema: [false] or true
            mps.Records = false;  % return slice records: [[false] or true
            mps.Contigua = false;  % return slice contigua (in matrix frame): [false] or true
            mps.ChanNames = false;  % return channnel names: [false] or true
            mps.ChanFreqs = false;  % return channnel sampling frequencies: [false] or true
        end
    end

    if (nargin_copy == 0)
        if (nargout == 1)
            mat = mps;  % return mps as mat for "mps = matrix_MED" (mps in mat output position)
            mps = [];
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
                errordlg('Parameter labels must be a strings or char arrays', 'Matrix MED');
                return;
            end
        end

        value = varargin{i + 1};
        switch (label)
            case 'Data'
                mps.Data = value;
            case 'SampDimMode'
                mps.SampDimMode = value;
            case 'SampDim'
                mps.SampDim = value;
            case 'ExtMode'
                mps.ExtMode = value;
            case 'Start'
                mps.Start = value;
            case 'End'
                mps.End = value;
            case 'Pass'
                mps.Pass = value;
            case 'IdxChan'
                mps.IdxChan = value;
            case 'Filt'
                mps.Filt = value;
            case 'LowCut'
                mps.LowCut = value;
            case 'HighCut'
                mps.HighCut = value;
            case 'Scale'
                mps.Scale = value;
            case 'Format'
                mps.Format = value;
            case 'Padding'
                mps.Padding = value;
            case 'Interp'
                mps.Interp = value;
            case 'Binterp'
                mps.Binterp = value;
            case 'Persist'
                mps.Persist = value;
            case 'Detrend'
                mps.Detrend = value;
            case 'Ranges'
                mps.Ranges = value;
            case 'Extrema'
                mps.Extrema = value;
            case 'Records'
                mps.Records = value;
            case 'Contigua'
                mps.Contigua = value;
            case 'ChanNames'
                mps.ChanNames = value;
            case 'ChanFreqs'
                mps.ChanFreqs = value;
        end
    end

    % Check structure elements

    % Data
    if (ischar(mps.Data) == false)
        if (isempty(mps.Data))
            errordlg('''Data'' parameter is required', 'Matrix MED');  % empty not OK
            return;
        end
        if (isstring(mps.Data))
            mps.Data = char(mps.Data);
        elseif (iscell(mps.Data))
            for i = 1:numel(mps.Data)
                if (ischar(mps.Data{i}) == false)
                    if (isstring(mps.Data{i}))
                        mps.Data{i} = char(mps.Data{i});
                    else
                        errordlg('''Data'' must be a cell array of strings or char arrays', 'Matrix MED');
                        return;
                   end
                end
            end
        else
            errordlg('''Data'' must be a string or char array', 'Matrix MED');
            return;
        end
    end

    % Get full paths for Data
    mps.Data = get_full_paths(mps.Data);

    % SampDimMode
    mps.SampDimMode = condition_named_string(mps.SampDimMode, 'count', 2);
    if (isnan(mps.SampDimMode))
        errordlg('''SampDimMode'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    if (ischar(mps.SampDimMode))
        switch (mps.SampDimMode)
            case 'count'
            case 'rate'
            otherwise
                errordlg('''SampDimMode'' options: count, rate', 'Matrix MED');
                return;
        end
    end

    % SampDim
    if (isempty(mps.SampDim) == true)
        errordlg('''SampDim'' must be specified', 'Matrix MED');  % empty not OK
        return;
    end
    if (isnumeric(mps.SampDim) == false)
            errordlg('''SampDim'' must be a number', 'Matrix MED');
            return;
    elseif (mps.SampDim <= 0)
        errordlg('''SampDim'' must be positive', 'Matrix MED');
        return;
    end

    % ExtMode
    mps.ExtMode = condition_named_string(mps.ExtMode, 'time', 2);
    if (isnan(mps.TimeMode))
        errordlg('''TimeMode'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    if (ischar(mps.ExtMode))
        switch (mps.ExtMode)
            case 'time'
            case 'indicies'
            otherwise
                errordlg('''ExtMode'' options: time, indices', 'Matrix MED');
                return;
        end
    end
    
    % Start
    mps.Start = condition_named_num(mps.Start);
    if (isnan(mps.Start))
        errordlg('''Start'' must be ''start'', a number, or empty', 'Matrix MED');  % empty OK
        return;
    end
    if (ischar(mps.Start))
        if (strcmp(mps.Start, 'start') == false)
            errordlg('''Start'' string options: start', 'Matrix MED');
            return;
        end
    end

    % End
    mps.End = condition_named_num(mps.End);
    if (isnan(mps.End))
        errordlg('''End'' must be ''end'', a number, or empty', 'Matrix MED');  % empty OK
        return;
    end
    if (ischar(mps.End))
        if (strcmp(mps.End, 'end') == false)
            errordlg('''End'' string options: end', 'Matrix MED');
            return;
        end
    end

    % TimeMode
    mps.TimeMode = condition_named_string(mps.TimeMode, 'duration', 2);
    if (isnan(mps.TimeMode))
        errordlg('''TimeMode'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    if (ischar(mps.TimeMode))
        switch (mps.TimeMode)
            case {'relative', 0}
            case {'absolute', 1}
            otherwise
                errordlg('''TimeMode'' options: duration, end_time', 'Matrix MED');
                return;
        end
    end
    
    % Pass
    if (isempty(mps.Pass))
        mps.Pass = DEFAULT_PASSWORD;
    elseif (ischar(mps.Pass) == false)
        if (isstring(mps.Pass))
            mps.Pass = char(mps.Pass);
        else
            errordlg('''Pass'' must be a string, char array, or empty', 'Matrix MED');  % empty OK
            return;
        end
    end

    % IdxChan
    if (isempty(mps.IdxChan) == false)
        if (ischar(mps.IdxChan) == false)
            if (isstring(mps.IdxChan))
                mps.IdxChan = char(mps.IdxChan);
            else
                errordlg('''IdxChan'' must be a string, char array, or empty', 'Matrix MED');  % empty OK
                return;
            end
        end
    end

    % Filt
    mps.Filt = condition_named_string(mps.Filt, 'antialias', 6);
    if (isnan(mps.Filt))
        errordlg('''Filt'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    switch (mps.Filt)
        case {'antialias', 0}
        case {'none', 1}
        case {'lowpass', 2}
        case {'highpass', 3}
        case {'bandpass', 4}
        case {'bandstop', 5}
        otherwise
            errordlg('''Filt'' options: antialias, none, lowpass, highpass, bandpass, bandstop', 'Matrix MED');
            return;
    end

    % LowCut
    if (isempty(mps.LowCut) == false)  % empty OK
        if (isscalar(mps.LowCut) == false)
            errordlg('''LowCut'' must be a number, or empty', 'Matrix MED');
            return;
        elseif (mps.LowCut <= 0)
            errordlg('''LowCut'' must be positive', 'Matrix MED');
            return;
        end
    end

    % HighCut
    if (isempty(mps.HighCut) == false)  % empty OK
        if (isscalar(mps.HighCut) == false)
            errordlg('''HighCut'' must be a number, or empty', 'Matrix MED');
            return;
        elseif (mps.HighCut <= 0)
            errordlg('''HighCut'' must be positive', 'Matrix MED');
            return;
        end
    end

    % Check Filter
    need_low_cutoff = false;
    need_high_cutoff = false;
    switch (mps.Filt)
        case {'lowpass', 2}
            need_high_cutoff = true;
        case {'highpass', 3}
            need_low_cutoff = true;
        case {'bandpass', 4}
            need_low_cutoff = true;
            need_high_cutoff = true;
        case {'bandstop', 5}
            need_low_cutoff = true;
            need_high_cutoff = true;
    end
    if (need_low_cutoff == true)
        if (isempty(mps.LowCut))
            errordlg('''LowCut'' must be specified for the selected filter type', 'Matrix MED');
            return;
        end
    end
    if (need_high_cutoff == true)
        if (isempty(mps.HighCut))
            errordlg('''HighCut'' must be specified for the selected filter type', 'Matrix MED');
            return;
        end
    end
    
    % Scale
    if (isempty(mps.Scale))  % empty OK
        mps.Scale = 1;
    elseif (isscalar(mps.Scale) == false)
        errordlg('''Scale'' must be a number', 'Matrix MED');
        return;
    end

    % Format
    mps.Format = condition_named_string(mps.Format, 'double', 4);
    if (isnan(mps.Format))
        errordlg('''Format'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    switch (mps.Format)
        case {'double', 0}
        case {'single', 1}
        case {'int32', 2}
        case {'int16', 3}
        otherwise
            errordlg('''Format'' options: double, single, int32, int16', 'Matrix MED');
            return;
    end

    % Padding
    mps.Padding = condition_named_string(mps.Padding, 'none', 3);
    if (isnan(mps.Padding))
        errordlg('''Padding'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    switch (mps.Padding)
        case {'none', 0}
        case {'zero', 1}
        case {'nan', 2}
        otherwise
            errordlg('''Padding'' options: none, zero, nan', 'Matrix MED');
            return;
    end

    % Interp
    mps.Interp = condition_named_string(mps.Interp, 'none', 6);
    if (isnan(mps.Interp))
        errordlg('''Interp'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    switch (mps.Interp)
        case {'linear_makima', 0}
        case {'linear_spline', 1}
        case {'linear', 2}
        case {'spline', 3}
        case {'makima', 4}
        case {'binterp', 5}
        otherwise
            errordlg('''Interp'' options: linear_makima, linear_spline, linear, spline, makima, binterp', 'Matrix MED');
            return;
    end

    % Binterp
    mps.Binterp = condition_named_string(mps.Binterp, 'mean', 4);
    if (isnan(mps.Binterp))
        errordlg('''Binterp'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    switch (mps.Binterp)
        case {'mean', 0}
        case {'median', 1}
        case {'center', 2}
        case {'fast', 3}
        otherwise
            errordlg('''Binterp'' options: mean, median, center, fast', 'Matrix MED');
            return;
    end

    % Check Interpolation
    need_binterp_mode = false;
    if (isscalar(mps.Interp))
        if (mps.Interp == 5)
            need_binterp_mode = true;
        end
    elseif (ischar(mps.Interp))
        if (strcmp(mps.Interp, 'binterp'))
            need_binterp_mode = true;
        end
    end
    if (need_binterp_mode == true)
        if (isempty(mps.Binterp))
            errordlg('''BinInterpolation'' must be specified for the selected interplation type', 'Matrix MED');
            return;
        end
    end
    
    % Persist
    mps.Persist = condition_named_string(mps.Persist, 'none', 6);
    if (isnan(mps.Persist))
        errordlg('''Persist'' must be a string, char array, index, or empty', 'Matrix MED');  % empty OK
        return;
    end
    switch (mps.Persist)
        case {'none', 0}
        case {'open', 1}
        case {'close', 2}
        case {'read', 4}
        case {'read_new', 5}
        case {'read_close', 6}
        otherwise
            if (isscalar(mps.Persist) == true)
                errordlg('''Persist'' numeric options: 0, 1, 2, 4, 5, 6 (note there is no 3)', 'Matrix MED');
            else
                errordlg('''Persist'' options: none, open, close, read, read_new, read_close', 'Matrix MED');
            end
            return;
    end

    % Detrend
    mps.Detrend = condition_logical(mps.Detrend, false);
    if (isnan(mps.Detrend))
            errordlg('''Detrend'' options: true, false', 'Matrix MED');
        return;
    end

    % Ranges
    mps.Ranges = condition_logical(mps.Ranges, false);
    if (isnan(mps.Ranges))
        errordlg('''Ranges'' options: true, false', 'Matrix MED');
        return;
    end

    % Extrema
    mps.Extrema = condition_logical(mps.Extrema, false);
    if (isnan(mps.Extrema))
        errordlg('''Extrema'' options: true, false', 'Matrix MED');
        return;
    end
 
    % Records
    mps.Records = condition_logical(mps.Records, false);
    if (isnan(mps.Records))
        errordlg('''Records'' options: true, false', 'Matrix MED');
        return;
    end

    % Contigua
    mps.Contigua = condition_logical(mps.Contigua, false);
    if (isnan(mps.Contigua))
        errordlg('''Contigua'' options: true, false', 'Matrix MED');
        return;
    end

    % ChanNames
    mps.ChanNames = condition_logical(mps.ChanNames, false);
    if (isnan(mps.ChanNames))
        errordlg('''ChanNames'' options: true, false', 'Matrix MED');
        return;
    end

    % ChanFreqs
    mps.ChanFreqs = condition_logical(mps.ChanFreqs, false);
    if (isnan(mps.ChanFreqs))
        errordlg('''ChanFreqs'' options: true, false', 'Matrix MED');
        return;
    end

    % convert to numerical values where applicable
    if (NUMERIC_VALUES == true)

        % SampleDimensionMode
        if (ischar(mps.SampCnt))
            switch (mps.SampCnt)
                case 'count'
                    mps.SampCnt = 0;
                case 'rate'
                    mps.SampCnt = 1;
            end
        end

        % ExtMode
        if (ischar(mps.ExtMode))
            switch (mps.ExtMode)
                case 'time'
                    mps.ExtMode = 0;
                case 'indices'
                    mps.ExtMode = 1;
            end
        end

        % TimeMode
        if (ischar(mps.TimeMode))
            switch (mps.TimeMode)
                case 'duration'
                    mps.TimeMode = 0;
                case 'end_time'
                    mps.TimeMode = 1;
            end
        end

        % Filter
        if (ischar(mps.Filt))
            switch (mps.Filt)
                case 'antialias'
                    mps.Filt = 0;
                case 'none'
                    mps.Filt = 1;
                case 'lowpass'
                    mps.Filt = 2;
                 case 'highpass'
                    mps.Filt = 3;
                case 'bandpass'
                    mps.Filt = 4;
                case 'bandstop'
                    mps.Filt = 5;
            end
        end

        % Format
        if (ischar(mps.Format))
            switch (mps.Format)
                case 'double'
                    mps.Format = 0;
                case 'single'
                    mps.Format = 1;
                case 'int32'
                    mps.Format = 2;
                 case 'int16'
                    mps.Format = 3;
            end
        end

        % Padding
        if (ischar(mps.Padding))
            switch (mps.Padding)
                case 'none'
                    mps.Padding = 0;
                case 'zero'
                    mps.Padding = 1;
                case 'nan'
                    mps.Padding = 2;
            end
        end

        % Interpolation
        if (ischar(mps.Interp))
            switch (mps.Interp)
                case 'linear_makima'
                    mps.Interp = 0;
                case 'linear_spline'
                    mps.Interp = 1;
                case 'linear'
                    mps.Interp = 2;
                case 'spline'
                    mps.Interp = 3;
                case 'makima'
                    mps.Interp = 4;
                case 'binterp'
                    mps.Interp = 5;
            end
        end

        % BinInterpolation
        if (ischar(mps.Binterp))
            switch (mps.Binterp)
                case 'mean'
                    mps.Binterp = 0;
                case 'median'
                    mps.Binterp = 1;
                case 'center'
                    mps.Binterp = 2;
                case 'fast'
                    mps.Binterp = 3;
            end
        end

        % Persistence (NOTE: there is no 3, these are OR'd values (3 would be open & close)
        if (ischar(mps.Persist))
            switch (mps.Persist)
                case 'none'
                    mps.Persist = 0;
                case 'open'
                    mps.Persist = 1;
                case 'close'
                    mps.Persist = 2;
                case 'read'
                    mps.Persist = 4;
                case 'read_new'
                    mps.Persist = 5;
                case 'read_close'
                    mps.Persist = 6;
            end
        end
    end


    % Call mex function
    try
        mat = matrix_MED_exec(mps);
        if (islogical(mat))  % can be true, false, or structure
            if (mat == false)
                errordlg('Read error', 'Matrix MED');
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
                mat = matrix_MED_exec(mps);
                if (islogical(mat))  % can be true, false, or structure
                    if (mat == false)
                        errordlg('Read error', 'Matrix MED');
                    end
                    return;
                end
            otherwise
                rethrow(ME);
        end
    end
    
end


% returns logical, empty set, or NaN on error
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
