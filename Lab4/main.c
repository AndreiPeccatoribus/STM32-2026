#include "main.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

/* ====== Настройки ====== */
#define TS_SEC     0.001f      // период дискретизации T_s = 1 мс
#define OMEGA_R    1.0f        // рабочая частота, рад/с
#define A_AMP      1.0f        // амплитуда задающего сигнала, В
#define B_OFF      1.65f       // постоянная составляющая, В
#define INPUT_STEP 1           // 1 - ступенька 1 В,
                               // 0 - A_AMP*sin(OMEGA_R*t)+B_OFF

/* ====== Корректирующее устройство (ПКУ) ======
   ПКУ(z) = C_B0 + C_B1*z^-1  =>  u[n] = C_B0*e[n] + C_B1*e[n-1] */
#define C_B0  13065.0
#define C_B1 -12935.0

/* ====== Дискретная модель неизменяемой части ======
   y[n] = P_B0*u[n] + P_B1*u[n-1] + P_B2*u[n-2] + P_B3*u[n-3]
          - P_A1*y[n-1] - P_A2*y[n-2] - P_A3*y[n-3] */
#define P_B0  1.18455342e-7
#define P_B1  3.55366027e-7
#define P_B2  3.55366027e-7
#define P_B3  1.18455342e-7
#define P_A1 -2.89481166
#define P_A2  2.79057095
#define P_A3 -0.89575930

extern TIM_HandleTypeDef htim6;
extern DAC_HandleTypeDef hdac1;

/* ====== SWV printf ====== */
int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; ++i)
    {
        ITM_SendChar(*ptr++);
    }
    return len;
}

/* ====== Состояние регулятора и объекта (double) ====== */
static double e_prev = 0.0;
static double u_1 = 0.0, u_2 = 0.0, u_3 = 0.0;
static double y_1 = 0.0, y_2 = 0.0, y_3 = 0.0;

static inline double controller_step(double e)
{
    double u = C_B0 * e + C_B1 * e_prev;
    e_prev = e;
    return u;
}

static inline double plant_step(double u)
{
    double y = P_B0 * u + P_B1 * u_1 + P_B2 * u_2 + P_B3 * u_3
             - P_A1 * y_1 - P_A2 * y_2 - P_A3 * y_3;
    u_3 = u_2; u_2 = u_1; u_1 = u;
    y_3 = y_2; y_2 = y_1; y_1 = y;
    return y;
}

#define USE_SWV_ONLY 1

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM6_Init();
#if !USE_SWV_ONLY
    MX_DAC1_Init();
    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
#endif
    HAL_TIM_Base_Start_IT(&htim6);

    /* Чтобы printf с %f работал:
       в настройках линковщика добавьте -u _printf_float
       или печатайте scaled integers. */
    while (1)
    {
        __WFI();
    }
}

/* ====== 1 кГц цикл ====== */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM6) return;

    static uint32_t k = 0;
    float t = (float)k * TS_SEC;

    /* Задающее воздействие */
#if INPUT_STEP
    float phi = 1.0f;
#else
    float phi = A_AMP * sinf(OMEGA_R * t) + B_OFF;
#endif

    /* Контур: y_1 - выход объекта на прошлом такте */
    double e = phi - y_1;
    double u = controller_step(e);
    double y = plant_step(u);

#if !USE_SWV_ONLY
    double u_clip = u;
    if (u_clip < 0.0) u_clip = 0.0;
    if (u_clip > 3.3) u_clip = 3.3;
    uint32_t dac = (uint32_t)lrint((u_clip / 3.3) * 4095.0);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac);
#endif

    if ((k % 10u) == 0u)
    {
        printf("%lu,%.6f,%.6f,%.6f,%.6f\r\n",
               (unsigned long)k, phi, y, e, u);
    }
    ++k;
}
