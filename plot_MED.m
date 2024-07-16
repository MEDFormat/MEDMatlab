function plot_MED(session)
    %
    %   plot_MED() requires 1 input
    %
    %   Prototype:
    %   plot_MED(session);
    %
    %   plot_MED() will plot the data in the session.
    %   Traces are offset in the plot for visualization; they do not represent recorded DC offsets.
    %
    %   Input Arguments:
    %   session: a Matlab session structure obtained from read_MED(), read_MED_GUI(), read_MED_exec(), or exported from view_MED().
    %
    %   Copyright Dark Horse Neuro, 2020
    
    if nargin ~= 1
        help plot_MED;
        return;
    end

    n_chans = numel(session.channels);
    rv = zeros(n_chans, 1);
    v = {};
    
    for i = 1:n_chans
        tv = session.channels(i).data;
        mv = median(tv);
        tv = tv - mv;
        try
            rv(i) = range(tv);  % range() requires toolbox
        catch
            rv(i) = max(tv) - min(tv);
        end
        v{i} = tv;
    end
    
    % check for variable frequency (& minimum length => even if same
    % sampling frequencies, channels can have slightly different length due
    % to time extrapolation rounding from variable block starts
    sf = session.channels(1).metadata.sampling_frequency;
    var_freq = false;
    min_len = length(session.channels(1).data);
    for i = 2:n_chans
        if (session.channels(i).metadata.sampling_frequency ~= sf)
            var_freq = true;
        end
        if (length(session.channels(i).data) < min_len)
            min_len = length(session.channels(i).data);
        end
    end
    if (var_freq == false)
        n = min_len;
        t =(0:(n - 1))' / sf;
    end
    
    % plot
    scale = max(rv);
    offset = scale / 2;
    yt = zeros(n_chans, 1);
    ytl = {};
    figure;
    set(gca, 'TickLabelInterpreter', 'none')
    hold on;
    for i = 1:n_chans
        if (var_freq == true)
            n = numel(session.channels(i).data);
            sf = session.channels(i).metadata.sampling_frequency;
            t = (0:(n - 1))' / sf;
        end
        tv = cell2mat(v(i));
        yt(i) = offset;
        ytl{i} = session.channels(i).metadata.channel_name;
        tv = tv + offset;
        offset = offset + scale;
        plot(t, tv(1:n));
    end
    set(gca, 'YDir', 'reverse');   % put first channel at top, and make negative up
    yticks(yt);
    yticklabels(ytl);
    ylabel('Channels', 'FontSize', 14);
    xlabel('Time (seconds)', 'FontSize', 14);
    set(gca, 'Xlim', [0 t(end)]);
    set(gcf, 'Name', ['  ' inputname(1) ' from session "' session.metadata.session_name '"']);
    hold off;
    
end

