clear;                                          % Limpa variáveis do workspace
clc;                                            % Limpa o Command Window

% Configuração da porta serial
arduinoPort = "COM7";                           % Porta COM usada pelo Arduino
baudRate = 115200;                              % Taxa de comunicação serial (bps)

% Inicializa a porta serial se ainda não estiver configurada
if ~exist('porto', 'var')
    porto = serialport(arduinoPort, baudRate);  % Configura a porta serial
    configureTerminator(porto, "CR/LF");        % Define terminador de linha
    porto.Timeout = 30;                         % Define tempo limite de espera
    pause(2);                                   % Aguarda inicialização do Arduino
end

disp("Solicitando medições para 360°...");
writeline(porto, "0");                          % Envia comando para iniciar medições

% Inicializa a matriz de resultados
matriz = zeros(360, 10);                        % Matriz para armazenar as medições (ângulos x amostras)
linhaAtual = 1;                                 % Índice da linha da matriz

try
    while true
        % Lê a resposta enviada pelo Arduino
        resposta = readline(porto);
        disp("Recebido do Arduino: " + resposta);

        % Verifica se todas as medições foram concluídas
        if contains(resposta, "Envio completo.")
            break;                                                  % Sai do loop quando o envio for concluído
        elseif contains(resposta, "Recebido do Arduino:") || contains(resposta, "Iniciando")
            % Ignora mensagens de status
            continue;
        else
            % Converte a resposta em um vetor de números
            nova_linha = str2double(split(strtrim(resposta), ',')); % Divide a string e converte para números
            if all(~isnan(nova_linha)) && length(nova_linha) == 10
                matriz(linhaAtual, :) = nova_linha;                 % Adiciona à matriz
                linhaAtual = linhaAtual + 1;
            end
        end
    end
catch ME
    disp("Erro na comunicação: " + ME.message);                     % Mostra mensagem de erro, se ocorrer
end

disp("Matriz de medições coletadas (ângulo x amostras):");
disp(matriz);                                                       % Mostra a matriz coletada

clear porto;                                                        % Fecha e limpa a configuração da porta serial

