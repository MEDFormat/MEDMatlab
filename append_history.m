function append_history(cmd, exec_time)
    % exec_time is in microseconds
    % tic; <command>; exec_time = round(toc * 1e6);

    hist_path = fullfile(prefdir,'History.xml');
    if (exist(hist_path,'file'))
        % read history file
        txt = fileread(hist_path);

        % condition cmd
        if (isstring(cmd))
            cmd = char(cmd);
        end
        len = length(cmd);
        if (cmd(len) ~= ';')
            cmd(len + 1) = ';';
        end

        % build xml history entry
        xml_cmd = ['<command execution_time=' '"' num2str(exec_time) '">' cmd '</command>' newline];
        new_txt = insertBefore(txt, '</session>', xml_cmd);

        % write out file
        fp = fopen(hist_path, 'w');
        fwrite(fp, new_txt, 'char');
        fclose(fp);
    else
        beep
        fprintf(2, 'File not found: ''History.xml''\n');
        disp('Cannot update history.');
        return
    end
end