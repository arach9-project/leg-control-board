Register map
============

.. list-table:: Registers represented by the driver
   :header-rows: 1
   :widths: 15 35 50

   * - Address
     - Symbol
     - Use
   * - ``0x00``
     - ``DRV8316_REG_IC_STATUS``
     - Summary diagnostics
   * - ``0x01`` / ``0x02``
     - ``DRV8316_REG_STATUS_1`` / ``STATUS_2``
     - Bridge, thermal, SPI, charge-pump, buck, and OTP detail
   * - ``0x03``
     - ``DRV8316_REG_CTRL_1``
     - Register lock
   * - ``0x04``
     - ``DRV8316_REG_CTRL_2``
     - Fault clear, PWM, slew, and SDO
   * - ``0x05``
     - ``DRV8316_REG_CTRL_3``
     - OTW, OVP, and 100% PWM frequency
   * - ``0x06``
     - ``DRV8316_REG_CTRL_4``
     - OCP and driver state
   * - ``0x07``
     - ``DRV8316_REG_CTRL_5``
     - Current sense and recirculation
   * - ``0x08``
     - ``DRV8316_REG_CTRL_6``
     - Buck regulator
   * - ``0x09``–``0x0B``
     - ``DRV8316_REG_CTRL_7``–``CTRL_9``
     - Defined; no public fields currently exposed
   * - ``0x0C``
     - ``DRV8316_REG_CTRL_10``
     - Delay compensation

The 16-bit SPI frame contains direction, a 6-bit address, parity, and 8-bit
data. The driver generates parity and uses read-modify-write operations to
preserve unrelated bits. The datasheet remains the source of truth for
reserved bits, defaults, and device semantics.

