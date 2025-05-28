clear;                                                          % Clear variables from workspace
clc;                                                            % Clear the command window

% Automatically detect the available COM ports
availablePorts = serialportlist("available");

if isempty(availablePorts)
    error("No available COM ports detected.");
end

% Display the list of available ports
disp("Available COM ports:");
disp(availablePorts);

% Serial port configuration
arduinoPort = availablePorts{1};                                % Automatically choose the first available port
baudRate    = 115200;                                           % Serial communication baudrate (bps)

% Initialize the serial port
serialPort         = serialport(arduinoPort, baudRate);         % Configure the serial port
serialPort.Timeout = 120;                                       % Set the timeout duration (seconds)
configureTerminator(serialPort, "CR/LF");                       % Set the line terminator

% Wait for Arduino to initialize the serial port
waitForArduino(serialPort, "Ready?", ...
    "Waiting for Arduino Serial Port to initialize...");

% Command and wait Arduino to home the joints
writeline(serialPort, "Set.");
waitForArduino(serialPort, "Go.", ...
    "Waiting for Arduino to finish homing the joints...");

% Initialize the result vector
degreeResolution  = 5;                                          % Must be an integer
theta             = [0:degreeResolution:180, ...                % Azimuth
                     -degreeResolution:-degreeResolution:-180+degreeResolution];
phi               = 0:0;                                        % Elevation
measurementValues = zeros(length(theta) + 1, ... 
                          length(phi) + 1);                     % Matrix to store measurements for two motors

% Start sampling the antenna
disp("Requesting measurements...");
for motor2Degree = phi
    for motor1Degree = theta
        while true
            try
                % Send the angles and read the response
                response = safeWriteRead(serialPort, sprintf("%d,%d", ...
                                                             motor1Degree, ...
                                                             motor2Degree));

                % Convert and validate numeric data
                data = str2double(split(response, ','));
                if any(isnan(data))
                    error("Invalid numeric format.");
                end

                % Store the mean of the first samples
                rowIndex = floor(mod(motor1Degree, 360) /...
                                 degreeResolution) + 1;
                colIndex = floor(mod(motor2Degree, 360) /...
                                 degreeResolution) + 1;
                measurementValues(rowIndex, colIndex) = ...
                    mean(data(1:end));
                break;

            catch ME
                fprintf("Error at angles %d,%d: %s. Response was: %s. Retrying...\n", ...
                        motor1Degree, motor2Degree, ME.message, response);
                pause(0.1);
            end
        end
    end
end

clear serialPort;                                               % Close the serial port

% Map theta [-180,180] to [0,360]
mappedAngles            = mod(theta, 360);
[sortedAngles, sortIdx] = sort(mappedAngles);
sortedMeasurementValues = measurementValues(sortIdx);

% Close the circle for polarplot continuity
fullAnglesClosed        = [sortedAngles, sortedAngles(1)];
MeasurementValuesClosed = [sortedMeasurementValues, ...
                           sortedMeasurementValues(1)];

% Rotate plot by +270 deg so that 180° becomes 90° (top)
rotatedAngles = mod(fullAnglesClosed + 270, 360);

% Plot
figure;
polarplot(deg2rad(rotatedAngles), MeasurementValuesClosed, 'LineWidth', 1.5);
set(gca, 'ThetaZeroLocation','top','ThetaDir','counterclockwise');
rlim([-75 0]);
title('Antenna Radiation Pattern');
text(-0.2, -0.9, 'Azimuth', 'Units', 'normalized');
text(-0.9, -0.2, 'Gain (dB)', 'Units', 'normalized');

disp("Collected measurements:");
disp(MeasurementValuesClosed(:));

% Function for safe serial communication with error handling
function response = safeWriteRead(serialPort, message)
    try
        if nargin > 1
            writeline(serialPort, message);
        end

        response = readline(serialPort);
        if startsWith(response, "Error: ")
            error("Arduino error: %s", extractAfter(response, "Error: "));
        end
    catch ME
        error("Communication error: %s", ME.message);
    end
end

function waitForArduino(serialPort, expectedResponse, message)
    disp(message);
    while true
        try
            response = safeWriteRead(serialPort);
            if strcmp(response, expectedResponse)
                break;
            end
        catch
            pause(0.5);
        end
    end
end
