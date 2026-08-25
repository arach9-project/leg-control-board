Troubleshooting
===============

``initialize()`` returns ``HAL_ERROR``
   Check that both ``Config::spi`` and the chip-select port are non-null and
   that HAL GPIO/SPI initialization ran first.

Transfers return ``HAL_BUSY``
   The HAL SPI state was not ready. Serialize access and check for unfinished
   DMA, interrupt, RTOS-task, or other peripheral-owner activity.

Every read fails parity or raises the sensor error flag
   Verify SPI mode 1, MSB-first order, 8-bit HAL data size, a maximum 10 MHz
   clock, active-low software chip select, signal integrity, supply, and common
   ground. Then use ``clearCommunicationErrors()`` to inspect the latched cause.

``sample()`` fails while angle polling works
   Full sampling rejects incomplete offset compensation, CORDIC overflow, and
   too-strong or too-weak magnetic-field flags. Inspect the returned diagnostic
   fields and correct magnet alignment, strength, or air gap.

``readNextAngle()`` fails immediately
   Call ``beginContinuousAngleRead()`` first. After any fast-read failure, prime
   the pipeline again before retrying.

Microsecond delay stalls
   The driver depends on the Cortex DWT cycle counter and ``SystemCoreClock``.
   Confirm DWT support and that the clock value is current. The implementation
   currently targets STM32G4.

