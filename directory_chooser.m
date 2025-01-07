

function [directories, parent_directory] = directory_chooser(varargin)

    %   [directories, parent_directory] = directory_chooser([filters], [start_directory], [stop_filters], [show_hidden])
    %
    %   e.g. 
    %	filters = {'medd', 'ticd'};
    %   start_dir = '/Volumes/my_disk/my_data';
    %   stop_filters = {'ticd'};  %% nothing below these directory types => a double click on these == select, not open
    %	[files/directories, parent_directory] = directory_chooser(filters, start_dir, stop_filters);
       
    %   Copyright Dark Horse Neuro, 2020

    DEFAULT_START_DIRECTORY = pwd;

    if (nargin > 3)
        help directory_chooser;
        return;
    end
    
    filters = [];
    startDirectory = [];
    stop_filters = [];
    show_hidden = false;
    
    if (nargin >= 1)
        filters = varargin{1};
    end
    if (nargin >= 2)
        startDirectory = varargin{2};  % string or char array
    end
    if (nargin >= 3)
        stop_filters = varargin{3};  % cell array of strings or char arrays
    end
    
    if (nargin == 4)
        show_hidden = varargin{4};  % boolean
    end

    OS = computer;
    WINDOWS = false;
    DIR_DELIM = '/';
    switch OS
        case 'MACI64'       % MacOS, Intel
            SYS_FONT_SIZE = 13;
        case 'MACA64'       % MacOS, Apple Silicon
            SYS_FONT_SIZE = 13;
        case 'GLNXA64'      % Linux
            SYS_FONT_SIZE = 9;
        case 'PCWIN64'      % Windows
            SYS_FONT_SIZE = 10;
            WINDOWS = true;
            DIR_DELIM = '\';
            known_servers_path = which('directory_chooser');
            len = length(known_servers_path) - 19;  % 'directory_chooser.m'
            known_servers_path = known_servers_path(1:len);
            known_servers_path = [known_servers_path '.dc_known_servers'];
        otherwise           % Unknown OS
            SYS_FONT_SIZE = 9;
    end

    directories = {};
    parent_directory = '';

    % Globals
    parentDirectoryString = '';
    parentDirectoryList = {};
    parentDirectoryLevels = 0;
    directoryList = {};


    % ------------ GUI Layout ---------------

    % Figure
    fig = figure('Units','pixels', ...
        'Position',[200 175 298 695], ...
        'HandleVisibility','on', ...
        'IntegerHandle','off', ...
        'Toolbar','none', ...
        'Menubar','none', ...
        'NumberTitle','off', ...
        'Name','Directory Chooser', ...
        'Resize', 'off', ...
        'CloseRequestFcn', @figureCloseCallback);
    
    panelColor = get(fig, 'Color');

    % Axes
    axes('parent', fig, ...
        'Units', 'pixels', ...
        'Position', [1 1 298 695], ...
        'Xlim', [1 700], 'Ylim', [1 700], ...
        'Visible', 'off');
    
    % Parent Directory Label
    text('Position', [57 677], ...
        'String', 'Parent Directory:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'left', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Parent Directory Popup
    parentDirectoryPopup = uicontrol(fig,...
        'Style', 'popupmenu', ...
        'String', {}, ...
        'Position', [20 635 262 25], ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'FontWeight', 'bold', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @parentDirectoryPopupCallback);
 
    % Parent Directory Contents Label
    text('Position', [57 602], ...
        'String', 'Contents:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'left', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Directory Listbox
    directoryListbox = uicontrol(fig, ...
        'Style', 'listbox', ...
        'String', {}, ...
        'Position', [25 75 250 510], ...
        'FontSize', SYS_FONT_SIZE,...
        'FontName', 'FixedWidth', ...
        'FontWeight', 'bold', ...
        'HorizontalAlignment', 'center', ...
        'Min', 1, ...
        'Max', 65536, ...
        'Callback', @directoryListboxCallback);
    
    % Select Pushbutton
    uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Select', ...
        'Position', [60 25 180 30], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @selectPushbuttonCallback);
    
    % Add Server Pushbutton
    addServerPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Add Server', ...
        'Position', [182 617 100 20], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @addServerPushbuttonCallback);
    
    % Add Server Pushbutton
    clearServersPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Clear Servers', ...
        'Position', [55 617 123 20], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @clearServersPushbuttonCallback);
    
    
    % ------------- Callbacks ---------------
    
    % Figure Close
    function figureCloseCallback(~, ~)
        delete(fig);
    end

    % Parent Directory Popup
    function parentDirectoryPopupCallback(~, ~)
        parentDirectoryLevels = parentDirectoryPopup.Value;
        parentDirectoryList = parentDirectoryList(1:parentDirectoryLevels);
        parentDirectoryPopup.String = parentDirectoryList;
        updateParentDirectoryString();
        updateParentDirectoryList();
        updateDirectoryList();
    end

    function directoryListboxCallback(~, ~)
        selected = directoryListbox.String(directoryListbox.Value);
        n_selected = length(selected);
        
        % double click (no double click on multiple selection for now)
        if (n_selected == 1) %#ok<ISCL>
            if (strcmp(get(gcf, 'selectiontype'), 'open') == true)
                % up one level
                if (strcmp(selected, '..') == true)
                    parentDirectoryLevels = parentDirectoryLevels - 1;
                    parentDirectoryList = parentDirectoryList(1:parentDirectoryLevels);
                    parentDirectoryPopup.String = parentDirectoryList;
                    parentDirectoryPopup.Value = parentDirectoryLevels;
                    updateParentDirectoryString();
                    updateDirectoryList();
                % down one level
                else
                    passed_stop_filters = checkStopFilters();
                    if (passed_stop_filters == true)
                        if (parentDirectoryLevels == 1)
                            parentDirectoryString = '';
                        end
                        parentDirectoryString = [parentDirectoryString DIR_DELIM char(selected)];
                        updateParentDirectoryList();
                        updateDirectoryList();
                    else  % treat double click as selection
                        selectPushbuttonCallback();
                    end
                end
            end
        else  % multiple selection
            checkSelected();
        end
    end

    function selectPushbuttonCallback(~, ~)
        % check if selected is '..'
        if (directoryListbox.Value(1) == 1)
            selected = directoryListbox.String(directoryListbox.Value);
            if (strcmp(selected, '..') == 1)
                parentDirectoryLevels = parentDirectoryLevels - 1;
                parentDirectoryList = parentDirectoryList(1:parentDirectoryLevels);
                parentDirectoryPopup.String = parentDirectoryList;
                parentDirectoryPopup.Value = parentDirectoryLevels;
                updateParentDirectoryString();
                updateDirectoryList();
                return;
            end
        end
        % check that selection passes filters
        checkSelected();
        if (directoryListbox.Value(1) ~= 1)
            directories = directoryListbox.String(directoryListbox.Value);
            if (WINDOWS == true)
                parent_directory = parentDirectoryString(2:end);
            else
                parent_directory = parentDirectoryString;
            end
            figureCloseCallback();
        end
    end

    function addServerPushbuttonCallback(~, ~)
        % dialog
        add_server_dlg = dialog('Position', [400 400 300 150], ...
            'Name', 'Add Server');
        uicontrol('Parent', add_server_dlg, ...
            'Style', 'text', ...
            'Position', [10 88 70 30], ...
            'HorizontalAlignment', 'right', ...
            'String','Server Name:');
        dlg_txtbx = uicontrol('Parent', add_server_dlg, ...
            'Style', 'edit', ...
            'Position', [87 98 201 25], ...
            'HorizontalAlignment', 'left');
        dlg_add_btn = uicontrol('Parent', add_server_dlg, ...
            'Style', 'pushbutton', ...
            'Position', [95 20 70 25], ...
            'String', 'Add', ...
            'Callback', @dlgButtonsCallback);
        uicontrol('Parent', add_server_dlg, ...
            'Style', 'pushbutton', ...
            'Position', [15 20 70 25], ...
            'String', 'Clear', ...
            'Callback', @dlgButtonsCallback);

        uicontrol(dlg_txtbx)
        uiwait(add_server_dlg);
        if (isvalid(add_server_dlg))  % user hit close button
            close(add_server_dlg);
        end

        % Dialog Buttons (nested)
        function dlgButtonsCallback(src, ~)
            if (src == dlg_add_btn)
                % add to known servers file
                d = winGetKnownServers();
                num_servers = numel(d);
                for i = 1:num_servers
                    d(i).name = d(i).name(3:end);  % remove '\\'
                end
                
                if (isempty(dlg_txtbx.String) == false)
                    % add server
                    num_servers = num_servers + 1;
                    d(num_servers).isdir = true;
                    % get rid of "\\" if entered
                    name = dlg_txtbx.String;
                    if (name(1) == '\' && name(2) == '\')
                        name = name(3:end);
                    end
                    d(num_servers).name = name;

                    % sort
                    serverList = cell(num_servers, 1);
                    for i = 1:num_servers
                        serverList{i} = d(i).name;
                    end
                    case_insens_sort = sortrows([serverList' upper(serverList')], 2);
                    for i = 1:num_servers
                        d(i).name = char(case_insens_sort(i, 1));
                    end
                    clear case_insens_sort serverList

                    % write out
                    fp = fopen(known_servers_path, 'w');
                    for i = 1:num_servers
                        fwrite(fp, [d(i).name newline]);
                    end
                    fclose(fp);

                    updateDirectoryList();
                end
            end
            close(add_server_dlg);
        end

    end

    function clearServersPushbuttonCallback(~, ~)
        system(['del /a ' known_servers_path]);
        updateDirectoryList();
    end

    % ----------- Initializations -----------
    
    % Parent Directory String
    if (isempty(startDirectory)) 
        parentDirectoryString = [DIR_DELIM DEFAULT_START_DIRECTORY];
    else
        if (WINDOWS == true)
            if (startDirectory(1) == DIR_DELIM)
                parentDirectoryString = [DIR_DELIM 'C:' startDirectory];
            elseif (startDirectory(1) >= 'A' && startDirectory(1) <= 'Z')
                parentDirectoryString = [DIR_DELIM startDirectory];
            else
                parentDirectoryString = [DIR_DELIM pwd DIR_DELIM startDirectory];
            end
        else  % MacOS or Linux
            if (startDirectory(1) == DIR_DELIM)  
                parentDirectoryString = startDirectory;
            else
                parentDirectoryString = [pwd DIR_DELIM startDirectory];
            end
        end
    end
    
    % Parent Directory Popup
    updateParentDirectoryList();
    
    % Directory Contents Listbox
    updateDirectoryList();
    
    % User starting point
    uicontrol(directoryListbox); 


    % ---------- Support Functions ----------
    
    function updateParentDirectoryString()
        if (parentDirectoryLevels == 1)
                parentDirectoryString = DIR_DELIM;
        else
            parentDirectoryString = '';
            for i = 2:parentDirectoryLevels
                parentDirectoryString = [parentDirectoryString DIR_DELIM char(parentDirectoryList(i))]; %#ok<AGROW>
            end
        end
    end

    function updateParentDirectoryList()
        parentDirectoryLevels = 0;
        d_list = {};
        d_name = parentDirectoryString;
        len = length(d_name);
        
        while (len > 1)
            i = len;
            while (d_name(i) ~= DIR_DELIM)
                i = i - 1;
            end
            if (i ~= len)
                name = d_name((i + 1):length(d_name));
                parentDirectoryLevels = parentDirectoryLevels + 1;
                d_list{parentDirectoryLevels} = name; %#ok<AGROW>
            end
            len = i - 1;
            d_name = d_name(1:len);
        end
        parentDirectoryLevels = parentDirectoryLevels + 1;
        d_list{parentDirectoryLevels} = DIR_DELIM;
        parentDirectoryList = flip(d_list);
        
        parentDirectoryPopup.String = parentDirectoryList;
        parentDirectoryPopup.Value = parentDirectoryLevels;
    end

    function updateDirectoryList()
        directoryList = {};
        if (parentDirectoryLevels > 1)
            directoryList{1} = '..';
            i = 1;
        else
            i = 0;
        end
        if (WINDOWS == true)
            if (parentDirectoryLevels == 1)
                % make add server button visible
                set(addServerPushbutton, 'Visible', 'on');
                set(clearServersPushbutton, 'Visible', 'on');
                d = winGetDriveList();
            else
                set(addServerPushbutton, 'Visible', 'off');
                set(clearServersPushbutton, 'Visible', 'off');
                if (parentDirectoryLevels == 2)
                    if parentDirectoryString(3) == ':'  % lettered drive
                        d = dir([parentDirectoryString(2:end) DIR_DELIM]);
                    else
                        d = dir(['\\' parentDirectoryString(2:end) DIR_DELIM]);  % server
                    end
                else
                    d = dir(parentDirectoryString(2:end));
                end
            end
        else
            d = dir(parentDirectoryString);
        end
        len = length(d);
        for j = 1:len
            if (d(j).isdir == true)
                if ((d(j).name(1) == '.' || d(j).name(1) == '$') && show_hidden == false)
                    continue;
                end
                i = i + 1;
                directoryList{i} = d(j).name;
            end
        end
        
        if (parentDirectoryLevels > 1)
            case_insens_sort = sortrows([directoryList' upper(directoryList')], 2);
            directoryList = case_insens_sort(:, 1);
        end
        directoryListbox.String = directoryList;
        directoryListbox.Value = 1;
    end

    function n_selected = checkSelected()        
        n_filters = numel(filters);
        values = directoryListbox.Value;
        n_selected = length(values);
        if (isempty(filters))
            return;
        end
 
        for i = 1:n_selected
            selected_str = char(directoryListbox.String(values(i)));
            passed_filter = 0;
            for j = 1:n_filters
                filter = char(filters(j));
                filt_len = length(filter) - 1;
                if (length(selected_str) > filt_len)
                    if (strcmp(selected_str((end - filt_len):end), filter) == 1)
                        passed_filter = 1;
                        break;
                    end
                end
            end
            if (passed_filter == 0)
                values(i) = 0;
            end
        end
        values = values(values ~= 0);
        if isempty(values)
            values = 1;
        end
        directoryListbox.Value = values;
        n_selected = length(values);
    end

    function passed_filters = checkStopFilters()
        passed_filters = true;
        if (isempty(stop_filters))
            return;
        end
        
        n_filters = numel(stop_filters);
        values = directoryListbox.Value;
        n_selected = length(values);
        if (n_selected ~= 1)
            return;
        end
        
        selected_str = char(directoryListbox.String(values(1)));
        for i = 1:n_filters
            filter = char(stop_filters(i));
            filt_len = length(filter) - 1;
            if (length(selected_str) > filt_len)
                if (strcmp(selected_str((end - filt_len):end), filter) == 1)
                    passed_filters = false;
                    break;
                end
            end
        end
    end

    function d = winGetDriveList()
        d = winGetKnownServers();
        [r, t] = system('wmic logicaldisk get name');
        if (r ~= 0)
            return;
        end
        offset = 2;
        n_drives = numel(d);
        len = length(t);
        while (offset <= len)
            if (t(offset) == ':')
                n_drives = n_drives + 1;
                d(n_drives).isdir = true;
                d(n_drives).name = [t(offset - 1) ':'];
            end
            offset = offset + 1;
        end
    end

    function d = winGetKnownServers()

        d = [];
        fp = fopen(known_servers_path, 'r');
        if (fp == -1)
            return;
        end
        buf = fread(fp, 'char=>char');
        fclose(fp);

        len = length(buf);
        known_server_count = 0;
        i = 1;
        while (i <= len)
            % skip empty newlines or carriage returns
            j = i;
            while (j <= len)
                if (buf(j) ~= 10) && (buf(j) ~= 13)  
                    break;
                end
                j = j + 1;
            end    
            i = j;   % start of server name
            while (j <= len)  % skip empty newlines or carriage returns
                j = j + 1;
                if (buf(j) == 10 || buf(j) == 13)  % newline or carriage return
                    known_server_count = known_server_count + 1;
                    server_name = buf(i:(j - 1))';
                    d(known_server_count).isdir = true; %#ok<AGROW>
                    d(known_server_count).name = ['\\' server_name]; %#ok<AGROW>
                    break;
                end
            end
            i = j + 1;
        end
    end

    % wait for figure close
    uiwait(fig);
    delete(fig);

end  % End Directory Chooser
