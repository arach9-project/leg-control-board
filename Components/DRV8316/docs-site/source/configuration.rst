Device configuration
====================

Setters validate enum ranges, read the register, update only the selected
field, and write it back. Check every HAL status::

   HAL_StatusTypeDef configureDriver()
   {
       HAL_StatusTypeDef status = driver.unlockRegisters();
       if (status != HAL_OK) return status;
       status = driver.setPwmMode(DRV8316_PWM_MODE_6X);
       if (status != HAL_OK) return status;
       status = driver.setSlewRate(DRV8316_SLEW_RATE_50V_US);
       if (status != HAL_OK) return status;
       status = driver.setOcpMode(DRV8316_OCP_MODE_LATCHED);
       if (status != HAL_OK) return status;
       status = driver.setCurrentSenseGain(DRV8316_CSA_GAIN_0_15V_A);
       if (status != HAL_OK) return status;
       return driver.lockRegisters();
   }

Typed getter/setter pairs cover register lock and SDO mode; PWM mode and slew
rate; overtemperature and overvoltage reporting; overcurrent mode, level,
retry, deglitch, and cycle-by-cycle behavior; driver state, current-sense gain,
ASR, AAR, and recirculation; buck state, voltage, current limit, and sequencing;
and delay target and compensation. Confirm every choice against the hardware
design and datasheet.

