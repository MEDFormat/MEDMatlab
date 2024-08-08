

function matrix_MED_GUI()

    % matrix_MED_GUI()
    % GUI for matrix_MED()
       
    % Copyright Dark Horse Neuro, 2021

    % Defaults (user can change these)
    DEFAULT_DATA_DIRECTORY = pwd;
    DEFAULT_PASSWORD = 'L2_password';  % example_data password == 'L1_password' or 'L2_password'
    DEFAULT_VARIABLE_NAME = 'mat';
    SHOW_MATRIX_MED_COMMAND = true;

    OS = computer;
    DIR_DELIM = '/';
    switch OS
        case 'MACI64'       % MacOS, Intel
            SYS_FONT_SIZE = 12;
        case 'MACA64'       % MacOS, Apple Silicon
            SYS_FONT_SIZE = 12;
        case 'GLNXA64'      % Linux
            SYS_FONT_SIZE = 7;
        case 'PCWIN64'      % Windows
            SYS_FONT_SIZE = 9;
            DIR_DELIM = '\';
        otherwise           % Unknown OS
            SYS_FONT_SIZE = 9;
    end

    [READ_MED_PATH, ~, ~] = fileparts(which('matrix_MED'));
    RESOURCES = [READ_MED_PATH DIR_DELIM 'Resources'];
    if (isempty(which('matrix_MED_exec')))
        addpath(RESOURCES, READ_MED_PATH, '-begin');
        savepath;
        msg = ['Added ', RESOURCES, ' to your search path.' newline];
        beep
        fprintf(2, '%s', msg);  % 2 == stderr, so red in command window
    end

    
    % Constants
    MAX_PASSWORD_LENGTH = 15;

    MIN_F_LEFT = 1;
    MIN_F_BOTTOM = 1;
    MIN_F_WIDTH = 520;
    MIN_F_HEIGHT = 600;
    INITIAL_X_F_OFFSET = 200;
    INITIAL_Y_F_OFFSET = 100;
    AX_OFFSET = 25;
    TEXT_BOX_HEIGHT = 30;
    TEXT_BOX_WIDTH = 150;
    HALF_TEXT_BOX_HEIGHT = TEXT_BOX_HEIGHT / 2;

    % Globals
    dataDirectory = DEFAULT_DATA_DIRECTORY;
    sessionDirectory = '';
    channelList = {};
    mat = [];
    specsChanged = true;
    sessionSelected = false;
    persist_str = 'none';
    persist_num = 0;


    fWidth = MIN_F_WIDTH;
    fHeight = MIN_F_HEIGHT;
    axLeft = 1;
    % axBottom = 1;
    axRight = fWidth - (2 * AX_OFFSET);
    axTop = fHeight - (2 * AX_OFFSET);
    faxWidth = axRight;
    faxHeight = axTop;
    faxLeft = AX_OFFSET + 1;
    faxBottom = AX_OFFSET + 1;
    faxRight = fWidth - AX_OFFSET;
    faxTop = fHeight - AX_OFFSET - HALF_TEXT_BOX_HEIGHT;

    % ------------ Initialize GUI ---------------

    fig = figure('Units','pixels', ...
        'Position', [INITIAL_X_F_OFFSET INITIAL_Y_F_OFFSET fWidth fHeight], ...
        'HandleVisibility','on', ...
        'IntegerHandle','off', ...
        'Renderer','painters', ...
        'Toolbar','none', ...
        'Menubar','none', ...
        'NumberTitle','off', ...
        'Name','Matrix MED GUI', ...
        'Resize', 'on', ...
        'CloseRequestFcn', @figureCloseCallback);

    % Do this seperately to prevent auto-call to resize callback during object creation
    set(fig, 'SizeChangedFcn', @figureResizeCallback);

    % Get figure background color
    panelColor = get(fig, 'Color');

    % Axes
    ax = axes('parent', fig, ...
        'Units', 'pixels', ...
        'Position', [faxLeft faxBottom axRight axTop], ...
        'Xlim', [1 axRight], 'Ylim', [1 axTop], ...
        'Visible', 'off');

    % Session Directory Label (link to top-left, axis coords)
    sessionDirectoryLabel = text('Position', [axLeft axTop], ...
        'String', 'Session Directory:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'left', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Session Directory Textbox (link to top & width, figure coords)
    sessionDirectoryTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', sessionDirectory, ...
        'Position', [faxLeft (faxTop - 25) faxWidth TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @sessionDirectoryTextboxCallback);

    % Channel Listbox Label (link to top-left, axis coords)
    channelListboxLabel = text('Position', [axLeft (axTop - 70)], ...
        'String', 'Channel List:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'left', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');
 
    % Channel Listbox (link to width & height, figure coords)
    channelListbox = uicontrol(fig, ...
        'Style', 'listbox', ...
        'String', {}, ...
        'Position', [faxLeft (faxBottom + 100) (faxWidth - 270) (faxHeight - 180)], ...
        'FontSize', SYS_FONT_SIZE,...
        'FontName', 'FixedWidth', ...
        'FontWeight', 'bold', ...
        'HorizontalAlignment', 'center', ...
        'Min', 1, ...
        'Max', 65536);
    channelListbox.Value = [];

    % Select Channels Pushbutton (link to bottom-left, figure coords)
    selectChannelsPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Select Session/Channels', ...
        'Position', [faxLeft (faxBottom + 60) (faxWidth - 270) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @selectChannelsPushbuttonCallback);

    % Trim to Selected Pushbutton (link to bottom-left, figure coords)
    trimToSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Trim to Selected', ...
        'Position', [faxLeft (faxBottom + 30) (faxWidth - 270) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @trimToSelectedPushbuttonCallback);

    % Remove Selected Pushbutton (link to bottom-left, figure coords)
    removeSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Remove Selected', ...
        'Position', [faxLeft faxBottom (faxWidth - 270) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @removeSelectedPushbuttonCallback);

    % Password Label (link to top-right, axis coords)
    passwordLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 93)], ...
        'String', 'Password:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Password Textbox
    passwordTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 93) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'KeyPressFcn', @password_entry, ...
        'Callback', @passwordTextboxCallback);
    passwordTextbox.UserData.password = DEFAULT_PASSWORD;
    passwordTextbox.String = repmat('*', [1 length(DEFAULT_PASSWORD)]);

    % Times Label (link to top-right, axis coords)
    timesLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 147)], ...
        'String', 'Slice Extents:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Time Mode Radiobuttons (link to top-right, figure coords)
    absoluteRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'absolute', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 147) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE - 2, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @timeModeRadiobuttonsCallback);
    relativeRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'relative', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 147) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE - 2, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Enable', 'on', ...
        'Callback', @timeModeRadiobuttonsCallback);

    % Start Label (link to top-right, axis coords)
    startLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 178)], ...
        'String', 'Start Time:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Start Textbox (link to top-right, figure coords)
    startTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', 'start', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 178) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @limitTextboxCallback);

    % End Label (link to top-right, axis coords)
    endLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 213)], ...
        'String', 'End Time:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % End Textbox (link to top-right, figure coords)
    endTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', 'end', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 213) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @limitTextboxCallback);
   
    % Out Samps Label (link to top-right, axis coords)
    outSampsLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 269)], ...
        'String', ['Output ' newline 'Samples:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Out Samps Textbox (link to top-right, figure coords)
    outSampsTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', '', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 269) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
       
    % Antialias Checkbox (link to top-right, figure coords)
    antialiasCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Antialias', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 320) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
    
    % Detrend Checkbox (link to top-right, figure coords)
    detrendCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Detrend', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 345) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
    
    % Trace Ranges Checkbox (link to top-right, figure coords)
    traceRangesCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Trace Ranges', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 370) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
    
    % Persistence Mode Label (link to top-right, axis coords)
    persistenceModeLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 410)], ...
        'String', ['Persistence ' newline 'Mode:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Persistence Mode Dropdown (link to top-right, figure coords)
    persistenceModeDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'None (default)', 'Open', 'Close', 'Read', 'Read New', 'Read Close'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 418) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

    % Variable Name Label (link to top-right, axis coords)
    variableNameLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 460)], ...
        'String', ['Variable ' newline 'Name:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Variable Name Textbox (link to top-right, figure coords)
    variableNameTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', DEFAULT_VARIABLE_NAME, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 460) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

    % Export to Workspace Pushbutton (link to top-right, figure coords)
    exportToWorkspacePushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Export to Workspace', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 505) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @exportToWorkspacePushbuttonCallback);
    
    % Plot Pushbutton
    plotPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Plot', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 535) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @plotPushbuttonCallback);

    % Set initial focus to Select Channels Pushbutton
    uicontrol(selectChannelsPushbutton);


    % ------------- Callbacks ---------------
    
    function figureResizeCallback(~, ~)
        
        fPos = get(fig, 'Position');
        sizeViolation = false;
        if (fPos(1) < MIN_F_LEFT)
            fPos(1) = MIN_F_LEFT;
            sizeViolation = true;
        end
        if (fPos(2) < MIN_F_BOTTOM)
            fPos(2) = MIN_F_BOTTOM;
            sizeViolation = true;
        end
        if (fPos(3) < MIN_F_WIDTH)
            fPos(3) = MIN_F_WIDTH;
            sizeViolation = true;
        end
        if (fPos(4) < MIN_F_HEIGHT)
            fPos(4) = MIN_F_HEIGHT;
            sizeViolation = true;
        end

        if (sizeViolation == true)
            set(fig, 'Position', fPos);
        end

        fWidth = fPos(3);
        fHeight = fPos(4);
        axRight = fWidth - (2 * AX_OFFSET);
        axTop = fHeight - (2 * AX_OFFSET);
        faxWidth = axRight;
        faxHeight = axTop;
        faxRight = fWidth - AX_OFFSET;
        faxTop = fHeight - AX_OFFSET - HALF_TEXT_BOX_HEIGHT;

        % Axes
        set(ax, 'Position', [faxLeft faxBottom axRight axTop], ...
            'Xlim', [1 axRight], 'Ylim', [1 axTop], ...
            'Visible', 'off');
         
        % Session Directory Label (link to top-left, axis coords)
        set(sessionDirectoryLabel, 'Position', [axLeft axTop]);
    
        % Session Directory Textbox (link to top & width, figure coords)
        set(sessionDirectoryTextbox, 'Position', [faxLeft (faxTop - 25) faxWidth TEXT_BOX_HEIGHT]);
    
        % Channel Listbox Label (link to top-left, axis coords)
        set(channelListboxLabel, 'Position', [axLeft (axTop - 70)]);
     
        % Channel Listbox (link to width & height, figure coords)
        set(channelListbox, 'Position', [faxLeft (faxBottom + 100) (faxWidth - 270) (faxHeight - 180)]);

        % Select Channels Pushbutton (link to bottom-left, figure coords)
        set(selectChannelsPushbutton, 'Position', [faxLeft (faxBottom + 60) (faxWidth - 270) TEXT_BOX_HEIGHT]);
    
        % Trim to Selected Pushbutton (link to bottom-left, figure coords)
        set(trimToSelectedPushbutton, 'Position', [faxLeft (faxBottom + 30) (faxWidth - 270) TEXT_BOX_HEIGHT]);

        % Remove Selected Pushbutton (link to bottom-left, figure coords)
        set(removeSelectedPushbutton, 'Position', [faxLeft faxBottom (faxWidth - 270) TEXT_BOX_HEIGHT]);

        % Password Label (link to top-right, axis coords)
        set(passwordLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 93)]);
    
        % Password Textbox (link to top-right, figure coords)
        set(passwordTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 93) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Times Label (link to top-right, axis coords)
        set(timesLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 147)]);

        % Time Mode Radiobuttons
        set(absoluteRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 147) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(relativeRadiobutton, 'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 147) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);

        % Start Label (link to top-right, axis coords)
        set(startLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 178)]);
    
        % Start Textbox (link to top-right, figure coords)
        set(startTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 178) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
    
        % End Label (link to top-right, axis coords)
        set(endLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 213)]);
    
        % End Textbox (link to top-right, figure coords)
        set(endTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 213) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
    
         % Out Samps Label (link to top-right, axis coords)
        set(outSampsLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 269)]);
    
        % Out Samps Textbox (link to top-right, figure coords)
        set(outSampsTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 269) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
              
        % Antialias Checkbox (link to top-right, figure coords)
        set(antialiasCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 320) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT])   
        
        % Detrend Checkbox (link to top-right, figure coords)
        set(detrendCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 345) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
        
        % Trace Ranges Checkbox (link to top-right, figure coords)
        set(traceRangesCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 370) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);  
        
        % Export to Workspace Pushbutton (link to top-right, figure coords)
        set(exportToWorkspacePushbutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 505) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

         % Persistence Mode Label (link to top-right, axis coords)
        set(persistenceModeLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 410)]);
    
        % Persistence Mode Dropdown (link to top-right, figure coords)
        set(persistenceModeDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 418) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

         % Variable Name Label (link to top-right, axis coords)
        set(variableNameLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 460)]);
    
        % Variable Name Textbox (link to top-right, figure coords)
        set(variableNameTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 460) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
              
        % Plot Pushbutton (link to top-right, figure coords)
        set(plotPushbutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 535) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

    end  % figureResizeCallback()

    
    % Figure Close
    function figureCloseCallback(~, ~)
        delete(fig);
        return;
    end  % figureCloseCallback()


    % Select Channels Pushbutton
    function selectChannelsPushbuttonCallback(~, ~)
        [channelList, sessionDirectory] = directory_chooser({'medd', 'ticd'}, dataDirectory);
        sessionSelected = false;
        if (~isempty(channelList))
            [~, ~, ext] = fileparts(channelList(1));
            if (strcmp(ext, '.medd') == true)
                sessionSelected = true;
                sessionDirectory = [sessionDirectory DIR_DELIM char(channelList(1))];
                dirList = dir([sessionDirectory DIR_DELIM '*.ticd']);
                n_chans = numel(dirList);
                channelList = {};
                for i = 1:n_chans
                    [~, name, ~] = fileparts(dirList(i).name);
                    channelList(i) = cellstr(name);
                end
            else
                n_chans = numel(channelList);
                for i = 1:n_chans
                    [~, name, ~] = fileparts(channelList(i));
                    channelList(i) = cellstr(name);
                end
            end
            % don't include invisible files
            j = 0;
            n_chans = numel(channelList);
            for i = 1:n_chans
                name = char(channelList(i));
                if (name(1) ~= '.' && name(1) ~= '$')  % do not show invisible channels
                        j = j + 1;
                        channelList(j) = cellstr(name);
                end
            end
            channelList = channelList(1:j);
        end
        channelListbox.String = channelList;
        sessionDirectoryTextbox.String = sessionDirectory;
        channelListbox.Value = [];
    end  % selectChannelsPushbuttonCallback()


    % Trim to Selected Pushbutton
    function trimToSelectedPushbuttonCallback(~, ~)
        n_selected = numel(channelListbox.Value);
        for i = 1:n_selected
            channelListbox.String(i) = channelListbox.String(channelListbox.Value(i));
        end
        channelListbox.Value = [];
        channelListbox.String = channelListbox.String(1:n_selected);
    end  % trimToSelectedPushbuttonCallback()


    % Remove Selected Pushbutton
    function removeSelectedPushbuttonCallback(~, ~)
        n_selected = numel(channelListbox.Value);
        n_chans = numel(channelListbox.String);
        i = 1;  % selected values
        j = 0;  % new string idx
        for k = 1:n_chans
            if (i <= n_selected)
                if (channelListbox.Value(i) == k)
                    i = i + 1;
                    continue;
                end
            end
            j = j + 1;
            channelListbox.String(j) = channelListbox.String(k);
        end
        channelListbox.Value = [];
        channelListbox.String = channelListbox.String(1:j);
    end  % removeSelectedPushbuttonCallback()


    % Session Directory Textbox (actually sets data directory & calls selectChannels())
    function sessionDirectoryTextboxCallback(~, ~)
        dataDirectory = sessionDirectoryTextbox.String;
        sessionDirectory = '';
        sessionDirectoryTextbox.String = '';
        channelListbox.Value = [];
        channelListbox.String = {};
        selectChannelsPushbuttonCallback;
    end  % sessionDirectoryTextboxCallback()


    % Time Mode Radiobuttons
    function timeModeRadiobuttonsCallback(src, ~)
        if (src == absoluteRadiobutton)
            if (absoluteRadiobutton.Value == false)
                relativeRadiobutton.Value = true;
            else
                relativeRadiobutton.Value = false;
            end
        else
            if (relativeRadiobutton.Value == false)
                absoluteRadiobutton.Value = true;
            else
                absoluteRadiobutton.Value = false;
            end
        end

        check_limits();
        specsChanged = true;
    end  % timeModeRadiobuttonsCallback()


    function limitTextboxCallback(~, ~)
        check_limits();
        specsChanged = true;
    end  % limitTextboxCallback()

    
    % Password Textbox
    function passwordTextboxCallback(~, ~)
        if (length(passwordTextbox.String) > MAX_PASSWORD_LENGTH)
            errordlg('Password is too long', 'Read MED GUI Error');
            passwordTextbox.String = '';
        end
        specsChanged = true;
    end  % passwordTextboxCallback()


    % Export to Workspace Pushbutton
    function exportToWorkspacePushbuttonCallback(~, ~)
        if specsChanged == true
            success = get_data();
            if success == false
                errordlg('matrix_MED() error', 'Read MED GUI');
                return;
            end
        end
        
        assignin('base', variableNameTextbox.String, mat);
        msgbox('Data Exported');
    end  % exportToWorkspacePushbuttonCallback()


    % Plot Pushbutton
    function plotPushbuttonCallback(~, ~)
        if specsChanged == true
            success = get_data();
            if success == false
                errordlg('matrix_MED() error', 'Read MED GUI');
                return;
            end
        end      

        figure;
        t = linspace(double(mat.start_time), double(mat.end_time), size(mat.samples, 1))';
        plot(t, mat.samples);
    end  % plotPushbuttonCallback()



    % ---------- Support Functions ----------

    function [limit] = convert_limits(string)
        if (strcmp(string, 'start') == true)
            limit = string;
        elseif (strcmp(string, 'end') == true)
            limit = string;
        else
            limit = int64(str2double(string));
        end
    end  % convertLimits()


    function check_limits()
        lim = convert_limits(startTextbox.String);
        if (isnumeric(lim) == true)
            lim = abs(lim);
            if (absoluteRadiobutton.Value == true)
                startTextbox.String = num2str(lim);
            else
                startTextbox.String = num2str(-lim);
            end
        end

        lim = convert_limits(endTextbox.String);
        if (isnumeric(lim) == true)
            lim = abs(lim);
            if (absoluteRadiobutton.Value == true)
                endTextbox.String = num2str(lim);
            else
                endTextbox.String = num2str(-lim);
            end
        end
    end  % check_limits()


    function [success] = get_data()
        set(fig, 'Pointer', 'watch');
        pause(0.1);  % let cursor change (if no pause it gets into mex function before it cann switch it)

        clear mat;
        success = false;
        chan_list = get_chan_list();
        if (numel(chan_list) == 0)
            set(fig, 'Pointer', 'arrow');
            errordlg('No MED session or channels are specified');
            return;
        end
        start_time = convert_limits(startTextbox.String);
        end_time = convert_limits(endTextbox.String);

        n_out_samps = int64(str2double(outSampsTextbox.String));
        if (n_out_samps <= 0)
            set(fig, 'Pointer', 'arrow');
            errordlg('Invalid output sample count');
            return;
        end
        
        if (antialiasCheckbox.Value == true)
            antialias = 'true';
        else
            antialias = 'false';
        end

        if (detrendCheckbox.Value == true)
            detrend = 'true';
        else
            detrend = 'false';
        end

        if (traceRangesCheckbox.Value == true)
            trace_ranges = 'true';
        else
            trace_ranges = 'false';
        end

        persist_val = persistenceModeDropdown.Value;
        switch (persist_val)
            case 2
                persist_str = 'open';
                persist_num = 1;
            case 3
                persist_str = 'close';
                persist_num = 2;
            case 4
                persist_str = 'read';
                persist_num = 4;
            case 5
                persist_str = 'read new';
                persist_num = 5;
            case 6
                persist_str = 'read close';
                persist_num = 6;
            otherwise
                persist_str = 'none';
                persist_num = 0;
        end

        if (SHOW_MATRIX_MED_COMMAND == true)
            show_command(chan_list);
        end

        mat = matrix_MED(chan_list, n_out_samps, start_time, end_time, passwordTextbox.UserData.password, antialias, detrend, trace_ranges, persist_num);

        if (isempty(mat))
            success = false;
        else
            success = true;            
        end
        specsChanged = false;

        set(fig, 'Pointer', 'arrow');
    end  % get_data()


    function show_command(chan_list)   
        
        % build 'file_list' string
        n_chans = numel(chan_list);
        fl = [newline 'file_list = {'];

        for i = 1:(n_chans - 1)
            fl = [fl '''' char(chan_list(i)) ''', '];
        end
        fl = [fl '''' char(chan_list(n_chans)) '''};' newline];

        disp(fl);

        % build command string
        if (strcmp(startTextbox.String, 'start') == true)
            start_time = '''start''';
        else
            start_time = startTextbox.String;
        end
        if (strcmp(endTextbox.String, 'end') == true)
            end_time = '''end''';
        else
            end_time = endTextbox.String;
        end

        n_out_samps = outSampsTextbox.String;

        if (antialiasCheckbox.Value == true)
            antialias = 'true';
        else
            antialias = 'false';
        end 

        if (detrendCheckbox.Value == true)
            detrend = 'true';
        else
            detrend = 'false';
        end 

        if (traceRangesCheckbox.Value == true)
            trace_ranges = 'true';
        else
            trace_ranges = 'false';
        end 

        command = [variableNameTextbox.String ' = matrix_MED(file_list, ' n_out_samps ', ' start_time ', ' end_time  ', ''<password>'', ' antialias ', ' detrend ', ' trace_ranges ', ''' persist_str ''');' newline];
        disp(command);

    end  % show_command()


    function [chan_list] = get_chan_list()
        chan_names = channelListbox.String;
        
        if (sessionSelected == true)
            chan_list{1} = sessionDirectory;
            return;
        end

        n_chans = length(chan_names);
        chan_list = cell(n_chans, 1);
        for i = 1:n_chans
            chan_list{i} = [sessionDirectory DIR_DELIM char(channelListbox.String(i)) '.ticd'];
        end
    end  % get_chan_list()

end  % read_MED_GUI()

