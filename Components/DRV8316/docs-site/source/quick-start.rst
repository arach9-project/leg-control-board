Quick start
===========

Create a long-lived driver after the HAL handles exist::

   #include "drv8316/drv8316.hpp"

   Drv8316 driver({
       .spi = &hspi1,
       .timer = &htim1,
       .chipSelect = {GPIOA, GPIO_PIN_4},
       .fault = {GPIOB, GPIO_PIN_0},
       .sleep = {GPIOB, GPIO_PIN_1},
       .spiTimeoutMs = 100U,
   });

   if (driver.initialize() != HAL_OK ||
       driver.setPwmMode(DRV8316_PWM_MODE_6X) != HAL_OK ||
       driver.setSlewRate(DRV8316_SLEW_RATE_50V_US) != HAL_OK) {
       Error_Handler();
   }

``initialize()`` validates configuration, enables the DWT cycle counter,
deasserts chip select, wakes the IC, waits 2 ms, unlocks control registers,
activates the driver, clears faults, refreshes diagnostics, and selects 0.15
V/A current-sense gain. It leaves registers unlocked. Call
``lockRegisters()`` after applying settings when runtime writes are unwanted.

