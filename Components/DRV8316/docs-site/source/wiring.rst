Wiring and pinout
=================

.. image:: _static/drv8316-pinout.png
   :alt: DRV8316 40-pin package pinout
   :width: 720px
   :align: center

Connect ``SCLK`` to SPI clock, ``SDI`` to MOSI, ``SDO`` to MISO, and ``nSCS``
to the configured active-low GPIO. ``nSLEEP`` controls device power state and
is required by the driver. ``nFAULT`` is an optional active-low input. Route
``INHx`` and ``INLx`` according to the selected 3x or 6x PWM mode.

The diagram is an IC package pinout, not a board wiring diagram. Verify common
ground, logic levels, decoupling, charge-pump parts, current-sense routing,
motor-supply protection, thermal-pad connection, and layout against the
:download:`DRV8316 datasheet <_static/drv8316-datasheet.pdf>`.

.. warning::

   Power-stage wiring and configuration errors can damage the IC, motor,
   controller, or test equipment. The software documentation does not replace
   TI's ratings, reference schematic, or layout guidance.

