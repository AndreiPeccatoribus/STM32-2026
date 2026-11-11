T  = 5e-5;                                  % период квантования, с
s  = tf('s');                               % переменная s
W1 = 0.9441 / (1.6134e-7*s^2 + 5.8761e-4*s + 1);
W2 = 1      / (4.2937e-8*s^2 + 1.1447e-4*s + 1);
W3 = 1      / (2.4759e-8*s^2 + 2.4159e-5*s + 1);
W  = W1 * W2 * W3;                          % фильтр целиком
Wz1 = c2d(W1, T, 'tustin');
Wz2 = c2d(W2, T, 'tustin');
Wz3 = c2d(W3, T, 'tustin');
Wz  = Wz1 * Wz2 * Wz3;               % цифровой фильтр целиком

[num1, den1] = tfdata(Wz1, 'v');     % коэффициенты первого звена
[num2, den2] = tfdata(Wz2, 'v');
[num3, den3] = tfdata(Wz3, 'v');
w = logspace(2, log10(0.99*pi/T), 1000);   % рад/с, до Найквиста
figure;
bode(W, 'b-', Wz, 'r--', w);   % непрерывный и цифровой
grid on;
legend('Непрерывный', 'Цифровой');
lam = (2/T) * tan(w*T/2);   % псевдочастота
[mag, ph] = bode(W, lam);   % непрерывный фильтр на lam
ph = squeeze(ph);
semilogx(w, ph); grid on;   % по оси x – реальная частота w
xlabel('\omega, рад/с'); ylabel('Фаза, град');
[num, den] = tfdata(Wz, 'v');   % коэффициенты при z^6 ... z^0
