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
measurementValues       = zeros(360, 360);                       % Matrix to store measurements for two motors

for motor1Degree = 0:359
    for motor2Degree = 0:359
        
        while true
            try
                % Send the angles and read the response
                response = writeread(serialPort, sprintf("%d,%d", ...
                                                         motor1Degree, ...
                                                         motor2Degree));
                
                % Check for Arduino error message
                if startsWith(response, "Error:")
                    error("Arduino error: %s", extractAfter(response, ...
                                                            "Error:"));
                end
                
                % Convert and validate numeric data
                data = str2double(split(response, ','));
                if any(isnan(data))
                    error("Invalid numeric format.");
                end
                
                % Store the mean of the first samples
                measurementValues(motor1Degree + 1, motor2Degree + 1) = ...
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
