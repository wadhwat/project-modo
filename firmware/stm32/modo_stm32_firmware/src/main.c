// First test program

#include "stm32f4xx_hal.h"
int main(void)
{
    HAL_Init(); // prepare the STM32 HAL environment
    // Most importantly, configures SysTick
    // Allows for delays such as HAL_Delay(500);



    // Enable the peripheral clock supplying GPIO port C.
    __HAL_RCC_GPIOC_CLK_ENABLE();


    // Creates an instance of GPIO_InitTypeDef and initialized all fields to 0
    /*
        typedef struct
        {
            uint32_t Pin;
            uint32_t Mode;
            uint32_t Pull;
            uint32_t Speed;
            uint32_t Alternate;
        } GPIO_InitTypeDef;
    */
    GPIO_InitTypeDef led_config = {0};

    led_config.Pin = GPIO_PIN_1;
    led_config.Mode = GPIO_MODE_OUTPUT_PP; // configures the pin as an output
                                           // PP means push-pull. Can drive  output in either direction
                                           // HIGH → approx 3.3 V
                                           // LOW  → approx 0 V
    led_config.Pull = GPIO_NOPULL; // Not an input, so disable pull resistors
    led_config.Speed = GPIO_SPEED_FREQ_LOW; // We don't need a fast output, so low speed is fine
                                            // defines slew rate

    HAL_GPIO_Init(GPIOC, &led_config);
    /*
    Configures the GPIO port
    GPIOC is the GPIO port to configure
    We pass the address of our configuration structure so that HAL_GPIO_Init() can read its fields:
        Pin   → GPIO_PIN_1
        Mode  → GPIO_MODE_OUTPUT_PP
        Pull  → GPIO_NOPULL
        Speed → GPIO_SPEED_FREQ_LOW
    HAL then translates those choices into GPIO register settings.
    */

    while (1) // toggle the LED twice a second
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_1);
        HAL_Delay(3000);
    }
}

/*
 * HAL_Delay() depends on the HAL tick being incremented by the
 * SysTick interrupt.
 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}