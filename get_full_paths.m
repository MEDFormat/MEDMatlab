
function full_paths = get_full_paths(input)
    
    OS = computer;
    DIR_DELIM = '/';
    switch OS
        case 'MACI64'       % MacOS, Intel
        case 'MACA64'       % MacOS, Apple silicon
        case 'GLNXA64'      % Linux
        case 'PCWIN64'      % Windows
            DIR_DELIM = '\';
        otherwise           % Unknown OS
            fprintf(2, 'get_full_paths(): unrecognized operating system\n');
    end

    switch class(input)
        case 'cell'
            full_paths = cellstr(input);
            for i = 1:numel(input)
                p = fileparts(full_paths(i));
                if isempty(p)
                    full_paths(i) = cellstr([pwd DIR_DELIM full_paths{i}]);
                end
            end
        case 'char'
            p = fileparts(input);
            if isempty(p)
                full_paths = [pwd DIR_DELIM input];
            else
                full_paths = input;
            end
        case 'string'
            full_paths = char(input);
            p = fileparts(full_paths);
            if isempty(p)
                full_paths = [pwd DIR_DELIM full_paths];
            end
        otherwise
            fprintf(2, 'get_full_paths(): input class must be ''cell'', ''char,'' or ''string''\n');
            full_paths = [];
    end
    
end
    
