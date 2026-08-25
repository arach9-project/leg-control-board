export PATH="$PATH:/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/Resources/bin/"
STM32_Programmer_CLI -c port=SWD freq=8000 mode=NORMAL -swv freq=84 portnumber=0 -RA
