#ifndef AS5048A_ADAPTOR_HPP
#define AS5048A_ADAPTOR_HPP

#include "as5048a/driver.hpp"

/* C++-only access to the actual objects. Never include this file from C. */
extern As5048a::Driver encoder1;
extern As5048a::Driver encoder2;
extern As5048a::Driver encoder3;

#endif // !AS5048A_ADAPTOR_HPP
