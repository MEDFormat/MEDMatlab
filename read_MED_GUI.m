

function read_MED_GUI(varargin)

    % read_MED_GUI()
    %
    % GUI for read_MED(rps)
    % 
    % rps: a read_MED parameter structure; if passed fields will be set to the values in rps on open
    %
    % Copyright Dark Horse Neuro, 2021

    % Defaults (user can change these)
    DEFAULT_DATA_DIRECTORY = pwd;
    DEFAULT_PASSWORD = 'L2_password';  % example_data password == 'L1_password' or 'L2_password'
    DEFAULT_SLICE_NAME = 'slice';
    DEFAULT_PARAMETERS_NAME = 'rps';
    SHOW_READ_MED_COMMAND = true;

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

    [READ_MED_PATH, ~, ~] = fileparts(which('read_MED'));
    RESOURCES = [READ_MED_PATH DIR_DELIM 'Resources'];
    if (isempty(which('read_MED_exec')))
        addpath(RESOURCES, READ_MED_PATH, '-begin');
        savepath;
        msg = ['Added ', RESOURCES, ' to your search path.' newline];
        beep
        fprintf(2, '%s', msg);  % 2 == stderr, so red in command window
    end

    % default read_MED paramters
    if (nargin)
        if (nargin > 1)
            errordlg('read_MED_GUI can only accept one input', 'Read MED GUI');
            return;
        elseif (isstruct(varargin{1}) == false)
            errordlg('The input argument must a read_MED parameter structure', 'Read MED GUI');
            return;
        end
        in_rps = varargin{1};
        set_in_values = true;
    else
        in_rps = read_MED;
        set_in_values = false;
    end
    out_rps = in_rps;  % out_rps is modified & compared to in_rps to build command
    
    % Constants
    MAX_PASSWORD_LENGTH = 16;

    MIN_F_LEFT = 1;
    MIN_F_BOTTOM = 1;
    MIN_F_WIDTH = 600;
    MIN_F_HEIGHT = 820;
    INITIAL_X_F_OFFSET = 200;
    INITIAL_Y_F_OFFSET = 100;
    AX_OFFSET = 25;
    TEXT_BOX_HEIGHT = 30;
    TEXT_BOX_WIDTH = 170;
    HALF_TEXT_BOX_HEIGHT = TEXT_BOX_HEIGHT / 2;

    % Globals
    dataDirectory = DEFAULT_DATA_DIRECTORY;
    sessionDirectory = '';
    channelList = {};
    slice = [];
    specsChanged = true;
    sessionSelected = false;

    % figure & axes constants
    fWidth = MIN_F_WIDTH;
    fHeight = MIN_F_HEIGHT;
    axLeft = 1;
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
        'Toolbar','none', ...
        'Menubar','none', ...
        'NumberTitle','off', ...
        'Name','Read MED GUI', ...
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
        'Position', [faxLeft (faxBottom + 100) (faxWidth - 290) (faxHeight - 180)], ...
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
        'Position', [faxLeft (faxBottom + 60) (faxWidth - 290) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @selectChannelsPushbuttonCallback);

    % Trim to Selected Pushbutton (link to bottom-left, figure coords)
    trimToSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Trim to Selected', ...
        'Position', [faxLeft (faxBottom + 30) (faxWidth - 290) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @trimToSelectedPushbuttonCallback);

    % Remove Selected Pushbutton (link to bottom-left, figure coords)
    removeSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Remove Selected', ...
        'Position', [faxLeft faxBottom (faxWidth - 290) TEXT_BOX_HEIGHT], ...
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

    % Extents Label (link to top-right, axis coords)
    extentsLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 160)], ...
        'String', ['Slice ' newline 'Extents:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Extent Mode Radiobuttons (link to top-right, figure coords)
    timesRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'times', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 153) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @extentModeRadiobuttonsCallback);
    indicesRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'indices', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 153) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @extentModeRadiobuttonsCallback);

    % Time Mode Radiobuttons (link to top-right, figure coords)
    absoluteRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'absolute', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 182) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @timeModeRadiobuttonsCallback);
    relativeRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'relative', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 182) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Enable', 'on', ...
        'Callback', @timeModeRadiobuttonsCallback);

    % Start Label (link to top-right, axis coords)
    startLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 218)], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 218) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @limitTextboxCallback);

    % End Label (link to top-right, axis coords)
    endLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 253)], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 253) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @limitTextboxCallback);
 
    % Reference Channel Label (link to top-right, axis coords)
    referenceChannelLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 288)], ...
        'String', ['Indices ' newline 'Channel:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth', ...
        'Visible', 'off');
   
    % Reference Channel Textbox (link to top-right, figure coords)
    referenceChannelTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', '', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 288) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @specsChangedCallback);

    % Use Selected (channel to use as index reference) Pushbutton (link to top-right, figure coords)
    useSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Use Selected', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 320) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @useSelectedPushbuttonCallback);
    
    % Units Label (link to top-right, axis coords)
    unitsLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 288)], ...
        'String', 'Units:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Units Radiobuttons (link to top-right, figure coords)
    secondsRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'seconds', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 288) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @unitsModeRadiobuttonsCallback);
    usecsRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'µsecs', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 288) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Enable', 'on', ...
        'Callback', @unitsModeRadiobuttonsCallback);

    % Filter Label (link to top-right, figure coords)
    filterLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 361)], ...
        'String', 'Filter:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Filter Dropdown (link to top-right, figure coords)
    filterDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'none', 'lowpass', 'highpass', 'bandpass', 'bandstop'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 368) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @filterDropdownCallback);

    % Low Cutoff Label (link to top-right, axis coords)
    lowCutoffLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 393)], ...
        'String', 'Low Cutoff:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth', ...
        'Visible', 'off');

    % Low Cutoff Textbox (link to top-right, figure coords)
    lowCutoffTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 393) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @specsChangedCallback);

    % High Cutoff Label (link to top-right, axis coords)
    highCutoffLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 428)], ...
        'String', 'High Cutoff:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth', ...
        'Visible', 'off');

    % High Cutoff Textbox (link to top-right, figure coords)
    highCutoffTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 428) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @specsChangedCallback);

    % Format Label (link to top-right, figure coords)
    formatLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 471)], ...
        'String', 'Format:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Format Dropdown (link to top-right, figure coords)
    formatDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'double', 'single', 'int32', 'int16'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 478) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    
    % Metadata Checkbox (link to top-right, figure coords)
    metadataCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Metadata', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 513) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);
    
    % Records Checkbox (link to top-right, figure coords)
    recordsCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Records', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 538) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);
    
    % Contigua Checkbox (link to top-right, figure coords)
    contiguaCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Contigua', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 563) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);
    
    % Persistence Mode Label (link to top-right, figure coords)
    persistenceModeLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 601)], ...
        'String', 'Persistence:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Persistence Dropdown (link to top-right, figure coords)
    persistenceDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'none', 'open', 'close', 'read', 'read_new', 'read_close'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 608) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % slice Name Label (link to top-right, axis coords)
    sliceNameLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 640)], ...
        'String', 'Slice Name:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Slice Name Textbox (link to top-right, figure coords)
    sliceNameTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', DEFAULT_SLICE_NAME, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 640) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % slice Name Label (link to top-right, axis coords)
    parametersNameLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 680)], ...
        'String', ['Parameters ' newline 'Name:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Slice Name Textbox (link to top-right, figure coords)
    parametersNameTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', DEFAULT_PARAMETERS_NAME, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 680) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % Plot Pushbutton (link to bottom-right, figure coords)
    plotPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Plot', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxBottom + 30) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @plotPushbuttonCallback);

    % Export to Workspace Pushbutton (link to bottom-right, figure coords)
    exportToWorkspacePushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Export to Workspace', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) faxBottom TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @exportToWorkspacePushbuttonCallback);

    % set initial values from passed rps
    if (set_in_values)
        set_initial_field_values()
    end
    
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
        set(channelListbox, 'Position', [faxLeft (faxBottom + 100) (faxWidth - 290) (faxHeight - 180)]);

        % Select Channels Pushbutton (link to bottom-left, figure coords)
        set(selectChannelsPushbutton, 'Position', [faxLeft (faxBottom + 60) (faxWidth - 290) TEXT_BOX_HEIGHT]);
    
        % Trim to Selected Pushbutton (link to bottom-left, figure coords)
        set(trimToSelectedPushbutton, 'Position', [faxLeft (faxBottom + 30) (faxWidth - 290) TEXT_BOX_HEIGHT]);

        % Remove Selected Pushbutton (link to bottom-left, figure coords)
        set(removeSelectedPushbutton, 'Position', [faxLeft faxBottom (faxWidth - 290) TEXT_BOX_HEIGHT]);

        % Password Label (link to top-right, axis coords)
        set(passwordLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 93)]);
    
        % Password Textbox (link to top-right, figure coords)
        set(passwordTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 93) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Extents Label (link to top-right, axis coords)
        set(extentsLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 160)]);

        % Extent Mode Radiobuttons
        set(timesRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 153) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(indicesRadiobutton,'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 153) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        
        % Time Mode Radiobuttons
        set(absoluteRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 182) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(relativeRadiobutton, 'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 182) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);

        % Start Label (link to top-right, axis coords)
        set(startLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 218)]);
    
        % Start Textbox (link to top-right, figure coords)
        set(startTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 218) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
    
        % End Label (link to top-right, axis coords)
        set(endLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 253)]);
    
        % End Textbox (link to top-right, figure coords)
        set(endTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 253) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
         
        % Reference Channel Label (link to top-right, axis coords)
        set(referenceChannelLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 288)]);
    
        % Reference Channel Textbox (link to top-right, figure coords)
        set(referenceChannelTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 288) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Use Selected Pushbutton (link to top-right, figure coords)
        set(useSelectedPushbutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 320) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
        
        % Units Label (link to top-right, axis coords)
        set(unitsLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 288)]);
    
        % Units Radiobuttons
        set(secondsRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 288) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(usecsRadiobutton, 'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 288) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);

        % Filter Label (link to top-right, figure coords)
        set(filterLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 361)]); 
        
        % Filter Dropdown (link to top-right, figure coords)
        set(filterDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 368) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]); 
        
        % Low Cutoff Label (link to top-right, figure coords)
        set(lowCutoffLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 393)]); 
        
        % Low Cutoff Textbox (link to top-right, figure coords)
        set(lowCutoffTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 393) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]); 
        
        % High Cutoff Label (link to top-right, figure coords)
        set(highCutoffLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 428)]); 
        
        % High Cutoff Textbox (link to top-right, figure coords)
        set(highCutoffTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 428) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]); 
        
        % Format Label (link to top-right, figure coords)
        set(formatLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 471)]); 
        
        % Format Dropdown (link to top-right, figure coords)
        set(formatDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 478) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]); 
               
        % Metadata Checkbox (link to top-right, figure coords)
        set(metadataCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 513) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
      
        % Records Checkbox (link to top-right, figure coords)
        set(recordsCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 538) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]); 
      
        % Contigua Checkbox (link to top-right, figure coords)
        set(contiguaCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 563) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
 
        % Persistence Mode Label (link to top-right, figure coords)
        set(persistenceModeLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 601)]);

        % Persistence Mode Dropdown (link to top-right, figure coords)
        set(persistenceDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 608) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Slice Name Label (link to top-right, figure coords)
        set(sliceNameLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 640)]);

        % Slice Name Textbox (link to top-right, figure coords)
        set(sliceNameTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 640) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Parameters Name Label (link to top-right, figure coords)
        set(parametersNameLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 680)]);

        % Parameters Name Textbox (link to top-right, figure coords)
        set(parametersNameTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 680) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Plot Pushbutton (link to bottom-right, figure coords)
        set(plotPushbutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxBottom + 30) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Export to Workspace Pushbutton (link to bottom-right, figure coords)
        set(exportToWorkspacePushbutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) faxBottom TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

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
        referenceChannelTextbox.String = '';

        specsChanged = true;
    end  % selectChannelsPushbuttonCallback()


    % Trim to Selected Pushbutton
    function trimToSelectedPushbuttonCallback(~, ~)
        n_selected = numel(channelListbox.Value);
        for i = 1:n_selected
            channelListbox.String(i) = channelListbox.String(channelListbox.Value(i));
        end
        channelListbox.Value = [];
        channelListbox.String = channelListbox.String(1:n_selected);
        sessionSelected = false;

        specsChanged = true;
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

        specsChanged = true;
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


    % Extent Mode Radiobuttons
    function extentModeRadiobuttonsCallback(src, ~)
        if (src == timesRadiobutton)
            if (timesRadiobutton.Value == false)
                indicesRadiobutton.Value = true;
            else
                indicesRadiobutton.Value = false;
            end
        else
            if (indicesRadiobutton.Value == false)
                timesRadiobutton.Value = true;
            else
                timesRadiobutton.Value = false;
            end
        end

        if (timesRadiobutton.Value == true)
            referenceChannelTextbox.Visible = false;
            useSelectedPushbutton.Visible = false;
            referenceChannelLabel.Visible = false;
            relativeRadiobutton.Enable = 'on';
            secondsRadiobutton.Visible = true;
            usecsRadiobutton.Visible = true;
            unitsLabel.Visible = true;
            startLabel.String = 'Start Time:';
            endLabel.String = 'End Time:';
        else
            relativeRadiobutton.Enable = 'off';
            absoluteRadiobutton.Value = true;
            relativeRadiobutton.Value = false;
            secondsRadiobutton.Visible = false;
            usecsRadiobutton.Visible = false;
            unitsLabel.Visible = false;            
            referenceChannelTextbox.Visible = true;
            useSelectedPushbutton.Visible = true;
            referenceChannelLabel.Visible = true;
            startLabel.String = 'Start Index:';
            endLabel.String = 'End Index:';
        end

        check_limits();
        specsChanged = true;

    end  % extentModeRadiobuttonsCallback()


    % Time Mode Radiobuttons
    function timeModeRadiobuttonsCallback(src, ~)
        if (src == absoluteRadiobutton)
            if (indicesRadiobutton.Value == true)
                absoluteRadiobutton.Value = true;  % no relative indices
            end
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

        % if switched to absolute, make negatives positive
        if (timesRadiobutton.Value == true)
            lim = convert_limits(startTextbox.String);
            if (isnumeric(lim) == true)
                if (absoluteRadiobutton.Value == true)
                    if (lim < 0)
                       startTextbox.String = num2str(-lim);
                    end
                end
            end
    
            lim = convert_limits(endTextbox.String);
            if (isnumeric(lim) == true)
                if (absoluteRadiobutton.Value == true)
                    if (lim < 0)
                        endTextbox.String = num2str(-lim);
                    end
                end
            end
        end

        check_limits();
        specsChanged = true;
        
    end  % timeModeRadiobuttonsCallback()


    function filterDropdownCallback(~, ~)
        if (filterDropdown.Value == 1 )  % none (no cutoffs)
            set(lowCutoffLabel, 'Visible', 'off');
            set(lowCutoffTextbox, 'Visible', 'off');
            set(highCutoffLabel, 'Visible', 'off');
            set(highCutoffTextbox, 'Visible', 'off');
        elseif (filterDropdown.Value == 2)  % lowpass (one cutoff)
            set(lowCutoffLabel, 'Visible', 'off');
            set(lowCutoffTextbox, 'Visible', 'off');
            set(highCutoffLabel, 'Visible', 'on');
            set(highCutoffTextbox, 'Visible', 'on');
        elseif (filterDropdown.Value == 3) % highpass (one cutoff)
            set(lowCutoffLabel, 'Visible', 'on');
            set(lowCutoffTextbox, 'Visible', 'on');
            set(highCutoffLabel, 'Visible', 'off');
            set(highCutoffTextbox, 'Visible', 'off');
        else   % bandpass or bandstop (both cutoffs)
            set(lowCutoffLabel, 'Visible', 'on');
            set(lowCutoffTextbox, 'Visible', 'on');
            set(highCutoffLabel, 'Visible', 'on');
            set(highCutoffTextbox, 'Visible', 'on');
        end
    end


    function limitTextboxCallback(~, ~)
        check_limits();
        specsChanged = true;
    end  % limitTextboxCallback()

    
    % Use Selected Pushbutton
    function useSelectedPushbuttonCallback(~, ~)
        n_selected = numel(channelListbox.Value);
        if (n_selected < 1)
            errordlg('No channel selected', 'Read MED GUI');
        elseif (n_selected > 1)
            errordlg('Only one channel can be selected', 'Read MED GUI');
        else
            referenceChannelTextbox.String = char(channelListbox.String(channelListbox.Value(1)));
            specsChanged = true;
        end
        channelListbox.Value = [];
   end  % useSelectedPushbuttonCallback()


    % Units Radiobuttons
    function unitsModeRadiobuttonsCallback(src, ~)
        if (src == secondsRadiobutton)
            if (secondsRadiobutton.Value == false)
                usecsRadiobutton.Value = true;
            else
                usecsRadiobutton.Value = false;
            end
        else
            if (usecsRadiobutton.Value == false)
                secondsRadiobutton.Value = true;
            else
                secondsRadiobutton.Value = false;
            end
        end

        % scale times
        start_lim = convert_limits(startTextbox.String);
        if (isscalar(start_lim))
            if (secondsRadiobutton.Value == true)
                start_lim = start_lim / 1000000;  % can be float
            else
                start_lim = round(start_lim * 1000000);  % must be integer
            end
            startTextbox.String = num2str(start_lim);
        end
        end_lim = convert_limits(endTextbox.String);
        if (isscalar(end_lim))
            if (secondsRadiobutton.Value == true)
                end_lim = end_lim / 1000000;  % can be float
            else
                end_lim = round(end_lim * 1000000);  % must be integer
            end
            endTextbox.String = num2str(end_lim);
        end

    end  % unitsModeRadiobuttonsCallback()


    % Password Textbox
    function passwordTextboxCallback(~, ~)
        len = length(passwordTextbox.String);
        if (len > MAX_PASSWORD_LENGTH)
            errordlg('Password is too long', 'Read MED GUI');
            passwordTextbox.UserData.password = '';
            passwordTextbox.String = '';
        elseif len == 0
            passwordTextbox.UserData.password = '';
        end
        specsChanged = true;
    end  % passwordTextboxCallback()


    % Export to Workspace Pushbutton
    function exportToWorkspacePushbuttonCallback(~, ~)
        if (specsChanged == true)
            success = get_data();
            if success == false
                return;
            end
        end
        
        assignin('base', sliceNameTextbox.String, slice);
        assignin('base', parametersNameTextbox.String, out_rps);
        msgbox('Data Exported');

        % set up parameters for another export
        in_rps = out_rps;

    end  % exportToWorkspacePushbuttonCallback()


    % Plot Pushbutton
    function plotPushbuttonCallback(~, ~)
        if specsChanged == true
            saved_show_status = SHOW_READ_MED_COMMAND;
            SHOW_READ_MED_COMMAND = false;
            success = get_data();
            SHOW_READ_MED_COMMAND = saved_show_status;
            if success == false
                return;
            end
        end
        
        plot_MED(slice);
    end  % plotPushbuttonCallback()


    % Generic Specs Changed Callback
    function specsChangedCallback(~, ~)
        specsChanged = true;
    end  % specsChangedCallback()



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
            if (timesRadiobutton.Value == true)
                if (lim < 0)
                    if (absoluteRadiobutton.Value == true)
                        absoluteRadiobutton.Value = false;
                        relativeRadiobutton.Value = true;
                    end
                elseif (relativeRadiobutton.Value == true)
                    startTextbox.String = num2str(-lim); % make negative
                end
            elseif (lim < 0)
                startTextbox.String = num2str(-lim); % no negative indices
            end
        end

        lim = convert_limits(endTextbox.String);
        if (isnumeric(lim) == true)
            if (timesRadiobutton.Value == true)
                if (lim < 0)
                    if (absoluteRadiobutton.Value == true)
                        absoluteRadiobutton.Value = false;
                        relativeRadiobutton.Value = true;
                    end
                elseif (relativeRadiobutton.Value == true)
                    endTextbox.String = num2str(-lim); % make negative
                end
            elseif (lim < 0)
                endTextbox.String = num2str(-lim); % no negative indices
            end
        end

    end  % check_limits()


    % set initial field values from passed rps
    function set_initial_field_values()
        [path, ~, ext] = fileparts(in_rps.Data{1});
        if (strcmp(ext, '.medd') == true)
            sessionSelected = true;
            sessionDirectory = in_rps.Data{1};
            dirList = dir([in_rps.Data{1} DIR_DELIM '*.ticd']);
            n_chans = numel(dirList);
            channelList = {};
            for i = 1:n_chans
                [~, name, ~] = fileparts(dirList(i).name);
                channelList(i) = cellstr(name); 
            end
        else
            sessionDirectory = path;
            sessionSelected = false;
            n_chans = numel(in_rps.Data);
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
        channelListbox.String = channelList;
        sessionDirectoryTextbox.String = sessionDirectory;
        channelListbox.Value = [];
        if (strcmp(in_rps.ExtMode, 'time') == true || in_rps.ExtMode == 0)
            timesRadiobutton.Value = true;
            indicesRadiobutton.Value = false;
        else   
            timesRadiobutton.Value = false;
            indicesRadiobutton.Value = true;
        end
        if (~isempty(in_rps.IdxChan))
            referenceChannelTextbox.String = in_rps.IdxChan;
            indicesRadiobutton.Value = true;
            extentModeRadiobuttonsCallback(indicesRadiobutton, []);
        end
        if (~ischar(in_rps.Start))
            startTextbox.String = num2str(in_rps.Start);
            secondsRadiobutton.Value = false;
            usecsRadiobutton.Value = true;
            if (in_rps.Start < 0)
                relativeRadiobutton.Value = true;
                absoluteRadiobutton.Value = false;
            else
                relativeRadiobutton.Value = false;
                absoluteRadiobutton.Value = true;
            end
        end
        if (~ischar(in_rps.End))
            endTextbox.String = num2str(in_rps.End);
            secondsRadiobutton.Value = false;
            usecsRadiobutton.Value = true;
            if (in_rps.End < 0)
                relativeRadiobutton.Value = true;
                absoluteRadiobutton.Value = false;
            else
                relativeRadiobutton.Value = false;
                absoluteRadiobutton.Value = true;
            end
        end
        passwordTextbox.UserData.password = in_rps.Pass;
        passwordTextbox.String = repmat('*', [1 length(in_rps.Pass)]);
        switch (in_rps.Format)
            case {'double', 0}
                formatDropdown.Value = 1;
            case {'single', 1}
                formatDropdown.Value = 2;
            case {'int32', 2}
                formatDropdown.Value = 3;
            case {'int16', 3}
                formatDropdown.Value = 4;
            otherwise
                formatDropdown.Value = 1;
        end
        switch (in_rps.Filt)
            case {'none', 0}
                filterDropdown.Value = 1;
            case {'lowpass', 1}
                filterDropdown.Value = 2;
            case {'highpass', 2}
                filterDropdown.Value = 3;
            case {'bandpass', 3}
                filterDropdown.Value = 4;
            case {'bandstop', 4}
                filterDropdown.Value = 5;
            otherwise
                filterDropdown.Value = 1;
        end
        
        switch (in_rps.Persist)
            case {'none', 0}
                persistenceDropdown.Value = 1;
            case {'open', 1}
                persistenceDropdown.Value = 2;
            case {'close', 2}
                persistenceDropdown.Value = 3;
            case {'read', 4}
                persistenceDropdown.Value = 4;
            case {'read_new', 5}
                persistenceDropdown.Value = 5;
            case {'read_close', 6}
                persistenceDropdown.Value = 6;
            otherwise
                persistenceDropdown.Value = 1;
        end
        metadataCheckbox.Value = in_rps.Metadata;
        recordsCheckbox.Value = in_rps.Records;
        contiguaCheckbox.Value = in_rps.Contigua;
    end


    function [success] = get_data()
        set(fig, 'Pointer', 'watch');
        pause(0.1);  % let cursor change (if no pause it gets into mex function before it cann switch it)

        clear slice;
        success = false;

        % data_paths
        data_paths = get_data_paths();
        if (numel(data_paths) == 0)
            set(fig, 'Pointer', 'arrow');
            errordlg('No MED session or channels are specified', 'Matrix MED GUI');
            return;
        end
        out_rps.Data = data_paths;

        % slice limits
        start_lim = convert_limits(startTextbox.String);
        end_lim = convert_limits(endTextbox.String);
        if (timesRadiobutton.Value == true)
            out_rps.ExtMode = 'time';
            start_time = start_lim;
            if (isscalar(start_time))
                if (secondsRadiobutton.Value == true)
                    start_time = round(start_time * 1000000);
                end
            end
            end_time = end_lim;
            if (isscalar(end_time))
                if (secondsRadiobutton.Value == true)
                    end_time = round(end_time * 1000000);
                end
            end
            out_rps.Start = start_time;
            out_rps.End = end_time;
        else
            out_rps.ExtMode = 'indices';
            out_rps.Start = start_lim;
            out_rps.End = end_lim;
        end

        % password
        if (isempty(passwordTextbox.UserData.password))
            out_rps.Pass = [];
        else
            out_rps.Pass = passwordTextbox.UserData.password;
        end

        % index_chan
        if (isempty(referenceChannelTextbox.String))
            out_rps.IdxChan = [];
        else
            out_rps.IdxChan = referenceChannelTextbox.String;
        end

        % format
        format = formatDropdown.Value;
        out_rps.Format = formatDropdown.String{format};

        % filter
        filter = filterDropdown.Value;
        out_rps.Filt = filterDropdown.String{filter};

        % cutoffs
        low_cutoff = [];
        high_cutoff = [];
        if (filter == 2 || filter >= 4)  % lowpass, bandpass, bandstop
            high_cutoff = str2double(highCutoffTextbox.String);
            if (isnan(high_cutoff) || high_cutoff < 0)
                set(fig, 'Pointer', 'arrow');
                if (isnan(high_cutoff))
                    errordlg('High cutoff frequency must be specified', 'Matrix MED GUI');
                else
                    errordlg('High cutoff frequency must be a positive number', 'Matrix MED GUI');
                end
                return;
            end
        end
        if (filter >= 3)  % highpass, bandpass, bandstop
            low_cutoff = str2double(lowCutoffTextbox.String);
            if (isnan(low_cutoff) || low_cutoff < 0)
                set(fig, 'Pointer', 'arrow');
                if (isnan(low_cutoff))
                    errordlg('Low cutoff frequency must be specified', 'Matrix MED GUI');
                else
                    errordlg('Low cutoff frequency must be a positive number', 'Matrix MED GUI');
                end
                return;
            end
        end
        out_rps.LowCut = low_cutoff;
        out_rps.HighCut = high_cutoff;

        % metadata
        out_rps.Metadata = metadataCheckbox.Value;

        % records
        out_rps.Records = recordsCheckbox.Value;

        % contigua
        out_rps.Contigua = contiguaCheckbox.Value;

        % persistence
        persistence = persistenceDropdown.Value;
        out_rps.Persist = persistenceDropdown.String{persistence};
        
        tic;
        slice = read_MED_exec(out_rps);
        exec_time = round(toc * 1e6);

        if (SHOW_READ_MED_COMMAND == true)
            show_command(data_paths, exec_time);
        end

       if islogical(slice)  % can be true, false, or structure
            if slice == false
                set(fig, 'Pointer', 'arrow');
                errordlg('Read error', 'Read MED GUI');
                success = false;
                return;
            end
        end
        success = true;            
        specsChanged = false;

        set(fig, 'Pointer', 'arrow');
    end  % get_data()


    function show_command(data_paths, cl_exec_time)
        
        % build command string
        command = ['[' sliceNameTextbox.String ', ' parametersNameTextbox.String '] = read_MED('];

        % compare Data
        new_cmd = false;
        if (numel(out_rps.Data) == numel(in_rps.Data))
            for i = 1:numel(out_rps.Data)
                if (strcmp(out_rps.Data{i}, in_rps.Data{i}) == false)
                    new_cmd = true;
                    break;
                end
            end
        else
            new_cmd = true;
        end

        % build 'data_paths' string
        if (new_cmd == true)
            dir_str = 'data_path';
            n_dirs = numel(data_paths);
            if (n_dirs > 1)
                dir_str = [dir_str 's'];
            end
            
            fl = [dir_str ' = {'];
            for i = 1:(n_dirs - 1)
                fl = [fl '''' char(data_paths(i)) ''', ']; %#ok<AGROW>
            end
            fl = [fl '''' char(data_paths(n_dirs)) '''};'];
    
            tic;
            v = evalin('base', ['logical(exist(''' dir_str ''', ''var''))']);
            fl_exec_time = round(toc * 1e6);

            if (v == true)
                fprintf(2, '\nThe variable ''%s'' exists in the workspace. To overwrite, execute:\n', dir_str);
            else
                evalin('base', fl);
                fprintf(2, '\nExecuted:\n');
                append_history(fl, fl_exec_time);
            end
            disp(fl);

            % 'data_path(s)' exists
            cl = dir_str;
            if (v == true)
                    cl = ['<' cl '>'];
            end
            command = [command '''Data'', ' cl ', '];
        else
            command = [command parametersNameTextbox.String ', '];
        end

        % ExtMode
        if (indicesRadiobutton.Value == true)  % not default
            if (strcmp(out_rps.ExtMode, in_rps.ExtMode) == false)  % different from prior
                command = [command '''ExtMode'', ''' out_rps.ExtMode ''', '];
            end
        end

        % Start
        if (ischar(out_rps.Start) == false)  % not default
           if (isempty(in_rps.Start))  % (all comparisons with empty are false)
                command = [command '''Start'', ' num2str(int64(out_rps.Start)) ', '];
           elseif (out_rps.Start ~= in_rps.Start)  % different from prior
                command = [command '''Start'', ' num2str(int64(out_rps.Start)) ', '];
            end
        end

        % End
        if (ischar(out_rps.End) == false)  % not default
            if (isempty(in_rps.End))  % (all comparisons with empty are false)
                command = [command '''End'', ' num2str(int64(out_rps.End)) ', '];
            elseif (out_rps.End ~= in_rps.End)  % different from prior
                command = [command '''End'', ' num2str(int64(out_rps.End)) ', '];
            end
        end
        
        % Pass
        if (~isempty(out_rps.Pass))  % password exists
            if (strcmp(out_rps.Pass, in_rps.Pass) == false)  % different from prior
                command = [command '''Pass'', ''<password>'', '];
            end
        end

        % IdxChan
        if (~isempty(out_rps.IdxChan))  % not default
            if (strcmp(out_rps.IdxChan, in_rps.IdxChan) == false)  % different from prior
                command = [command '''IdxChan'', ''' out_rps.IdxChan ''', '];
            end
        end
        
        % Format
        if (formatDropdown.Value ~= 1)  % not default
            if (strcmp(out_rps.Format, in_rps.Persist) == false)  % different from prior       
                command = [command '''Format'', ''' out_rps.Format ''', '];
            end
        end

        % Filt
        if (filterDropdown.Value ~= 1)  % not default
            if (strcmp(out_rps.Filt, in_rps.Filt) == false)  % different from prior       
                command = [command '''Filt'', ''' out_rps.Filt ''', '];
            end
        end

        % LowCut
        if (~isempty(out_rps.LowCut))  % not default
            if (isempty(in_rps.LowCut)) % (all comparisons with empty are false)
                command = [command '''LowCut'', ' num2str(out_rps.LowCut) ', '];
            elseif (out_rps.LowCut ~= in_rps.LowCut)  % different from prior
                command = [command '''LowCut'', ' num2str(out_rps.LowCut) ', '];
            end
        end

        % HighCut
        if (~isempty(out_rps.HighCut))  % not default
            if (isempty(in_rps.HighCut)) % (all comparisons with empty are false)
                command = [command '''HighCut'', ' num2str(out_rps.HighCut) ', '];
            elseif (out_rps.HighCut ~= in_rps.HighCut)  % different from prior
                command = [command '''HighCut'', ' num2str(out_rps.HighCut) ', '];
            end
        end

        % Persist
        if (persistenceDropdown.Value ~= 1)  % not default
            if (strcmp(out_rps.Persist, in_rps.Persist) == false)  % different from prior       
                command = [command '''Persist'', ''' out_rps.Persist ''', '];
            end
        end
        
        % Metadata
        if (out_rps.Metadata == false)  % not default
            if (out_rps.Metadata ~= in_rps.Metadata)  % different from prior      
                command = [command '''Metadata'', false, '];
            end
        end

        % Records
        if (out_rps.Records == false)  % not default
            if (out_rps.Records ~= in_rps.Records)  % different from prior      
                command = [command '''Records'', false, '];
            end
        end

        % Contigua
        if (out_rps.Contigua == false)  % not default
            if (out_rps.Contigua ~= in_rps.Contigua)  % different from prior      
                command = [command '''Contigua'', false, '];
            end
        end
      
        command = [command(1:(end - 2)) ');'];  % get rid of terminal ', ' & add closure
        fprintf(2, '\nExecuted:\n');
        disp(command);
        append_history(command, cl_exec_time);

    end  % show_command()



    function [data_paths] = get_data_paths()
        chan_names = channelListbox.String;
        
        if (sessionSelected == true)
            data_paths{1} = sessionDirectory;
            return;
        end

        n_chans = length(chan_names);
        data_paths = cell(n_chans, 1);
        for i = 1:n_chans
            data_paths{i} = [sessionDirectory DIR_DELIM char(channelListbox.String(i)) '.ticd'];
        end
    end  % get_data_paths()

end  % read_MED_GUI()

