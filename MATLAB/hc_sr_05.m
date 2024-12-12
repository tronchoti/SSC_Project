% ASK USER THE FILE TO OPEN
[file, location] = uigetfile('*.txt');

if isequal(file,0)
    disp("User didn't choose a file");
else
    fprintf("File selected: %s\n", fullfile(location, file));
end

data = readtable(fullfile(location, file));

T = data{:,1};

t = linspace(0,height(data), height(data));

%disp(data(:,"Humidity"));

A = std(T);
B = mean(T);
fprintf("Standard deviation: %f\n", A);
fprintf("Mean: %f\n", B);
plot(t, T);


