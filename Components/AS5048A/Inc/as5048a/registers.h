#ifndef AS5048A_REGISTERS_H
#define AS5048A_REGISTERS_H

#include <stdint.h>

/*
 * AS5048A Magnetic Rotary Encoder
 *
 * SPI frame:
 *
 * Command frame, MOSI:
 *   Bit 15     : PAR  - even parity
 *   Bit 14     : R/Wn - 1 = read, 0 = write
 *   Bits 13:0  : register address
 *
 * Read response frame, MISO:
 *   Bit 15     : PAR  - even parity
 *   Bit 14     : EF   - error flag
 *   Bits 13:0  : register data
 *
 * Write-data frame, MOSI:
 *   Bit 15     : PAR  - even parity
 *   Bit 14     : must be 0
 *   Bits 13:0  : data
 */

/* -------------------------------------------------------------------------- */
/* SPI frame layout                                                           */
/* -------------------------------------------------------------------------- */

#define AS5048A_SPI_PARITY_OFFSET 15U
#define AS5048A_SPI_RW_OFFSET 14U
#define AS5048A_SPI_ERROR_OFFSET 14U

#define AS5048A_SPI_PARITY_MASK (0x01U << AS5048A_SPI_PARITY_OFFSET)
#define AS5048A_SPI_RW_MASK (0x01U << AS5048A_SPI_RW_OFFSET)
#define AS5048A_SPI_ERROR_MASK (0x01U << AS5048A_SPI_ERROR_OFFSET)

#define AS5048A_SPI_ADDRESS_OFFSET 0U
#define AS5048A_SPI_ADDRESS_MASK 0x3FFFU

#define AS5048A_SPI_DATA_OFFSET 0U
#define AS5048A_SPI_DATA_MASK 0x3FFFU

#define AS5048A_SPI_READ AS5048A_SPI_RW_MASK
#define AS5048A_SPI_WRITE 0x0000U

/*
 * A NOP command is commonly used for retrieving the response to the
 * preceding command because AS5048A SPI responses are delayed by one frame.
 */
#define AS5048A_SPI_NOP_COMMAND 0x0000U

/* -------------------------------------------------------------------------- */
/* Register addresses                                                         */
/* -------------------------------------------------------------------------- */

/* Control and error registers */
#define AS5048A_REG_NOP 0x0000U
#define AS5048A_REG_CLEAR_ERROR 0x0001U
#define AS5048A_REG_PROGRAMMING_CONTROL 0x0003U

/* Programmable customer settings */
#define AS5048A_REG_ZERO_POSITION_HIGH 0x0016U
#define AS5048A_REG_ZERO_POSITION_LOW 0x0017U

/* Readout registers */
#define AS5048A_REG_DIAGNOSTICS_AGC 0x3FFDU
#define AS5048A_REG_MAGNITUDE 0x3FFEU
#define AS5048A_REG_ANGLE 0x3FFFU

/* Common aliases */
#define AS5048A_REG_CLEAR_ERROR_FLAG AS5048A_REG_CLEAR_ERROR
#define AS5048A_REG_PROGRAMMING_CTRL AS5048A_REG_PROGRAMMING_CONTROL
#define AS5048A_REG_DIAG_AGC AS5048A_REG_DIAGNOSTICS_AGC

/* -------------------------------------------------------------------------- */
/* CLEAR_ERROR register: 0x0001                                               */
/* -------------------------------------------------------------------------- */

/*
 * Reading this register returns the communication error causes and clears
 * the stored errors.
 */

#define AS5048A_CLEAR_ERROR_FRAMING_OFFSET 0U
#define AS5048A_CLEAR_ERROR_COMMAND_INVALID_OFFSET 1U
#define AS5048A_CLEAR_ERROR_PARITY_OFFSET 2U

#define AS5048A_CLEAR_ERROR_FRAMING_MASK                                       \
    (0x01U << AS5048A_CLEAR_ERROR_FRAMING_OFFSET)

#define AS5048A_CLEAR_ERROR_COMMAND_INVALID_MASK                               \
    (0x01U << AS5048A_CLEAR_ERROR_COMMAND_INVALID_OFFSET)

#define AS5048A_CLEAR_ERROR_PARITY_MASK                                        \
    (0x01U << AS5048A_CLEAR_ERROR_PARITY_OFFSET)

