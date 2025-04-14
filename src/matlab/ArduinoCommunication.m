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
arduinoPort             = availablePorts{1};                    % Automatically choose the first available port
baudRate                = 115200;                               % Serial communication baudrate (bps)

% Initialize the serial port
serialPort              = serialport(arduinoPort, baudRate);    % Configure the serial port
serialPort.Timeout      = 30;                                   % Set the timeout duration (seconds)
configureTerminator(serialPort, "CR/LF");                       % Set the line terminator
pause(5);

writeline(serialPort, "Go.");
disp("Waiting for Arduino to initialize...");
while true
    try
        response = safeWriteRead(serialPort);                   % Wait for initialization
        if strcmp(response, "Ready.")                           % Check for successful initialization
            break;
        end
    catch
        pause(0.5);                                             % Retry after a short delay
    end
end

disp("Requesting measurements for 360 degrees...");

% Initialize the result vector
samplesPerDegree        = 10;
degreeResolution        = 5;                                     % Must be an integer
theta                   = 0:0;                                   % Azimuth
phi                     = 0:degreeResolution:359;                % Elevation
measurementValues       = zeros(length(theta), length(phi));     % Matrix to store measurements for two motors

for motor2Degree = theta
    for motor1Degree = phi
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
                rowIndex = motor1Degree / degreeResolution + 1;
                colIndex = motor2Degree / degreeResolution + 1;
                measurementValues(rowIndex, colIndex) = ...
                    mean(data(1:samplesPerDegree));
                break;

            catch ME
                fprintf("Error at angles %d,%d: %s. Retrying...\n", ...
                        motor1Degree, motor2Degree, ME.message);
                pause(0.1);
            end
        end
    end
end

disp("Collected measurements:");
disp(measurementValues);

clear serialPort;                                               % Close the serial port


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