Installation
============

The driver expects an STM32G4 CubeMX CMake project, HAL GPIO and SPI support,
optional HAL timer support for ``setPwm()``, CMSIS DWT support, and C++17.

Clone it into the firmware tree::

   mkdir -p Components
   git clone https://github.com/sylas-project/drv8316.git Components/DRV8316

Or add a reproducible submodule::

   git submodule add https://github.com/sylas-project/drv8316.git Components/DRV8316
   git submodule update --init --recursive

After CubeMX defines ``stm32cubemx``, link the component::

   add_subdirectory(Components/DRV8316)
   target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE drv8316)

Then include ``drv8316/drv8316.hpp``. The component exports its ``Inc`` path.
The header directly includes ``stm32g4xx_hal.h``; another STM32 family requires
adapting that dependency.

