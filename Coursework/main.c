/* Фрагменты main.c: вставляются между метками USER CODE,
   остальное генерирует CubeMX.  */

/* USER CODE BEGIN Includes */
#include "cheby1_filter.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 0 */
volatile float filter_in;    /* вход фильтра, В */
volatile float filter_out;   /* выход фильтра, В */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;   /* колбэк общий для таймеров */

    HAL_ADC_Start(&hadc1);                /* 1. читаем АЦП */
    HAL_ADC_PollForConversion(&hadc1, 1);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);

    filter_in  = raw * (3.3f / 4095.0f);  /* код АЦП в вольты */
    filter_out = cheby1_filter(filter_in); /* 2. шаг фильтра */

    float y = filter_out;                 /* 3. выводим на ЦАП */
    if (y < 0.0f) y = 0.0f;               /* ограничение до приведения */
    if (y > 3.3f) y = 3.3f;
    uint32_t dac = (uint32_t)(y * (4095.0f / 3.3f) + 0.5f);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac);
}
/* USER CODE END 0 */

/* USER CODE BEGIN 2 */
HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);  /* калибровка */
HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);                   /* запуск ЦАП */
HAL_TIM_Base_Start_IT(&htim2);        /* запуск таймера с прерываниями */
/* USER CODE END 2 */