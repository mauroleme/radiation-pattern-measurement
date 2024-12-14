clear;                                                          % Clear variables from workspace
clc;                                                            % Clear the command window

% Serial port configuration
arduinoPort = "COM7";                                           % COM port used by Arduino
baudRate    = 115200;                                           % Serial communication baudrate (bps)

% Initialize the serial port
serialPort          = serialport(arduinoPort, baudRate);        % Configure the serial port
serialPort.Timeout  = 30;                                       % Set the timeout duration (seconds)
configureTerminator(serialPort, "CR/LF");                       % Set the line terminator

disp("Waiting for Arduino to initialize...");
while true
    try
        response = readline(serialPort);                        % Read response from Arduino
        if strcmp(response, "Ready.")                           % Check for successful initialization
            break;
        elseif strncmp(response, "Error", 5)                    % Check for error messages
            error("Error during setup.");
        end
    catch
        pause(0.5);                                             % Retry after a short delay
    end
end


disp("Resquesting measurements for 360 degrees...");

% Initialize the result vector
samplesPerDegree        = 10;
measurementValues       = zeros(360, 1);                        % Vector to store final measurements

for degree = 0:359
    try
        response                = writeread(serialPort, "1");
        currentDegreeSamples    = str2double(split(response, ','));
        if any(isnan(currentDegreeSamples))
            error("Invalind response format.");
        end
    catch
        currentDegreeSamples    = -1 * ones(samplesPerDegree, 1);
    end
    measurementValues(degree + 1)   = mean(currentDegreeSamples);
    fprintf("Degree %3d: Value %.2f\n", degree, measurementValues(degree + 1));
end

disp("Collected measurements:");
disp(measurementValues);

% Reverse the indexing logic for position 181 (180 degrees) onward, as the
% returned values are received in a circular order: starting from -1 degree
% (359 degrees) and progressing to 179 degrees (181 degrees)
measurementValues(181:360) = flip(measurementValues(181:360));

clear port;                                                     % Close the serial port
