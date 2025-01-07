function view_MED(varargin)

    %   view_MED([page], [password])
    %
    %   Copyright Dark Horse Neuro, 2021

    % ------------ Defaults ---------------
    
    DEFAULT_PASSWORD = 'L2_password';  % DHN example data passwords: 'L1_password' & 'L2_password'
    DEFAULT_DATA_DIRECTORY = pwd;
    DEFAULT_WINDOW_USECS = 1e7;  % 10 seconds

    % ---------- Clean mex slate ----------

    evalin('base', 'clear load_session matrix_MED_exec read_MED_exec add_record_exec delete_record_exec')

    % ------------ GUI layout -------------

    OS = computer;
    DIR_DELIM = '/';
    switch OS
        case 'MACI64'       % MacOS, Intel
            SYS_FONT_SIZE = 10;
        case 'MACA64'       % MacOS, Apple Silicon
            SYS_FONT_SIZE = 10;
        case 'GLNXA64'      % Linux
            SYS_FONT_SIZE = 7;
        case 'PCWIN64'      % Windows
            SYS_FONT_SIZE = 8;
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
        fprintf(2, '%s', msg);  % 2 == stderr, so prints in red in command window
    end

    % set up default records settings
    REC_NUM_TYPES = 5;
    REC_HFOc_IDX = 1;
    REC_NlxP_IDX = 2;
    REC_Note_IDX = 3;
    REC_Seiz_IDX = 4;
    REC_Sgmt_IDX = 5;
    rec_settings = cell(REC_NUM_TYPES, 1);

    % set colors
    LIGHT_BLUE = [0.30 0.75 0.93]; %#ok<NASGU>
    DARK_BLUE = [0.0 0.45 0.74];
    DARK_ORANGE = [0.91 0.45 0.05];
    DARK_YELLOW = [0.93 0.69 0.13];
    DARK_PURPLE = [0.49 0.18 0.56];
    LIGHT_GREEN = [0.0 0.70 0.0];
    MEDIUM_GREEN = [0.47 0.67 0.19]; %#ok<NASGU>
    DARK_GREEN = [0.0 0.45 0.0];
    DARK_RED = [0.63 0.08 0.18];
    LIGHT_GRAY = [0.9 0.9 0.9];
    MEDIUM_GRAY = [0.45 0.45 0.45]; %#ok<NASGU>
    DARK_GRAY = [0.25 0.25 0.25];

    % get settings path
    settings_path = which('view_MED');
    len = length(settings_path) - 10;  % 'view_MED.m'
    settings_path = settings_path(1:len);
    settings_path = [settings_path '.vm_settings'];
    rec_d = [];  % records dialog (global so can leave open)
    read_rec_settings();

    % set up matrix parameter structure
    mps = matrix_MED('numeric');
    mps.Persist = 4;  % read
    mps.Detrend = 1;
    mps.Contigua = 1;
    mps.ChanFreqs = 1;

    FORWARD = 1;
    BACKWARD = 2;

    panel_color = get(0,'DefaultUicontrolBackgroundColor');
    pix_per_cm = get(groot, 'ScreenPixelsPerInch') / 2.54;
    points_per_cm = 28.3465;
    points_per_pix = points_per_cm / pix_per_cm;
    label_font_size = SYS_FONT_SIZE;

    logo = imread([RESOURCES DIR_DELIM 'Dark Horse Neuro Logo.png']);
    [neh_signal, neh_sf] = audioread([RESOURCES DIR_DELIM 'neh.wav']);
    neh_signal = neh_signal / 4;
    scale = 1;
    uV_per_cm = 1;
    screen_size = get(groot, 'ScreenSize');
    screen_x_pix = screen_size(3);
    screen_y_pix = screen_size(4);
    sess_map_ax_height = 11;
    sess_map_ax_width = 0;
    data_ax_width = 0;
    data_ax_height = 0;
    data_ax_left = 0;
    data_ax_right = 0;
    data_ax_bot = 0;
    data_ax_top = 0;
    export_num = 0;
    ax_mouse_down = false;
    any_interaction = false;

    % Figure
    fig = figure('Units', 'pixels', ...
        'Position',[100 50 (screen_x_pix - 200) (screen_y_pix - 150)], ...
        'HandleVisibility', 'callback', ...
        'IntegerHandle', 'off', ... 
        'Toolbar', 'none', ...
        'MenuBar', 'none', ...
        'NumberTitle', 'off', ...
        'Visible', 'off', ...
        'Interruptible', 'on', ...
        'BusyAction', 'cancel',  ...
        'KeyPressFcn', @key_press_callback, ...
        'KeyReleaseFcn', @key_release_callback, ...
        'CloseRequestFcn', @figure_close_callback, ...
        'ResizeFcn', @resize, ...
        'WindowButtonDownFcn', @ax_mouse_down_callback, ...
        'WindowButtonUpFcn', @ax_mouse_up_callback);

    % Data Axes
    data_ax = axes('Parent', fig, ...
        'Units', 'pixels', ...
        'TickDir', 'out', ...
        'TickLength',[.005, 0], ...
        'Box', 'off', ...
        'XTick', [], ...
        'YTick', [], ...
        'XLimMode', 'manual', ...
        'YLimMode', 'manual', ...
        'YDir', 'reverse');
    colors = get(data_ax, 'ColorOrder');
    n_colors = size(colors, 1);
    mono_color = colors(1, :);
    
    % X Tick Units Label
    x_tick_units_label = uicontrol(fig, ...
        'Style', 'text', ...
        'Units', 'pixels', ...
        'String', 'secs:', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right');

    % Axis Time Strings
    time_str_ax = axes('Parent', fig, ...
        'Units', 'pixels', ...
        'XTick', [], ...
        'YTick', [], ...
        'Visible', 'off', ...
        'XLimMode', 'manual', ...
        'YLimMode', 'manual', ...
        'ButtonDownFcn', @sess_map_callback);

    axis_start_time_string = text(time_str_ax, ...
        'Units', 'pixels', ...
        'String', '', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontAngle', 'italic', ...
        'HorizontalAlignment', 'left', ...
        'ButtonDownFcn', @axis_time_callback);

    axis_end_time_string = text(time_str_ax, ...
        'Units', 'pixels', ...
        'String', '', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontAngle', 'italic', ...
        'HorizontalAlignment', 'right', ...
        'ButtonDownFcn', @axis_time_callback);

    % Label Axes
    label_ax = axes('Parent', fig, ...
        'Units', 'pixels', ...
        'Color', panel_color, ...
        'XTick', [], ...
        'YTick', [], ...
        'Visible', 'off', ...
        'XLimMode', 'manual', ...
        'YLimMode', 'manual');
    
    % Session Map Axes    
    sess_map_ax = axes('Parent', fig, ...
        'Units', 'pixels', ...
        'TickLength',[0 0], ...
        'XTick', [], ...
        'YTick', [], ...
        'Box', 'on', ...
        'Color', 'white', ...
        'XLimMode', 'manual', ...
        'YLimMode', 'manual', ...
        'ButtonDownFcn', @sess_map_callback);
        
    % Logo Axes    
    logo_ax = axes('Parent', fig, ...
        'Units', 'pixels', ...
        'XLim', [1, 190], ...
        'XLim', [1, 72], ...
        'Color', panel_color, ...
        'XTick', [], ...
        'YTick', [], ...
        'visible', 'off', ...
        'XLimMode', 'manual', ...
        'YLimMode', 'manual'); 

    % Page Movement Buttons
    forward_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', '=>', ...
        'FontSize', SYS_FONT_SIZE + 4, ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'ButtonDownFcn', @page_movement_callback, ...
        'KeyPressFcn', @key_press_callback, ...
        'KeyReleaseFcn', @key_release_callback, ...
        'Callback', @page_movement_callback);

    back_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', '<=', ...
        'FontSize', SYS_FONT_SIZE + 4, ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'ButtonDownFcn', @page_movement_callback, ...
        'KeyPressFcn', @key_press_callback, ...
        'KeyReleaseFcn', @key_release_callback, ...
        'Callback', @page_movement_callback);

    % Antialias Button
    antialias_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'String', 'Antialiasing is On', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Callback', @antialias_callback);
    
    % Autoscale Button
    autoscale_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Autoscaling is On', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Callback', @autoscale_callback);

    % Add Record Button (note this is a toggle button)
    add_record_button = uicontrol(fig, ...
        'Style', 'togglebutton', ...
        'Units', 'pixels', ...
        'String', 'Add Record', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Callback', @add_record_callback);

    % Export Data button
    export_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Export to Workspace', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Callback', @export_callback);

    % Baseline Correction Button
    baseline_correct_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Baseline Correction is On', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Callback', @baseline_callback);

    % Gain Textbox & Label
    gain_textbox_label = uicontrol(fig, ...
        'Style', 'text', ...
        'Units', 'pixels', ...
        'String', 'µV/cm:', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right');
    
    gain_textbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Units', 'pixels', ...
        'String', '', ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'left', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'KeyPressFcn', @key_press_callback, ...
        'Callback', @gain_callback);

    % Timebase Textbox & Label
    timebase_textbox_label = uicontrol(fig, ...
        'Style', 'text', ...
        'Units', 'pixels', ...
        'String', 's/page:', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right');
    
    timebase_textbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Units', 'pixels', ...
        'String', '', ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'left', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'KeyPressFcn', @key_press_callback, ...
        'Callback', @timebase_callback);

    % Current Time Textbox & Label
    current_time_textbox_label = uicontrol(fig, ...
        'Style', 'text', ...
        'Units', 'pixels', ...
        'String', 'page (s):', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'right');
    
    current_time_textbox = uicontrol(fig, ...
        'Style', 'edit', ...
        'Units', 'pixels', ...
        'String', '', ...
        'BackgroundColor', 'white', ...
        'FontSize', SYS_FONT_SIZE, ...
        'HorizontalAlignment', 'left', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'KeyPressFcn', @key_press_callback, ...
        'Callback', @current_time_callback);

    % Amplitude Direction Button
    amplitude_direction_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Negative is Up', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @amplitude_direction_callback);
    
    % View Selected Button
    view_selected_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'View Selected', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Enable', 'off', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @view_selected_callback);

    % Remove Selected Button
    remove_selected_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Remove Selected', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Enable', 'off', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @view_selected_callback);

    % Add Channels Button
    add_channels_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Add Channels', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @add_channels_callback);

    % Deselect All Button
    deselect_all_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Deselect All', ...
        'FontSize', SYS_FONT_SIZE, ...
        'Visible', 'off', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @deselect_all_callback);

    % Records Checkbox
    records_checkbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'Value', 0, ...
        'BackgroundColor', panel_color, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @records_callback);

    % Records Button
    records_button = uicontrol(fig, ...
        'Style', 'pushbutton', ...
        'Units', 'pixels', ...
        'String', 'Records', ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @records_button_callback);

    % Trace Ranges Checkbox
    ranges_checkbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Trace Ranges', ...
        'Value', 0, ...
        'BackgroundColor', panel_color, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @trace_ranges_callback);

    % Monochrome Checkbox
    monochrome_checkbox = uicontrol(fig, ...
        'Style', 'checkbox', ...
        'String', 'Monochrome', ...
        'Value', 0, ...
        'BackgroundColor', panel_color, ...
        'FontSize', SYS_FONT_SIZE, ...
        'FontName', 'FixedWidth', ...
        'HorizontalAlignment', 'left', ...
        'Interruptible', 'off', ...
        'BusyAction', 'cancel', ...
        'Callback', @monochrome_callback);


    % ----------- Startup ----------------
    
    wind_usecs = DEFAULT_WINDOW_USECS;
    x_ax_inds = [];
    x_tick_inds = [];
    full_page_width = [];
    plot_handles = [];
    logo_handle = [];
    sess = [];
    password = DEFAULT_PASSWORD;
    chan_paths = {};
    mps.Data = {};
    chan_labels = {};
    x_tick_labels = {};
    label_ax_width = 0;
    raw_page = [];
    movement_direction = FORWARD;
    antialiased_channels = 0;
    raw_page = [];
    range_patches = [];
    discont_lines = [];
    record_lines = [];
    sess_map_records_lines = [];
    currently_plotting = false;
        
    % Parse inputs
    if (nargin >= 1)
        sess = varargin{1};
        if (nargin == 2)
            password = varargin{2};
            if isstring(password)
                password = char(password);
                mps.Pass = password;
            end
        end
    end
    
    if (isempty(sess))
        startup_sess_passed = false;
        filters = {'medd', 'ticd'};
        stop_filters = {'ticd'};
        [chan_list, sess_dir] = directory_chooser(filters, DEFAULT_DATA_DIRECTORY, stop_filters);
        if (isempty(chan_list))
            errordlg('No MED data selected', 'View MED');
            return;
        end
        n_chans = numel(chan_list);
        if (n_chans == 1)
            [~, ~, ext] = fileparts(chan_list(1));
            if (strcmp(ext, '.medd') == true)
                sess_dir = [sess_dir DIR_DELIM char(chan_list(1))];
                dir_list = dir([sess_dir DIR_DELIM '*.ticd']);
                if (isempty(dir_list))
                    errordlg('No MED data in selected session', 'View MED');
                    return;
                end
                n_chans = numel(dir_list);
                chan_list = cell(n_chans, 1);
                for ii = 1:n_chans
                    [~, name, ~] = fileparts(dir_list(ii).name);
                    chan_list(ii) = cellstr([name '.ticd']);
                end
                clear dir_list;
            end
        end
        chan_paths = cell(n_chans, 1);
        for ii = 1:n_chans
            chan_paths{ii} = [sess_dir DIR_DELIM chan_list{ii}];
        end
        mps.Data = chan_paths;
        page_start = 0;
        page_end = -DEFAULT_WINDOW_USECS;  % set limits to default page width
    else  % get channels & page limits from passed session
        startup_sess_passed = true;
        n_chans = numel(sess.channels);
        chan_paths = cell(n_chans, 1);
        chan_list = cell(n_chans, 1);
        for ii = 1:n_chans
            chan_paths{ii} = sess.channels(ii).metadata.path;
            chan_list{ii} = sess.channels(ii).name;
        end
        mps.Data = chan_paths;
        page_start = sess.metadata.slice_start_time;
        page_end = sess.metadata.slice_end_time;
        wind_usecs = (page_end - page_start) + 1;
        clear sess;
    end

    % Set up wait pointer timer
    WAIT_POINTER_DELAY = 0.667;  % seconds
    new_page_secs = 0;
    new_page_timer = [];
    potentially_increased_plot_time = false;

    set(fig, 'Pointer', 'watch');
    reset_pointer = true;
    drawnow;
    
    % load session
    [sess, sess_record_times, tmp_disconts] = load_session(chan_paths, password);
    clear load_session;
    if (isempty(sess))
        errordlg('read_MED() error', 'View MED');
        return;
    end

    % get channels & page limits from loaded session (order and limits can differ from request)
    if (startup_sess_passed == false)
        n_chans = numel(sess.channels);
        chan_paths = cell(n_chans, 1);
        chan_list = cell(n_chans, 1);
        for ii = 1:n_chans
            chan_paths{ii} = sess.channels(ii).metadata.path;
            chan_list{ii} = sess.channels(ii).metadata.channel_name;
        end
        mps.Data = chan_paths;
        page_start = sess.metadata.start_time;
        page_end = (page_start + wind_usecs) - 1;
    end

    sess_dir = sess.metadata.path;
    recording_time_offset = sess.metadata.recording_time_offset;
    sess_start = sess.metadata.session_start_time;
    sess_end = sess.metadata.session_end_time;
    sess_duration = double(sess_end - sess_start);
    curr_usec = double(page_start - sess_start);
    set(current_time_textbox, 'String', num2str(double(curr_usec) / 1e6, '%0.6f'));
    set(timebase_textbox, 'String', num2str(double(wind_usecs) / 1e6, '%0.6f'));
    set(fig, 'Name', ['View MED: ' sess.metadata.session_name]);
    clear sess;
    
    NEGATIVE_UP = 1;  % data y axis is inverted
    NEGATIVE_DOWN = -1;
    amplitude_direction = NEGATIVE_UP;
    mps.Detrend = true;
    monochrome_flag = false;
    AA_FILT = 0; % 0 antialias, 1 none  (may offer others in future (DO NOT convert to logical))
    NO_FILT = 1;
    mps.Filt = AA_FILT;  % antialias
    autoscale_flag = true;
    mps.Ranges = false;
    mps.Records = false;
    add_record_flag = false;
    calendar_time_flag = true;
    uUTC_flag = false;
    oUTC_flag = false;
    screen_sf = [];
    shift_pressed = false;
  
    n_discontigua = numel(tmp_disconts);
    if (n_discontigua)
        discontigua = cell(n_discontigua, 1);
        for ii = 1:n_discontigua
            discontigua{ii}.start_time = double(tmp_disconts(ii).start_time);
            discontigua{ii}.end_time = double(tmp_disconts(ii).end_time);
            discontigua{ii}.start_prop = double(tmp_disconts(ii).start_proportion);
            discontigua{ii}.end_prop = double(tmp_disconts(ii).end_proportion);
            % Z coordinate 1's put patch above current page, but below record lines
            discontigua{ii}.patch = patch(sess_map_ax, ...
                [1, 1, 1, 1], [0, sess_map_ax_height, sess_map_ax_height, 0], [1, 1, 1, 1], ...
                DARK_GRAY, 'EdgeColor', 'none', 'ButtonDownFcn', @sess_map_callback);
        end
    end
    clear tmp_disconts;

    % Z coordinate 0's put patch below contigua & record lines
    curr_page_patch = patch(sess_map_ax, ...
        [1, 1, 1, 1], [0, sess_map_ax_height, sess_map_ax_height, 0], [0, 0, 0, 0], ...
        'red', 'EdgeColor', 'none', ...
        'ButtonDownFcn', @sess_map_callback);

    % Make channel labels
    selected_labels = [];
    create_labels();
   
    % Draw window
    currently_resizing = false;
    set(fig, 'Visible', 'on');  % calls resize()
    uicontrol(forward_button);


    % ------------ Support Functions ---------------

    function plot_page(get_new_data)

        if (currently_plotting == true)
            set(fig, 'Pointer', 'watch'); 
            reset_pointer = true;
            return;
        end
        currently_plotting = true;

        % start new page timer
        new_page_timer = tic;
        if (new_page_secs > WAIT_POINTER_DELAY || potentially_increased_plot_time == true)
            potentially_increased_plot_time = false;
            set(fig, 'Pointer', 'watch'); 
            drawnow;
            reset_pointer = true;
        end

        % raw_page, mins, maxs have decimation, detrending, & filtering (no offsetting, scaling, or inversion)
        if (get_new_data == true)

            % get new data
            set_page_limits();
            screen_sf = (full_page_width * double(1e6)) / double(wind_usecs);
            mps.Start = page_start;
            mps.End = page_end;
            raw_page = matrix_MED_exec(mps);
            if (isempty(raw_page))
                errordlg('Error reading data', 'View MED');
                return;
            end

            % get returned page times (may differ from requested)
            page_start = raw_page.slice_start_time;
            page_end = raw_page.slice_end_time;

            % set time strings
            curr_usec = double(page_start - sess_start);
            set(current_time_textbox, 'String', num2str(double(curr_usec) / 1e6, '%0.6f'));
            if calendar_time_flag == true
                set(axis_start_time_string, 'String', raw_page.slice_start_time_string);
                set(axis_end_time_string, 'String', raw_page.slice_end_time_string);
            elseif uUTC_flag == true
                set(axis_start_time_string, 'String', ['start µUTC: ' num2str(page_start + recording_time_offset)]);
                set(axis_end_time_string, 'String', ['end µUTC: ' num2str(page_end + recording_time_offset)]);
            elseif oUTC_flag == true
                set(axis_start_time_string, 'String', ['start oUTC: ' num2str(page_start)]);
                set(axis_end_time_string, 'String', ['end oUTC: ' num2str(page_end)]);
            end
            
            % see if any channels were antialiased
            antialiased_channels = 0;
            if (mps.Filt == AA_FILT)
                for i = 1:n_chans
                    if (screen_sf < raw_page.channel_sampling_frequencies(i))
                        antialiased_channels = antialiased_channels + 1;
                    end
                end
            end
            if (antialiased_channels)
                    aa_str = ['Antialiasing at ' num2str(screen_sf / 4, '%0.0f') ' Hz'];
                    if (antialiased_channels < n_chans)
                        aa_str = [aa_str '*'];  % asterisk indicates not all channels were filtered
                    end
                    set(antialias_button, 'String', aa_str);              
            else
                    set(antialias_button, 'String', 'Antialiasing is Off');            
            end

        end  % end get_new_data
  
        % page, mins, maxs have scaling, inversion, & offsetting (use copy)
        page = raw_page.samples;
        if (mps.Ranges == true)
            mins = raw_page.range_minima;
            maxs = raw_page.range_maxima;
        end       

        % subtract trace means to keep highly offset traces on screen
        if (mps.Detrend == false)
            for i = 1:n_chans
                tr_mn = mean(page(:, i));
                page(:, i) = page(:, i) - tr_mn;
                if (mps.Ranges == true)
                    mins(:, i) = mins(:, i) - tr_mn;
                    maxs(:, i) = maxs(:, i) - tr_mn;
                end
            end
        end

        % scale (+/- invert)
        pix_per_trace = data_ax_height / (n_chans + 1);
        if (autoscale_flag == true)
             % Matlab quantile() requires Statistics and Machine Learning Toolbox
            if (mps.Ranges == false)
                q = local_quantile(page, [0.01, 0.99]);
            else
                q(1) = local_quantile(mins, 0.01);
                q(2) = local_quantile(maxs, 0.99);
            end
            magnitude = q(2) - q(1);
            if (magnitude < 1)
                magnitude = 1;
            end
            scale = (pix_per_trace / magnitude);
        else
            scale = abs(scale);
        end   
        uV_per_cm = pix_per_cm / scale;
        set(gain_textbox, 'String', num2str(uV_per_cm, '%0.0f'));
        scale = scale * amplitude_direction;
        page = page * scale;
        if (mps.Ranges == true)
            mins = mins * scale;
            maxs = maxs * scale;
        end

        % offset traces (in plot window)
        pix_per_trace = (data_ax_height - 4) / n_chans;
        offset = (pix_per_trace / 2) + 2;
        for i = 1:n_chans
            r_offset = round(offset);
            page(:, i) = page(:, i) + r_offset;
            if (mps.Ranges == true)
                mins(:, i) = mins(:, i) + r_offset;
                maxs(:, i) = maxs(:, i) + r_offset;
            end
            offset = offset + pix_per_trace;
        end

        % plot
        if (isempty(plot_handles))
            cla(data_ax);
            offset = (pix_per_trace / 2) + 2;
            y_tick_inds = zeros(n_chans, 1);
            for i = 1:n_chans
                y_tick_inds(i) = round(offset);
                offset = offset + pix_per_trace;
            end
            hold(data_ax, 'on');
            set(data_ax, 'Xlim', [1, data_ax_width], 'Ylim', [1, data_ax_height], 'XTickLabels', x_tick_labels, 'YTickLabels', [], 'XTick', x_tick_inds, 'YTick', y_tick_inds);
            if (monochrome_flag == false)
                plot_handles = plot(data_ax, x_ax_inds, page);
            else
                plot_handles = plot(data_ax, x_ax_inds, page, 'Color', mono_color);
            end
            line(data_ax, [1, data_ax_width, data_ax_width, 1, 1], [1, 1, data_ax_height, data_ax_height, 1], 'Color', 'k');
            hold(data_ax, 'off');
        else
            for i = 1:n_chans
                set(plot_handles(i), 'YData', page(:, i));
            end
        end
        
        % clear old trace range patches
        if (~isempty(range_patches))
            for i = 1:numel(range_patches)
                delete(range_patches{i});
            end
            range_patches = [];
        end

        % draw trace range patches
        if (mps.Ranges == true)
            range_patches = cell(n_chans, 1);
            for i = 1:n_chans
                    patch_x = [(x_ax_inds(1):x_ax_inds(end))' ; (x_ax_inds(end):-1:x_ax_inds(1))'];
                    patch_y = [mins(:, i) ; flipud(maxs(:, i))];
                    patch_z = ones((2 * full_page_width), 1) * -1;
                    range_patches{i} = patch(data_ax, patch_x, patch_y, patch_z, LIGHT_GRAY, 'EdgeColor', 'none');
            end
        end

        % clear old discontinuity lines
        if (~isempty(discont_lines))
            for i = 1:numel(discont_lines)
                delete(discont_lines{i}.line);
                delete(discont_lines{i}.zag);
            end
            discont_lines = [];
        end

        % draw discontinuity lines
        n_contigua = numel(raw_page.contigua);
        if (n_contigua > 1)
            discont_lines = cell(n_contigua - 1, 1);
            for i = 2:n_contigua
                sess_map_x = double(raw_page.contigua(i).start_index) + (x_ax_inds(1) - 1);
                discont_lines{i - 1}.line = line(data_ax, [sess_map_x, sess_map_x], [20 data_ax_height], 'color', DARK_GRAY, 'LineWidth', 2, 'LineStyle', '--');
                patch_x = [-1, 11, 2, 7, -3, 2, -6, -1] + sess_map_x;
                patch_y = [1, 1, 5, 10, 15, 10, 5, 1];
                discont_lines{i - 1}.zag = patch(data_ax, patch_x, patch_y, DARK_GRAY, 'EdgeColor', 'none', 'ButtonDownFcn', @discont_line_callback);
            end
        end

        % clear old record lines
        if (~isempty(record_lines))
            for i = 1:numel(record_lines)
                if (~isempty(record_lines{i}))
                    delete(record_lines{i}.line);
                    delete(record_lines{i}.flag);
                end
            end
            record_lines = [];
        end

        % draw record lines
        if (mps.Records == true)
            record_lines = cell(numel(raw_page.records), 1);
            for i = 1:numel(record_lines)
                sess_map_x = double(raw_page.records{i}.start_index) + (x_ax_inds(1) - 1);
                code = raw_page.records{i}.type_code;
                for j = 1:REC_NUM_TYPES
                    if (rec_settings{j}.display == false)
                        continue;
                    end
                    if (code == rec_settings{j}.type_code)
                        col = rec_settings{j}.color;
                        record_lines{i}.line = line(data_ax, [sess_map_x, sess_map_x], [20 data_ax_height], 'color', col , 'LineWidth', 2, 'LineStyle', '--');
                        patch_x = [-1, 19, 14, 19, -1, -1] + sess_map_x;
                        patch_y = [1, 1, 7, 14, 14, 1];
                        record_lines{i}.flag = patch(data_ax, patch_x, patch_y, col, 'EdgeColor', 'none', 'ButtonDownFcn', @rec_line_callback);
                        break;
                    end
                end
                
            end
        end

        % update current page patch on session map
        window_offset = double(page_start - sess_start);
        sess_map_start_x = round(data_ax_width * (window_offset / sess_duration));
        window_offset = double(page_end - sess_start);
        sess_map_end_x = round(data_ax_width * (window_offset / sess_duration));
        if (sess_map_end_x == sess_map_start_x)
            sess_map_end_x = sess_map_start_x + 1;
        end   
        set(curr_page_patch, 'XData', [sess_map_start_x, sess_map_start_x, sess_map_end_x, sess_map_end_x]);

        % stop new page timer
        if (reset_pointer == true)
            set(fig, 'Pointer', 'arrow');
            drawnow;
            reset_pointer = false;
        end
        new_page_secs = toc(new_page_timer);
        currently_plotting = false;

    end  % end plot_page()

    function set_page_limits()
        if ((page_start + wind_usecs) >= sess_end)
            page_start = (sess_end - wind_usecs) + 1;
            movement_direction = BACKWARD;
        elseif (page_start < sess_start)
            page_start = sess_start;
            movement_direction = FORWARD;
        end

        if (n_discontigua == 0)
            page_end = (page_start + wind_usecs) - 1;
            return;
        end

        break_flag = false;
        for i = 1:n_discontigua
            if (page_start <= discontigua{i}.end_time)
                break_flag = true;  % loop exits with i == n_discontigua whether or not this was true
                break;
            end
        end
        if (break_flag == true)
            if (page_start >= discontigua{i}.start_time)  % inside discontiguon
                if (movement_direction == FORWARD)
                    page_start = discontigua{i}.end_time + 1;
                else  % movement_direction == BACKWARD
                    td = (discontigua{i}.end_time - page_start) + 1;
                    page_start = discontigua{i}.start_time - td;
                    set_page_limits();  % recurse - could have entered another discontiguon
                end
            end
        end
        page_end = (page_start + wind_usecs) - 1;

        % get contiguous usecs to page_end from page_start (data request is for relative time)
        if (break_flag == true) 
            if (page_end >= discontigua{i}.start_time) % there is at least one discontiguon between page_start & page_end
                discont_usecs = 0;
                for j = i:n_discontigua
                    if (discontigua{j}.start_time > page_end)
                        break;
                    end
                    discont_usecs = discont_usecs + int64(discontigua{j}.end_time - discontigua{j}.start_time) + 1;
                end
                if ((page_end + discont_usecs) > sess_end)
                    page_end = sess_end - discont_usecs;
                end
            end
        end
    end

    function create_labels()
        cla(label_ax);
        chan_labels = cell(n_chans, 1);
        for i = 1:n_chans   
            chan_labels{i} = text(label_ax, ...
            'Units', 'pixels', ...
            'String', chan_list{i}, ...
            'FontSize', SYS_FONT_SIZE, ...
            'Color', colors((mod((i - 1), n_colors) + 1), :), ...
            'HorizontalAlignment', 'right', ...
            'Interpreter', 'none', ...
            'ButtonDownFcn', @label_select_callback);
        end
        selected_labels = zeros(n_chans, 1);
    end

    function draw_labels()
        chan_dy = (data_ax_height - 4) / n_chans;
        chan_y = data_ax_height - (chan_dy / 2);        
        label_font_size = points_per_pix * chan_dy;
        if (label_font_size > SYS_FONT_SIZE)
            label_font_size = SYS_FONT_SIZE;
        end
        for i = 1:n_chans
            set(chan_labels{i}, 'Position', [label_ax_width, round(chan_y)], 'FontSize', label_font_size);
            chan_y = chan_y - chan_dy;
            c = colors((mod((i - 1), n_colors) + 1), :);
            if (selected_labels(i) == 1)
                set(chan_labels{i}, 'BackgroundColor', panel_color * 0.95);
                if (monochrome_flag == false)
                    set(chan_labels{i}, 'Color', c);
                else
                    set(chan_labels{i}, 'Color', 'red');
                end
            else
                if (monochrome_flag == false)
                    set(chan_labels{i}, 'Color', c);
                else
                    set(chan_labels{i}, 'Color', 'black');
                end                
            end
        end
    end

    function set_data_ax_x_labels()
        if (wind_usecs < 1000)  % us
            scale_factor = 1;
            x_tick_units = '(µs)';
        elseif (wind_usecs < 1000000)  % ms
            scale_factor = 1000;
            x_tick_units = '(ms)';
        elseif (wind_usecs < 60000000)  % sec
            scale_factor = 1000000;
            x_tick_units = '(sec)';
        elseif (wind_usecs < 3600000000)  % min
            scale_factor = 60000000;
            x_tick_units = '(min)';
        elseif (wind_usecs < 864000000000)  % hrs
            scale_factor = 3600000000;
            x_tick_units = '(hrs)';
        else  % days
            scale_factor = 864000000000;
            x_tick_units = '(days)';
        end

        vals = linspace(0, double(page_end - page_start), 11) / scale_factor;
        x_tick_labels = cell(11, 1);
        for i = 1:11
            x_tick_labels{i} = num2str(vals(i), 3);
        end

        set(data_ax, 'XTickLabels', x_tick_labels);
        set(x_tick_units_label, 'String', x_tick_units);
    end

    function set_movement_focus()
        ax_mouse_down = false;
        if (movement_direction == FORWARD)
            uicontrol(forward_button);
        else  % movement_direction == BACKWARD
            uicontrol(back_button);
        end       
    end

    function plot_sess_record_times()

        if (~isempty(sess_map_records_lines))
            for i = 1:REC_NUM_TYPES
                delete(sess_map_records_lines{i});
            end
        end
        if (records_checkbox.Value == false)
            return;
        end

        sess_map_records_lines = cell(REC_NUM_TYPES, 1);
        for i = 1:REC_NUM_TYPES
            if (rec_settings{i}.display == false)
                continue;
            end
            n_lines = numel(sess_record_times{i});
            if (n_lines == 0)
                continue;
            end
            ax_locs = round(sess_record_times{i} * sess_map_ax_width);
            ax_locs = unique(ax_locs);  % potentially a lot of overlap
            n_lines = numel(ax_locs);
            sess_map_records_lines{i} = gobjects(n_lines, 1);

            % layers
            % 0: current page
            % 1: discontigua
            % 2: HFOs
            % 3: Notes
            % 4: Port Values
            % 5: Seizures
            % 6: Segments
            layer = i + 1;
            for j = 1:n_lines
                x = ax_locs(j);
                sess_map_records_lines{i}(j) = line(sess_map_ax, [x x], [0 sess_map_ax_height], [layer, layer], 'Color', rec_settings{i}.color, 'ButtonDownFcn', @sess_map_callback);
            end
        end
        clear ax_locs;
    end

    function q = local_quantile(page, q_points)
        tot_samps = numel(page);
        sorted_page = sort(reshape(page, [tot_samps, 1]));
        q = zeros(size(q_points));
        for i = 1:numel(q_points)
            float_idx = q_points(i) * tot_samps;
            floor_idx = floor(float_idx);
            ceil_idx = floor_idx + 1;
            val = (ceil_idx - float_idx) * sorted_page(floor_idx);
            q(i) = val + ((float_idx - floor_idx) * sorted_page(ceil_idx));
        end
        clear sorted_page;
    end

    function read_rec_settings()
    
        fp = fopen(settings_path, 'r');
        if (fp == -1)
            rec_settings{REC_HFOc_IDX} = struct;
            rec_settings{REC_HFOc_IDX}.type_str = 'HFOc';
            rec_settings{REC_HFOc_IDX}.type_code = uint32(0x634F4648);
            rec_settings{REC_HFOc_IDX}.label = 'High Frequency Oscillation';
            rec_settings{REC_HFOc_IDX}.color = DARK_YELLOW;
            rec_settings{REC_HFOc_IDX}.color_str = 'Yellow';
            rec_settings{REC_HFOc_IDX}.display = true;
        
            rec_settings{REC_NlxP_IDX} = struct;
            rec_settings{REC_NlxP_IDX}.type_str = 'NlxP';
            rec_settings{REC_NlxP_IDX}.type_code = uint32(0x50786C4E);
            rec_settings{REC_NlxP_IDX}.label = 'Neuralynx Port Value';
            rec_settings{REC_NlxP_IDX}.color = DARK_ORANGE;
            rec_settings{REC_NlxP_IDX}.color_str = 'Orange';
            rec_settings{REC_NlxP_IDX}.display = true;

            rec_settings{REC_Note_IDX} = struct;
            rec_settings{REC_Note_IDX}.type_str = 'Note';
            rec_settings{REC_Note_IDX}.type_code = uint32(0x65746f4e);
            rec_settings{REC_Note_IDX}.label = 'Annotation';
            rec_settings{REC_Note_IDX}.color = DARK_GREEN;
            rec_settings{REC_Note_IDX}.color_str = 'Green';
            rec_settings{REC_Note_IDX}.display = true;
        
            rec_settings{REC_Seiz_IDX} = struct;
            rec_settings{REC_Seiz_IDX}.type_str = 'Seiz';
            rec_settings{REC_Seiz_IDX}.type_code = uint32(0x7a696553);
            rec_settings{REC_Seiz_IDX}.label = 'Seizure';
            rec_settings{REC_Seiz_IDX}.color = DARK_RED;
            rec_settings{REC_Seiz_IDX}.color_str = 'Red';
            rec_settings{REC_Seiz_IDX}.display = true;
        
            rec_settings{REC_Sgmt_IDX} = struct;
            rec_settings{REC_Sgmt_IDX}.type_str = 'Sgmt';
            rec_settings{REC_Sgmt_IDX}.type_code = uint32(0x746D6753);
            rec_settings{REC_Sgmt_IDX}.label = 'Segment';
            rec_settings{REC_Sgmt_IDX}.color = DARK_BLUE;
            rec_settings{REC_Sgmt_IDX}.color_str = 'Blue';
            rec_settings{REC_Sgmt_IDX}.display = true;

            write_rec_settings();
        else
            buf = fread(fp, 'char=>char');
            fclose(fp);

            buf = buf';
            len = length(buf);
            j = 1;
            for i = 1:REC_NUM_TYPES
                rec_settings{i} = struct;
                rec_settings{i}.type_str = buf(j:j + 3);
                j = j + 8; 
                str = buf(j:j + 7);
                rec_settings{i}.type_code = sscanf(str, '%x');
                % advance next entry
                j = j + 10;
                k = j + 1;
                while (buf(k) ~= ',')
                    k = k + 1;
                end
                rec_settings{i}.label = buf(j:(k - 1));
                j = k + 2;
                k = j + 1;
                while (buf(k) ~= ',')
                    k = k + 1;
                end
                str = buf(j:(k - 1));
                switch (str)
                    case 'Red'
                        rec_settings{i}.color = DARK_RED;
                    case 'Orange'
                        rec_settings{i}.color = DARK_ORANGE;
                    case 'Yellow'
                        rec_settings{i}.color = DARK_YELLOW;
                    case 'Green'
                        rec_settings{i}.color = DARK_GREEN;
                    case 'Blue'
                        rec_settings{i}.color = DARK_BLUE;
                    case 'Purple'
                        rec_settings{i}.color = DARK_PURPLE;
                end
                rec_settings{i}.color_str = str;
                j = k + 2;
                if (buf(j) == '1')
                    rec_settings{i}.display = true;
                else
                    rec_settings{i}.display = false;
                end
                while (buf(j) ~= 10 && buf(j) ~= 13)
                    j = j + 1;
                end
                j = j + 1;
            end
        end
    end

    function write_rec_settings()
        fp = fopen(settings_path, 'w');
        if (fp == -1)
            errordlg('Cannot create records settings file', 'View MED');
            return;
        end

        for i = 1:REC_NUM_TYPES
            str = sprintf('%s, 0x%08x, %s, %s, %d\n', rec_settings{i}.type_str, rec_settings{i}.type_code, rec_settings{i}.label, rec_settings{i}.color_str, rec_settings{i}.display);
            fwrite(fp, str);
        end
        fclose(fp);
    end


