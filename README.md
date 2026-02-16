# STM32 Breathing LED with FSM, Interrupts and UART CLI

Bare-Metal firmware for STM32F446RE (Nucleo-64). Implements a "breathing" LED effect using a non-blocking architecture, circular buffers, and an interrupt-driven Command Line Interface (CLI).


## New Features

* Interrupt-Driven UART: Fully asynchronous serial communication (115200 baud).
* Custom Ring Buffer: A thread-safe, generic circular buffer implementation using power-of-two masking for high performance.
* Async Logging: Non-blocking UART_Printf allows the system to log state transitions without stalling the CPU.
* Two-Way Interaction: Real-time control of breathing parameters (e.g., speed) via serial commands (+ / -).
* Atomic Operations: Critical sections protected via interrupt disabling to prevent race conditions between main and ISRs.

## Technical Specifications

| Parameter | Value |
| :--- | :--- |
|Microcontroller | STM32F446RET6 (ARM Cortex-M4) |
|Clock| Speed,84 MHz |
|PWM |Frequency,1 kHz (Resolution: 1000 steps) |
|UART |Config,115200 8N1 |
|Ring |Buffer,128 bytes (Power-of-two optimized) |
|Archi|tecture,Event-Driven / Interrupt-Driven |


---

## Project Architecture

1. Circular Buffer (circular_buffer.h/c)
   * A generic implementation for embedded systems:
   * Performance: Uses (i + 1) & mask instead of modulo.
   * Safety: Provides cb_push_safe for atomic access from the main loop.
   * Documentation: Fully documented in Doxygen format (English).

2. UART Driver with CLI
    * The USART2_IRQHandler manages both transmission and reception:
    * TX: Automatically empties the ring buffer into the DR register.
    * RX: Listens for user commands (+ to speed up, - to slow down) and updates the FSM parameters on the fly.

3. Finite State Machine (FSM)
    * Managed within TIM2_IRQHandler:
    * INHALING: Incremental brightness.
    * PAUSE_UP: Peak hold.
    * EXHALING: Decremental brightness.
    * PAUSE_DOWN: Minimum hold.
    * Event Logging: Reports state changes to UART only during transitions to save bandwidth.

---

## Project Structure

circular_buffer.h/c: Generic ring buffer library.
uart.h/c: UART initialization, UART_Printf, and ISR logic.
main.c: System setup and low-power background loop.
stm32f4xx_it.c: Unified interrupt service routines.

---

## Hardware Connection

| Peripheral | MCU Pin | Function | Description |
| :--- | :--- | :--- | :--- |
| **TIM2_CH1** | **PA5** | PWM Output | Internal Green LED (LD2) |
| **USART2_TX** | **PA2** | Telemetry | Serial Output (ST-Link VCP) |
| **USART2_RX** | **PA3** | Control | Serial Input (ST-Link VCP) |
| **EXTI13** | **PC13** | Input | User Blue Button (B1) |

---

## How to Use

1. Connect the Nucleo-F446RE via USB.
2. Open Serial Monitor in VS Code (115200 baud).
3. Observe the state transitions in the console.
4. Press '+' or '-' in the terminal to adjust the breathing speed in real-time.

---

## Future Improvements

Gamma Correction: Implementing a Look-Up Table (LUT) to match human eye perception.
DMA Integration: Offloading UART transmission from the CPU to the DMA controller.
Command Parser: Implementing a more robust CLI to handle strings like SET_SPEED 50.

---