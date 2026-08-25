STM32CubeMX setup
=================

Configure full-duplex master SPI using the mode, bit order, and maximum clock
in the current DRV8316 datasheet. Use software chip select and 8-bit HAL data
size: the driver sends each 16-bit protocol frame as two bytes in one
``HAL_SPI_TransmitReceive()`` call.

Configure ``nSCS`` and ``nSLEEP`` as push-pull outputs; ``nSCS`` idles high.
Configure optional active-low ``nFAULT`` as an input with an appropriate
pull-up. It may also feed EXTI for application-level notification, although
this driver polls it.

For ``setPwm()``, configure timer channels 1–3 as PWM outputs and start them::

   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
   HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

Compare values are raw timer ticks and must remain within the configured
auto-reload value.
