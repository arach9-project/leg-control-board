Register and protocol reference
===============================

SPI frames are 16 bits. Bit 15 is even parity. In a command, bit 14 selects a
read and bits 13:0 carry the address. In a response, bit 14 is the error flag
and bits 13:0 carry data. Read responses are delayed by one frame, so ordinary
reads send a command followed by NOP.

.. list-table:: Registers used by the driver
   :header-rows: 1
   :widths: 18 34 48

   * - Address
     - Symbol
     - Purpose
   * - ``0x0000``
     - ``AS5048A_REG_NOP``
     - Retrieves a pending response without requesting another register.
   * - ``0x0001``
     - ``AS5048A_REG_CLEAR_ERROR``
     - Returns and clears communication error flags.
   * - ``0x0016``
     - ``AS5048A_REG_ZERO_POSITION_HIGH``
     - Zero-position bits 13:6.
   * - ``0x0017``
     - ``AS5048A_REG_ZERO_POSITION_LOW``
     - Zero-position bits 5:0.
   * - ``0x3FFD``
     - ``AS5048A_REG_DIAGNOSTICS_AGC``
     - AGC and magnetic diagnostic flags.
   * - ``0x3FFE``
     - ``AS5048A_REG_MAGNITUDE``
     - 14-bit CORDIC magnitude.
   * - ``0x3FFF``
     - ``AS5048A_REG_ANGLE``
     - 14-bit compensated angle.

``registers.h`` also defines masks, offsets, angle conversion constants, field
helpers, response helpers, and zero-position split/combine macros. Programming
control symbols are defined for completeness, but the public driver provides
no OTP programming operation.