% ------------ Callback Functions ---------------

	% Figure resize function
    function resize(~, ~)
  
        % Reject all concommitant resize calls
        if (currently_resizing == true)
            return;
        end        
        currently_resizing = true;

        % Wait for user to stop resizing
        last_f_pos = get(fig, 'Position');
        while (true)
            pause(0.2);
            f_pos = get(fig, 'Position');
            if (f_pos(3) == last_f_pos(3) && f_pos(4) == last_f_pos(4))
                break;
            end
            last_f_pos = f_pos;
        end

        % Figure dimensions
        fig_left = 1;
        fig_right = round(f_pos(3));
        fig_bot = 1;
        fig_top = round(f_pos(4));

        min_size_flag = false;
        if (fig_right < 950)
            fig_right = 950;
            min_size_flag = true;
        end
        if (fig_top < 650)
            fig_top = 650;
            min_size_flag = true;
        end

        % check that window is entirely on screen
        if ((f_pos(1) + fig_right) > screen_x_pix)
            f_pos(1) = (screen_x_pix - fig_right) - 1;
            min_size_flag = true;
        end
        if ((f_pos(2) + fig_top) > screen_y_pix)   
            if (strcmp(OS, 'PCWIN64'))  % Windows window banner not counted in fig size
                f_pos(2) = (screen_y_pix - fig_top) - 26;
            else
                f_pos(2) = (screen_y_pix - fig_top) - 1;
            end
            min_size_flag = true;
        end
        if (min_size_flag)
            set(fig, 'Position', [f_pos(1), f_pos(2), fig_right, fig_top]);
        end

        % Data axes dimensions
        data_ax_left = fig_left + 160;
        data_ax_right = fig_right - 30;
        data_ax_width = (data_ax_right - data_ax_left) + 1;
        x_ax_inds = (1:data_ax_width)';
        x_tick_inds = linspace(1, data_ax_width, 11);
        set_data_ax_x_labels();
        full_page_width = data_ax_width;
        mps.SampDim = full_page_width;
        data_ax_bot = fig_bot + 165;
        data_ax_top = fig_top - 50;
        data_ax_height = (data_ax_top - data_ax_bot) + 1;

        set(data_ax, 'Position', [data_ax_left, data_ax_bot, data_ax_width, data_ax_height]);
        set(data_ax, 'XLim', [0, data_ax_width - 1], 'YLim', [0, data_ax_height - 1]);
        set(x_tick_units_label, 'Position', [33, (data_ax_bot - 27), 117, 20]);
       
        % Label axes dimensions
        label_ax_left = 30;
        label_ax_right = 150;
        label_ax_width = label_ax_right - label_ax_left;
        label_ax_bot = data_ax_bot;

        set(label_ax, 'Position', [label_ax_left, label_ax_bot, label_ax_width, data_ax_height]);
        set(label_ax, 'XLim', [1, label_ax_width], 'YLim', [1, data_ax_height]);
        
        % Session map axes dimensions
        sess_map_ax_left = data_ax_left;
        sess_map_ax_width = data_ax_width;
        sess_map_ax_bot = data_ax_bot - 45;
        % sess_map_ax_top = sess_map_ax_bot + 10;

        set(sess_map_ax, 'Position', [sess_map_ax_left, sess_map_ax_bot, sess_map_ax_width, sess_map_ax_height], ...
            'XLim', [0, sess_map_ax_width + 1], 'YLim', [0, sess_map_ax_height + 1]);

        % Time string axes dimensions
        time_str_ax_left = data_ax_left;
        time_str_ax_width = data_ax_width;
        time_str_ax_bot = data_ax_top + 9;
        time_str_ax_top = time_str_ax_bot + 10;
        time_str_ax_height = (time_str_ax_top - time_str_ax_bot) + 1;

        set(time_str_ax, 'Position', [time_str_ax_left, time_str_ax_bot, time_str_ax_width, time_str_ax_height], ...
            'XLim', [0, time_str_ax_width + 1], 'YLim', [0, time_str_ax_height]);
        set(axis_start_time_string, 'Position', [1, 1]);
        set(axis_end_time_string, 'Position', [time_str_ax_width, 1]);
        
        % Logo axes dimensions
        logo_ax_left = 20;
        logo_ax_right = logo_ax_left + 117;
        logo_ax_width = (logo_ax_right - logo_ax_left) + 1;
        logo_ax_bot = fig_bot + 25;
        logo_ax_top = logo_ax_bot + 44;
        logo_ax_height = (logo_ax_top - logo_ax_bot) + 1;
        set(logo_ax, 'Position', [logo_ax_left, logo_ax_bot, logo_ax_width, logo_ax_height]);
        set(logo_ax, 'XLim', [1, logo_ax_width], 'YLim', [1, logo_ax_height]);
        delete(logo_handle);
        logo_handle = imshow(logo, 'Parent', logo_ax);
        set(logo_handle, 'ButtonDownFcn', @logo_callback);

        button_group_right = data_ax_left + 363;
        label_group_left = data_ax_right - 155;
        mid_center_space = round((button_group_right + label_group_left) / 2);

        % Controls time_str_ax
        set(view_selected_button, 'Position', [data_ax_left - 3, 75, 130, 30]);
        set(remove_selected_button, 'Position', [data_ax_left - 3, 50, 130, 30]);
        set(add_channels_button, 'Position', [data_ax_left - 3, 25, 130, 30]);
        
        set(amplitude_direction_button, 'Position', [(data_ax_left + 125), 75, 130, 30]);
        set(baseline_correct_button, 'Position', [(data_ax_left + 125), 50, 130, 30]);
        set(antialias_button, 'Position', [(data_ax_left + 125), 25, 130, 30]);

        set(autoscale_button, 'Position', [(data_ax_left + 253), 75, 130, 30]);
        set(add_record_button, 'Position', [(data_ax_left + 253), 50, 130, 30]);
        set(export_button, 'Position', [(data_ax_left + 253), 25, 130, 30]);    

        set(back_button, 'Position', [(mid_center_space - 75), 35, 80, 50]);
        set(forward_button, 'Position', [(mid_center_space + 15), 35, 80, 50]);
        
        set(gain_textbox, 'Position', [(data_ax_right - 100), 25, 100, 20]);
        set(gain_textbox_label, 'Position', [(data_ax_right - 155), 25, 50, 20]);
        set(timebase_textbox, 'Position', [(data_ax_right - 100), 50, 100, 20]);
        set(timebase_textbox_label, 'Position', [(data_ax_right - 155), 50, 50, 20]);
        set(current_time_textbox, 'Position', [(data_ax_right - 100), 75, 100, 20]);
        set(current_time_textbox_label, 'Position', [(data_ax_right - 155), 75, 50, 20]);
        
        set(deselect_all_button, 'Position', [34, (data_ax_bot - 25), 81, 20]);
        set(records_checkbox, 'Position', [30, (data_ax_bot - 50), 117, 20]);
        set(records_button, 'Position', [53, (data_ax_bot - 47), 72, 14]);
        set(ranges_checkbox, 'Position', [30, (data_ax_bot - 68), 117, 20]);
        set(monochrome_checkbox, 'Position', [30, (data_ax_bot - 86), 117, 20]);

        % draw discontigua
        if (n_discontigua)
            for i = 1:n_discontigua
                sess_map_start_x = round(data_ax_width * discontigua{i}.start_prop);
                sess_map_end_x = round(data_ax_width * discontigua{i}.end_prop);
                if (sess_map_end_x == sess_map_start_x)
                    sess_map_end_x = sess_map_start_x + 1;
                end
                set(discontigua{i}.patch, 'XData', [sess_map_start_x, sess_map_start_x, sess_map_end_x, sess_map_end_x]);
            end
        end

        % draw session map record lines
        if (mps.Records == true)
            plot_sess_record_times();
        end

        % plot
        while (currently_plotting == true)
            pause(0.05);
        end
        draw_labels();
        plot_handles = [];
        potentially_increased_plot_time = true;
        plot_page(true);
        currently_resizing = false;
    end

    % Key Press Callback
    function key_press_callback(src, evt)
        key = lower(char(evt.Key));
        switch key
            case 'rightarrow'
                page_movement_callback(src, evt);
            case 'leftarrow'
                page_movement_callback(src, evt);
            case 'shift'
                shift_pressed = true;
            case {'uparrow', 'downarrow'}
                switch src
                    case {fig, gain_textbox, forward_button, back_button}
                        uicontrol(gain_textbox);
                        gain_callback(gain_textbox, evt);
                    case timebase_textbox
                        uicontrol(timebase_textbox);
                        timebase_callback(timebase_textbox, evt);
                    case current_time_textbox
                        switch key
                            case 'uparrow'
                                page_movement_callback(forward_button, evt);
                            case 'downarrow'
                                page_movement_callback(back_button, evt);
                        end
                end
        end 
    end

    % Key Release Callback
    function key_release_callback(~, evt)
        key = lower(char(evt.Key));
        switch key
            case 'shift'
                    shift_pressed = false;
        end
    end

    % Page Movement Callback
    function page_movement_callback(src, evt)

        left_click = 0;
        right_click = 0;
        modifier = '';
        key = '';
        switch class(evt)
            case 'matlab.ui.eventdata.MouseData'
                right_click = 1;
            case 'matlab.ui.eventdata.ActionData'
                left_click = 1;
            case 'matlab.ui.eventdata.UIClientComponentKeyEvent'
                modifier = lower(char(evt.Modifier));
                key = lower(char(evt.Key));
                switch key
                    case 'uparrow'
                        key = 'rightarrow';
                    case 'downarrow'
                        key = 'leftarrow';
                end
        end
        
        if (right_click || left_click)
            if right_click
                modifier = 'command';
            else
                modifier = 'no modifier';
            end
            switch src
                case forward_button
                    key = 'rightarrow';
                case back_button
                    key = 'leftarrow';
            end
        end
        
        switch modifier
            case 'command'
                page_shift = round(wind_usecs / 3);
            case 'alt'
                page_shift = round(wind_usecs / 10);
            otherwise
                page_shift = wind_usecs;
        end

        switch key
            case 'rightarrow'
                page_start = page_start + page_shift;
                movement_direction = FORWARD;
            case 'leftarrow'
                page_start = page_start - page_shift;
                movement_direction = BACKWARD;
        end

        plot_page(true);
        set_movement_focus();        
    end

    % Timebase Callback
    function timebase_callback(~, evt)
        
        key = '';
        modifier = '';
        temp_wind_usecs = str2double(get(timebase_textbox, 'String')) * 1e6;
        switch class(evt)
            case 'matlab.ui.eventdata.KeyData'   % unmodified arrow
                key = lower(char(evt.Key));
            case 'matlab.ui.eventdata.UIClientComponentKeyEvent'   % modified arrow
                key = lower(char(evt.Key));
                modifier = lower(char(evt.Modifier));
        end   
        switch modifier
            case 'command'
                multiplier = 2;
            case 'alt'
                multiplier = 1.1;
            otherwise
                multiplier = 1.4142;
        end    
        switch key
            case 'uparrow'
                temp_wind_usecs = temp_wind_usecs * multiplier;
            case 'downarrow'
                temp_wind_usecs = temp_wind_usecs / multiplier;
            otherwise
                key = 'enter';
        end

        if (temp_wind_usecs > 0)
            if (temp_wind_usecs > wind_usecs)
                potentially_increased_plot_time = true;
            end
            wind_usecs = temp_wind_usecs;
        else
            return;
        end
        page_end = (page_start + wind_usecs) - 1;
        if (page_end > sess_end)
            page_end = sess_end;
            wind_usecs = (page_end - page_start) + 1;
        end        
        set(timebase_textbox, 'String', num2str(wind_usecs / 1e6, '%0.6f'));
        set_data_ax_x_labels();

        plot_page(true);

        % return focus to timebase box if user used up/down arrows
        if (strcmp(key, 'enter') == true)
            set_movement_focus();
        else
            uicontrol(timebase_textbox);
        end
    end

	% Gain Callback
    function gain_callback(~, evt)
        
        key = '';
        modifier = '';
        switch class(evt)
            case 'matlab.ui.eventdata.KeyData'   % unmodified arrow
                key = lower(char(evt.Key));
            case 'matlab.ui.eventdata.UIClientComponentKeyEvent'   % modified arrow
                key = lower(char(evt.Key));
                modifier = lower(char(evt.Modifier));
            case 'matlab.ui.eventdata.ActionData'   % textbox entry
                uV_per_cm = str2double(get(gain_textbox, 'String'));
                scale = pix_per_cm / uV_per_cm;
        end        
        switch modifier
            case 'command'
                multiplier = 2;
            case 'alt'
                multiplier = 1.1;
            otherwise
                multiplier = 1.4142;
        end 
        switch key
            case 'uparrow'
                scale = scale * multiplier;
            case 'downarrow'
                scale = scale / multiplier;
            otherwise
                key = 'enter';
        end
       
        if (autoscale_flag == true)
            set(autoscale_button, 'String', 'Autoscaling is Off');
        end
        autoscale_flag = false;
        plot_page(false);

        % return focus to gain box if user used up/down arrows
        if (strcmp(key, 'enter') == true)
            set_movement_focus();
        else
            uicontrol(gain_textbox);
        end
    end

	% Monochrome Callback
    function monochrome_callback(~, ~)
        if (monochrome_checkbox.Value == true)
            monochrome_flag = true;
        else
            monochrome_flag = false;
        end
        
        for i = 1:n_chans            
            if (monochrome_flag == false)
                c = colors((mod((i - 1), n_colors) + 1), :);
                set(chan_labels{i}, 'Color', c);
                set(plot_handles(i), 'Color', c);
            else
                if (selected_labels(i) == 1)
                    set(chan_labels{i}, 'Color', 'red');
                else
                    set(chan_labels{i}, 'Color', 'black');
                end
                set(plot_handles(i), 'Color', mono_color);
            end
        end

        set_movement_focus();
    end

	% Antialias Callback
    function antialias_callback(~, ~)
        % set button text in plot_page() in case no traces need antialisiang
        if (mps.Filt == AA_FILT)
            mps.Filt = NO_FILT;
        else
            mps.Filt = AA_FILT;
            potentially_increased_plot_time = true;
        end

        plot_page(true);
        set_movement_focus();
    end

	% Baseline Callback
    function baseline_callback(src, ~)
        if (mps.Detrend == true)
            set(src, 'String', 'Baseline Correction is Off');
            mps.Detrend = false;
        else
            set(src, 'String', 'Baseline Correction is On');
            potentially_increased_plot_time = true;
            mps.Detrend = true;
        end

        plot_page(true);
        set_movement_focus();
    end

	% Amplitude Direction Callback
    function amplitude_direction_callback(~, ~)
        if (amplitude_direction == NEGATIVE_DOWN)
            set(amplitude_direction_button, 'String', 'Negative is Up');
            amplitude_direction = NEGATIVE_UP;
        else
            set(amplitude_direction_button, 'String', 'Negative is Down');
            amplitude_direction = NEGATIVE_DOWN;
        end

        plot_page(false);
        set_movement_focus();
    end

	% Autoscale Callback
    function autoscale_callback(~, ~)
        if (autoscale_flag == true)
            set(autoscale_button, 'String', 'Autoscaling is Off');
            autoscale_flag = false;
        else
            set(autoscale_button, 'String', 'Autoscaling is On');
            autoscale_flag = true;
            potentially_increased_plot_time = true;
        end

        plot_page(false);
        set_movement_focus();
    end

	% Current Time Callback
    function current_time_callback(~, ~)      
        curr_usec = round(str2double(get(current_time_textbox, 'String')) * 1e6);
        if curr_usec < 0
            curr_usec = 0;
        end       
        page_start = sess_start + curr_usec;  
        plot_page(true);
        set_movement_focus();
    end

	% Label Select Callback
    function label_select_callback(src, ~)

        for i = 1:n_chans
            if (strcmp(chan_list{i}, src.String) == true)
                break;
            end
        end
        selected = i;

        if (label_font_size < SYS_FONT_SIZE)
            pos = get(chan_labels{selected}, 'Position');
            if (selected_labels(selected))
                set(chan_labels{selected}, 'Position', [label_ax_width, pos(2)], 'HorizontalAlignment', 'right', 'FontSize', label_font_size);
            else
                set(chan_labels{selected}, 'Position', [0, pos(2)], 'HorizontalAlignment', 'left', 'FontSize', SYS_FONT_SIZE);
            end
        end

        prev_selected = selected;        
        if (shift_pressed == true)
            % find next selected higher in list
            for i = (selected - 1):-1:1
                if selected_labels(i)
                    prev_selected = i;
                    break;
                end
            end
            if (prev_selected == selected)
                % find next selected lower in list
                for i = (selected + 1):n_chans
                    if selected_labels(i)
                        prev_selected = selected;
                        selected = i;
                        break;
                    end
                end
            end
            if (prev_selected ~= selected)
                selected_labels(prev_selected:selected) = 1;
            end 
        else
            selected_labels(selected) = ~selected_labels(selected);
        end
        
        for i = prev_selected:selected
            if selected_labels(i)
                set(chan_labels{i}, 'FontAngle', 'italic', 'BackgroundColor', panel_color * 0.925);
                if (monochrome_flag == true)
                    set(chan_labels{i}, 'Color', 'red');
                end
            else
                set(chan_labels{i}, 'FontAngle', 'normal', 'BackgroundColor', panel_color);
                if (monochrome_flag == true)
                    set(chan_labels{i}, 'Color', 'black');
                end
            end
        end
        if (sum(selected_labels))
            set(view_selected_button, 'Enable', 'on');
            set(remove_selected_button, 'Enable', 'on');
            set(deselect_all_button, 'Visible', 'on');
        else
            set(deselect_all_button, 'Visible', 'off');
            set(view_selected_button, 'Enable', 'off');
            set(remove_selected_button, 'Enable', 'off');
            selected_labels = [];
            create_labels();
            draw_labels();
        end

        shift_pressed = false;
        set_movement_focus();
    end

    % View Selected Callback
    function view_selected_callback(src, ~)
        if src == view_selected_button
            val = 1;
        else % src == remove_selected_button
            val = 0;
        end
        
        % Get selected / unselected channels
        j = 0;
        for i = 1:n_chans
            if selected_labels(i) == val
                j = j + 1;
                chan_paths{j} = chan_paths{i};
                chan_list{j} = chan_list{i};
            end
        end
        
        % view all
        if (j == n_chans)
            return;
        % remove all
        elseif (j == 0 && val == 0)
            errordlg('At least one channel must be selected', 'View MED');
            chan_paths = {};
            mps.Data = {};
            chan_list = {};
            n_chans = 0;
            add_channels_callback();
            return;
        end

        n_chans = j;
        chan_paths = chan_paths(1:n_chans);
        mps.Data = chan_paths;
        chan_list = chan_list(1:n_chans);
        mps.Persist = 2; % close
        [~] = matrix_MED_exec(mps);  % close to reset channel dimension
        mps.Persist = 4; % read
        plot_handles = [];
        if (autoscale_flag == false)  % rescale plots for new trace set
            set(autoscale_button, 'String', 'Autoscaling is On');
            autoscale_flag = true;
        end
        plot_page(true);

        % Make channel labels
        set(deselect_all_button, 'Visible', 'off');
        selected_labels = [];
        create_labels();
        draw_labels();

        set_movement_focus();
    end

    % Add Channels Callback
    function add_channels_callback(~, ~)
        filters = {'ticd'};
        stop_filters = {'ticd'};
        [new_chan_list, new_sess_dir] = directory_chooser(filters, sess_dir, stop_filters);
        if (isempty(new_chan_list))
            if (n_chans == 0)
                errordlg('No channels selected for display. Exiting.', 'View MED');
                figure_close_callback();
                return;
            else
                errordlg('No MED data selected', 'View MED');
                return;
            end
        end
        if (strcmp(sess_dir, new_sess_dir) == false)
        	errordlg('Channels must be from the same MED session', 'View MED');
            return;
        end

        n_new_chans = numel(new_chan_list);
        for i = 1:n_new_chans
            n_chans = n_chans + 1;
            chan_paths{n_chans} = [sess_dir DIR_DELIM new_chan_list{i}];
        end
        chan_paths = unique(chan_paths);

        % close & reopen (not efficient, but probably not frequent)
        mps.Persist = 2;  % close
        [~] = matrix_MED_exec(mps);  % close matrix to force re-read
        mps.Persist = 4;  % read
        rps = read_MED('numeric');
        rps.Data = chan_paths;
        rps.Start = page_start;
        rps.End = page_start + 1e6;
        rps.Pass = password;
        sess = read_MED_exec(rps);
        clear read_MED_exec rps;
        if (isempty(sess))
            errordlg('read_MED() error', 'View MED');
            return;
        end
        n_chans = numel(sess.channels);
        chan_paths = cell(n_chans, 1);
        chan_list = cell(n_chans, 1);
        for i = 1:n_chans
            chan_paths{i} = sess.channels(i).metadata.path;
            chan_list{i} = sess.channels(i).name;
        end
        mps.Data = chan_paths;
        page_start = sess.metadata.slice_start_time;
        page_end = (page_start + wind_usecs) - 1;
        clear sess;
    
        % plot
        plot_handles = [];
        if (autoscale_flag == false)  % rescale plots for new trace set
            set(autoscale_button, 'String', 'Autoscaling is On');
            autoscale_flag = true;
        end
        potentially_increased_plot_time = true;
        plot_page(true);
                
        % Make channel labels
        set(deselect_all_button, 'Visible', 'off');
        selected_labels = [];
        create_labels();
        draw_labels();
        
        set_movement_focus();
    end

    % Session Map Callback
    function sess_map_callback(~, evt)
        x = evt.IntersectionPoint(1, 1);
    
        page_start = sess_start + round(sess_duration * (x / data_ax_width));

        plot_page(true);
        set_movement_focus();
    end

    % Export Callback
    function export_callback(~, ~)
        export_num = export_num + 1;
        d = dialog('Position', [400 400 250 150], ...
            'Name', 'Export to Workspace');
        d_txtbx_label = uicontrol('Parent', d, ...
            'Style', 'text', ...
            'Position', [10 102 70 30], ...
            'HorizontalAlignment', 'right', ...
            'String','Workspace Name:');
        d_txtbx = uicontrol('Parent', d, ...
            'Style','edit', ...
            'Position', [88 100 132 30], ...
            'HorizontalAlignment', 'left', ...
            'String', ['page_' num2str(export_num)]);
        d_raw_radbtn = uicontrol('Parent', d, ...
            'Style', 'radiobutton', ...
            'String', 'Raw Data', ...
            'Value', 1, ...
            'Position', [35 50 210 40], ...
            'BackgroundColor', panel_color, ...
            'FontSize', SYS_FONT_SIZE, ...
            'Callback', @d_radbtnsCallback);
        d_dsp_radbtn = uicontrol('Parent', d, ...
            'Style', 'radiobutton', ...
            'String', 'As Displayed', ...
            'Value', 0, ...
            'Position', [135 50 210 40], ...
            'BackgroundColor', panel_color, ...
            'FontSize', SYS_FONT_SIZE, ...
            'Callback', @d_radbtnsCallback);
        d_expt_btn = uicontrol('Parent', d, ...
            'Style', 'pushbutton', ...
            'Position', [95 20 70 25], ...
            'String', 'Export', ...
            'Callback', @d_expt_btnCallback);

        uiwait(d);
        if (isvalid(d))  % user hit close button
            close(d);
        end

        set_movement_focus();

        % Dialog export Button (nested)
        function d_expt_btnCallback(~, ~)
            if (new_page_secs > WAIT_POINTER_DELAY)
                set(d, 'Pointer', 'watch'); 
                drawnow;
            end
            rps = read_MED('numeric');
            rps.Data = chan_paths;
            rps.Start = page_start;
            rps.End = page_end;
            rps.Pass = password;            
            expt_sess = read_MED_exec(rps);
            clear read_MED_exec rps;
            if (isempty(expt_sess))
                close(d);
                errordlg('read_MED() error', 'View MED');
                return;
            end
            if (d_dsp_radbtn.Value == true)  % assign raw page data to session structure    
                if (mps.Filt == AA_FILT && expt_sess.metadata.sampling_frequency > screen_sf)
                        expt_sess.metadata.high_frequency_filter_setting = screen_sf / 4;
                end
                expt_sess.metadata.sampling_frequency = screen_sf;
                for i = 1:n_chans
                    expt_sess.channels(i).data = raw_page.samples(:, i);
                    if (mps.Filt == AA_FILT && expt_sess.channels(i).metadata.sampling_frequency > screen_sf)
                        expt_sess.channels(i).metadata.high_frequency_filter_setting = screen_sf / 4;
                    end
                    expt_sess.channels(i).metadata.sampling_frequency = screen_sf;
                end
            end
            
            var_name = d_txtbx.String;
            assignin('base', var_name, expt_sess);
            clear expt_sess;

           % change dialog box
            delete(d_txtbx);
            delete(d_raw_radbtn);
            delete(d_dsp_radbtn);
            set(d_txtbx_label, ...
                'Position', [10 80 230 30], ...
                'FontSize', SYS_FONT_SIZE + 4, ...
                'HorizontalAlignment', 'center', ...
                'String', ['"' var_name '" exported']);
            set(d_expt_btn, 'String', 'OK', 'Callback', @d_ok_btnCallback);
            function d_ok_btnCallback(~, ~)
                close(d)
            end

            if (new_page_secs > WAIT_POINTER_DELAY)
                set(d, 'Pointer', 'arrow'); 
                drawnow;
            end
            uiwait(d, 5);

            return;
        end

        % Dialog Radio Buttons (nested)
        function d_radbtnsCallback(src, ~)
            if (src == d_raw_radbtn)
                if (d_raw_radbtn.Value == false)
                    d_dsp_radbtn.Value = true;
                else
                    d_dsp_radbtn.Value = false;
                end
            else
                if (d_dsp_radbtn.Value == false)
                    d_raw_radbtn.Value = true;
                else
                    d_raw_radbtn.Value = false;
                end
            end
        end

    end  % end Export Callback

    % Axis Time Callback
    function axis_time_callback(src, ~)
        if calendar_time_flag == true   % calendar time => µUTC time
            if recording_time_offset ~= 0
                calendar_time_flag = false; oUTC_flag = false;
                uUTC_flag = true;
                set(axis_start_time_string, 'String', ['start µUTC: ' num2str(page_start + recording_time_offset)]);
                set(axis_end_time_string, 'String', ['end µUTC: ' num2str(page_end + recording_time_offset)]);
            else
                calendar_time_flag = false; 
                oUTC_flag = true;
                set(axis_start_time_string, 'String', ['start oUTC: ' num2str(page_start)]);
                set(axis_end_time_string, 'String', ['end oUTC: ' num2str(page_end)]);   
            end
        elseif uUTC_flag == true   % µUTC time => oUTC time
            uUTC_flag = false; calendar_time_flag = false;
            oUTC_flag = true;
            set(axis_start_time_string, 'String', ['start oUTC: ' num2str(page_start)]);
            set(axis_end_time_string, 'String', ['end oUTC: ' num2str(page_end)]);   
        elseif oUTC_flag == true   % oUTC time => calendar time
            uUTC_flag = false; oUTC_flag = false;
            calendar_time_flag = true;
            set(axis_start_time_string, 'String', raw_page.slice_start_time_string);
            set(axis_end_time_string, 'String', raw_page.slice_end_time_string);
        end
        
        % copy data to clipboard
        if (src == axis_start_time_string)  % copy start times
            page_start_str = num2str(curr_usec / 1e6,  '%0.6f');
            if recording_time_offset ~= 0  % include µUTC
                clipboard('copy', ['Page Start Time:' newline raw_page.slice_start_time_string newline 'µUTC: ' num2str(page_start + recording_time_offset) newline 'oUTC: ' num2str(page_start) newline 'Relative (s): ' page_start_str newline]);
            else  % don't include µUTC
                clipboard('copy', ['Page Start Time:' newline raw_page.slice_start_time_string newline 'oUTC: ' num2str(page_start) newline 'Relative (s): ' page_start_str newline]);
            end        
        else    % copy end times
            page_end_str = num2str((curr_usec + wind_usecs) / 1e6,  '%0.6f');
            if recording_time_offset ~= 0  % include µUTC
                clipboard('copy', ['Page End Time:' newline raw_page.slice_end_time_string newline 'µUTC: ' num2str(page_end + recording_time_offset) newline 'oUTC: ' num2str(page_end) newline 'Relative (s): ' page_end_str newline]);
            else  % don't include µUTC
                clipboard('copy', ['Page End Time:' newline raw_page.slice_end_time_string newline 'oUTC: ' num2str(page_end) newline 'Relative (s): ' page_end_str newline]);
            end
        end

        set_movement_focus();
    end

    % Deselect All Callback
    function deselect_all_callback(~, ~)
        for i = 1:n_chans
            if selected_labels(i)
                selected_labels(i) = 0;
                set(chan_labels{i}, 'FontAngle', 'normal', 'BackgroundColor', panel_color);
                if (monochrome_flag == true)
                    set(chan_labels{i}, 'Color', 'black');
                end
            end
        end
        set(deselect_all_button, 'Visible', 'off');
        selected_labels = [];
        create_labels();
        draw_labels();
        set_movement_focus();
    end

    % Trace Ranges Callback
    function trace_ranges_callback(~, ~)
        if (ranges_checkbox.Value == true)
            mps.Ranges = true;
            potentially_increased_plot_time = true;
            plot_page(true);  % need raw data to generate trace ranges
        else
            % clear old trace range patches
            if (~isempty(range_patches))
                for i = 1:numel(range_patches)
                    delete(range_patches{i});
                end
                range_patches = [];
            end
            mps.Ranges = 0;
            autoscale_flag = true;  % rescale traces
            potentially_increased_plot_time = false;
            plot_page(false);  % have raw data
        end
        set_movement_focus();
    end

    % Records Callback
    function records_callback(~, ~)
        if (records_checkbox.Value == true)
            mps.Records = true;
            plot_sess_record_times();
            plot_page(true);
        else
            mps.Records = false;
            % clear old record lines (don't need to plot)
            if (~isempty(record_lines))
                for i = 1:numel(record_lines)
                    if (~isempty(record_lines{i}))
                        delete(record_lines{i}.line);
                        delete(record_lines{i}.flag);
                    end
                end
                record_lines = [];
            end
            if (~isempty(sess_map_records_lines))
                for i = 1:REC_NUM_TYPES
                    delete(sess_map_records_lines{i});
                end
                sess_map_records_lines = [];
            end
        end
        set_movement_focus();
    end

    % Records Button Callback
    function records_button_callback(~, ~)

        if (isempty(rec_d) == false)
            set(rec_d, 'Position', [700 500 350 230]);
            figure(rec_d);
            return;
        end

        saved_records_checkbox_value = records_checkbox.Value;

        rec_d = dialog('Position', [700 500 350 230], ...
            'Name', 'Record Settings', ...
            'Color', panel_color, ...
            'WindowStyle', 'normal', ...
            'CloseRequestFcn', @recsCloseCallback);  % make dialog non-modal

        % close
        uicontrol('Parent', rec_d, ...
            'Position', [117 10 70 25], ...
            'String', 'Close',...
            'Callback', @recsCloseCallback);

        % revert
        uicontrol('Parent', rec_d, ...
            'Position', [194 10 70 25], ...
            'String', 'Revert',...
            'Callback', @recsRevertCallback);

        % save
        uicontrol('Parent', rec_d, ...
            'Position', [271 10 70 25], ...
            'String', 'Save',...
            'Callback', @recsSaveCallback);

        % Note record checkbox & dropdown
        recCheckboxes = cell(REC_NUM_TYPES, 1);
        recPopups = cell(REC_NUM_TYPES, 1);
        y_offset = 190;
        for i = 1:REC_NUM_TYPES
            recCheckboxes{i} = uicontrol('Parent', rec_d, ...
                'Position', [120 y_offset 200 25], ...
                'Style', 'checkbox', ...
                'String', [rec_settings{i}.label 's'], ...
                'Value', rec_settings{i}.display, ...
                'BackgroundColor', panel_color, ...
                'FontSize', SYS_FONT_SIZE, ...
                'HorizontalAlignment', 'left', ...
                'Interruptible', 'off', ...
                'BusyAction', 'cancel', ...
                'Callback', @recsApplyCallback);

            % get color value for popup
            switch (rec_settings{i}.color_str)
                case 'Red'
                    col_val = 1;
                case 'Orange'
                    col_val = 2;
                case 'Yellow'
                    col_val = 3;
                case 'Green'
                    col_val = 4;
                case 'Blue'
                    col_val = 5;
                case 'Purple'
                    col_val = 6;
                otherwise
                    col_val = 4;
            end

            % build HTML strings
            hmtl_strings = cell(6, 1);
            part1 = '<HTML><FONT bgcolor="';
            part2 = '" color="';
            part3 = '">&nbsp &nbsp &nbsp<FONT bgcolor="#FFFFFF" color="#000000">&nbsp ';
            part4 = '</FONT></HTML>';
            hex_str = sprintf('#%02x%02x%02x', round(DARK_RED(1) * 255), round(DARK_RED(2) * 255), round(DARK_RED(3) * 255));
            hmtl_strings{1} = [part1 hex_str part2 hex_str part3 'Red' part4];
            hex_str = sprintf('#%02x%02x%02x', round(DARK_ORANGE(1) * 255), round(DARK_ORANGE(2) * 255), round(DARK_ORANGE(3) * 255));
            hmtl_strings{2} = [part1 hex_str part2 hex_str part3 'Orange' part4];
            hex_str = sprintf('#%02x%02x%02x', round(DARK_YELLOW(1) * 255), round(DARK_YELLOW(2) * 255), round(DARK_YELLOW(3) * 255));
            hmtl_strings{3} = [part1 hex_str part2 hex_str part3 'Yellow' part4];
            hex_str = sprintf('#%02x%02x%02x', round(DARK_GREEN(1) * 255), round(DARK_GREEN(2) * 255), round(DARK_GREEN(3) * 255));
            hmtl_strings{4} = [part1 hex_str part2 hex_str part3 'Green' part4];
            hex_str = sprintf('#%02x%02x%02x', round(DARK_BLUE(1) * 255), round(DARK_BLUE(2) * 255), round(DARK_BLUE(3) * 255));
            hmtl_strings{5} = [part1 hex_str part2 hex_str part3 'Blue' part4];
            hex_str = sprintf('#%02x%02x%02x', round(DARK_PURPLE(1) * 255), round(DARK_PURPLE(2) * 255), round(DARK_PURPLE(3) * 255));
            hmtl_strings{6} = [part1 hex_str part2 hex_str part3 'Purple' part4];

            % build popups
            recPopups{i} = uicontrol('Parent', rec_d, ...
                'Position', [10 (y_offset + 5) 110 17], ...
                'Style', 'popupmenu', ...
                'FontSize', SYS_FONT_SIZE, ...
                'String', hmtl_strings, ...
                'Value', col_val, ..., 
                'HorizontalAlignment', 'left', ...
                'Interruptible', 'off', ...
                'BusyAction', 'cancel', ...
                'Callback', @recsApplyCallback);
            
            y_offset = y_offset - 30;
        end

         % recsApplyCallback (nested)
        function recsApplyCallback(~, ~)
            for j = 1:REC_NUM_TYPES
                switch (recPopups{j}.Value)
                    case 1
                        rec_settings{j}.color = DARK_RED;
                        rec_settings{j}.color_str = 'Red';
                    case 2
                        rec_settings{j}.color = DARK_ORANGE;
                        rec_settings{j}.color_str = 'Orange';
                    case 3
                        rec_settings{j}.color = DARK_YELLOW;
                        rec_settings{j}.color_str = 'Yellow';
                    case 4
                        rec_settings{j}.color = DARK_GREEN;
                        rec_settings{j}.color_str = 'Green';
                    case 5
                        rec_settings{j}.color = DARK_BLUE;
                        rec_settings{j}.color_str = 'Blue';
                    case 6
                        rec_settings{j}.color = DARK_PURPLE;
                        rec_settings{j}.color_str = 'Purple';
                end
                rec_settings{j}.display = recCheckboxes{j}.Value;
            end
            % show records
            if (records_checkbox.Value == false)
                records_checkbox.Value = true;
                mps.Records = true;
                new_data = true;
            else
                new_data = false;
            end
            plot_sess_record_times();
            plot_page(new_data);
        end

        % recsRevertCallback (nested)
        function recsRevertCallback(~, ~)
            read_rec_settings();
            for j = 1:REC_NUM_TYPES
                recPopups{j}.Value = j;
                recCheckboxes{j}.Value = rec_settings{j}.display;
            end
            if (records_checkbox.Value ~= saved_records_checkbox_value)
                records_checkbox.Value = saved_records_checkbox_value;
                if (records_checkbox.Value == true)
                    mps.Records = true;
                else
                    mps.Records = false;
                end
            end
            plot_sess_record_times();
            plot_page(false);
        end

        % recsSaveCallback (nested)
        function recsSaveCallback(~, ~)
            recsApplyCallback([], []);  % apply assumed
            write_rec_settings();
        end
    end

    % Record Setting Dialog Close
    function recsCloseCallback(~, ~)
        delete(rec_d);
        rec_d = [];
        set_movement_focus();
    end


    % Record Line Callback
    function rec_line_callback(src, ~)

        for k = 1:numel(record_lines)
            if (~isempty(record_lines{k}))
                if (src == record_lines{k}.flag)
                    rec_idx = k;
                    break;
                end
            end
        end

        coords = get(fig, 'Position');    % screen coordinates
        flag_screen_left = coords(1);
        flag_screen_top = coords(2);
        
        coords = get(data_ax, 'Position');    % figure coordinates
        flag_screen_left = flag_screen_left + coords(1);
        flag_screen_top = flag_screen_top + coords(2);

        coords = get(record_lines{rec_idx}.flag, 'XData');  % x axis coordinates
        flag_screen_left = flag_screen_left + coords(1);
        flag_screen_top = flag_screen_top + data_ax_height;

        rec_type = raw_page.records{rec_idx}.type_string;
        num_start_time_string = num2str(raw_page.records{rec_idx}.start_time);
        blurb = [ 'Type: ' rec_type ' v' raw_page.records{rec_idx}.version_string newline ...
            'Encryption: ' raw_page.records{rec_idx}.encryption_string newline ...
            'Start Time: ' raw_page.records{rec_idx}.start_time_string newline ...
            'Start Time (oUTC): ' num_start_time_string newline];

        switch rec_type
            case 'NlxP'
                title = 'Neuralynx Port Record';
                blurb = [blurb 'Subport: ' num2str(raw_page.records{rec_idx}.subport) newline 'Value: ' num2str(raw_page.records{rec_idx}.value)];
            case 'Note'
                title = 'Annotation Record';
                if strcmp(raw_page.records{rec_idx}.version_string, '1.001')
                    blurb = [blurb 'End Time: ' raw_page.records{rec_idx}.end_time_string newline];
                    if (isempty(raw_page.records{rec_idx}.end_time))
                        blurb = [blurb 'End Time (oUTC): <no entry>' newline];
                    else
                        blurb = [blurb 'End Time (oUTC): ' num2str(raw_page.records{rec_idx}.end_time) newline];
                    end
                end
                blurb = [blurb 'Text: ' raw_page.records{rec_idx}.text];
            case 'Seiz'
                title = 'Seizure Record';
                if strcmp(raw_page.records{rec_idx}.version_string, '1.000')
                    blurb = [blurb 'End Time: ' raw_page.records{rec_idx}.end_time_string newline];
                    if (isempty(raw_page.records{rec_idx}.end_time))
                        blurb = [blurb 'End Time (oUTC): <no entry>' newline];
                    else
                        blurb = [blurb 'End Time (oUTC): ' num2str(raw_page.records{rec_idx}.end_time) newline];
                    end
                    blurb = [blurb 'Description: ' raw_page.records{rec_idx}.description];
                end
            case 'Epoc'
                duration = double((raw_page.records{rec_idx}.end_time - raw_page.records{rec_idx}.start_time) + 1) / double(1e6);
                duration_string = [num2str(duration) ' (sec)'];
                title = 'Sleep Epoch Record';
                num_end_time_string = num2str(raw_page.records{rec_idx}.end_time);
                blurb = [blurb 'End Time: ' raw_page.records{rec_idx}.end_time_string newline ...
                    'End Time (oUTC): ' num_end_time_string newline ...
                    'Duration: ' duration_string newline ...
                    'Stage: ' raw_page.records{rec_idx}.stage_string newline ...
                    'Scorer ID: ' raw_page.records{rec_idx}.scorer_id newline];          
            case 'Sgmt'
                title = 'Segment Record';
                if ischar(raw_page.records{rec_idx}.start_sample_number)
                    start_sample_string = raw_page.records{rec_idx}.start_sample_number;
                else
                    start_sample_string = num2str(raw_page.records{rec_idx}.start_sample_number);
                end
                if ischar(raw_page.records{rec_idx}.end_sample_number)
                    end_sample_string = raw_page.records{rec_idx}.end_sample_number;
                else
                    end_sample_string = num2str(raw_page.records{rec_idx}.end_sample_number);
                end
                if ischar(raw_page.records{rec_idx}.segment_number)
                    segment_number_string = raw_page.records{rec_idx}.segment_number;
                else
                    segment_number_string = num2str(raw_page.records{rec_idx}.segment_number);
                end
                if strcmp(raw_page.records{rec_idx}.version_string, '1.000')
                    if ischar(raw_page.records{rec_idx}.segment_UID)
                        segment_UID_string = raw_page.records{rec_idx}.segment_UID;
                    else
                        segment_UID_string = num2str(raw_page.records{rec_idx}.segment_UID);
                    end
                else
                    segment_UID_string = [];
                end
                num_end_time_string = num2str(raw_page.records{rec_idx}.end_time);
                blurb = [blurb 'End Time: ' raw_page.records{rec_idx}.end_time_string newline ...
                    'End Time (oUTC): ' num_end_time_string newline ...
                    'Start Sample Number: ' start_sample_string newline ...
                    'End Sample Number: ' end_sample_string newline ...
                    'Segment Number: ' segment_number_string newline];
                if ischar(segment_UID_string)
                    blurb = [blurb 'Segment UID: ' segment_UID_string newline];
                end
                blurb = [blurb 'Description: ' raw_page.records{rec_idx}.description];             
            otherwise  % unknown record type
                title = 'Unknown Record';
        end

        clipboard('copy', blurb);
        blurb = [blurb newline newline '(copied to clipboard)'];

        % see if segment record
        if (strcmp(rec_type, 'Sgmt'))
            seg_rec = true;
            d_height = 225;
            b_height = 170;
        else
            seg_rec = false;
            d_height = 200;
            b_height = 145;
        end

        d_left = flag_screen_left - 170;
        d_bot = flag_screen_top - 260;
        d = dialog('Position', [d_left d_bot 340 d_height], ...
            'Name', title, ...
            'Color', 'white', ...
            'WindowStyle', 'normal', ...  % make dialog non-modal
            'WindowKeyPressFcn', @any_interaction_callback, ...
            'WindowButtonDownFcn', @any_interaction_callback);
        uicontrol('Parent', d, ...
           'Style', 'text', ...
           'FontSize', SYS_FONT_SIZE, ...
           'BackgroundColor', 'white', ...
           'HorizontalAlignment', 'left', ...
           'Position', [10 45 320 b_height], ...
           'Enable', 'inactive', ...  % fall through to dialog figure callbacks
           'String', blurb);

         if (seg_rec == false)  % don't allow deletion of Sgmt records
             uicontrol('Parent', d, ...
                'Style', 'pushbutton', ...
                'Position', [230 10 100 25], ...
                'String', 'Delete Record', ...
                'Callback', @d_delete_btnCallback);
         end

        % Dialog Button Callback (nested)
        function d_delete_btnCallback(~, ~)
            % confirm delete
            response = questdlg('Permanently delete this record?', 'View MED', 'Cancel', 'Yes', 'Cancel');
            switch response
                case 'Yes'
                case 'Cancel'
                    set(fig, 'Pointer', 'arrow');
                    return;
                otherwise
                    set(fig, 'Pointer', 'arrow');
                    return;
            end
            rec_time = raw_page.records{rec_idx}.start_time;
            rec_code = raw_page.records{rec_idx}.type_code;
            if (new_page_secs > WAIT_POINTER_DELAY)
                set(fig, 'Pointer', 'watch');
                reset_pointer = true;
                drawnow;
            end
            close(d);
            mps.Persist = 2;  % close
            [~] = matrix_MED_exec(mps);  % close to allow writing of record files by delete_record_exec()
            mps.Persist = 4; % set back to read
            err = delete_record_exec(chan_paths{1}, password, rec_time, rec_code);
            clear delete_record_exec;
            if (err < 0)
                set(fig, 'Pointer', 'arrow');
                if (err == -1)
                    errordlg('Error deleting record', 'View MED');
                else
                    errordlg('Insufficient access to delete record', 'View MED');
                end
                return;
            else
                delete(record_lines{rec_idx}.line);
                delete(record_lines{rec_idx}.flag);
            end

            % remove session map line
            rec_time_prop = double(rec_time - sess_start) / sess_duration;
            rec_str = raw_page.records{rec_idx}.type_string;
            for i = 1:REC_NUM_TYPES
                switch (rec_str)
                    case 'HFOc'
                        rec_type_idx = REC_HFOc_IDX;
                    case 'NlxP'
                        rec_type_idx = REC_NlxP_IDX;
                    case 'Note'
                        rec_type_idx = REC_Note_IDX;
                    case 'Seiz'
                        rec_type_idx = REC_Seiz_IDX;
                    case 'Sgmt'
                        rec_type_idx = REC_Sgmt_IDX;
                end
            end
            n_lines = numel(sess_record_times{rec_type_idx});
            for j = 1:n_lines
                if (sess_record_times{rec_type_idx}(j) == rec_time_prop)
                    break;
                end
            end
            if (j < n_lines)
                sess_record_times{rec_type_idx} = [sess_record_times{rec_type_idx}(1:(i - 1)); sess_record_times{rec_type_idx}((i + 1):end)];
            else
                sess_record_times{rec_type_idx} = [sess_record_times{rec_type_idx}(1:(end - 1))];
            end
            plot_sess_record_times();
            plot_page(true);
        end

        any_interaction = false;  % reset global
        total_time = 0;
        while (true)
            if (any_interaction == true) || (gcf ~= d) || (total_time >= 15)    
                break;
            end
            pause(0.1);
            total_time = total_time + 0.1;
        end
        if (isvalid(d))
            close(d);
        end

        set_movement_focus();
    end

    % Discontinuity Line Callback
    function discont_line_callback(src, ~)
       for i = 1:numel(discont_lines)
            if (src == discont_lines{i}.zag)
                discont_idx = i;
                break;
            end
        end

        coords = get(fig, 'Position');    % screen coordinates
        zag_screen_left = coords(1);
        zag_screen_top = coords(2);
        
        coords = get(data_ax, 'Position');    % figure coordinates
        zag_screen_left = zag_screen_left + coords(1);
        zag_screen_top = zag_screen_top + coords(2);

        coords = get(discont_lines{i}.zag, 'XData');  % x axis coordinates
        zag_screen_left = zag_screen_left + coords(1);
        zag_screen_top = zag_screen_top + data_ax_height;

        discont_start_time = raw_page.contigua(discont_idx).end_time + 1;
        discont_end_time = raw_page.contigua(discont_idx + 1).start_time - 1;
        discont_start_string = ['Start Time: ' raw_page.contigua(discont_idx).end_time_string];
        if discont_end_time <= discont_start_time
            discont_dur = 0;      
            discont_end_time = discont_start_time;  % can occur due to time rounding error (set duration to zero)
            discont_end_string = discont_start_string;
        else
            discont_dur = double((discont_end_time - discont_start_time) + 1);      
            discont_end_string = ['End Time: ' raw_page.contigua(discont_idx + 1).start_time_string];
        end
        if (discont_dur >= 3600000000)
            discont_dur = discont_dur / 3600000000;
            discont_unit = ' hours';
        elseif (discont_dur >= 60000000)
            discont_dur = discont_dur / 60000000;
            discont_unit = ' minutes';
        elseif (discont_dur >= 1000000)
            discont_dur = discont_dur / 1000000;
            discont_unit = ' seconds';
        elseif (discont_dur >= 1000)
            discont_dur = discont_dur / 1000;
            discont_unit = ' milliseconds';
        else
            discont_unit = ' microseconds';
        end
        duration_string = ['Duration: ' num2str(discont_dur) discont_unit];
        num_start_string = ['Start Time (oUTC): ' num2str(discont_start_time)];
        num_end_string = ['End Time (oUTC): ' num2str(discont_end_time)];

        blurb = [discont_start_string newline num_start_string newline discont_end_string newline num_end_string newline duration_string];
        clipboard('copy', blurb);
        blurb = [blurb newline newline '(copied to clipboard)'];

        d_left = zag_screen_left - 170;
        d_bot = zag_screen_top - 180;
        d = dialog('Position', [d_left d_bot 340 120], ...
            'Name', 'Discontinuity', ...
            'Color', 'white', ...
            'WindowStyle', 'normal', ...  % make dialog non-modal
            'WindowKeyPressFcn', @any_interaction_callback, ...
            'WindowButtonDownFcn', @any_interaction_callback);

        uicontrol('Parent', d, ...
           'Position', [10 10 320 100], ...
           'Style', 'text', ...
           'FontSize', SYS_FONT_SIZE, ...
           'BackgroundColor', 'white', ...
           'HorizontalAlignment', 'left', ...
           'Enable', 'inactive', ...  % fall through to dialog figure callbacks
           'String', blurb);

        any_interaction = false;  % reset global
        total_time = 0;
        while (true)
            if (any_interaction == true) || (gcf ~= d) || (total_time >= 15)    
                break;
            end
            pause(0.1);
            total_time = total_time + 0.1;
        end
        if (isvalid(d))
            close(d);
        end

        set_movement_focus();
    end

    % Add Record Callback
    function add_record_callback(~, ~)
        add_record_flag = add_record_button.Value;
        % rest handled by axis_mouse_down() -> add_record()
    end

    % Logo Callback
    function logo_callback(~, ~)
        d = dialog('Position', [400 400 220 140], 'Name', 'About View MED', 'Color', 'white');
        
        blurb = ['A Matlab viewer for MED format files' newline newline ...
            'written by Matt Stead' newline newline ...
            'Copyright Dark Horse Neuro, 2021' newline ...
            'Bozeman, Montana, USA' newline newline ...
            'w.a.t.i.w.'];
        
        uicontrol('Parent', d, ...
           'Style', 'text', ...
           'FontSize', SYS_FONT_SIZE, ...
           'BackgroundColor', 'white', ...
           'HorizontalAlignment', 'left', ...
           'Position', [10 10 200 120], ...
           'String', blurb);
        
        sound(neh_signal, neh_sf);

        uiwait(d, 15);
        if (isvalid(d))
            close(d);
        end

        set_movement_focus();
    end

    % Axis Drag Functions
    function ax_mouse_down_callback(~, ~)
        ax_mouse_down = true;  % reset by ax_mouse_up()

        curr_p = get(fig, 'CurrentPoint');
        x = round(curr_p(1));
        if (x <= data_ax_left || x >= data_ax_right)
            ax_mouse_down = false;
            return;
        end
        y = round(curr_p(2));
        if (y <= data_ax_bot || y >= data_ax_top)  % leave room for clicking on flags & discontinuities)
            ax_mouse_down = false;
            return;
        end

        % abort: user clicked & let go
        pause(0.3);
        if (ax_mouse_down == false)
            return;
        end

        % check for click & drag (zoom)
        fig_pos = get(fig, 'Position');  % convert to screen coordinates
        last_x = x + fig_pos(1);

        % add record mode
        if add_record_flag == true
            add_record(last_x);
            return;
        end

        last_y = y + fig_pos(2);
        pause(0.1);
        curr_p = get(0, 'PointerLocation');  % can't get new figure location until new click - must use screen coordingates
        new_x = round(curr_p(1));
        new_y = round(curr_p(2));
        dxy = sqrt(((new_x - last_x) ^ 2) + ((new_y - last_y) ^ 2));
        if (dxy >= 3)
            zoom(last_x, last_y);
            return;
        end

        % plot big page
        page_start = page_start - wind_usecs;
        wind_usecs = wind_usecs * 3;
        x_ax_inds = ((1 - data_ax_width):(2 * data_ax_width))';
        x_tick_inds = linspace(x_ax_inds(1), x_ax_inds(end), 31);
        full_page_width = data_ax_width * 3;
        mps.SampDim = full_page_width;
        plot_handles = [];
        plot_page(true);

        % drag
        set(fig, 'Pointer', 'hand'); 
        drawnow;
        cum_dx = 0;
        new_lims = [1 data_ax_width];
        while (ax_mouse_down == true)
            curr_p = get(0, 'PointerLocation');
            curr_x = round(curr_p(1));
            dx = last_x - curr_x;
            if (dx)
                new_lims = new_lims + dx;
                cum_dx = cum_dx + dx;
                last_x = curr_x;
                set(data_ax, 'XLim', new_lims);
            end
            pause(0.05);  % give ax_mouse_up_callback a chance to run
        end

        % get new page with dragged limits
        wind_usecs = round(wind_usecs / 3);
        new_page_secs = new_page_secs / 3;
        page_start = page_start + wind_usecs + round(wind_usecs * (cum_dx / data_ax_width));
        x_ax_inds = (1:data_ax_width)';
        x_tick_inds = linspace(1, data_ax_width, 11);
        full_page_width = data_ax_width;
        mps.SampDim = full_page_width;
        reset_pointer = true;
        plot_handles = [];        
        plot_page(true);
        set_movement_focus();
    end

    function add_record(x_orig)
        persistent adding_record;
        if adding_record == true
            return;
        end
        adding_record = true;

        fig_pos = get(fig, 'Position');  % convert to data axes coordinates
        xo = (x_orig - fig_pos(1)) - data_ax_left;
        new_record_line = [];
        new_record_flag = [];
        while ax_mouse_down == true
            curr_p = get(0, 'PointerLocation');
            dx = round(curr_p(1)) - x_orig;
            if (abs(dx) <= 1)
                pause(0.05);  % give ax_mouse_up_callback a chance to run
                continue;
            end

            % clear old record line
            if (~isempty(new_record_line))
                delete(new_record_line);
                delete(new_record_flag);
            end

            % draw new record line
            data_x = xo + dx;
            new_record_line = line(data_ax, [data_x, data_x], [20 data_ax_height], 'color', LIGHT_GREEN, 'LineWidth', 2, 'LineStyle', '--');
            patch_x = [-1, 19, 14, 19, -1, -1] + data_x;
            patch_y = [1, 1, 7, 14, 14, 1];
            new_record_flag = patch(data_ax, patch_x, patch_y, LIGHT_GREEN, 'EdgeColor', 'none', 'ButtonDownFcn', @rec_line_callback);

            pause(0.05);  % give ax_mouse_up_callback a chance to run
        end

        % show add record dialog
        add_rec = false;
        rec_text = '';
        rec_type = 'Note';
        rec_type_idx = REC_Note_IDX;
        rec_enc = 0;
        d = dialog('Position', [400 400 550 125], 'Name', 'Add Record');
        % Note Textbox & Label
        d_text_label = uicontrol('Parent', d, ...
            'Style', 'text', ...
            'Position', [33 85 60 25], ...
            'Units', 'pixels', ...
            'String', 'Note Text:', ...
            'FontSize', SYS_FONT_SIZE, ...
            'HorizontalAlignment', 'left');    
        d_note_textbox = uicontrol('Parent', d, ...
            'Style', 'edit', ...
            'Position', [33 65 489 25], ...
            'Units', 'pixels', ...
            'String', '', ...
            'BackgroundColor', 'white', ...
            'FontSize', SYS_FONT_SIZE, ...
            'HorizontalAlignment', 'left');
        uicontrol('Parent', d, ...
            'Style', 'text', ...
            'Position', [33 17 55 25], ...
            'Units', 'pixels', ...
            'String', 'Type:', ...
            'FontSize', SYS_FONT_SIZE, ...
            'HorizontalAlignment', 'left');    
        d_type_popup = uicontrol('Parent', d, ...
            'Style', 'popupmenu', ...
            'Position', [60 17 105 25], ...
            'String', {'Annotation', 'Seizure'}, ...
            'Callback', @d_type_ddCallback);
        uicontrol('Parent', d, ...
            'Style', 'text', ...
            'Position', [183 17 55 25], ...
            'Units', 'pixels', ...
            'String', 'Encryption:', ...
            'FontSize', SYS_FONT_SIZE, ...
            'HorizontalAlignment', 'left');    
        d_enc_popup = uicontrol('Parent', d, ...
            'Style', 'popupmenu', ...
            'Position', [235 17 105 25], ...
            'String', {'None', 'Level 1','Level 2'});
        uicontrol('Parent', d, ...
            'Style', 'pushbutton', ...
            'Position', [380 20 70 25], ...
            'String', 'Cancel', ...
            'Callback', @d_cancel_btnCallback);
        uicontrol('Parent', d, ...
            'Style', 'pushbutton', ...
            'Position', [455 20 70 25], ...
            'String', 'Add', ...
            'Callback', @d_add_btnCallback);
        uicontrol(d_note_textbox);

        % Dialog Button Callbacks (nested)
        function d_add_btnCallback(~, ~)
            add_rec = true;
            rec_enc = d_enc_popup.Value - 1;
            rec_text = d_note_textbox.String;
            if (d_type_popup.Value == 1)
                rec_type = 'Note';
                rec_type_idx = REC_Note_IDX;
            else
                rec_type = 'Seiz';
                rec_type_idx = REC_Seiz_IDX;
            end
            close(d);
        end
        function d_cancel_btnCallback(~, ~)        
            close(d);
        end
        function d_type_ddCallback(~, ~)
            if (d_type_popup.Value == 1)
                set(d_text_label, 'String', 'Note Text:');
            else
                set(d_text_label, 'String', 'Description:');
            end
        end

        uiwait(d);
        if (isvalid(d))  % user hit close button
            close(d);
        end

        if (add_rec == true)
            if (isempty(rec_text))
                if (strcmp(rec_type, 'Note') == true)
                    errordlg('No note text entered', 'View MED');
                end
                adding_record = false;
                delete(new_record_line);
                delete(new_record_flag);
                add_record_button.Value = false;
                add_record_flag = false;
                set(fig, 'Pointer', 'arrow');
                return;
            end
            if (new_page_secs > WAIT_POINTER_DELAY)
                set(fig, 'Pointer', 'watch');
                reset_pointer = true;
                drawnow;
            end
            page_duration = double(page_end - page_start) + 1;
            rec_time = page_start + round(page_duration * (data_x / data_ax_width));
   
            % add record to MED file
            mps.Persist = 2;  % close
            [~] = matrix_MED_exec(mps);  % close to allow writing of record files by add_record_exec()
            mps.Persist = 4; % set back to read
            err = add_record_exec(chan_paths{1}, password, rec_type, rec_time, rec_text, rec_enc);
            clear add_record_exec;
            if (err < 0)
                if (err == -1)
                    errordlg('Error adding record', 'View MED');
                else
                    errordlg('Insufficient access for selected encryption level', 'View MED');
                end
                adding_record = false;
                delete(new_record_line);
                delete(new_record_flag);
                add_record_button.Value = false;
                add_record_flag = false;
                set(fig, 'Pointer', 'arrow');
                return;
            end

            % add record line to session map
            new_rec_time_prop = double(rec_time - sess_start) / sess_duration;
            if (isempty(sess_record_times{rec_type_idx}))
                sess_record_times{rec_type_idx}(1) = new_rec_time_prop;
            else
                break_flag = false;
                for i = 1:numel(sess_record_times{rec_type_idx})
                    if (sess_record_times{rec_type_idx}(i) > new_rec_time_prop)
                        break_flag = true;
                        break;
                    end
                end
                if (break_flag == true)
                    sess_record_times{rec_type_idx} = [sess_record_times{rec_type_idx}(1:(i - 1)); new_rec_time_prop; sess_record_times{rec_type_idx}(i:end)];
                else
                    sess_record_times{rec_type_idx} = [sess_record_times{rec_type_idx}; new_rec_time_prop];
                end
            end

            % assume user would like to see new record
            if (mps.Records == false)
                records_checkbox.Value = true;
                mps.Records = true;
            end
            plot_sess_record_times();
        end

        % clear flag
        delete(new_record_line);
        delete(new_record_flag);

        % reset button
        add_record_button.Value = false;
        add_record_flag = false;

        % plot
        if add_rec == true
            plot_page(true);
        end

        adding_record = false;
        set_movement_focus();
    end


    function zoom(x_orig, y_orig)
        set(fig, 'Pointer', 'crosshair'); 
        fig_pos = get(fig, 'Position');  % convert to data axes coordinates
        xo = (x_orig - fig_pos(1)) - data_ax_left;
        yo = data_ax_height - ((y_orig - fig_pos(2)) - data_ax_bot);  % y axis inverted
        r = rectangle(data_ax, 'Position', [xo yo 0 0], 'EdgeColor', 'k', 'FaceColor', [0 0.5 0], 'FaceAlpha', 0.1);
        while ax_mouse_down == true
            curr_p = get(0, 'PointerLocation');
            dx = round(curr_p(1)) - x_orig;
            if (dx < 0)
                l = xo + dx;
                dx = abs(dx);
            else
                l = xo;
            end
            dy = round(y_orig - curr_p(2));  % y axis inverted
            if (dy < 0)
                b = yo + dy;
                dy = abs(dy);
            else
                b = yo;
            end
            set(r,'Position', [l b dx dy]);  % 4th argument is alpha
            pause(0.05);  % give ax_mouse_up_callback a chance to run
        end

        % highlight selected channels
        box_top = b;
        box_bot = (b + dy) - 1;
        sel_chans = zeros(n_chans, 1);
        for i = 1:n_chans
            chan_pos = get(chan_labels{i}, 'Position');
            chan_y = data_ax_height - chan_pos(2);  % label axes not inverted
            if chan_y >= box_top && chan_y <= box_bot
                sel_chans(i) = 1;
                set(chan_labels{i}, 'FontAngle', 'italic', 'BackgroundColor', panel_color * 0.925);
                if (monochrome_flag == true)
                    set(chan_labels{i}, 'Color', 'red');
                end
            end
        end

        % show zoom dialog
        zoom_chans = 0;
        zoom_time = 0;            
        d = dialog('Position', [400 400 235 130], 'Name', 'Zoom');
        d_chan_chkbox = uicontrol('Parent', d, ...
            'Style', 'checkbox', ...
            'String', 'Zoom to Selected Channels', ...
            'Value', true, ...
            'Position', [35 80 210 40], ...
            'BackgroundColor', panel_color, ...
            'FontSize', SYS_FONT_SIZE);
        d_time_chkbox = uicontrol('Parent', d, ...
            'Style', 'checkbox', ...
            'String', 'Zoom to Selected Time Range', ...
            'Value', true, ...
            'Position', [35 50 210 40], ...
            'BackgroundColor', panel_color, ...
            'FontSize', SYS_FONT_SIZE);
        d_ok_btn = uicontrol('Parent', d, ...
            'Style', 'pushbutton', ...
            'Position', [135 20 70 25], ...
            'String', 'OK', ...
            'Callback', @d_ok_btnCallback);
        uicontrol('Parent', d, ...
            'Style', 'pushbutton', ...
            'Position', [35 20 70 25], ...
            'String', 'Cancel', ...
            'Callback', @d_cancel_btnCallback);
        uicontrol(d_ok_btn);

        % Dialog Button Callbacks (nested)
        function d_ok_btnCallback(~, ~)
            zoom_chans = logical(d_chan_chkbox.Value);
            zoom_time = logical(d_time_chkbox.Value);
            if (new_page_secs > WAIT_POINTER_DELAY)
                set(d, 'Pointer', 'watch');
                drawnow;
            end
            reset_pointer = true;
            close(d);
        end
        function d_cancel_btnCallback(~, ~)
            close(d);
        end

        uiwait(d);
        if (isvalid(d))  % user hit close button
            close(d);
        end

        % unhighlight selected channels
        j = 0;
        for i = 1:n_chans
            if (sel_chans(i))
                j = j + 1;
                set(chan_labels{i}, 'FontAngle', 'normal', 'BackgroundColor', panel_color);
                if (monochrome_flag == true)
                    set(chan_labels{i}, 'Color', 'black');
                end
            end
        end
        if (j == 0 || j == n_chans)
            zoom_chans = false;
        end
        delete(r);

        if (zoom_chans == false && zoom_time == false)     
            set(fig, 'Pointer', 'arrow');
            set_movement_focus();
            return;
        end

        % Update channel lists & plotting variables
        if (zoom_chans == true)
            mps.Persist = 2;  % close
            [~] = matrix_MED_exec(mps);  % close matrix to force re-read
            mps.Persist = 4; % read
            j = 0;
            for i = 1:n_chans
                if (sel_chans(i))
                    j = j + 1;
                    if (j ~= i)
                        chan_paths{j} = chan_paths{i};
                        chan_list{j} = chan_list{i};
                    end
                end
            end
            n_chans = j;
            chan_paths = chan_paths(1:n_chans);
            mps.Data = chan_paths;
            chan_list = chan_list(1:n_chans);

            plot_handles = [];
            if (autoscale_flag == false)  % rescale plots for new trace set
                set(autoscale_button, 'String', 'Autoscaling is On');
                autoscale_flag = true;
            end   
        end

        % Update page limits
        if (zoom_time == true)
            page_duration = double(page_end - page_start) + 1;
            box_left = l;
            box_right = (l + dx) - 1;
            page_end = page_start + round(page_duration * (box_right / data_ax_width));
            page_start = page_start + round(page_duration * (box_left / data_ax_width));
            wind_usecs = (page_end - page_start) + 1;
            set(timebase_textbox, 'String', num2str(double(wind_usecs) / 1e6, '%0.6f'));
            set_data_ax_x_labels();
        end

        % Plot
        plot_page(true);

        % Make channel labels
        if (zoom_chans == true)
            selected_labels = [];
            create_labels();
            draw_labels();
        end

        set_movement_focus();
    end

    % Axis Mouse Up Callback
    function ax_mouse_up_callback(~, ~)
        ax_mouse_down = false;
    end

    function any_interaction_callback(~, ~)
        any_interaction = true;
    end

	% Figure Close Callback
    function figure_close_callback(~, ~)
        mps.Persist = 2;  % close
        [~] = matrix_MED_exec(mps);  % close matrix
        delete(fig);
        if (isempty(rec_d) == false)
            delete(rec_d);
        end
    end

    % comment out to allow control to return to command window after loading 
    % uiwait(fig);  % wait for fig to be deleted

end % view_MED

