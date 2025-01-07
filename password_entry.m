function password_entry(src, ~)

    % password stored in src.UserData.password
    % src == 'edit' textbox whose 'KeyPressFcn' == @password_entry
    % src 'Callback' is disabled during entry, and called when enter/return hit
    % there may be a better way to do this, but it works nicely


    if (isempty(src.UserData))  % store password & callback in UserData
        set(src, 'Interruptible', 'off');  % do not allow other key presses until this is done
        src.UserData = struct('password', '', 'callback', src.Callback);
        src.Callback = [];
    end
    c = get(gcf, 'CurrentCharacter');  % char(evt.Key) will not get shifted characters
    if (isempty(c))  % modifier key
        return;
    end

    % non-printable characters
    len = length(src.UserData.password);
    if (c < 33 || c > 126)  
        switch (c)
            case {8, 127}  % backspace, delete
                len = len - 1;
                src.UserData.password = src.UserData.password(1:len);
                ast_str = repmat('*', [1 len]);
                src.String = ast_str;  % first time display 
                drawnow;  % 'edit' builtin display replaces previous string
                src.String = ast_str;  % second display
                return;
            case {9, 10, 13, 27}  % tab, newline, carriage return, escape
                src.Callback = src.UserData.callback;  % src callback will be called on return
                set(src, 'Interruptible', 'on');  % reset default - not sure if this is necessary
                return;
            otherwise
                return;
        end
    end

    % printable characters
    src.UserData.password(len + 1) = c;
    ast_str = [repmat('*', [1 len]) c];
    src.String = ast_str;  % first time display (asterisks with last character)
    drawnow;   % 'edit' builtin display replaces previous string
    src.String = ast_str;  % second display
    pause(0.3);  % show asterisks with last character for a brief time
    ast_str(end) = '*';  % replace with all asterisks
    src.String = ast_str;
end

