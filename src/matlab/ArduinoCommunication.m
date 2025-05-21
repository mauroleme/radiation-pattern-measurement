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
serialPort.Timeout = 30;                                        % Set the timeout duration (seconds)
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
theta             = [0:degreeResolution:180, ...
                     -1:-degreeResolution:-179];                 % Azimuth
phi               = 0:0;                                        % Elevation
measurementValues = zeros(length(theta) + 1, ... 
                                length(phi) + 1);               % Matrix to store measurements for two motors

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
                rowIndex = mod(motor1Degree, 360) / degreeResolution + 1;
                colIndex = mod(motor2Degree, 360) / degreeResolution + 1;
                measurementValues(rowIndex, colIndex) = ...
                    mean(data(1:end));
                break;

            catch ME
                fprintf("Error at angles %d,%d: %s. Retrying...\n", ...
                        motor1Degree, motor2Degree, ME.message);
                pause(0.1);
            end
        end
    end
end

% Copy the 0 degree row to the 360 degree row to wrap it
measurementValues(end,:) = measurementValues(1,:);

disp("Collected measurements:");
disp(measurementValues);

clear serialPort;                                               % Close the serial port

figure;
thetaClosed       = [theta theta(1)];                           % Repeat the final -180 degree
measurementClosed = [measurementValues(1:end-1,1); measurementValues(1,1)];
polarplot(deg2rad(thetaClosed), measurementClosed);
rlim([-65 0]);
disp([measurementClosed(:)])

% Function for safe serial communication with error handling
function response = safeWriteRead(serialPort, message)
    try
        if nargin > 1
            writeline(serialPort, message);                     % Send data
        end
        response = readline(serialPort);                        % Read response from Arduino
        
        % Check for Arduino error message
        if startsWith(response, "Error: ")
            error("Arduino error: %s", extractAfter(response, "Error: "));
        end
    catch ME
        error("Communication error: %s", ME.message);           % Throw a detailed error message
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
