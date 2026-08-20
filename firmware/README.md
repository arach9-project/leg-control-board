# leg-control-board

To clone repo:

```bash
git clone --recurse-submodules git@github.com:sylas-project/leg_control_board.git
```

# DRV8316 Driver

Convert functions to STM32
Make docs
Publish library and docs
Send PWM signals

# STM32 Project Template

- make template that works with neovim and clangd and cmake, setup LSP

## FOC Control

## The Complete Process Flow

```
 [User Input] Target Angle
      │
      ▼
┌─────────────────────────────────────────────────────────┐
│ 1. OUTER LOOP: Position PID                             │
│    Compare: (Target Angle - Measured Encoder Angle)     │
│    Output: Target Velocity                              │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 2. MIDDLE LOOP: Velocity PID                            │
│    Compare: (Target Velocity - Measured Velocity)       │
│    Output: Target Torque Current (Iq_target)            │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 3. INNER LOOP: Current (FOC) Loop                       │
│                                                         │
│    a. Read Inputs                                       │
│       • Get Angle (θ) from Encoder                      │
│       • Get Phase Currents (Ia, Ib) from ADC            │
│                                                         │
│    b. Forward Math (3-Phase AC ──► 2D DC)               │
│       • Clarke Transform: (Ia, Ib) ──► (I_alpha, I_beta)│
│       • Park Transform:   (I_alpha, I_beta, θ) ──► (Iq, Id)
│                                                         │
│    c. Torque & Magnetization PIDs                       │
│       • Iq PID: Compare (Iq_target - Iq_measured) ──► Vq│
│       • Id PID: Compare (0 - Id_measured) ──────────► Vd│
│                                                         │
│    d. Reverse Math (2D DC ──► 3-Phase Duty Cycles)     │
│       • Inverse Park:  (Vq, Vd, θ) ──► (V_alpha, V_beta)│
│       • Space Vector PWM (SVPWM):                       │
│         (V_alpha, V_beta) ──► (DutyA, DutyB, DutyC)     │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│ 4. HARDWARE PWM GENERATION                              │
│    Write DutyA, DutyB, DutyC to PWM Timer Registers     │
└─────────────────────────────────────────────────────────┘
```

Stages:

| Stage | What You Build | What It Teaches / Achieves |
| --- | --- | --- |
| **Stage 1** | Open-Loop Velocity | Writes raw sine wave PWM to test power stage MOSFETs and motor wiring. No sensors needed. |
| **Stage 2** | Voltage-Mode Position Loop | Connects encoder. Runs Position PID $\rightarrow$ Inverse Park $\rightarrow$ PWM. (SimpleFOC style). |
| **Stage 3** | ADC Current Reading | Configures ADC to measure current sense resistors. Verifies phase current telemetry. |
| **Stage 4** | Full Current-Control FOC | Implements Clarke/Park & Current PIDs ($I_q, I_d$). True torque control achieved! |
| **Stage 5** | Timer + ADC DMA Interrupts | Moves Stage 4 into a hardware Timer/ADC DMA ISR for deterministic performance (ODrive style). |

Pipeline:

1. Encoder Read           ──►  Gets Rotor Angle (θ)
2. ADC Current Read       ──►  Gets Ia, Ib
3. Clarke Transform       ──►  (Ia, Ib) ──► I_alpha, I_beta
4. Park Transform         ──►  (I_alpha, I_beta, θ) ──► Iq_measured, Id_measured

5. Position PID           ──►  (Angle_target - Angle_meas) ──► Target Velocity
6. Velocity PID           ──►  (Vel_target - Vel_meas)     ──► Iq_target
7. Iq Current PID         ──►  (Iq_target - Iq_measured)   ──► Vq
8. Id Current PID         ──►  (0 - Id_measured)           ──► Vd

9. Inverse Park           ──►  (Vq, Vd, θ) ──► V_alpha, V_beta
10. Space Vector PWM      ──►  (V_alpha, V_beta) ──► DutyA, DutyB, DutyC
11. Update Timers         ──►  MOSFETs switch ──► Motor turns smoothly!
