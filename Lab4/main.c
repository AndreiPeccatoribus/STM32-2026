#include "main.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

/* ====== Настройки ====== */
#define TS_SEC     0.001f      // 1 мс
#define OMEGA_R    1.0f        // рад/с
#define A_AMP      1.0f        // амплитуда синуса, В
#define B_OFF      1.65f       // постоянная составляющая, В

/* ====== Дискретный регулятор из вашего черновика ======
   Если вы оставляете ту же структуру:
   C(z) = (b0 + b1 z^-1) / (1 + z^-1)
   => u[k] = -u[k-1] + b0*e[k] + b1*e[k-1]
*/
#define C_B0  25125.0f
#define C_B1 -24875.0f

/* ====== Дискретная модель объекта из вашего черновика ======
   y[k] = (b0*u[k] + b1*u[k-1] + b2*u[k-2] + b3*u[k-3]
          - a1*y[k-1] - a2*y[k-2]) / a0
*/
#define P_B0  1.1846e-7f
#define P_B1  3.5537e-7f
#define P_B2  3.5537e-7f
#define P_B3  1.1846e-7f
#define P_A0  2.8946f
#define P_A1 -2.7906f
#define P_A2  0.8958f

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

/* ====== Состояние регулятора ====== */
static float e_prev = 0.0f;
static float u_prev = 0.0f;

/* ====== Состояние объекта ====== */
static float u1 = 0.0f, u2 = 0.0f, u3 = 0.0f;
static float y1 = 0.0f, y2 = 0.0f;

static inline float controller_step(float e)
{
    float u = -u_prev + C_B0 * e + C_B1 * e_prev;

    e_prev = e;
    u_prev = u;
    return u;
}

static inline float plant_step(float u)
{
    float y = (P_B0 * u + P_B1 * u1 + P_B2 * u2 + P_B3 * u3
              - P_A1 * y1 - P_A2 * y2) / P_A0;

    u3 = u2; u2 = u1; u1 = u;
    y2 = y1; y1 = y;

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

    /* Эквивалентное входное воздействие */
    float phi = A_AMP * sinf(OMEGA_R * t) + B_OFF;

    /* Замкнутый контур */
    float e = phi - y1;                 // y1 = текущее значение выхода объекта
    float u = controller_step(e);
    float y = plant_step(u);

#if !USE_SWV_ONLY
    float u_clip = u;
    if (u_clip < 0.0f) u_clip = 0.0f;
    if (u_clip > 3.3f) u_clip = 3.3f;
    uint32_t dac = (uint32_t)lrintf((u_clip / 3.3f) * 4095.0f);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac);
#endif
    if ((k % 10u) == 0u)
    {
        printf("%lu,%.6f,%.6f,%.6f,%.6f\r\n",
               (unsigned long)k, phi, y, e, u);
    }
    y1 = y;
    ++k;
}
