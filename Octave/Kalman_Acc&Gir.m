clear; clc;

% ====== Parámetros ======
dt = 0.01;          % 100 Hz
t = 0:dt:10;        % 10 segundos

% Señal real (ángulo verdadero)
theta_real = 10 * sin(0.5*t);

% Sensores simulados
gyro = gradient(theta_real, dt) + randn(size(t))*0.5;  % ruido
acc  = theta_real + randn(size(t))*2;                   % ruido

% ====== Kalman ======
x = 0;    % estado (ángulo)
P = 1;

Q = 0.01; % ruido del modelo
R = 4;    % ruido del acelerómetro

theta_kalman = zeros(size(t));

for k = 1:length(t)
    % --- Predicción ---
    x = x + gyro(k)*dt;
    P = P + Q;

    % --- Medición ---
    K = P / (P + R);
    x = x + K * (acc(k) - x);
    P = (1 - K) * P;

    theta_kalman(k) = x;
end

% ====== Gráfica ======
figure;
plot(t, theta_real, 'k', 'LineWidth', 2); hold on;
plot(t, acc, 'r:');
plot(t, theta_kalman, 'b', 'LineWidth', 2);
legend('Ángulo real','Acelerómetro','Kalman');
xlabel('Tiempo (s)');
ylabel('Ángulo (°)');
title('Filtro de Kalman - Fusión Gyro + Acelerómetro');
grid on;

