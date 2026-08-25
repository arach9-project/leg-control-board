Class API
=========

``Drv8316::Config`` contains required ``spi``, ``chipSelect``, and ``sleep``;
optional ``timer`` and ``fault``; and a 100 ms default SPI timeout. The object
is non-copyable and non-movable, and referenced HAL objects must outlive it.

Lifecycle and diagnostics
-------------------------

``initialize()`` wakes and prepares the device. ``wake()`` and ``sleep()``
drive ``nSLEEP``. ``isFaultPinAsserted()`` polls ``nFAULT``.
``refreshDiagnostics()`` updates the snapshot returned by ``diagnostics()``.
``clearFaults()`` requests device fault clearing. All hardware operations other
than the pin/query helpers return ``HAL_StatusTypeDef``.

Register configuration
----------------------

``lockRegisters()`` and ``unlockRegisters()`` are convenience operations.
Every configurable field also has a ``get…(Enum &value)`` and
``set…(Enum value)`` pair:

.. list-table:: Typed field methods
   :header-rows: 1
   :widths: 25 75

   * - Group
     - Method suffixes
   * - Interface
     - ``RegisterLock``, ``SdoMode``
   * - Bridge
     - ``SlewRate``, ``PwmMode``, ``DriverState``
   * - Protection
     - ``OvertemperatureReporting``, ``OvervoltageProtection``,
       ``OvervoltageLevel``, ``Pwm100Frequency``
   * - Overcurrent
     - ``OcpMode``, ``OcpLevel``, ``OcpRetryTime``, ``OcpDeglitchTime``,
       ``OcpCycleByCycle``
   * - Current sense
     - ``CurrentSenseGain``, ``Asr``, ``Aar``, ``IlimRecirculation``
   * - Buck regulator
     - ``BuckState``, ``BuckVoltage``, ``BuckCurrentLimit``,
       ``BuckPowerSequencing``
   * - Delay
     - ``DelayTarget``, ``DelayCompensation``

PWM
---

``HAL_StatusTypeDef setPwm(uint16_t phaseA, uint16_t phaseB, uint16_t phaseC)``
sets timer channel 1–3 compare registers. See :doc:`pwm` for its boundaries.

