

function matrix_MED_GUI(varargin)

    % matrix_MED_GUI([mps])
    %
    % GUI for matrix_MED()
    %
    % mps: a matrix parameter structure; if passed fields will be set to the values in mps on open
    %
    % Copyright Dark Horse Neuro, 2021

    % Defaults (user can change these)
    DEFAULT_DATA_DIRECTORY = pwd;
    DEFAULT_PASSWORD = 'L2_password';  % example_data passwords == 'L1_password' or 'L2_password'
    DEFAULT_MATRIX_NAME = 'mat';
    DEFAULT_PARAMETERS_NAME = 'mps';
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

    [MATRIX_MED_PATH, ~, ~] = fileparts(which('matrix_MED'));
    RESOURCES = [MATRIX_MED_PATH DIR_DELIM 'Resources'];
    if (isempty(which('matrix_MED_exec')))
        addpath(RESOURCES, MATRIX_MED_PATH, '-begin');
        savepath;
        msg = ['Added ', RESOURCES, ' to your search path.' newline];
        beep
        fprintf(2, '%s', msg);  % 2 == stderr, so red in command window
    end

    
    % Constants
    MAX_PASSWORD_LENGTH = 15;

    MIN_F_LEFT = 1;
    MIN_F_BOTTOM = 1;
    MIN_F_WIDTH = 850;
    MIN_F_HEIGHT = 800;
    INITIAL_X_F_OFFSET = 200;
    INITIAL_Y_F_OFFSET = 100;
    AX_OFFSET = 25;
    TEXT_BOX_HEIGHT = 30;
    TEXT_BOX_WIDTH = 160;
    HALF_TEXT_BOX_HEIGHT = TEXT_BOX_HEIGHT / 2;

    % Globals
    dataDirectory = DEFAULT_DATA_DIRECTORY;
    sessionDirectory = '';
    channelList = {};
    mat = [];
    specsChanged = true;
    sessionSelected = false;

    % matrix_MED arguments
    data_paths = [];
    out_samps = [];
    out_freq = [];
    start_time = [];
    end_time = [];
    filter = [];
    scale = [];
    format = [];
    persistence = [];
    chan_names = [];
    padding = [];
    interpolation = [];
    binterpolation = [];

    % default matrix paramters
    if (nargin)
        if (nargin > 1)
            errordlg('matrix_MED_GUI can only accept one input', 'Matrix MED GUI');
            return;
        elseif (isstruct(varargin{1}) == false)
            errordlg('The input argument must a matrix_MED parameter structure', 'Matrix MED GUI');
            return;
        end
        in_mps = varargin{1};
        set_in_values = true;
    else
        in_mps = matrix_MED;
        set_in_values = false;
    end
    out_mps = in_mps;  % out_mps is modified & compaped to in_mps to build command

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
        'Position', [faxLeft (faxBottom + 100) (faxWidth - 550) (faxHeight - 180)], ...
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
        'Position', [faxLeft (faxBottom + 60) (faxWidth - 550) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @selectChannelsPushbuttonCallback);

    % Trim to Selected Pushbutton (link to bottom-left, figure coords)
    trimToSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Trim to Selected', ...
        'Position', [faxLeft (faxBottom + 30) (faxWidth - 550) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @trimToSelectedPushbuttonCallback);

    % Remove Selected Pushbutton (link to bottom-left, figure coords)
    removeSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Remove Selected', ...
        'Position', [faxLeft faxBottom (faxWidth - 550) TEXT_BOX_HEIGHT], ...
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

    % Sample Dimension (link to top-right, axis coords)
    sampleDimensionLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 93)], ...
        'String', ['Sample ' newline 'Dimension:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Output Dimension Radiobuttons (link to top-right, figure coords)
    countRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'count', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 88) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @outputDimensionRadiobuttonsCallback);
    rateRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'rate (Hz)', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2) - 285) (faxTop - 88) ((TEXT_BOX_WIDTH / 2) + 20) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @outputDimensionRadiobuttonsCallback);

    % Sample Dimension Textbox (link to top-right, figure coords)
    sampleDimensionTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', '', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 120) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

    % Extents Label (link to top-right, axis coords)
    extentsLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 180)], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @extentModeRadiobuttonsCallback);
    indicesRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'indices', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2) - 280) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @timeModeRadiobuttonsCallback);
    relativeRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'relative', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2) - 280) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Enable', 'on', ...
        'Callback', @timeModeRadiobuttonsCallback);

    % Reference Channel Label (link to top-right, axis coords)
    referenceChannelLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 180)], ...
        'String', ['Index ' newline 'Channel:'], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 178) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 210) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @useSelectedPushbuttonCallback);
    
    % Matrix Time Mode Label (link to top-right, axis coords)
    timeModeLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 173)], ...
        'String', 'Time Mode:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth', ...
        'Visible', 'on');

    % Matrix Time Extents Radiobuttons (link to top-right, figure coords)
    durationRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'duration', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'on', ...
        'Callback', @matrixTimeModeRadiobuttonsCallback);
    endTimeRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'end time', ...
        'Value', 1, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'on', ...
        'Callback', @matrixTimeModeRadiobuttonsCallback);

    % Matrix Time Units Label (link to top-right, axis coords)
    timeUnitsLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 197)], ...
        'String', 'Units:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth', ...
        'Visible', 'on');

    % Matrix Time Units Radiobuttons (link to top-right, figure coords)
    secondsRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'seconds', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'on', ...
        'Callback', @matrixTimeUnitsRadiobuttonsCallback);
    usecsRadiobutton = uicontrol(fig, ...
        'Style', 'radiobutton', ...
        'String', 'µsecs', ...
        'Value', 0, ...
        'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'on', ...
        'Callback', @matrixTimeUnitsRadiobuttonsCallback);

    % Start Label (link to top-right, axis coords)
    startLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 229)], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 230) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @limitTextboxCallback);

    % End Label (link to top-right, axis coords)
    endLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 264)], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 265) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @limitTextboxCallback);

    % Scale Label (link to top-right, axis coords)
    scaleLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 531)], ...
        'String', 'Scale:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Scale Textbox
    scaleTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', '1.0', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 533) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % Format Label (link to top-right, figure coords)
    formatLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 400)], ...
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
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 407) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
         'Callback', @specsChangedCallback);

   % Filter Label (link to top-right, axis coords)
    filterLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 335)], ...
        'String', 'Filter:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Filter Dropdown (link to top-right, figure coords)
    filterDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'antialias', 'none', 'lowpass', 'highpass', 'bandpass', 'bandstop'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 341) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'Value', 1, ...
        'HorizontalAlignment', 'left', ...
        'Callback', @filterDropdownCallback);

    % Low Cutoff Label (link to top-right, axis coords)
    lowCutoffLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 320)], ...
        'String', 'Low Cutoff:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'Visible', 'off', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Low Cutoff Textbox (link to top-right, figure coords)
    lowCutoffTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 321) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'Visible', 'off', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

    % High Cutoff Label (link to top-right, axis coords)
    highCutoffLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 355)], ...
        'String', 'High Cutoff:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'Visible', 'off', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % High Cutoff Textbox (link to top-right, figure coords)
    highCutoffTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 356) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'Visible', 'off', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

    % Padding Label (link to top-right, axis coords)
    paddingLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 400)], ...
        'String', 'Padding:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

   % Padding Dropdown (link to top-right, figure coords)
    paddingDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'none', 'zero', 'nan'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 407) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'Value', 1, ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % Interpolation Label (link to top-right, axis coords)
    interpolationLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 466)], ...
        'String', 'Interp:', ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Interpolation Dropdown (link to top-right, figure coords)
    interpDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'linear_makima', 'linear_spline', 'linear', 'spline', 'makima', 'binterp'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 473) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'Value', 1, ...
        'HorizontalAlignment', 'left', ...
        'Callback', @interpDropdownCallback);

    % Bin Interpolation Label (link to top-right, axis coords)
    binterpLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 466)], ...
        'String', ['Binterp ' newline 'Mode:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'Visible', 'off', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

   % Bin Interpolation Mode (link to top-right, figure coords)
    binterpDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'mean', 'median', 'center', 'fast'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 473) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'Value', 1, ...
        'Visible', 'off', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);
    
   % Persistence Label (link to top-right, figure coords)
    persistenceLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 532)], ...
        'String', ['Persist ' newline 'Mode:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Persistence Dropdown (link to top-right, figure coords)
    persistenceDropdown = uicontrol(fig, ...
        'Style', 'popupmenu', ...
        'String', {'none', 'open', 'close', 'read', 'read_new', 'read_close'}, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 539) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'Value', 1, ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % Detrend Checkbox (link to top-right, figure coords)
    detrendCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Detrend', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 601) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % Ranges Checkbox (link to top-right, figure coords)
    rangesCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Ranges', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 624) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % Extrema Checkbox (link to top-right, figure coords)
    extremaCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Extrema', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 647) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);
 
    % Records Checkbox (link to top-right, figure coords)
    recordsCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Records', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 670) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);

    % Contigua Checkbox (link to top-right, figure coords)
    contiguaCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Contigua', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 693) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback);
 
    % Channel Names Checkbox (link to top-right, figure coords)
    channelNamesCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Channel Names', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 716) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

    % Channel Frequencies Checkbox (link to top-right, figure coords)
    channelFrequenciesCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Channel Frequencies', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 739) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback); 
 
    % Matrix Name Label (link to top-right, axis coords)
    matrixNameLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 607)], ...
        'String', ['Matrix ' newline 'Name:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Matrix Name Textbox (link to top-right, figure coords)
    matrixNameTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', DEFAULT_MATRIX_NAME, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 607) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Callback', @specsChangedCallback); 

    % Parameters Name Label (link to top-right, axis coords)
    parametersNameLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 642)], ...
        'String', ['Parameters ' newline 'Name:'], ...
        'Color', 'k', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right', ...
        'FontWeight', 'bold', ...
        'FontName', 'FixedWidth');

    % Parameters Name Textbox (link to top-right, figure coords)
    parametersNameTextbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'String', DEFAULT_PARAMETERS_NAME, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 642) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
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
    
    % set initial values from passed mps
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
        set(channelListbox, 'Position', [faxLeft (faxBottom + 100) (faxWidth - 550) (faxHeight - 180)]);

        % Select Channels Pushbutton (link to bottom-left, figure coords)
        set(selectChannelsPushbutton, 'Position', [faxLeft (faxBottom + 60) (faxWidth - 550) TEXT_BOX_HEIGHT]);
    
        % Trim to Selected Pushbutton (link to bottom-left, figure coords)
        set(trimToSelectedPushbutton, 'Position', [faxLeft (faxBottom + 30) (faxWidth - 550) TEXT_BOX_HEIGHT]);

        % Remove Selected Pushbutton (link to bottom-left, figure coords)
        set(removeSelectedPushbutton, 'Position', [faxLeft faxBottom (faxWidth - 550) TEXT_BOX_HEIGHT]);

        % Password Label (link to top-right, axis coords)
        set(passwordLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 93)]);
    
        % Password Textbox (link to top-right, figure coords)
        set(passwordTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 93) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Sample Dimension (link to top-right, axis coords)
        set(sampleDimensionLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 93)]);

        % Sample Dimension Radiobuttons (link to top-right, figure coords)
        set(countRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 88) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(rateRadiobutton, 'Position', [(faxRight - (TEXT_BOX_WIDTH / 2) - 285) (faxTop - 88) ((TEXT_BOX_WIDTH / 2) + 20) TEXT_BOX_HEIGHT]);

        % Sample Dimension Textbox (link to top-right, figure coords)
        set(sampleDimensionTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 120) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Extents Label (link to top-right, axis coords)
        set(extentsLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 180)]);

        % Extent Mode Radiobuttons
        set(timesRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(indicesRadiobutton,'Position', [(faxRight - (TEXT_BOX_WIDTH / 2) - 280) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        
        % Time Mode Radiobuttons
        set(absoluteRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(relativeRadiobutton, 'Position', [(faxRight - (TEXT_BOX_WIDTH / 2) - 280) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);

        % Reference Channel Label (link to top-right, axis coords)
        set(referenceChannelLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 180)]);
    
        % Reference Channel Textbox (link to top-right, figure coords)
        set(referenceChannelTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 178) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Use Selected Pushbutton (link to top-right, figure coords)
        set(useSelectedPushbutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 210) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
        
        % Matrix Time Mode Label (link to top-right, axis coords)
        set(timeModeLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 173)]);

        % Matrix Time Extents Radiobuttons (link to top-right, figure coords)
        set(durationRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(endTimeRadiobutton, 'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 173) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        
        % Matrix Time Units Label (link to top-right, axis coords)
        set(timeUnitsLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 197)]);

        % Matrix Time Units Radiobuttons (link to top-right, figure coords)
        set(secondsRadiobutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        set(usecsRadiobutton, 'Position', [(faxRight - (TEXT_BOX_WIDTH / 2)) (faxTop - 197) (TEXT_BOX_WIDTH / 2) TEXT_BOX_HEIGHT]);
        
        % Start Label (link to top-right, axis coords)
        set(startLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 229)]);
    
        % Start Textbox (link to top-right, figure coords)
        set(startTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 230) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
    
        % End Label (link to top-right, axis coords)
        set(endLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 264)]);
    
        % End Textbox (link to top-right, figure coords)
        set(endTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 280) (faxTop - 265) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Scale Label (link to top-right, figure coords)
        set(scaleLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 531)]);

        % Scale Textbox (link to top-right, figure coords)
        set(scaleTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 533) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Format Label (link to top-right, figure coords)
        set(formatLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 400)]);

        % Format Dropdown (link to top-right, figure coords)
        set(formatDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 407) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Filter Label (link to top-right, axis coords)
        set(filterLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 335)]);

        % Filter Dropdown (link to top-right, figure coords)
        set(filterDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 341) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Low Cutoff Label (link to top-right, axis coords)
        set(lowCutoffLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 320)]);

        % Low Cutoff Textbox (link to top-right, figure coords)
        set(lowCutoffTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 321) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % High Cutoff Label (link to top-right, axis coords)
        set(highCutoffLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 355)]);

        % High Cutoff Textbox (link to top-right, figure coords)
        set(highCutoffTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 356) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Padding Label (link to top-right, axis coords)
        set(paddingLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 400)]);

        % Padding Dropdown (link to top-right, figure coords)
        set(paddingDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 407) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Interpolation Label (link to top-right, axis coords)
        set(interpolationLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 466)]);

        % Interpolation Dropdown (link to top-right, figure coords)
        set(interpDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 473) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Binterp Label (link to top-right, axis coords)
        set(binterpLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 466)]);

        % Binterp Mode (link to top-right, figure coords)
        set(binterpDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 473) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Persistence Label (link to top-right, figure coords)
        set(persistenceLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 290) (axTop - 532)]);

        % Persistence Dropdown (link to top-right, figure coords)
        set(persistenceDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 290) (faxTop - 539) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Detrend Checkbox (link to top-right, figure coords)
        set(detrendCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 601) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Ranges Checkbox (link to top-right, figure coords)
        set(rangesCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 624) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Extrema Checkbox (link to top-right, figure coords)
        set(extremaCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 647) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Records Checkbox (link to top-right, figure coords)
        set(recordsCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 670) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Contigua Checkbox (link to top-right, figure coords)
        set(contiguaCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 693) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
 
        % Channel Names Checkbox (link to top-right, figure coords)
        set(channelNamesCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 716) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Channel Frequencies Checkbox (link to top-right, figure coords)
        set(channelFrequenciesCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH - 287) (faxTop - 739) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
                      
        % Matrix Name Label (link to top-right, axis coords)
        set(matrixNameLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 607)]);
    
        % Matrix Name Textbox (link to top-right, figure coords)
        set(matrixNameTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 607) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
        
        % Parameters Name Label (link to top-right, axis coords)
        set(parametersNameLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 642)]);
    
        % Parameters Name Textbox (link to top-right, figure coords)
        set(parametersNameTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 642) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
        
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


    % Output Dimension Radiobuttons
    function outputDimensionRadiobuttonsCallback(src, ~)
        if (src == countRadiobutton)
            if (countRadiobutton.Value == false)
                rateRadiobutton.Value = true;
            else
                rateRadiobutton.Value = false;
            end
        else
            if (rateRadiobutton.Value == false)
                countRadiobutton.Value = true;
            else
                countRadiobutton.Value = false;
            end
        end

        specsChanged = true;
    end  % outputDimensionRadiobuttonsCallback()

    
    % Matrix Time Mode Radiobuttons
    function matrixTimeModeRadiobuttonsCallback(src, ~)
        if (src == endTimeRadiobutton)
            if (endTimeRadiobutton.Value == false)
                durationRadiobutton.Value = true;
            else
                durationRadiobutton.Value = false;
            end
        else
            if (durationRadiobutton.Value == false)
                endTimeRadiobutton.Value = true;
            else
                endTimeRadiobutton.Value = false;
            end
        end
        specsChanged = true;

        % change 'End' / 'Duration'
        if (durationRadiobutton.Value == true)
            endLabel.String = 'Duration:';
        else
            endLabel.String = 'End Time:';
        end
        if (isempty(startTextbox.String) || isempty(endTextbox.String))
            return;
        end
        if (strcmp(startTextbox.String, 'start'))
            if (relativeRadiobutton.Value == true)  % can only do relative time from 'start'
                start_val = 0;
            else
                warndlg('Duration mode from ''start'' requires relative time', 'Matrix MED GUI');
                absoluteRadiobutton.Value = false;
                relativeRadiobutton.Value = true;
                if (strcmp(endTextbox.String, 'end') == true)
                    endTextbox.String = [];
                end
                return;
            end
        else
            start_val = str2double(startTextbox.String);
        end
        if (secondsRadiobutton.Value == true)
            start_val = start_val * 1000000;
        end
        if (relativeRadiobutton.Value == true)
            start_val = -start_val;
        end
        if (durationRadiobutton.Value == true)
            if (strcmp(endTextbox.String, 'end'))  % do not know session end time
                warndlg('Cannot calculate duration to ''end''', 'Matrix MED GUI');
                endTextbox.String = [];
                return;
            end
            end_val = str2double(endTextbox.String);
            if (secondsRadiobutton.Value == true)
                end_val = end_val * 1000000;
            end
            if (relativeRadiobutton.Value == true)
                end_val = -end_val;
            end
            dur = (end_val - start_val) + 1;
            if (secondsRadiobutton.Value == true)
                dur = dur / 1000000;
            end
            endTextbox.String = num2str(dur);
        else
            endLabel.String = 'End Time:';
            if (isempty(startTextbox.String) || isempty(endTextbox.String))
                return;
            end
            dur = str2double(endTextbox.String);
            if (secondsRadiobutton.Value == true)
                dur = dur * 1000000;
            end
            end_val = (start_val + dur) - 1;
            if (relativeRadiobutton.Value == true)
                end_val = -end_val;
            end
            if (secondsRadiobutton.Value == true)
                end_val = end_val / 1000000;
            end
            endTextbox.String = num2str(end_val);
        end
    end  % outputDimensionRadiobuttonsCallback()


    % Matrix Time Units Radiobuttons
    function matrixTimeUnitsRadiobuttonsCallback(src, ~)
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

   end  % outputDimensionRadiobuttonsCallback()


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
            startLabel.String = 'Start Time:';
            if (durationRadiobutton.Value == true)
                endLabel.String = 'Duration:';
            else
                endLabel.String = 'End Time:';
            end
            relativeRadiobutton.Enable = 'on';            
            referenceChannelTextbox.Visible = false;
            useSelectedPushbutton.Visible = false;
            referenceChannelLabel.Visible = false;
            timeModeLabel.Visible = true;            
            durationRadiobutton.Visible = true;            
            endTimeRadiobutton.Visible = true;
            timeUnitsLabel.Visible = true;   
            secondsRadiobutton.Visible = true;            
            usecsRadiobutton.Visible = true;
        else
            startLabel.String = 'Start Index:';
            endLabel.String = 'End Index:';
            absoluteRadiobutton.Value = true;
            relativeRadiobutton.Value = false;
            relativeRadiobutton.Enable = 'off';
            timeModeLabel.Visible = false;            
            durationRadiobutton.Visible = false;            
            endTimeRadiobutton.Visible = false;
            timeUnitsLabel.Visible = false;   
            secondsRadiobutton.Visible = false;            
            usecsRadiobutton.Visible = false;
            endTimeRadiobutton.Visible = false;            
            referenceChannelTextbox.Visible = true; 
            useSelectedPushbutton.Visible = true;
            referenceChannelLabel.Visible = true; 
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
            if (isscalar(lim) == true)
                if (absoluteRadiobutton.Value == true)
                    if (lim < 0)
                        lim = -lim;
                    end
                elseif (durationRadiobutton.Value == false)  % durations stay positive
                    if (lim > 0)
                        lim = -lim;
                    end
                end
                startTextbox.String = num2str(lim);
            end
    
            lim = convert_limits(endTextbox.String);
            if (isscalar(lim) == true)
                if (absoluteRadiobutton.Value == true)
                    if (lim < 0)
                        lim = -lim;
                    end
                elseif (durationRadiobutton.Value == false)  % durations stay positive
                    if (lim > 0)
                        lim = -lim;
                    end
                end
                endTextbox.String = num2str(lim);
            end
        end

        check_limits();

        specsChanged = true;
    end  % timeModeRadiobuttonsCallback()


    function limitTextboxCallback(~, ~)
        check_limits();

        specsChanged = true;
    end  % limitTextboxCallback()

    
    % Use Selected Pushbutton
    function useSelectedPushbuttonCallback(~, ~)
        n_selected = numel(channelListbox.Value);
        if (n_selected < 1)
            errordlg('No channel selected', 'Matrix MED GUI');
        elseif (n_selected > 1)
            errordlg('Only one channel can be selected', 'Matrix MED GUI');
        else
            referenceChannelTextbox.String = char(channelListbox.String(channelListbox.Value(1)));
            specsChanged = true;
        end
        channelListbox.Value = [];

        specsChanged = true;
   end  % useSelectedPushbuttonCallback()


    % Password Textbox
    function passwordTextboxCallback(~, ~)
        len = length(passwordTextbox.String);
        if (len > MAX_PASSWORD_LENGTH)
            errordlg('Password is too long', 'Matrix MED GUI');
            passwordTextbox.UserData.password = '';
            passwordTextbox.String = '';
        elseif len == 0
            passwordTextbox.UserData.password = '';
        end

        specsChanged = true;
    end  % passwordTextboxCallback()

    % Filter Dropdown
    function filterDropdownCallback(~, ~)        
        if (filterDropdown.Value <= 2 )  % none or antialias (no cutoffs)
            lowCutoffLabel.Visible = false;
            lowCutoffTextbox.Visible = false;
            highCutoffLabel.Visible = false;
            highCutoffTextbox.Visible = false;
        elseif (filterDropdown.Value >= 5)  % bandpass or bandstop (both cutoffs)
            lowCutoffLabel.Visible = true;
            lowCutoffTextbox.Visible = true;
            highCutoffLabel.Visible = true;
            highCutoffTextbox.Visible = true;
        elseif (filterDropdown.Value == 3)  % lowpass (one cutoff)
            lowCutoffLabel.Visible = false;
            lowCutoffTextbox.Visible = false;
            highCutoffLabel.Visible = true;
            highCutoffTextbox.Visible = true;
        else % highpass (one cutoff)
            highCutoffLabel.Visible = false;
            highCutoffTextbox.Visible = false;
            lowCutoffLabel.Visible = true;
            lowCutoffTextbox.Visible = true;
        end

        specsChanged = true;
    end  % filterDropdownCallback()

    function interpDropdownCallback(~, ~)
        if (interpDropdown.Value == 6)  % binterp
            set(binterpLabel, 'Visible', 'on');
            set(binterpDropdown, 'Visible', 'on');
        else
            set(binterpLabel, 'Visible', 'off');
            set(binterpDropdown, 'Visible', 'off');
        end

        specsChanged = true;
    end  % interpDropdownCallback()

    % Export to Workspace Pushbutton
    function exportToWorkspacePushbuttonCallback(~, ~)
        if (specsChanged == true)
            success = get_data();
            if success == false
                return;
            end
        end
        
        assignin('base', matrixNameTextbox.String, mat);
        assignin('base', parametersNameTextbox.String, out_mps);
        msgbox('Data Exported');

        % set up parameters for another export
        in_mps = out_mps;

    end  % exportToWorkspacePushbuttonCallback()


    % Plot Pushbutton
    function plotPushbuttonCallback(~, ~)
        if specsChanged == true
            success = get_data();
            if success == false
                return;
            end
        end
        
        plot(mat.samples);
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
            limit = str2double(string);
            if (isnan(limit))
                limit = [];
            end
        end
    end  % convertLimits()


    function check_limits()

        lim = convert_limits(startTextbox.String);
        if (isscalar(lim) == true)
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
        if (isscalar(lim) == true)
            if (timesRadiobutton.Value == true)
                if (lim < 0)
                    if (absoluteRadiobutton.Value == true)
                        absoluteRadiobutton.Value = false;
                        relativeRadiobutton.Value = true;
                    end
                elseif (relativeRadiobutton.Value == true)
                    if (durationRadiobutton.Value == false)
                        endTextbox.String = num2str(-lim); % make negative, unless duration
                    end
                end
            elseif (lim < 0)
                endTextbox.String = num2str(-lim); % no negative indices
            end
        end

    end  % check_limits()


    % set initial field values from passed mps
    function set_initial_field_values()
        [path, ~, ext] = fileparts(in_mps.Data{1});
        if (strcmp(ext, '.medd') == true)
            sessionSelected = true;
            sessionDirectory = in_mps.Data{1};
            dirList = dir([in_mps.Data{1} DIR_DELIM '*.ticd']);
            n_chans = numel(dirList);
            channelList = {};
            for i = 1:n_chans
                [~, name, ~] = fileparts(dirList(i).name);
                channelList(i) = cellstr(name); 
            end
        else
            sessionDirectory = path;
            sessionSelected = false;
            n_chans = numel(in_mps.Data);
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
        if (strcmp(in_mps.SampDimMode, 'count') == true || in_mps.SampDimMode == 0)
            countRadiobutton.Value = true;
            rateRadiobutton.Value = false;
        else   
            countRadiobutton.Value = false;
            rateRadiobutton.Value = true;
        end
        sampleDimensionTextbox.String = in_mps.SampDim;
        if (strcmp(in_mps.ExtMode, 'time') == true || in_mps.ExtMode == 0)
            timesRadiobutton.Value = true;
            indicesRadiobutton.Value = false;
        else   
            timesRadiobutton.Value = false;
            indicesRadiobutton.Value = true;
        end
        if (~isempty(in_mps.IdxChan))
            referenceChannelTextbox.String = in_mps.IdxChan;
            indicesRadiobutton.Value = true;
            extentModeRadiobuttonsCallback(indicesRadiobutton, []);
        end
        if (strcmp(in_mps.TimeMode, 'duration') == true || in_mps.TimeMode == 0)
            durationRadiobutton.Value = true;
            endTimeRadiobutton.Value = false;
        else
            durationRadiobutton.Value = false;
            endTimeRadiobutton.Value = true;
        end
        if (~ischar(in_mps.Start))
            startTextbox.String = num2str(in_mps.Start);
            secondsRadiobutton.Value = false;
            usecsRadiobutton.Value = true;
            if (in_mps.Start < 0)
                relativeRadiobutton.Value = true;
                absoluteRadiobutton.Value = false;
            else
                relativeRadiobutton.Value = false;
                absoluteRadiobutton.Value = true;
            end
        end
        if (~ischar(in_mps.End))
            endTextbox.String = num2str(in_mps.End);
            secondsRadiobutton.Value = false;
            usecsRadiobutton.Value = true;
            if (in_mps.End < 0)
                relativeRadiobutton.Value = true;
                absoluteRadiobutton.Value = false;
            else
                relativeRadiobutton.Value = false;
                absoluteRadiobutton.Value = true;
            end
        end
        passwordTextbox.UserData.password = in_mps.Pass;
        passwordTextbox.String = repmat('*', [1 length(in_mps.Pass)]);
        if (~isempty(in_mps.LowCut))
            lowCutoffTextbox.String = num2str(in_mps.LowCut);
        end
        if (~isempty(in_mps.HighCut))
            highCutoffTextbox.String = num2str(in_mps.HighCut);
        end
        switch (in_mps.Filt)
            case {'antialias', 0}
                filterDropdown.Value = 1;
            case {'none', 1}
                filterDropdown.Value = 2;
            case {'lowpass', 2}
                filterDropdown.Value = 3;
            case {'highpass', 3}
                filterDropdown.Value = 4;
            case {'bandpass', 4}
                filterDropdown.Value = 5;
            case {'bandstop', 5}
                filterDropdown.Value = 6;
            otherwise
                 filterDropdown.Value = 1;
        end
        filterDropdownCallback([], []);
        switch (in_mps.Format)
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
        switch (in_mps.Padding)
            case {'none', 0}
                paddingDropdown.Value = 1;
            case {'zero', 1}
                paddingDropdown.Value = 2;
            case {'nan', 2}
                paddingDropdown.Value = 3;
            otherwise
                 paddingDropdown.Value = 1;
        end
        switch (in_mps.Interp)
            case {'linear_makima', 0}
                interpDropdown.Value = 1;
            case {'linear_spline', 1}
                interpDropdown.Value = 2;
            case {'linear', 2}
                interpDropdown.Value = 3;
            case {'spline', 3}
                interpDropdown.Value = 4;
            case {'makima', 4}
                interpDropdown.Value = 5;
            case {'binterp', 5}
                interpDropdown.Value = 6;
            otherwise
                 interpDropdown.Value = 1;
        end
        switch (in_mps.Binterp)
            case {'mean', 0}
                binterpDropdown.Value = 1;
            case {'median', 1}
                binterpDropdown.Value = 2;
            case {'center', 2}
                binterpDropdown.Value = 3;
            case {'fast', 3}
                binterpDropdown.Value = 4;
            otherwise
                binterpDropdown.Value = 1;
        end
        interpDropdownCallback([], []);
        switch (in_mps.Persist)
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
        if (~isempty(in_mps.Scale))
            if (in_mps.Scale == round(in_mps.Scale))
                scaleTextbox.String = num2str(in_mps.Scale, '%0.1f');
            else
                scaleTextbox.String = num2str(in_mps.Scale);
            end
        end
        detrendCheckbox.Value = in_mps.Detrend;
        rangesCheckbox.Value = in_mps.Ranges;
        extremaCheckbox.Value = in_mps.Extrema;      
        recordsCheckbox.Value = in_mps.Records;
        contiguaCheckbox.Value = in_mps.Contigua;
        channelNamesCheckbox.Value = in_mps.ChanNames;
        channelFrequenciesCheckbox.Value = in_mps.ChanFreqs;
    end


    function [success] = get_data()
        set(fig, 'Pointer', 'watch');
        pause(0.1);  % let cursor change (if no pause it gets into mex function before it cann switch it)

        clear mat;  % output sturucture
        success = false;

        % data_paths
        data_paths = get_data_paths();
        if (numel(data_paths) == 0)
            set(fig, 'Pointer', 'arrow');
            errordlg('No MED session or channels are specified', 'Matrix MED GUI');
            return;
        end
        out_mps.Data = data_paths;

        % out_samps / out_freq
        if (countRadiobutton.Value == true)
            out_samps = str2double(sampleDimensionTextbox.String);
            if (isnan(out_samps) || out_samps <= 0)
                set(fig, 'Pointer', 'arrow');
                if (isnan(out_samps))
                    errordlg('Sample count must be specified', 'Matrix MED GUI');
                else
                    errordlg('Sample count must be a positive number', 'Matrix MED GUI');
                end
                return;
            end
            out_mps.SampDimMode = 'count';
            out_mps.SampDim = out_samps;
        else
            out_freq = str2double(sampleDimensionTextbox.String);
            if (isnan(out_freq) || out_freq <= 0)
                set(fig, 'Pointer', 'arrow');
                if (isnan(out_samps))
                    errordlg('Sample rate must be specified', 'Matrix MED GUI');
                else
                    errordlg('Sample rate must be a positive number', 'Matrix MED GUI');
                end
                return;
            end
            out_mps.SampDimMode = 'rate';
            out_mps.SampDim = out_freq;
        end

        % slice limits
        start_lim = convert_limits(startTextbox.String);
        end_lim = convert_limits(endTextbox.String);
        if (timesRadiobutton.Value == true)
            out_mps.ExtMode = 'time';
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
                if (durationRadiobutton.Value == true)
                    % calculate end time
                    if (relativeRadiobutton.Value == true)
                        if (isscalar(start_time))
                            end_time = (start_time - end_time) + 1;
                        else  % from 'start'
                            end_time = 1 - end_time;
                        end
                    else
                        if (isscalar(start_time))
                            end_time = (start_time + end_time) - 1;
                        else  % from 'start'
                            end_time = 1 - end_time;  % don't have session start time, use relative time 
                        end
                    end
                end
            end
            out_mps.Start = start_time;
            out_mps.End = end_time;
        else
            out_mps.ExtMode = 'indices';
            out_mps.Start = start_lim;
            out_mps.End = end_lim;
        end

        % time mode
        if (timesRadiobutton.Value == true)
            if (durationRadiobutton.Value == true)
                out_mps.TimeMode = 'duration';
            else
                out_mps.TimeMode = 'end_time';
            end
        else
            out_mps.TimeMode = [];  % TimeMode unnecssary for 
        end
        
        % password
        if (isempty(passwordTextbox.UserData.password))
            out_mps.Pass = [];
        else
            out_mps.Pass = passwordTextbox.UserData.password;
        end

        % index_chan
        if (isempty(referenceChannelTextbox.String))
            out_mps.IdxChan = [];
        else
            out_mps.IdxChan = referenceChannelTextbox.String;
        end

        % filter
        filter = filterDropdown.Value;
        out_mps.Filt = filterDropdown.String{filter};

        % cutoffs
        low_cutoff = [];
        high_cutoff = [];
        if (filter == 3 || filter >= 5)  % lowpass, bandpass, bandstop
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
        if (filter >= 4)  % highpass, bandpass, bandstop
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
        out_mps.LowCut = low_cutoff;
        out_mps.HighCut = high_cutoff;

        % scale
        scale = str2double(scaleTextbox.String);
        if (isnan(scale))
            set(fig, 'Pointer', 'arrow');
            errordlg('Scale must be a number', 'Matrix MED GUI');
            return;
        end
        out_mps.Scale = scale;

        % format
        format = formatDropdown.Value;
        out_mps.Format = formatDropdown.String{format};

        % persistence
        persistence = persistenceDropdown.Value;
        out_mps.Persist = persistenceDropdown.String{persistence};

        % detrend
        out_mps.Detrend = detrendCheckbox.Value;

        % ranges
        out_mps.Ranges = rangesCheckbox.Value;

        % extrema
        out_mps.Extrema = extremaCheckbox.Value;

        % records
        out_mps.Records = recordsCheckbox.Value;

        % contigua
        out_mps.Contigua = contiguaCheckbox.Value;

        % chan_names
        out_mps.ChanNames = channelNamesCheckbox.Value;

        % chan_freqs
        out_mps.ChanFreqs = channelFrequenciesCheckbox.Value;

        % padding
        padding = paddingDropdown.Value;
        out_mps.Padding = paddingDropdown.String{padding};

        % interpolation
        interpolation = interpDropdown.Value;
        out_mps.Interp = interpDropdown.String{interpolation};

        % binterpolation
        binterpolation = binterpDropdown.Value;
        out_mps.Binterp = binterpDropdown.String{binterpolation};

        % show command
        if (SHOW_MATRIX_MED_COMMAND == true)
            show_command(data_paths);
        end

        % run command
        mat = matrix_MED_exec(out_mps);
        % handle error
        if (islogical(mat))  % can be true, false, or structure
            if (mat == false)
                set(fig, 'Pointer', 'arrow');
                errordlg('Read error', 'Matrix MED GUI');
                return;
            end
        end

        success = true;            
        specsChanged = false;
        set(fig, 'Pointer', 'arrow');

    end  % get_data()


    function show_command(data_paths)   
        
        % build command string
        command = ['[' matrixNameTextbox.String ', ' parametersNameTextbox.String '] = matrix_MED('];

        % compare Data
        new_cmd = false;
        if (numel(out_mps.Data) == numel(in_mps.Data))
            for i = 1:numel(out_mps.Data)
                if (strcmp(out_mps.Data{i}, in_mps.Data{i}) == false)
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
    
            v = evalin('base', ['logical(exist(''' dir_str ''', ''var''))']);
            if (v == true)
                fprintf(2, '\nThe variable ''%s'' exists in the workspace. To overwrite, execute:\n', dir_str);
            else
                evalin('base', fl);
                fprintf(2, '\nExecuted:\n');
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

        % SampDimMode
        if (absoluteRadiobutton.Value == true)  % not default
            if (strcmp(out_mps.SampDimMode, in_mps.SampDimMode) == false)  % different from prior
                command = [command  '''SampDimMode'', ''' out_mps.SampDimMode ''', '];
            end
        end
    
        % SampDim
        if (isempty(in_mps.SampDim)) % (all comparisons with empty are false)
            command = [command '''SampDim'', ' num2str(out_mps.SampDim) ', '];
        elseif (out_mps.SampDim ~= in_mps.SampDim) % different from prior
            command = [command '''SampDim'', ' num2str(out_mps.SampDim) ', '];
        end

        % ExtMode
        if (indicesRadiobutton.Value == true)  % not default
            if (strcmp(out_mps.ExtMode, in_mps.ExtMode) == false)  % different from prior
                command = [command '''ExtMode'', ''' out_mps.ExtMode ''', '];
            end
        end

        % Start
        if (ischar(out_mps.Start) == false)  % not default
            if (isempty(in_mps.Start)) % (all comparisons with empty are false)
                command = [command '''Start'', ' num2str(int64(out_mps.Start)) ', '];
            elseif (out_mps.Start ~= in_mps.Start)  % different from prior
                command = [command '''Start'', ' num2str(int64(out_mps.Start)) ', '];
            end
        end

        % End
        
        if (ischar(out_mps.End) == false)  % not default
            if (isempty(in_mps.End)) % (all comparisons with empty are false)
                command = [command '''End'', ' num2str(int64(out_mps.End)) ', '];
            elseif (out_mps.End ~= in_mps.End)  % different from prior
                command = [command '''End'', ' num2str(int64(out_mps.End)) ', '];
            end
        end
        
        % TimeMode
        if (timesRadiobutton.Value == true)  % not default
            if (strcmp(out_mps.TimeMode, in_mps.TimeMode) == false)  % different from prior
                if (endTimeRadiobutton.Value == true)
                    command = [command '''TimeMode'', ''' out_mps.TimeMode ''', '];
                end
            end
        end

        % Pass
        if (~isempty(out_mps.Pass))  % password exists
            if (strcmp(out_mps.Pass, in_mps.Pass) == false)  % different from prior
                command = [command '''Pass'', ''<password>'', '];
            end
        end

        % IdxChan
        if (~isempty(out_mps.IdxChan))  % not default
            if (strcmp(out_mps.IdxChan, in_mps.IdxChan) == false)  % different from prior
                command = [command '''IdxChan'', ''' out_mps.IdxChan ''', '];
            end
        end

        % Filt
        if (filter ~= 1)  % not default
            if (strcmp(out_mps.Filt, in_mps.Filt) == false)  % different from prior
                command = [command '''Filt'', ''' out_mps.Filt ''', '];
            end
        end

        % LowCut
        if (~isempty(out_mps.LowCut))  % not default
            if (isempty(in_mps.LowCut)) % (all comparisons with empty are false)
                command = [command '''LowCut'', ' num2str(out_mps.LowCut) ', '];
            elseif (out_mps.LowCut ~= in_mps.LowCut)  % different from prior
                command = [command '''LowCut'', ' num2str(out_mps.LowCut) ', '];
            end
        end

        % HighCut
        if (~isempty(out_mps.HighCut))  % not default
            if (isempty(in_mps.HighCut)) % (all comparisons with empty are false)
                command = [command '''HighCut'', ' num2str(out_mps.HighCut) ', '];
            elseif (out_mps.HighCut ~= in_mps.HighCut)  % different from prior
                command = [command '''HighCut'', ' num2str(out_mps.HighCut) ', '];
            end
        end

        % Scale
        if (out_mps.Scale ~= 1)  % not default
            if (out_mps.Scale ~= in_mps.Scale)  % different from prior
                if (out_mps.Scale == round(out_mps.Scale))
                    command = [command '''Scale'', ' num2str(out_mps.Scale, '%0.1') ', '];
                else
                    command = [command '''Scale'', ' num2str(out_mps.Scale) ', '];
                end
            end
        end

        % Format
        if (format ~= 1)  % not default
            if (strcmp(out_mps.Format, in_mps.Format) == false)  % different from prior
                command = [command '''Format'', ''' out_mps.Format ''', '];
            end
        end
        
        % Padding
        if (padding ~= 1)  % not default
            if (strcmp(out_mps.Padding, in_mps.Padding) == false)  % different from prior
                command = [command '''Padding'', ''' out_mps.Padding ''', '];
            end
        end
        
        % Interpolation
        if (interpolation ~= 1)  % not default
            if (strcmp(out_mps.Interp, in_mps.Interp) == false)  % different from prior
                command = [command '''Interp'', ''' out_mps.Interp ''', '];
            end
        end
        
        % BinInterpolation
        if (binterpolation ~= 1)  % not default
            if (strcmp(out_mps.Binterp, in_mps.Binterp) == false)  % different from prior
                command = [command '''Binterp'', ''' out_mps.Binterp ''', '];
            end
        end
        
        % Persistence
        if (persistence ~= 1)  % not default
            if (strcmp(out_mps.Persist, in_mps.Persist) == false)  % different from prior       
                command = [command '''Persist'', ''' out_mps.Persist ''', '];
            end
        end
        
        % Detrend
        if (out_mps.Detrend == true)  % not default
            if (out_mps.Detrend ~= in_mps.Detrend)  % different from prior      
                command = [command '''Detrend'', true, '];
            end
        end

        % Ranges
        if (out_mps.Ranges == true)  % not default
            if (out_mps.Ranges ~= in_mps.Ranges)  % different from prior      
                command = [command '''Ranges'', true, '];
            end
        end

        % Extrema
        if (out_mps.Extrema == true)  % not default
            if (out_mps.Extrema ~= in_mps.Extrema)  % different from prior      
                command = [command '''Extrema'', true, '];
            end
        end

        % Records
        if (out_mps.Records == true)  % not default
            if (out_mps.Records ~= in_mps.Records)  % different from prior      
                command = [command '''Records'', true, '];
            end
        end

        % Contigua
        if (out_mps.Contigua == true)  % not default
            if (out_mps.Contigua ~= in_mps.Contigua)  % different from prior      
                command = [command '''Contigua'', true, '];
            end
        end

        % ChanNames
        if (out_mps.ChanNames == true)  % not default
            if (out_mps.ChanNames ~= in_mps.ChanNames)  % different from prior      
                command = [command '''ChanNames'', true, '];
            end
        end

        % ChanFreqs
        if (out_mps.ChanFreqs == true)  % not default
            if (out_mps.ChanFreqs ~= in_mps.ChanFreqs)  % different from prior      
                command = [command '''ChanFreqs'', true, '];
            end
        end
      
        command = [command(1:(end - 2)) ');'];  % get rid of terminal ', ' & add closure
        fprintf(2, '\nExecuted:\n');
        disp(command);        

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

end  % matrix_MED_GUI()

