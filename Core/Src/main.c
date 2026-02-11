
#include "main.h"
#include "stm32f446xx.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_it.h"
#include <stdint.h>
#include <stdbool.h>
// define PWM params
#define PSC_VAL 84UL
#define ARR_VAL 1000UL
#define CCR_VAL
#define PWM_MODE_1 6UL

typedef enum {INHALING, PAUSE_UP, EXHALING, PAUSE_DOWN} BreathState;

volatile uint32_t ms_ticks = 0;         // ms ticks for delay
volatile int32_t brightness_step = 1;   // brightness step for LED
const int32_t PAUSE_DURATION = 500;     // pause duration of breath


static void led_init(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  (void)RCC->AHB1ENR;
  // set pin mode AF
  GPIOA->MODER &= ~GPIO_MODER_MODER5_Msk;
  GPIOA->MODER |= GPIO_MODER_MODER5_1;
  // select AF1 TIM2_CH1 
  GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5_Msk;
  GPIOA->AFR[0] |= GPIO_AFRL_AFSEL5_0;
  // set medium speed
  GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED5_Msk;
  GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED5_0;
}

static void button_init(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
  (void)RCC->AHB1ENR;
  // set mode input
  GPIOC->MODER &= ~GPIO_MODER_MODER13_Msk;
  // pull-up button
  GPIOC->PUPDR &= ~GPIO_PUPDR_PUPDR13;
  GPIOC->PUPDR |= GPIO_PUPDR_PUPDR13_0;
}

static void button_interrupt_init(void) {
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  (void)RCC->APB2ENR;
  // Set 13 line to C port
  SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13_Msk;
  SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC;
  // Enable interrupt for 13 line
  EXTI->IMR &= ~EXTI_IMR_MR13_Msk;
  EXTI->IMR |= EXTI_IMR_IM13;
  // Set type of the event as "falling trigger"
  EXTI->FTSR &= ~EXTI_FTSR_TR13_Msk;
  EXTI->FTSR |= EXTI_FTSR_TR13;
  // Enable 40id in NVIC
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void sysTick_init(void) {
  // interrupt each 1ms. therefore LOAD = 84Mhz / 1000ms. 84000 ticks for one ms.
  uint32_t N = 84000UL;
  SysTick->LOAD = N - 1UL;
  SysTick->VAL = 0UL;
  // Get MHz from CPU, enable interrupt, start countdown
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_CLKSOURCE_Msk;
}

static inline void delay_ms(uint32_t ms) { 
  uint32_t start_ms = ms_ticks;
  while((ms_ticks - start_ms) < ms);
}

static void tim2_init(void) {
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  (void)RCC->APB1ENR;
  TIM2->PSC = PSC_VAL - 1UL;
  // PWM 1Khz: 1000 PWM full cycles in sec (each cycle has 1000 tick)
  // Summary: 1000 cycles * 1000 ticks = 1 million ticks in sec
  TIM2->ARR = ARR_VAL - 1UL;
  // Duty cycle. ARR = 1000, therefore 10% of 1000 = 100; 20% of 1000 = 200, etc. 
  TIM2->CCR1 = 0UL; // Start from 0%
  TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // CC1S output (0x00 by default)
  // OC1M PWM Mode 1 channel 1 (while CNT < CCR pin is 1, else 0)
  TIM2->CCMR1 |= (PWM_MODE_1 << TIM_CCMR1_OC1M_Pos);
  TIM2->CCMR1 |= TIM_CCMR1_OC1PE; // Enable preload
  // Enable CCER
  TIM2->CCER &= ~TIM_CCER_CC1E_Msk;
  TIM2->CCER |= TIM_CCER_CC1E; 
  // Enable generate interrupts (when CNT == ARR)
  TIM2->DIER &= ~TIM_DIER_UDE_Msk;
  TIM2->DIER |= TIM_DIER_UIE;
  HAL_NVIC_EnableIRQ(TIM2_IRQn); // enable interrupt in NVIC
  // Start timer
  TIM2->CR1 &= ~TIM_CR1_CEN_Msk;
  TIM2->CR1 |= TIM_CR1_CEN;
}

void SysTick_Handler(void) {
  ms_ticks++;
}

// increases speed of breathing
void EXTI15_10_IRQHandler(void) {
  if(EXTI->PR & EXTI_PR_PR13) {
    EXTI->PR = EXTI_PR_PR13;

    static uint32_t last_time = 0;
    if ((ms_ticks - last_time) < 200) return; // ignore debounce
    last_time = ms_ticks;

    brightness_step += 2; 
    if(brightness_step > 20) brightness_step = 1;
  }
}


void TIM2_IRQHandler(void) {
  // if PWM pass 1 full cycle (CNT == ARR)
  static int32_t brightness = 0;
  static BreathState state = INHALING;
  static int32_t pause_timer = 0;

  if(TIM2->SR & TIM_SR_UIF) {
    TIM2->SR &= ~TIM_SR_UIF_Msk; // reset flag
    (void)TIM2->SR; // save delay
    // Breath states
    switch (state) {
      case INHALING:
        brightness += brightness_step;
        if(brightness >= 1000) {
          brightness = 1000;
          state = PAUSE_UP;
          pause_timer = 0;
        }
        break;
      case PAUSE_UP:
        pause_timer++;
        if(pause_timer >= PAUSE_DURATION) {
          state = EXHALING;
        }
        break;
      case EXHALING:
        brightness -= brightness_step;
        if(brightness <= 0) {
          brightness = 0;
          state = PAUSE_DOWN;
          pause_timer = 0;
        }
        break;
      case PAUSE_DOWN:
        pause_timer++;
        if(pause_timer >= PAUSE_DURATION) {
          state = INHALING;
        }
        break;
    }
    TIM2->CCR1 = (uint32_t)brightness;
  }
}


void SystemClock_Config(void);

int main(void) {
  SystemClock_Config();
  led_init();
  button_init();
  button_interrupt_init();
  sysTick_init();
  tim2_init();

  while (1) {
   __WFI();
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
  }

}
