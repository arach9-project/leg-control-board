TIM settings
Prescaler (0): divides the timer clock by PSC+1. At 0, the counter runs at full clock speed.
Counter Mode (Center Aligned mode1): the counter counts up to the period value, then back down to 0. This gives symmetric PWM, which is what you want for FOC. It has lower harmonic distortion, and the middle of the PWM period is a clean point to sample phase currents. Mode1 sets the compare flags only while counting down.
Dithering: adds fractional resolution to the period, which you don't need.
Counter Period (4250): the auto-reload value (ARR), which sets the PWM frequency. In center-aligned mode, f_PWM = f_timer / (2 × ARR). If the timer clock is 170 MHz, that's about 20 kHz. At your 16 MHz config it would be about 1.9 kHz, so changing the system clock also changes your PWM frequency unless you adjust this. Duty resolution is about 12 bits (0 to 4250).
Internal Clock Division: doesn't affect the PWM frequency. It sets the clock used for dead-time and input filters.
Repetition Counter (0): the update event fires once every N+1 counter cycles. It's used to run the control loop or ADC triggers slower than the PWM frequency. At 0, it updates every cycle.
Auto-reload preload (Enable): a new period value takes effect at the next update event instead of immediately, which avoids glitches mid-cycle.
