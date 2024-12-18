

function read_MED_GUI()

    % read_MED_GUI()
    % GUI for read_MED()
       
    % Copyright Dark Horse Neuro, 2021

    % Defaults (user can change these)
    DEFAULT_DATA_DIRECTORY = pwd;
    DEFAULT_PASSWORD = 'L2_password';  % example_data password == 'L1_password' or 'L2_password'
    DEFAULT_VARIABLE_NAME = 'slice';
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

    
    % Constants
    MAX_PASSWORD_LENGTH = 15;

    MIN_F_LEFT = 1;
    MIN_F_BOTTOM = 1;
    MIN_F_WIDTH = 540;
    MIN_F_HEIGHT = 700;
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
    persist_str = 'none';
    persist_num = 0;


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
        'String', 'Start:', ...
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
        'String', 'End:', ...
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
    referenceChannelLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 310)], ...
        'String', ['Indices' newline 'Channel:'], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 310) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off');

    % Use Selected (channel to use as index reference) Pushbutton (link to top-right, figure coords)
    useSelectedPushbutton = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'String', 'Use Selected', ...
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 345) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Visible', 'off', ...
        'Callback', @useSelectedPushbuttonCallback);
    
    % Samples as Singles Checkbox (link to top-right, figure coords)
    samplesAsSinglesCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Samples as Singles', ...
        'Value', 0, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 391) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
    
    % Metadata Checkbox (link to top-right, figure coords)
    metadataCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Metadata', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 417) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
    
    % Records Checkbox (link to top-right, figure coords)
    recordsCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Records', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 443) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
    
    % Contigua Checkbox (link to top-right, figure coords)
    contiguaCheckbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Contigua', ...
        'Value', 1, ...
        'Position', [(faxRight - TEXT_BOX_WIDTH - 3) (faxTop - 468) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', panelColor, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');
    
    % Persistence Mode Label (link to top-right, figure coords)
    persistenceModeLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 511)], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 518) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

    % Variable Name Label (link to top-right, axis coords)
    variableNameLabel = text('Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 555)], ...
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
        'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 555) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT], ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left');

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
 
        % Variable Name Label (link to top-right, axis coords)
        set(variableNameLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 494)]);
    
        % Variable Name Textbox (link to top-right, figure coords)
        set(variableNameTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 494) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
        
        % Reference Channel Label (link to top-right, axis coords)
        set(referenceChannelLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 320)]);
    
        % Reference Channel Textbox (link to top-right, figure coords)
        set(referenceChannelTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 310) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

        % Use Selected Pushbutton (link to top-right, figure coords)
        set(useSelectedPushbutton, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 345) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
        
        % Samples as Singles Checkbox (link to top-right, figure coords)
        set(samplesAsSinglesCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 391) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]); 
        
        % Metadata Checkbox (link to top-right, figure coords)
        set(metadataCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 417) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
      
        % Records Checkbox (link to top-right, figure coords)
        set(recordsCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 443) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]); 
      
        % Contigua Checkbox (link to top-right, figure coords)
        set(contiguaCheckbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 469) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);
 
        % Persistence Mode Label (link to top-right, figure coords)
        set(persistenceModeLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 511)]);

        % Persistence Mode Dropdown (link to top-right, figure coords)
        set(persistenceModeDropdown, 'Position', [(faxRight - TEXT_BOX_WIDTH - 7) (faxTop - 518) (TEXT_BOX_WIDTH + 16) TEXT_BOX_HEIGHT]);

        % Variable Name Label (link to top-right, figure coords)
        set(variableNameLabel, 'Position', [(axRight - TEXT_BOX_WIDTH - 10) (axTop - 555)]);

        % Variable Name Textbox (link to top-right, figure coords)
        set(variableNameTextbox, 'Position', [(faxRight - TEXT_BOX_WIDTH) (faxTop - 555) TEXT_BOX_WIDTH TEXT_BOX_HEIGHT]);

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
            set(relativeRadiobutton, 'Enable', 'on');            
            set(referenceChannelTextbox, 'Visible', 'off');
            set(useSelectedPushbutton, 'Visible', 'off');
            set(referenceChannelLabel, 'Visible', 'off');
        else
            set(relativeRadiobutton, 'Enable', 'off');
            absoluteRadiobutton.Value = true;
            relativeRadiobutton.Value = false;
            set(referenceChannelTextbox, 'Visible', 'on');
            set(useSelectedPushbutton, 'Visible', 'on');
            set(referenceChannelLabel, 'Visible', 'on');
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


    % Password Textbox
    function passwordTextboxCallback(~, ~)
        if (length(passwordTextbox.String) > MAX_PASSWORD_LENGTH)
            errordlg('Password is too long', 'Read MED GUI');
            passwordTextbox.String = '';
        end
        specsChanged = true;
    end  % passwordTextboxCallback()


    % Export to Workspace Pushbutton
    function exportToWorkspacePushbuttonCallback(~, ~)
        if specsChanged == true
           success = get_data();
           if success == false
                return;
            end
        end
        
        assignin('base', variableNameTextbox.String, slice);
        msgbox('Data Exported');
    end  % exportToWorkspacePushbuttonCallback()


    % Plot Pushbutton
    function plotPushbuttonCallback(~, ~)
        if specsChanged == true
            success = get_data();
            if success == false
                return;
            end
        end
        
        plot_MED(slice);
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


    function [success] = get_data()
        set(fig, 'Pointer', 'watch');
        pause(0.1);  % let cursor change (if no pause it gets into mex function before it cann switch it)

        clear slice;
        success = false;
        file_list = get_file_list();
        if (numel(file_list) == 0)
            set(fig, 'Pointer', 'arrow');
            errordlg('No MED session or channels are specified', 'Read MED GUI');
            return;
        end
        start_lim = convert_limits(startTextbox.String);
        end_lim = convert_limits(endTextbox.String);
        if (timesRadiobutton.Value == true)
            start_time = start_lim;
            end_time = end_lim;
            start_index = [];
            end_index = [];
        else
            start_time = [];
            end_time = [];
            start_index = start_lim;
            end_index = end_lim;
        end
        
        if (samplesAsSinglesCheckbox.Value == true)
            samps_as_singles = 'true';
        else
            samps_as_singles = 'false';
        end

        if (metadataCheckbox.Value == true)
            metadata = 'true';
        else
            metadata = 'false';
        end

        if (recordsCheckbox.Value == true)
            records = 'true';
        else
            records = 'false';
        end

        if (contiguaCheckbox.Value == true)
            contigua = 'true';
        else
            contigua = 'false';
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
        
        if (SHOW_READ_MED_COMMAND == true)
            show_command(file_list);
        end

        slice = read_MED(file_list, start_time, end_time, start_index, end_index, passwordTextbox.UserData.password, referenceChannelTextbox.String, samps_as_singles, persist_num, metadata, records, contigua);

        if islogical(slice)  % can be true, false, or structure
            if slice == false
                set(fig, 'Pointer', 'arrow');
                errordlg('read_MED() error', 'Read MED GUI');
                success = false;
                return;
            end
        end
        success = true;            
        specsChanged = false;

        set(fig, 'Pointer', 'arrow');
    end  % get_data()


    function show_command(file_list)   
        
        % build 'file_list' string
        n_files = numel(file_list);
        fl = [newline 'file_list = {'];

        for i = 1:(n_files - 1)
            fl = [fl '''' char(file_list(i)) ''', '];
        end
        fl = [fl '''' char(file_list(n_files)) '''};' newline];

        v = evalin('base', 'logical(exist(''file_list'', ''var''))');
        if (v == true)
            fprintf(2, '\nFile list variable exists in workspace. To replace, execute: ');
        else
            evalin('base', fl);
            fprintf(2, '\nExecuted: ');
        end
        disp(fl);
    
        % build command string
        command = '';
        last_default_set = false;

        default_val = true;
        if (contiguaCheckbox.Value == true)
            contigua = '[]';
        else
            contigua = 'false';
            default_val = false;
        end
        if (last_default_set == true)
            command = [command ', ' contigua];
        elseif (default_val == false)
            command = contigua;
            last_default_set = true;
        end 

        default_val = true;
        if (recordsCheckbox.Value == true)
            records = '[]';
        else
            records = 'false';
            default_val = false;
        end
        if (last_default_set == true)
            command = [records ', ' command];
        elseif (default_val == false)
            command = records;
            last_default_set = true;
        end 

        default_val = true;
        if (metadataCheckbox.Value == true)
            metadata = '[]';
        else
            metadata = 'false';
            default_val = false;
        end
        if (last_default_set == true)
            command = [metadata ', ' command];
        elseif (default_val == false)
            command = metadata;
            last_default_set = true;
        end 

        default_val = true;
        if (persist_num > 0)
            persist_str = ['''' persist_str ''''];
            default_val = false;
        else
            persist_str = '[]';
        end 
        if (last_default_set == true)
            command = [persist_str ', ' command];
        elseif (default_val == false)
            command = persist_str;
            last_default_set = true;
        end 

        default_val = true;
        if (samplesAsSinglesCheckbox.Value == true)
            samps_as_singles = 'true';
            default_val = false;
        else
            samps_as_singles = '[]';
        end 
        if (last_default_set == true)
            command = [samps_as_singles ', ' command];
        elseif (default_val == false)
            command = samps_as_singles;
            last_default_set = true;
        end 

        default_val = true;
        if (isempty(referenceChannelTextbox.String))
            ref_chan_str = '[]';
        elseif (get(referenceChannelTextbox, 'Visible') == false)
            ref_chan_str = '[]';
        else
            ref_chan_str = ['''' referenceChannelTextbox.String ''''];
            default_val = false;
        end
        if (last_default_set == true)
            command = [ref_chan_str ', ' command];
        elseif (default_val == false)
            command = ref_chan_str;
            last_default_set = true;
        end

        if (isempty(passwordTextbox.UserData.password))
            password_str = '[]';
        else
            password_str = '''<password>''';
            default_val = false;
        end
        if (last_default_set == true)
            command = [password_str ', ' command];
        elseif (default_val == false)
            command = password_str;
            last_default_set = true;
        end

        default_val = true;
        if (strcmp(startTextbox.String, 'start') == true)
            start_str = '[]';
        else
            start_str = startTextbox.String;
            default_val = false;
        end
        if (strcmp(endTextbox.String, 'end') == true)
            end_str = '[]';
        else
            end_str = endTextbox.String;
            default_val = false;
        end

        if (timesRadiobutton.Value == true)
            start_time = start_str;
            end_time = end_str;
            if (last_default_set == true)
                command = [start_time ', ' end_time ', [], [], ' command];
            elseif (default_val == false)
                command = [start_time ', ' end_time];
                last_default_set = true;
            end 
        else
            start_index = start_str;
            end_index = end_str;
            if (last_default_set == true)
                command = ['[], [], ' start_index ', ' end_index ', ' command];
            elseif (default_val == false)
                command = ['[], [], ' start_index ', ' end_index];
                last_default_set = true;
            end 
        end

        if (last_default_set == true)
            command = [variableNameTextbox.String ' = read_MED(file_list, ' command  ');' newline];
        else
            command = [variableNameTextbox.String ' = read_MED(file_list);' newline];
        end

        fprintf(2, 'Executed: \n');
        disp(command);

    end  % show_command()


    function [file_list] = get_file_list()
        chan_names = channelListbox.String;
        
        if (sessionSelected == true)
            file_list{1} = sessionDirectory;
            return;
        end

        n_chans = length(chan_names);
        file_list = cell(n_chans, 1);
        for i = 1:n_chans
            file_list{i} = [sessionDirectory DIR_DELIM char(channelListbox.String(i)) '.ticd'];
        end
    end  % get_file_list()

end  % read_MED_GUI()

