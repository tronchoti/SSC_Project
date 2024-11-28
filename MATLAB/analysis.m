% ASK USER THE FILE TO OPEN
[file, location] = uigetfile('*.txt');

if isequal(file,0)
    disp("User didn't choose a file");
else
    fprintf("File selected: %s\n", fullfile(location, file));
end

data = readtable(fullfile(location, file));

disp(data(1:10,1:2));

t = linspace(0,height(data),height(data));
x = data.Temperature;
y = data.Humidity;

plot(t,x);
title('Temperature and humidity');

hold on

plot(t, y);

hold off

