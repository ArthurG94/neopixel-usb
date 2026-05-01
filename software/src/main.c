#include "stm32f0xx_hal.h"

#define LED_NUMBER 1U
#define WS2812_BITS_PER_LED 24U
#define WS2812_RESET_SLOTS 100U
#define WS2812_TIMER_PERIOD 60U
#define WS2812_T0H_TICKS 20
#define WS2812_T1H_TICKS 40
#define WS2812_FRAME_SLOTS (LED_NUMBER * WS2812_BITS_PER_LED + WS2812_RESET_SLOTS)

static uint16_t ws2812Frame[WS2812_FRAME_SLOTS];

static void Error_Handler(void)
{
    while (1)
    {
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }

    SystemCoreClockUpdate();

    if (HAL_RCC_GetSysClockFreq() != 48000000U)
    {
        Error_Handler();
    }
}

static void WS2812_GPIO_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF2_TIM17;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void WS2812_TIM_DMA_Init(void)
{
    __HAL_RCC_TIM17_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    TIM17->CR1 = TIM_CR1_ARPE;
    TIM17->PSC = 0U;
    TIM17->ARR = WS2812_TIMER_PERIOD - 1U;
    TIM17->CCR1 = 0U;
    TIM17->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
    TIM17->CCER = TIM_CCER_CC1E;
    TIM17->BDTR = TIM_BDTR_MOE;
    TIM17->DIER = TIM_DIER_UDE;
    TIM17->EGR = TIM_EGR_UG;

    DMA1_Channel1->CCR = 0U;
    DMA1_Channel1->CPAR = (uint32_t)&TIM17->CCR1;
}

static void WS2812_SetLedColor(uint8_t red, uint8_t green, uint8_t blue)
{
    uint32_t color;

    uint32_t grb = ((uint32_t)green << 16) | ((uint32_t)red << 8) | (uint32_t)blue;

    for (uint32_t bit = 0U; bit < WS2812_BITS_PER_LED; ++bit)
    {
        uint32_t mask = 1UL << (23U - bit);
        ws2812Frame[bit] = (grb & mask) ? WS2812_T1H_TICKS : WS2812_T0H_TICKS;
    }

    for (uint32_t slot = WS2812_BITS_PER_LED; slot < WS2812_FRAME_SLOTS; ++slot)
    {
        ws2812Frame[slot] = 0U;
    }
}

static HAL_StatusTypeDef WS2812_Send(void)
{
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = DMA_IFCR_CGIF1 | DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;

    TIM17->CR1 &= ~TIM_CR1_CEN;
    TIM17->CNT = 0U;
    TIM17->CCR1 = ws2812Frame[0];

    DMA1_Channel1->CMAR = (uint32_t)&ws2812Frame[1];
    DMA1_Channel1->CNDTR = WS2812_FRAME_SLOTS - 1U;
    DMA1_Channel1->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_PSIZE_0 | DMA_CCR_MSIZE_0 | DMA_CCR_PL_1;

    DMA1_Channel1->CCR |= DMA_CCR_EN;
    TIM17->CR1 |= TIM_CR1_CEN;

    uint32_t deadline = HAL_GetTick() + 10U;
    while ((DMA1->ISR & (DMA_ISR_TCIF1 | DMA_ISR_TEIF1)) == 0U)
    {
        if (HAL_GetTick() > deadline)
        {
            DMA1_Channel1->CCR &= ~DMA_CCR_EN;
            TIM17->CR1 &= ~TIM_CR1_CEN;
            TIM17->CCR1 = 0U;
            DMA1->IFCR = DMA_IFCR_CGIF1 | DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;
            return HAL_TIMEOUT;
        }
    }

    uint32_t isr = DMA1->ISR;

    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    TIM17->CR1 &= ~TIM_CR1_CEN;
    TIM17->CCR1 = 0U;
    DMA1->IFCR = DMA_IFCR_CGIF1 | DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;

    if ((isr & DMA_ISR_TEIF1) != 0U)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

int main(void)
{
    volatile HAL_StatusTypeDef wsStatus = HAL_OK;

    HAL_Init();
    SystemClock_Config();

    WS2812_GPIO_Init();
    WS2812_TIM_DMA_Init();

    while (1)
    {
        WS2812_SetLedColor(255, 0U, 0U);
        wsStatus = WS2812_Send();

        if (wsStatus != HAL_OK)
        {
            Error_Handler();
        }

        HAL_Delay(300U);

        WS2812_SetLedColor(0U, 24U, 0U);
        wsStatus = WS2812_Send();

        if (wsStatus != HAL_OK)
        {
            Error_Handler();
        }

        HAL_Delay(300U);

        WS2812_SetLedColor(0U, 0U, 24U);
        wsStatus = WS2812_Send();

        if (wsStatus != HAL_OK)
        {
            Error_Handler();
        }

        HAL_Delay(300U);
    }
}