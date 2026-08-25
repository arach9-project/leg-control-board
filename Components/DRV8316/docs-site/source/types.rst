Types and enums
===============

All public device types are C-compatible and declared in
``drv8316/drv8316_types.h``. Enum families represent register lock, SDO output,
slew rate, PWM mode, thermal warning, overvoltage protection and level, 100%
PWM frequency, OCP mode/level/retry/deglitch/CBC, driver state, current-sense
gain, ASR, AAR, current-limit recirculation, buck state/voltage/current limit/
sequencing, and delay target/compensation.

``DRV8316_Diagnostics_t`` stores the raw ``ic_status``, ``status_1``, and
``status_2`` bytes. Decode them with masks from ``drv8316_registers.h``. The
snapshot changes only after ``refreshDiagnostics()``.

