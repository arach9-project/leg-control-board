DRV8316 STM32 driver
====================

.. image:: _static/ti-logo.png
   :alt: Texas Instruments logo
   :width: 150px
   :align: right

A typed C++17 STM32 HAL driver for the Texas Instruments DRV8316 integrated
three-phase motor driver. It provides parity-protected SPI register access,
strongly typed configuration, cached fault diagnostics, sleep control, and
three-channel PWM compare updates.

.. image:: _static/drv8316.png
   :alt: DRV8316 integrated circuit package
   :width: 500px
   :align: center

This is an independent community project, not an official TI product. TI
assets and marks remain the property of Texas Instruments. Consult the
:download:`bundled datasheet <_static/drv8316-datasheet.pdf>` for electrical,
timing, protection, schematic, layout, and package requirements.

.. toctree::
   :maxdepth: 2
   :caption: Start here

   installation
   stm32cubemx
   wiring
   quick-start

.. toctree::
   :maxdepth: 2
   :caption: Guides

   configuration
   diagnostics
   pwm

.. toctree::
   :maxdepth: 2
   :caption: Reference

   api
   types
   register-map
   troubleshooting
   documentation
   licensing