#define AS5048A_CLEAR_ERROR_ALL_MASK                                           \
    (AS5048A_CLEAR_ERROR_FRAMING_MASK |                                        \
     AS5048A_CLEAR_ERROR_COMMAND_INVALID_MASK |                                \
     AS5048A_CLEAR_ERROR_PARITY_MASK)

/* Alternative names matching the datasheet terminology */
#define AS5048A_ERROR_FRAMING_OFFSET AS5048A_CLEAR_ERROR_FRAMING_OFFSET
#define AS5048A_ERROR_FRAMING_MASK AS5048A_CLEAR_ERROR_FRAMING_MASK

#define AS5048A_ERROR_COMMAND_INVALID_OFFSET                                   \
    AS5048A_CLEAR_ERROR_COMMAND_INVALID_OFFSET

#define AS5048A_ERROR_COMMAND_INVALID_MASK                                     \
    AS5048A_CLEAR_ERROR_COMMAND_INVALID_MASK

#define AS5048A_ERROR_PARITY_OFFSET AS5048A_CLEAR_ERROR_PARITY_OFFSET
#define AS5048A_ERROR_PARITY_MASK AS5048A_CLEAR_ERROR_PARITY_MASK

/* -------------------------------------------------------------------------- */
/* PROGRAMMING_CONTROL register: 0x0003                                       */
/* -------------------------------------------------------------------------- */

/*
 * Warning:
 * OTP programming is permanent. Normal angle reading does not require
 * writing this register.
 */

#define AS5048A_PROGRAMMING_ENABLE_OFFSET 0U
#define AS5048A_PROGRAMMING_BURN_OFFSET 3U
#define AS5048A_PROGRAMMING_VERIFY_OFFSET 6U

#define AS5048A_PROGRAMMING_ENABLE_MASK                                        \
    (0x01U << AS5048A_PROGRAMMING_ENABLE_OFFSET)

#define AS5048A_PROGRAMMING_BURN_MASK (0x01U << AS5048A_PROGRAMMING_BURN_OFFSET)

#define AS5048A_PROGRAMMING_VERIFY_MASK                                        \
    (0x01U << AS5048A_PROGRAMMING_VERIFY_OFFSET)

/* -------------------------------------------------------------------------- */
/* ZERO_POSITION_HIGH register: 0x0016                                        */
/* -------------------------------------------------------------------------- */

/*
 * Register bits 7:0 contain zero-position bits 13:6.
 *
 * A complete 14-bit zero-position value is split as:
 *
 *   ZERO_POSITION_HIGH[7:0] = zero_position[13:6]
 *   ZERO_POSITION_LOW [5:0] = zero_position[5:0]
 */

#define AS5048A_ZERO_HIGH_OFFSET 0U
#define AS5048A_ZERO_HIGH_MASK 0x00FFU

#define AS5048A_ZERO_HIGH_SOURCE_OFFSET 6U
#define AS5048A_ZERO_HIGH_SOURCE_MASK 0x3FC0U

/* -------------------------------------------------------------------------- */
/* ZERO_POSITION_LOW register: 0x0017                                         */
/* -------------------------------------------------------------------------- */

#define AS5048A_ZERO_LOW_OFFSET 0U
#define AS5048A_ZERO_LOW_MASK 0x003FU

#define AS5048A_ZERO_LOW_SOURCE_OFFSET 0U
#define AS5048A_ZERO_LOW_SOURCE_MASK 0x003FU

/* Helpers for splitting and joining the zero-position value */
#define AS5048A_ZERO_POSITION_MASK 0x3FFFU

#define AS5048A_ZERO_POSITION_HIGH_VALUE(position)                             \
    ((((uint16_t)(position)) >> 6U) & AS5048A_ZERO_HIGH_MASK)

#define AS5048A_ZERO_POSITION_LOW_VALUE(position)                              \
    (((uint16_t)(position)) & AS5048A_ZERO_LOW_MASK)

#define AS5048A_ZERO_POSITION_COMBINE(high_value, low_value)                   \
    ((uint16_t)(((((uint16_t)(high_value)) & AS5048A_ZERO_HIGH_MASK) << 6U) |  \
                (((uint16_t)(low_value)) & AS5048A_ZERO_LOW_MASK)))

/* -------------------------------------------------------------------------- */
/* DIAGNOSTICS_AGC register: 0x3FFD                                           */
/* -------------------------------------------------------------------------- */

