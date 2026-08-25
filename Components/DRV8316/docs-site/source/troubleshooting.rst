Troubleshooting
===============

``initialize()`` returns ``HAL_ERROR``
   Confirm ``spi``, ``chipSelect``, and ``sleep``. ``fault`` and ``timer`` are
   optional until their methods are used.

Transfers return ``HAL_BUSY``
   Serialize access to the SPI handle and check for active DMA, interrupts,
   RTOS tasks, or another device owner.

SPI faults or incorrect reads
   Verify CPOL/CPHA, MSB-first ordering, clock frequency, 8-bit HAL data size,
   chip-select timing, ground, and SDO mode. Inspect ``STATUS_2`` for SPI flags.

``isFaultPinAsserted()`` is always false
   An invalid fault pin returns false. Check its port, pin, pull-up, and
   active-low interpretation.

PWM does not change
   Supply a timer, configure and start channels 1–3, verify alternate-function
   routing, stay within the timer period, and match device PWM mode to wiring.

Microsecond delay stalls
   Confirm Cortex DWT support and an accurate ``SystemCoreClock``. The current
   implementation targets STM32G4.

