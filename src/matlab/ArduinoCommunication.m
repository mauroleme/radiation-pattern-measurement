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
        
        validDataReceived = false;
        while ~validDataReceived
            try
                % Send the angles and read the response
                response = writeread(serialPort, sprintf("%d,%d", ...
                                                         motor1Degree, ...
                                                         motor2Degree));
                
                % Process the received measurements
                currentDegreeSamples = str2double(split(response, ','));
                if any(isnan(currentDegreeSamples))
                    error("Invalid response format.");
                end
                
                % If no error, set validDataReceived to true
                validDataReceived = true;
            catch
                fprintf("Error received for angles %d,%d. Retrying...\n", ...
                        motor1Degree, motor2Degree);
            end
        end
        
        % Store the average value of the received measurements in the matrix
        measurementValues(motor1Degree + 1, motor2Degree + 1) = ...
            mean(currentDegreeSamples(1:samplesPerDegree));
        
    end
end

disp("Collected measurements:");
disp(measurementValues);

clear serialPort;                                               % Close the serial port
