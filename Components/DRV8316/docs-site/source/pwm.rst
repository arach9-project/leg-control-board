PWM output
==========

``setPwm(phaseA, phaseB, phaseC)`` writes raw compare values to timer channels
1, 2, and 3::

   const uint16_t period = __HAL_TIM_GET_AUTORELOAD(&htim1);
   driver.setPwm(period / 2U, period / 2U, period / 2U);

It returns ``HAL_ERROR`` when no timer was configured. It does not start or
stop PWM, constrain values to the period, insert dead time, select the device's
3x/6x mode, or implement commutation or field-oriented control. Those remain
responsibilities of CubeMX and the motor-control layer.

