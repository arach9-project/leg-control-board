API reference
=============

Include ``as5048a/as5048a.hpp``. ``As5048a`` is neither copyable nor movable;
the HAL handle and GPIO referenced by its configuration must remain valid.

``As5048a::Pin``
----------------

``GPIO_TypeDef *port`` and ``uint16_t number`` describe one HAL GPIO. The
``isValid()`` helper returns true when ``port`` is non-null.

``As5048a::Config``
-------------------

``SPI_HandleTypeDef *spi`` is required. ``Pin chipSelect`` is the required
active-low chip select. ``uint32_t spiTimeoutMs`` is passed to
``HAL_SPI_TransmitReceive`` and defaults to 100 ms.

Construction and initialization
-------------------------------

``explicit As5048a(const Config &config) noexcept``
   Stores the configuration. It performs no hardware access.

``HAL_StatusTypeDef initialize()``
   Validates SPI and chip select, enables the Cortex DWT cycle counter, drives
   chip select high, waits 10 ms, and performs a NOP transfer. Returns
   ``HAL_ERROR`` for invalid configuration or propagates the HAL transfer
   status.

Sampling
--------

``HAL_StatusTypeDef sample(AS5048A_Sample_t &sample)``
   Reads angle, magnitude, and diagnostics; converts the angle to degrees and
   radians; checks magnetic health; and marks the result valid. The output is
   always initialized. Returns ``HAL_OK`` only for a complete healthy sample.

``HAL_StatusTypeDef beginContinuousAngleRead()``
   Sends the first angle command to prime the sensor's delayed-response SPI
   pipeline. Call once before ``readNextAngle()``.

``HAL_StatusTypeDef readNextAngle(AS5048A_Angle_t &angle)``
   Retrieves the previous angle response while requesting the next one. Checks
   response parity and the sensor error flag, fills all angle units, and
   unprimes the pipeline on failure.

Zero and error control
----------------------

``HAL_StatusTypeDef readZeroPosition(uint16_t &zeroPosition)``
   Reads and combines the high and low zero-position fields into a 14-bit
   count. The output is cleared before access.

``HAL_StatusTypeDef writeZeroPosition(uint16_t zeroPosition)``
   Splits and writes a 14-bit zero offset. Values above ``0x3FFF`` return
   ``HAL_ERROR`` without a write. This does not program OTP.

``HAL_StatusTypeDef setCurrentPositionAsZero()``
   Clears the existing offset, reads the physical angle, then writes that
   angle as the new volatile zero offset.

``HAL_StatusTypeDef clearCommunicationErrors(uint16_t &rawErrors)``
   Reads ``CLEAR_ERROR``. The read returns the latched framing, invalid-command,
   and parity bits and clears them in the sensor.

Status and timing behavior
--------------------------

Public hardware methods return STM32 ``HAL_StatusTypeDef``. ``HAL_BUSY`` means
the SPI peripheral was not ready. Transfer timeout or bus failure statuses are
propagated. Protocol parity, sensor error flags, invalid inputs, invalid
configuration, and unhealthy full-sample diagnostics report ``HAL_ERROR``.
Transfers use a DWT busy-wait to provide 1 µs chip-select timing gaps.

