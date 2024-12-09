clear;                                                      % Limpa variáveis do workspace
clc;                                                        % Limpa o Command Window

% Configuração da porta serial
arduinoPort = "COM7";                                       % Porta COM usada pelo Arduino
baudRate    = 115200;                                       % Taxa de comunicação serial (bps)

% Inicializa a porta serial se ainda não estiver configurada
if ~exist('porto', 'var')
    porto           = serialport(arduinoPort, baudRate);    % Configura a porta serial
    porto.Timeout   = 30;                                   % Define tempo limite de espera
    configureTerminator(porto, "CR/LF");                    % Define terminador de linha
end
pause(10);                                                  % Aguarda inicialização do Arduino
disp("Solicitando medições para 360°...");

% Inicializa a matriz de resultados
numAmostrasPorGrau  = 10;
valores             = zeros(360, 1);                        % Vetor para armazenar as medições totais
valoresDoGrau       = zeros(numAmostrasPorGrau, 1);         % Vetor para armazenar as medições de um grau

for i=1:360
    try
        valoresDoGrau   = str2double(split(writeread(porto,"1"), ','));
    catch
        valoresDoGrau   = -1;
    end
    valores(i) = mean(valoresDoGrau);
    disp("Valor:");
    disp(valores(i));
    pause(0.1);
end

disp("Vetor de medições coletadas:");
disp(valores);                                              % Mostra a o vetor coletado

clear porto;                                                % Fecha e limpa a configuração da porta serial
