export PATH="/opt/ST/STM32CubeCLT_1.22.0/STM32CubeProgrammer/bin"
STM32_Programmer_CLI -c port=SWD -w build/*.elf -rst