/*
 * Bits 7:0  : Automatic Gain Control value
 * Bit 8     : OCF       - offset compensation finished
 * Bit 9     : COF       - CORDIC overflow
 * Bit 10    : COMP_LOW  - magnetic field too strong
 * Bit 11    : COMP_HIGH - magnetic field too weak
 */

#define AS5048A_DIAG_AGC_OFFSET 0U
#define AS5048A_DIAG_AGC_MASK 0x00FFU

#define AS5048A_DIAG_OCF_OFFSET 8U
#define AS5048A_DIAG_COF_OFFSET 9U
#define AS5048A_DIAG_COMP_LOW_OFFSET 10U
#define AS5048A_DIAG_COMP_HIGH_OFFSET 11U

#define AS5048A_DIAG_OCF_MASK (0x01U << AS5048A_DIAG_OCF_OFFSET)

#define AS5048A_DIAG_COF_MASK (0x01U << AS5048A_DIAG_COF_OFFSET)

#define AS5048A_DIAG_COMP_LOW_MASK (0x01U << AS5048A_DIAG_COMP_LOW_OFFSET)

#define AS5048A_DIAG_COMP_HIGH_MASK (0x01U << AS5048A_DIAG_COMP_HIGH_OFFSET)

#define AS5048A_DIAG_FLAGS_MASK                                                \
    (AS5048A_DIAG_OCF_MASK | AS5048A_DIAG_COF_MASK |                           \
     AS5048A_DIAG_COMP_LOW_MASK | AS5048A_DIAG_COMP_HIGH_MASK)

#define AS5048A_DIAG_MAGNET_ERROR_MASK                                         \
    (AS5048A_DIAG_COF_MASK | AS5048A_DIAG_COMP_LOW_MASK |                      \
     AS5048A_DIAG_COMP_HIGH_MASK)

/*
 * AGC interpretation:
 *   0   = strong magnetic field / minimum gain
 *   255 = weak magnetic field / maximum gain
 */
#define AS5048A_AGC_MIN_VALUE 0U
#define AS5048A_AGC_MAX_VALUE 255U

/* -------------------------------------------------------------------------- */
/* MAGNITUDE register: 0x3FFE                                                 */
/* -------------------------------------------------------------------------- */

#define AS5048A_MAGNITUDE_OFFSET 0U
#define AS5048A_MAGNITUDE_MASK 0x3FFFU

/* -------------------------------------------------------------------------- */
/* ANGLE register: 0x3FFF                                                     */
/* -------------------------------------------------------------------------- */

#define AS5048A_ANGLE_OFFSET 0U
#define AS5048A_ANGLE_MASK 0x3FFFU

#define AS5048A_ANGLE_RESOLUTION_BITS 14U
#define AS5048A_ANGLE_COUNTS_PER_REV 16384U
#define AS5048A_ANGLE_MAX_COUNT 16383U

#define AS5048A_DEGREES_PER_COUNT (360.0F / (float)AS5048A_ANGLE_COUNTS_PER_REV)

#define AS5048A_RADIANS_PER_COUNT                                              \
    (6.2831853071795864769F / (float)AS5048A_ANGLE_COUNTS_PER_REV)

/* -------------------------------------------------------------------------- */
/* Field extraction helpers                                                   */
/* -------------------------------------------------------------------------- */

#define AS5048A_FIELD_GET(value, mask, offset)                                 \
    ((((uint16_t)(value)) & ((uint16_t)(mask))) >> (offset))

#define AS5048A_FIELD_PREP(value, mask, offset)                                \
    ((uint16_t)((((uint16_t)(value)) << (offset)) & ((uint16_t)(mask))))

#define AS5048A_RESPONSE_HAS_ERROR(response)                                   \
    ((((uint16_t)(response)) & AS5048A_SPI_ERROR_MASK) != 0U)

#define AS5048A_RESPONSE_DATA(response)                                        \
    (((uint16_t)(response)) & AS5048A_SPI_DATA_MASK)

#define AS5048A_DIAG_GET_AGC(value)                                            \
    AS5048A_FIELD_GET((value), AS5048A_DIAG_AGC_MASK, AS5048A_DIAG_AGC_OFFSET)

#define AS5048A_ANGLE_GET_COUNT(value)                                         \
    (((uint16_t)(value)) & AS5048A_ANGLE_MASK)

#define AS5048A_MAGNITUDE_GET(value)                                           \
    (((uint16_t)(value)) & AS5048A_MAGNITUDE_MASK)

#endif /* AS5048A_REGISTERS_H */
