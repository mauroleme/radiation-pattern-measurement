% Script MATLAB completo para plotar padrão de radiação polar
% com 0 graus no topo, incremento anti-horário, lobo principal em 90 graus
% e marcação do Beamwidth (-3 dB).
% O cálculo do Beamwidth é baseado no pico dos dados originais (0-360).
% Assume ângulos de 0 a 360 graus com passo de 5 graus e 73 pontos.

clear; % Limpa todas as variáveis do workspace
clc;   % Limpa a janela de comando
close all; % Fecha todas as figuras abertas

% --- DEFINIÇÃO DOS DADOS ---

% Os ângulos originais em graus (0, 5, 10, ..., 360). 73 pontos.
theta_original = (0:5:360)'; % Cria um vetor coluna de 0 a 360 com passo de 5 graus

% *** SEUS VALORES DE MEDIÇÃO REAIS (em dB) ***
% SUBSTITUA o conteúdo abaixo pelos SEUS 73 valores reais, na ordem de 0 a 360.
% CONFIRME o uso de PONTO decimal (.), não vírgula (,).
measurementValues_original = [
-35.0281;
-35.4444;
-36.0463;
-36.6871;
-37.2872;
-38.2123;
-39.1716;
-40.3643;
-41.5084;
-42.3451;
-42.6768;
-42.5517;
-42.0981;
-41.8455;
-42.1109;
-42.8694;
-44.4508;
-46.8686;
-49.7222;
-51.0080;
-51.9728;
-51.9958;
-50.6162;
-48.0123;
-43.9967;
-39.3215;
-35.2067;
-30.8599;
-27.0407;
-24.5529;
-22.5109;
-20.9124;
-19.7534;
-18.9694;
-18.3847;
-18.1188;
-18.0208; % <--- Este é o valor em 0 graus (-18.0208 dB)
-18.1220;
-18.5596;
-19.3390;
-20.3038;
-21.6179; % <--- Valor próximo a -21.0208 dB (-21.6179 em 135 graus original)
-23.3981;
-25.7716;
-28.6692;
-32.5666;
-38.5790;
-46.8362;
-49.8863;
-48.8828;
-49.0785;
-49.0639;
-47.2675;
-45.3248;
-44.6525;
-45.1931;
-46.8536;
-48.3271;
-46.6904;
-43.7300;
-41.2250;
-40.4130;
-40.2198;
-40.6988;
-40.7112;
-40.0031;
-38.5593;
-37.0596;
-35.8618;
-35.0761;
-34.6875;
-34.6933;
-35.0281 % <--- Este é o valor em 360 graus (-35.0281 dB)
]; % <-- Certifique-se de que há 73 valores aqui

% --- Verificação ---
if length(measurementValues_original) ~= length(theta_original)
    error('Erro: O número de valores de medição (%d) não corresponde ao número de ângulos (%d). Verifique seus dados.', ...
           length(measurementValues_original), length(theta_original));
end
if length(theta_original) ~= 73
     warning('Esperado 73 pontos para o range 0 a 360 com passo de 5 graus. Verifique seus dados.');
end


% --- Processamento dos dados para Plotagem ---
% Aplicar a transformação de ângulo para o plot final (leva o lobo de 180 original para 90 plot).
% Se com '270 - theta_original' e 0 no topo o lobo foi para 90, mantenha assim.
theta_plot = 270 - theta_original;

% Os valores de medição originais serão plotados nos ângulos transformados.
measurement_to_plot = measurementValues_original;


% --- Criação da figura e dos eixos polares ---
figure;
hax = polaraxes; % Cria os eixos polares e armazena o handle (hax)

% --- Plotagem do Padrão ---
% polarplot espera ângulos em RADIANOS. Converter os ângulos transformados.
polarplot(hax, deg2rad(theta_plot), measurement_to_plot, 'LineWidth', 1.5, 'DisplayName', 'Padrão de Radiação');

% --- Configurar o eixo polar (hax) ---
hax.ThetaZeroLocation = 'top';      % 0 graus no topo
hax.ThetaDir = 'counterclockwise';  % Incremento anti-horário
rlim(hax, [-65 0]);                 % Limites do raio em dB
hax.RTick = [-60 -40 -20 0];        % Ticks do raio

% --- Adicionar título ao gráfico ---
title('Padrão de Radiação com Beamwidth (-3 dB)');

% --- Melhorias visuais (Opcional) ---
hax.RGrid = 'on';
hax.ThetaGrid = 'on';
hax.FontSize = 10;


% --- Cálculo e Plotagem do Beamwidth (-3 dB) ---

% 1. Encontrar o valor máximo (pico) nos DADOS ORIGINAIS e o nível de -3 dB
[peak_val_original, peak_idx_original] = max(measurementValues_original);
threshold_val_original = peak_val_original - 3; % Calcula o nível de -3 dB abaixo do pico original

% O ângulo original onde o pico ocorre
peak_angle_original = theta_original(peak_idx_original);

num_points = length(measurementValues_original); % 73 pontos

% 2. Encontrar os índices dos bordos do lobo principal nos DADOS ORIGINAIS
% Caminha para a "esquerda" (ângulos decrescentes) a partir do pico original
idx_left_original = peak_idx_original;
while measurementValues_original(idx_left_original) > threshold_val_original
    % Tratar o wrap-around (do 0 para o 360)
    prev_idx = mod(idx_left_original - 2 + num_points, num_points) + 1;
    if prev_idx == peak_idx_original % Chegou de volta ao pico antes de cair 3dB
         warning('O lobo principal não cai 3dB abaixo do pico em 360 graus.');
         left_angle_3db_original = NaN;
         break;
    end
    idx_left_original = prev_idx;
