Using the driver
================

Create one long-lived object after CubeMX's SPI handle exists::

   #include "as5048a/as5048a.hpp"

   As5048a encoder({
       .spi = &hspi1,
       .chipSelect = {GPIOA, GPIO_PIN_4},
       .spiTimeoutMs = 100U,
   });

   void startEncoder()
   {
       if (encoder.initialize() != HAL_OK) {
           Error_Handler();
       }
   }

Full validated samples
----------------------

``sample()`` reads angle, magnitude, and diagnostics. It clears the supplied
structure first and sets ``valid`` only after every transfer succeeds and the
diagnostic state is healthy::

   AS5048A_Sample_t sample{};
   if (encoder.sample(sample) == HAL_OK && sample.valid) {
       useAngle(sample.angle.degrees);
   } else {
       handleEncoderFault();
   }

The call returns ``HAL_ERROR`` when offset compensation is unfinished, CORDIC
overflow is set, or the magnet is reported too strong or too weak. Fields read
before a later failure can still contain data, but ``valid`` remains false.

Fast angle polling
------------------

The AS5048A response belongs to the preceding SPI command. The fast path primes
that pipeline once, then returns one angle per subsequent transaction::

   if (encoder.beginContinuousAngleRead() != HAL_OK) {
       Error_Handler();
   }

   AS5048A_Angle_t angle{};
   while (encoder.readNextAngle(angle) == HAL_OK) {
       controlLoop(angle.radians);
   }

Parity, sensor error, or HAL transfer failure unprimes the stream. Call
``beginContinuousAngleRead()`` again before retrying. This path intentionally
does not validate magnitude or magnetic diagnostic flags.

Zero position
-------------

``writeZeroPosition()`` accepts counts from 0 through 16,383. To make the
current shaft position read as zero::

   if (encoder.setCurrentPositionAsZero() != HAL_OK) {
       handleEncoderFault();
   }

These methods write the zero-position registers; they do not burn OTP. The
offset must therefore be restored after power cycling if the application needs
it. OTP programming is permanent and is deliberately not exposed by this API.

