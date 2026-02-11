# STM32 Breathing LED with FSM and Interrupts
A Bare-Metal firmware for STM32F446RE (Nucleo-64) that implements a "breathing" LED effect using hardware timers, interrupts, and a Finite State Machine (FSM).

## Features
Hardware PWM Generation: Utilizes TIM2 Channel 1 for smooth LED brightness control.
Event-Driven Architecture: Entirely interrupt-based logic; the CPU remains in low-power mode (WFI) most of the time.
Finite State Machine (FSM): Managed breathing cycles: INHALING, PAUSE_UP, EXHALING, and PAUSE_DOWN.
Interactive Control: User Button (B1) on PC13 changes the breathing speed.
Software Debouncing: Integrated timing logic to handle mechanical button bounce using SysTick.

---

## Parameter,Value

Microcontroller,STM32F446RET6 (ARM Cortex-M4)
Clock Speed,84 MHz
PWM Frequency,1 kHz
PWM Resolution,1000 steps (ARR = 999)
Interaction,EXTI13 (External Interrupt)

---

## How it Works

1. PWM & Timer Logic
The LED is connected to PA5 (LD2 on Nucleo). TIM2 is configured in PWM Mode 1. The brightness is controlled by updating the CCR1 register within the TIM2_IRQHandler every 1ms.
2. State Machine
The breathing pattern follows a structured sequence to mimic natural breathing:
Inhaling: Linear increase of PWM duty cycle.
Pause Up: 500ms delay at maximum brightness.
Exhaling: Linear decrease of PWM duty cycle.
Pause Down: 500ms delay at zero brightness.
3. Button Debouncing
To prevent erratic behavior from mechanical noise, the EXTI15_10_IRQHandler uses a timestamp-based comparison:
```C
if ((ms_ticks - last_press_time) > 200) {
    // Update breathing step...
}
```

## Project Structure

main.c: Hardware initialization and main low-power loop.
stm32f4xx_it.c: Interrupt Service Routines (ISRs) for SysTick, TIM2, and EXTI.
system_clock.c: PLL configuration for 84MHz operation.

---

## Requirements
### Hardware Connection
| Peripheral | MCU Pin | Function | Description |
| :--- | :--- | :--- | :--- |
| **TIM2_CH1** | PA5 | PWM Output | Internal Green LED (LD2) |
| **EXTI13** | PC13 | Input | User Blue Button (B1) |
| **SYS_SWD** | PA13/PA14 | Debug | ST-LINK Interface |


Toolchain: VScode / GNU Arm Embedded Toolchain.
Hardware: Nucleo-F446RE (or any STM32F4 with minor pin remapping).

---

## Future Improvements

Gamma Correction: Implementing a Look-Up Table (LUT) for exponential/sinusoidal brightness to match human eye perception.
DMA Integration: Moving PWM updates to DMA to further reduce CPU load.
UART Telemetry: Reporting state changes and brightness levels via Serial.