end
% idx_left_original é o primeiro índice à esquerda do pico onde valor <= threshold.
% Pontos para interpolação: idx_left_original e o ponto anterior a ele.
left_edge_idx2_original = idx_left_original; % Ponto <= threshold
left_edge_idx1_original = mod(idx_left_original - 2 + num_points, num_points) + 1; % Ponto > threshold


% Caminha para a "direita" (ângulos crescentes) a partir do pico original
idx_right_original = peak_idx_original;
while measurementValues_original(idx_right_original) > threshold_val_original
     % Tratar o wrap-around (do 360 para o 0)
     next_idx = mod(idx_right_original, num_points) + 1;
     if next_idx == peak_idx_original % Chegou de volta ao pico
         right_angle_3db_original = NaN;
         break;
     end
     idx_right_original = next_idx;
end
% idx_right_original é o primeiro índice à direita do pico onde valor <= threshold.
% Pontos para interpolação: o ponto anterior a idx_right_original e idx_right_original.
right_edge_idx1_original = mod(idx_right_original - 2 + num_points, num_points) + 1; % Ponto > threshold
right_edge_idx2_original = idx_right_original; % Ponto <= threshold


% 3. Interpolar para encontrar os ângulos exatos de -3 dB nos DADOS ORIGINAIS
left_angle_3db_original = NaN;
right_angle_3db_original = NaN;

% Interpolação para o bordo esquerdo (nos dados originais)
if ~isnan(left_edge_idx1_original) && ~isnan(left_edge_idx2_original) && left_edge_idx1_original ~= left_edge_idx2_original
     angles_segment_left = [theta_original(left_edge_idx1_original), theta_original(left_edge_idx2_original)];
     values_segment_left = [measurementValues_original(left_edge_idx1_original), measurementValues_original(left_edge_idx2_original)];

     [values_sorted_left, sort_order_left] = sort(values_segment_left);
     angles_sorted_left = angles_segment_left(sort_order_left);

     left_angle_3db_original = interp1(values_sorted_left, angles_sorted_left, threshold_val_original, 'linear', 'extrap');
end

% Interpolação para o bordo direito (nos dados originais)
if ~isnan(right_edge_idx1_original) && ~isnan(right_edge_idx2_original) && right_edge_idx1_original ~= right_edge_idx2_original
    angles_segment_right = [theta_original(right_edge_idx1_original), theta_original(right_edge_idx2_original)];
    values_segment_right = [measurementValues_original(right_edge_idx1_original), measurementValues_original(right_edge_idx2_original)];

    [values_sorted_right, sort_order_right] = sort(values_segment_right);
    angles_sorted_right = angles_segment_right(sort_order_right);

    right_angle_3db_original = interp1(values_sorted_right, angles_sorted_right, threshold_val_original, 'linear', 'extrap');
end


% 4. Calcular o Beamwidth e exibir resultados (usando ângulos originais)
beamwidth_deg = NaN; % Inicializa

fprintf('Pico do Padrão Original: %.2f dB em %.2f graus\n', peak_val_original, peak_angle_original);
fprintf('Nível de -3 dB (original): %.2f dB\n', threshold_val_original);

if ~isnan(left_angle_3db_original) && ~isnan(right_angle_3db_original)
    % Calcular a diferença angular circular entre os dois pontos de -3 dB originais
    angle_diff_rad = angdiff(deg2rad(left_angle_3db_original), deg2rad(right_angle_3db_original));
    beamwidth_deg = abs(rad2deg(angle_diff_rad));

    fprintf('Pontos de -3 dB (original): %.2f graus e %.2f graus\n', left_angle_3db_original, right_angle_3db_original);
    fprintf('Largura de Feixe (-3 dB): %.2f graus\n', beamwidth_deg);

    % --- Transformar os ângulos de -3 dB para a coordenada do plot final ---
    % Aplique a mesma transformação que usou para plotar os dados
    angles_3db_plot = 270 - [left_angle_3db_original, right_angle_3db_original];

else
    fprintf('Não foi possível encontrar os dois pontos de -3 dB no padrão original.\n');
    angles_3db_plot = []; % Não plotar nada se não encontrou os dois pontos
end


% 5. Plotar as linhas e marcadores de -3 dB no gráfico existente (usando ângulos TRANSFORMADOS)
hold(hax, 'on');

% O centro radial é o limite inferior do eixo radial
center_radial_value = rlim(hax);
center_radial_value = center_radial_value(1); % Pega o limite inferior (-65 dB)

if ~isempty(angles_3db_plot) && all(~isnan(angles_3db_plot))
     for k = 1:length(angles_3db_plot)
         % Desenha uma linha radial do centro até o nível do threshold original (-21.0208 dB)
         polarplot(hax, deg2rad([angles_3db_plot(k), angles_3db_plot(k)]), ...
                         [center_radial_value, threshold_val_original], 'r--', 'LineWidth', 1.5, 'DisplayName', '-3 dB Linha'); % Linha vermelha tracejada

     end
     % Adicionar marcadores nos pontos exatos de -3 dB na curva plotada.
     % Note que usamos os ângulos TRANSFORMADOS (angles_3db_plot) e o valor do THRESHOLD ORIGINAL.
      polarplot(hax, deg2rad(angles_3db_plot), repmat(threshold_val_original, size(angles_3db_plot)), 'ro', 'MarkerSize', 8, 'DisplayName', '-3 dB Ponto'); % Marcadores circulares vermelhos
end

% Adicionar Legenda para identificar os elementos no gráfico
legend(hax, 'Location', 'southoutside');

hold(hax, 'off');

% Fim do Script