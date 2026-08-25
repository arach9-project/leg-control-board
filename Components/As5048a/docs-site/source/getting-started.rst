Installation and setup
======================

Requirements
------------

The library requires an STM32G4 project generated with STM32CubeMX, STM32 HAL
GPIO and SPI support, CMSIS DWT cycle-counter support, CMake, and a C++17
compiler. The public header currently includes ``stm32g4xx_hal.h`` directly.

Clone the driver
----------------

From the firmware repository root::

   mkdir -p Components
   git clone https://github.com/sylas-project/as5048a.git Components/As5048a

For a reproducible dependency, add it as a submodule instead::

   git submodule add https://github.com/sylas-project/as5048a.git Components/As5048a
   git submodule update --init --recursive

Link it with CMake
------------------

Add the component after CubeMX has defined ``stm32cubemx``::

   add_subdirectory(Components/As5048a)
   target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE as5048a)

The target exports its ``Inc`` directory. Application code can then use::

   #include "as5048a/as5048a.hpp"

STM32CubeMX SPI configuration
-----------------------------

Configure a master, two-line full-duplex SPI peripheral with 8-bit HAL data
size, MSB first, software NSS, clock polarity low, and capture on the second
edge (SPI mode 1). Keep SCK at or below 10 MHz. Disable hardware CRC. The
driver sends each 16-bit sensor frame as two bytes while holding chip select
low.

Configure the chip-select pin as a push-pull output, idle high. Initialize the
HAL, GPIO, and SPI before calling ``As5048a::initialize()``. Do not allow DMA,
interrupt code, or another task to use the same SPI peripheral concurrently.

