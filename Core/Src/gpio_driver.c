#include "gpio_driver.h"
#include "stm32f446xx.h"
#include "main.h"
#include <stdint.h>
#include "pwm.h"

volatile uint32_t ms_ticks = 0;         // ms ticks for delay


void led_init(void) {
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

void button_init(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
  (void)RCC->AHB1ENR;
  // set mode input
  GPIOC->MODER &= ~GPIO_MODER_MODER13_Msk;
  // pull-up button
  GPIOC->PUPDR &= ~GPIO_PUPDR_PUPDR13;
  GPIOC->PUPDR |= GPIO_PUPDR_PUPDR13_0;
}

void button_interrupt_init(void) {
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