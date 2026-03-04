clear; clc;

dt = 0.01;
t = 0:dt:20;

% =========================
% 1) Movimiento real
% =========================
roll_real  = 10 * sin(0.4*t);
pitch_real = 8  * sin(0.3*t);
yaw_real   = 30 * sin(0.2*t);

% =========================
% 2) Sensores simulados
% =========================
gyro_roll  = gradient(roll_real, dt)  + randn(size(t))*0.4;
gyro_pitch = gradient(pitch_real, dt) + randn(size(t))*0.4;
gyro_yaw   = gradient(yaw_real, dt)   + randn(size(t))*0.4;

acc_roll  = roll_real  + randn(size(t))*2;
acc_pitch = pitch_real + randn(size(t))*2;

mag_yaw = yaw_real + randn(size(t))*3;

% =========================
% 3) Kalman simple por eje
% =========================
Q = 0.01;
R_acc = 4;
R_mag = 9;

roll = 0;  pitch = 0;  yaw = 0;
P_r = 1;   P_p = 1;    P_y = 1;

roll_k = zeros(size(t));
pitch_k = zeros(size(t));
yaw_k = zeros(size(t));

for k = 1:length(t)

    % ---- ROLL ----
    roll = roll + gyro_roll(k)*dt;
    P_r = P_r + Q;
    K = P_r / (P_r + R_acc);
    roll = roll + K*(acc_roll(k) - roll);
    P_r = (1 - K)*P_r;

    % ---- PITCH ----
    pitch = pitch + gyro_pitch(k)*dt;
    P_p = P_p + Q;
    K = P_p / (P_p + R_acc);
    pitch = pitch + K*(acc_pitch(k) - pitch);
    P_p = (1 - K)*P_p;

    % ---- YAW ----
    yaw = yaw + gyro_yaw(k)*dt;
    P_y = P_y + Q;
    K = P_y / (P_y + R_mag);
    yaw = yaw + K*(mag_yaw(k) - yaw);
    P_y = (1 - K)*P_y;

    roll_k(k)  = roll;
    pitch_k(k) = pitch;
    yaw_k(k)   = yaw;
end

% =========================
% 4) Gráficas
% =========================
figure;

subplot(3,1,1)
plot(t, roll_real,'k', t, roll_k,'b');
title('ROLL'); grid on; legend('Real','Kalman');

subplot(3,1,2)
plot(t, pitch_real,'k', t, pitch_k,'b');
title('PITCH'); grid on; legend('Real','Kalman');

subplot(3,1,3)
plot(t, yaw_real,'k', t, yaw_k,'b');
title('YAW'); grid on; legend('Real','Kalman');

