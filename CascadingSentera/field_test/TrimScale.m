% TrimScale
% trims then scales data between specified limits
    % trim: anything above or below limits set to limit
    % scale: scaled from current range to specified range
function new_data = TrimScale(old_data, trim_min, trim_max, old_scale_min, old_scale_max, new_scale_min, new_scale_max)
new_data = old_data;

% trim
new_data(new_data < trim_min) = trim_min;
new_data(new_data > trim_max) = trim_max;

% scale
k = (new_scale_max - new_scale_min) / (old_scale_max - old_scale_min);
offset = new_scale_min - old_scale_min;

new_data = (new_data + offset) * k;
end