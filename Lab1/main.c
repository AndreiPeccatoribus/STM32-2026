/* Фрагменты main.c: вставляются между метками USER CODE,
   остальное генерирует CubeMX. */

/* USER CODE BEGIN Includes */
#include <math.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
#define LOOP_MS 2                 // HAL_Delay(1) на самом деле ждёт 2 мс
volatile uint32_t count_main = 0;
volatile float x = 0.0f;
/* USER CODE END PV */

/* Бесконечный цикл в main: внутри него свой только блок USER CODE 3 */
while (1)
{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    count_main++;
    x = sinf(0.001f * 3.14159265f * count_main);

    // половина периода мигания в мс: 250 даёт 2 Гц, 50 даёт 10 Гц
    uint32_t half_period_ms = 250;
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) half_period_ms = 50;

    if (count_main % (half_period_ms / LOOP_MS) == 0)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
    }
    HAL_Delay(1);
}
/* USER CODE END 3 */
