
function full_paths = get_full_paths(input)
    
    OS = computer;
    DIR_DELIM = '/';
    windows = false;
    switch OS
        case 'MACI64'       % MacOS, Intel
        case 'MACA64'       % MacOS, Apple silicon
        case 'GLNXA64'      % Linux
        case 'PCWIN64'      % Windows
            windows = true;
            DIR_DELIM = '\';
        otherwise           % Unknown OS
            fprintf(2, 'get_full_paths(): unrecognized operating system\n');
    end

    if isstring(input)
        input = char(input);
    end

    cwd = pwd;
    switch class(input)
        case 'cell'
            full_paths = input;
            for i = 1:numel(input)
                tmp_str = full_paths{i};  % use braces to get contents of cell
                if isstring(tmp_str)
                    tmp_str = char(tmp_str);
                elseif ischar(tmp_str) == false
                    fprintf(2, 'get_full_paths(): cell elements must be class ''char,'' or ''string''\n');
                    full_paths = [];
                    return;
                end     
                p = fileparts(tmp_str);
                if isempty(p)
                    full_paths{i} = [cwd DIR_DELIM tmp_str];
                elseif tmp_str(1) == DIR_DELIM
                        full_paths{i} = tmp_str;  % use braces to set contents of cell
                elseif windows == true
                    if tmp_str(2) == ':' || tmp_str(2) == DIR_DELIM
                        full_paths{i} = tmp_str;  % use braces to set contents of cell
                    else
                        full_paths{i} = [cwd DIR_DELIM tmp_str];  % use braces to set contents of cell
                    end
                else
                    full_paths{i} = [cwd DIR_DELIM tmp_str];  % use braces to set contents of cell
                end
            end
        case 'char'
            p = fileparts(input);
            if isempty(p)
                full_paths = [cwd DIR_DELIM input];
            elseif input(1) == DIR_DELIM
                    full_paths = input;
            elseif windows == true
                if input(2) == ':' || input(2) == DIR_DELIM
                    full_paths = input;
                else
                    full_paths = [cwd DIR_DELIM input];
                end
            else
                full_paths = [cwd DIR_DELIM input];
            end
        otherwise
            fprintf(2, 'get_full_paths(): input class must be ''cell'', ''char,'' or ''string''\n');
            full_paths = [];
    end
    
end
    
