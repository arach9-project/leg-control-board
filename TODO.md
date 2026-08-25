## FOC

- [ ] Read current values accurately, make CurrentSense class for reading current values and calibration
- [ ] Add interrupts for nFault and other status bits and print, enter error state
- [ ] Enable Advanced Timer Output Not Maintained/Enabled (MOE Bit)
- [x] Enable using DRV8316 Driver Enable / Sleep Pin (DRVOFF or NSLEEP)
- [x] Move motor (turn on DRVOFF, turn on nSleep)
- [x] Implement open loop

## CAN communication

## CAN interface

Commands and responses

## Integration

- [ ] Connect to Main Control Board
- [ ] Flash via CAN from main control board
- [ ] Command via CAN from main control board

## Logging

- Gotta make a mount for the motor such that it aligns with the Radar coordinates, and use that to remove the offset between target theta and mechanical angle
- Gotta add button to calibrate the electrical_offset angle
- Even when the motor reaches close to its target goal it jitters
- Current position controller oscillates and swings around the target, when the goal is to reach target quickly and slow down to avoid overshoot
- Current STM32Cube monitor graphs are too quick, and too disorganized and annoying to use. WIll have to organize them.
- A radial background for the radar that shows the sectors of a BLDC would be better.
- A better mount, to hold motor to table and angle indicator needed.
- Reorganization of repo required, react style main project code, with folder for CAD, add stm32 cube monitor files
