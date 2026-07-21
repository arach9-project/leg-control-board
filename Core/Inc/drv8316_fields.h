#ifndef DRV8316_FIELDS_H
#define DRV8316_FIELDS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Assumptions:
 * - DRV8316_HandleTypeDef and HAL_StatusTypeDef are declared by your main
 * driver header.
 * - DRV8316_Read_Register() and DRV8316_Write_Register() already exist.
 * - Register address macros are defined as shown below.
 */

#define DRV8316_REG_CTRL_1 0x03U
#define DRV8316_REG_CTRL_2 0x04U
#define DRV8316_REG_CTRL_3 0x05U
#define DRV8316_REG_CTRL_4 0x06U
#define DRV8316_REG_CTRL_5 0x07U
#define DRV8316_REG_CTRL_6 0x08U
#define DRV8316_REG_CTRL_10 0x0CU

/* ============================================================
 * Common enable/disable type
 * ============================================================ */

typedef enum { DRV8316_DISABLE = 0U, DRV8316_ENABLE = 1U } DRV8316_Enable_t;

/* ============================================================
 * CTRL_3 — register address 0x05
 * ============================================================ */

#define DRV8316_CTRL3_OTW_REP_OFFSET 0U
#define DRV8316_CTRL3_OVP_EN_OFFSET 2U
#define DRV8316_CTRL3_OVP_SEL_OFFSET 3U
#define DRV8316_CTRL3_PWM_100_DUTY_OFFSET 4U

#define DRV8316_CTRL3_OTW_REP_MASK                                             \
    (0x01U << DRV8316_CTRL3_OTW_REP_OFFSET) /* Bit [0] */

#define DRV8316_CTRL3_OVP_EN_MASK                                              \
    (0x01U << DRV8316_CTRL3_OVP_EN_OFFSET) /* Bit [2] */

#define DRV8316_CTRL3_OVP_SEL_MASK                                             \
    (0x01U << DRV8316_CTRL3_OVP_SEL_OFFSET) /* Bit [3] */

#define DRV8316_CTRL3_PWM_100_DUTY_MASK                                        \
    (0x01U << DRV8316_CTRL3_PWM_100_DUTY_OFFSET) /* Bit [4] */

typedef enum {
    DRV8316_OTW_REPORT_DISABLED = 0U,
    DRV8316_OTW_REPORT_ENABLED = 1U
} DRV8316_OTW_Report_t;

typedef enum {
    DRV8316_OVP_DISABLED = 0U,
    DRV8316_OVP_ENABLED = 1U
} DRV8316_OVP_Enable_t;

typedef enum {
    DRV8316_OVP_LEVEL_34V = 0U,
    DRV8316_OVP_LEVEL_22V = 1U
} DRV8316_OVP_Level_t;

typedef enum {
    DRV8316_PWM_100_DUTY_FREQ_20KHZ = 0U,
    DRV8316_PWM_100_DUTY_FREQ_40KHZ = 1U
} DRV8316_PWM100DutyFrequency_t;

/* ============================================================
 * CTRL_4 — register address 0x06
 * ============================================================ */

#define DRV8316_CTRL4_OCP_MODE_OFFSET 0U
#define DRV8316_CTRL4_OCP_LVL_OFFSET 2U
#define DRV8316_CTRL4_OCP_RETRY_OFFSET 3U
#define DRV8316_CTRL4_OCP_DEG_OFFSET 4U
#define DRV8316_CTRL4_OCP_CBC_OFFSET 6U
#define DRV8316_CTRL4_DRV_OFF_OFFSET 7U

#define DRV8316_CTRL4_OCP_MODE_MASK                                            \
    (0x03U << DRV8316_CTRL4_OCP_MODE_OFFSET) /* Bits [1:0] */

#define DRV8316_CTRL4_OCP_LVL_MASK                                             \
    (0x01U << DRV8316_CTRL4_OCP_LVL_OFFSET) /* Bit [2] */

#define DRV8316_CTRL4_OCP_RETRY_MASK                                           \
    (0x01U << DRV8316_CTRL4_OCP_RETRY_OFFSET) /* Bit [3] */

#define DRV8316_CTRL4_OCP_DEG_MASK                                             \
    (0x03U << DRV8316_CTRL4_OCP_DEG_OFFSET) /* Bits [5:4] */

#define DRV8316_CTRL4_OCP_CBC_MASK                                             \
    (0x01U << DRV8316_CTRL4_OCP_CBC_OFFSET) /* Bit [6] */

#define DRV8316_CTRL4_DRV_OFF_MASK                                             \
    (0x01U << DRV8316_CTRL4_DRV_OFF_OFFSET) /* Bit [7] */

typedef enum {
    DRV8316_OCP_MODE_LATCHED_SHUTDOWN = 0U,
    DRV8316_OCP_MODE_AUTOMATIC_RETRY = 1U,
    DRV8316_OCP_MODE_REPORT_ONLY = 2U,
    DRV8316_OCP_MODE_DISABLED = 3U
} DRV8316_OCP_Mode_t;

typedef enum {
    DRV8316_OCP_LEVEL_16A = 0U,
    DRV8316_OCP_LEVEL_24A = 1U
} DRV8316_OCP_Level_t;

typedef enum {
    DRV8316_OCP_RETRY_TIME_5MS = 0U,
    DRV8316_OCP_RETRY_TIME_500MS = 1U
} DRV8316_OCP_RetryTime_t;

typedef enum {
    DRV8316_OCP_DEGLITCH_0_2US = 0U,
    DRV8316_OCP_DEGLITCH_0_6US = 1U,
    DRV8316_OCP_DEGLITCH_1_25US = 2U,
    DRV8316_OCP_DEGLITCH_1_6US = 3U
} DRV8316_OCP_DeglitchTime_t;

typedef enum {
    DRV8316_OCP_CBC_DISABLED = 0U,
    DRV8316_OCP_CBC_ENABLED = 1U
} DRV8316_OCP_CBC_t;

typedef enum {
    DRV8316_DRIVER_ACTIVE = 0U,
    DRV8316_DRIVER_STANDBY = 1U
} DRV8316_DriverState_t;

/* ============================================================
 * CTRL_5 — register address 0x07
 * ============================================================ */

#define DRV8316_CTRL5_CSA_GAIN_OFFSET 0U
#define DRV8316_CTRL5_EN_ASR_OFFSET 2U
#define DRV8316_CTRL5_EN_AAR_OFFSET 3U
#define DRV8316_CTRL5_ILIM_RECIR_OFFSET 6U

#define DRV8316_CTRL5_CSA_GAIN_MASK                                            \
    (0x03U << DRV8316_CTRL5_CSA_GAIN_OFFSET) /* Bits [1:0] */

#define DRV8316_CTRL5_EN_ASR_MASK                                              \
    (0x01U << DRV8316_CTRL5_EN_ASR_OFFSET) /* Bit [2] */

#define DRV8316_CTRL5_EN_AAR_MASK                                              \
    (0x01U << DRV8316_CTRL5_EN_AAR_OFFSET) /* Bit [3] */

#define DRV8316_CTRL5_ILIM_RECIR_MASK                                          \
    (0x01U << DRV8316_CTRL5_ILIM_RECIR_OFFSET) /* Bit [6] */

typedef enum {
    DRV8316_CSA_GAIN_0_15V_A = 0U,
    DRV8316_CSA_GAIN_0_30V_A = 1U,
    DRV8316_CSA_GAIN_0_60V_A = 2U,
    DRV8316_CSA_GAIN_1_20V_A = 3U
} DRV8316_CSA_Gain_t;

typedef enum {
    DRV8316_ASR_DISABLED = 0U,
    DRV8316_ASR_ENABLED = 1U
} DRV8316_ASR_Enable_t;

typedef enum {
    DRV8316_AAR_DISABLED = 0U,
    DRV8316_AAR_ENABLED = 1U
} DRV8316_AAR_Enable_t;

typedef enum {
    DRV8316_ILIM_RECIRCULATION_BRAKE = 0U,
    DRV8316_ILIM_RECIRCULATION_COAST = 1U
} DRV8316_ILIM_Recirculation_t;

/* ============================================================
 * CTRL_6 — register address 0x08
 * ============================================================ */

#define DRV8316_CTRL6_BUCK_DIS_OFFSET 0U
#define DRV8316_CTRL6_BUCK_SEL_OFFSET 1U
#define DRV8316_CTRL6_BUCK_CL_OFFSET 3U
#define DRV8316_CTRL6_BUCK_PS_DIS_OFFSET 4U

#define DRV8316_CTRL6_BUCK_DIS_MASK                                            \
    (0x01U << DRV8316_CTRL6_BUCK_DIS_OFFSET) /* Bit [0] */

#define DRV8316_CTRL6_BUCK_SEL_MASK                                            \
    (0x03U << DRV8316_CTRL6_BUCK_SEL_OFFSET) /* Bits [2:1] */

#define DRV8316_CTRL6_BUCK_CL_MASK                                             \
    (0x01U << DRV8316_CTRL6_BUCK_CL_OFFSET) /* Bit [3] */

#define DRV8316_CTRL6_BUCK_PS_DIS_MASK                                         \
    (0x01U << DRV8316_CTRL6_BUCK_PS_DIS_OFFSET) /* Bit [4] */

typedef enum {
    DRV8316_BUCK_ENABLED = 0U,
    DRV8316_BUCK_DISABLED = 1U
} DRV8316_BuckDisable_t;

typedef enum {
    DRV8316_BUCK_VOLTAGE_3V3 = 0U,
    DRV8316_BUCK_VOLTAGE_5V0 = 1U,
    DRV8316_BUCK_VOLTAGE_4V0 = 2U,
    DRV8316_BUCK_VOLTAGE_5V7 = 3U
} DRV8316_BuckVoltage_t;

typedef enum {
    DRV8316_BUCK_CURRENT_LIMIT_600MA = 0U,
    DRV8316_BUCK_CURRENT_LIMIT_150MA = 1U
} DRV8316_BuckCurrentLimit_t;

typedef enum {
    DRV8316_BUCK_POWER_SEQUENCE_ENABLED = 0U,
    DRV8316_BUCK_POWER_SEQUENCE_DISABLED = 1U
} DRV8316_BuckPowerSequence_t;

/* ============================================================
 * CTRL_10 — register address 0x0C
 * ============================================================ */

#define DRV8316_CTRL10_DLY_TARGET_OFFSET 0U
#define DRV8316_CTRL10_DLYCMP_EN_OFFSET 4U

#define DRV8316_CTRL10_DLY_TARGET_MASK                                         \
    (0x0FU << DRV8316_CTRL10_DLY_TARGET_OFFSET) /* Bits [3:0] */

#define DRV8316_CTRL10_DLYCMP_EN_MASK                                          \
    (0x01U << DRV8316_CTRL10_DLYCMP_EN_OFFSET) /* Bit [4] */

typedef enum {
    DRV8316_DELAY_TARGET_0_0US = 0x0U,
    DRV8316_DELAY_TARGET_0_4US = 0x1U,
    DRV8316_DELAY_TARGET_0_6US = 0x2U,
    DRV8316_DELAY_TARGET_0_8US = 0x3U,
    DRV8316_DELAY_TARGET_1_0US = 0x4U,
    DRV8316_DELAY_TARGET_1_2US = 0x5U,
    DRV8316_DELAY_TARGET_1_4US = 0x6U,
    DRV8316_DELAY_TARGET_1_6US = 0x7U,
    DRV8316_DELAY_TARGET_1_8US = 0x8U,
    DRV8316_DELAY_TARGET_2_0US = 0x9U,
    DRV8316_DELAY_TARGET_2_2US = 0xAU,
    DRV8316_DELAY_TARGET_2_4US = 0xBU,
    DRV8316_DELAY_TARGET_2_6US = 0xCU,
    DRV8316_DELAY_TARGET_2_8US = 0xDU,
    DRV8316_DELAY_TARGET_3_0US = 0xEU,
    DRV8316_DELAY_TARGET_3_2US = 0xFU
} DRV8316_DelayTarget_t;

typedef enum {
    DRV8316_DELAY_COMPENSATION_DISABLED = 0U,
    DRV8316_DELAY_COMPENSATION_ENABLED = 1U
} DRV8316_DelayCompensation_t;

/* ============================================================
 * CTRL_1 — Register address 0x03
 * ============================================================ */

#define DRV8316_REG_CTRL_1 0x03U

#define DRV8316_CTRL1_REG_LOCK_OFFSET 0U

#define DRV8316_CTRL1_REG_LOCK_MASK                                            \
    (0x07U << DRV8316_CTRL1_REG_LOCK_OFFSET) /* Bits [2:0] */

typedef enum {
    DRV8316_REG_LOCK_UNLOCK = 0x03U, /* Write 011b to unlock */
    DRV8316_REG_LOCK_LOCK = 0x06U    /* Write 110b to lock */
} DRV8316_RegisterLock_t;

/* ============================================================
 * CTRL_2 — Register address 0x04
 * ============================================================ */

#define DRV8316_REG_CTRL_2 0x04U

#define DRV8316_CTRL2_CLR_FLT_OFFSET 0U
#define DRV8316_CTRL2_PWM_MODE_OFFSET 1U
#define DRV8316_CTRL2_SLEW_OFFSET 3U
#define DRV8316_CTRL2_SDO_MODE_OFFSET 5U

#define DRV8316_CTRL2_CLR_FLT_MASK                                             \
    (0x01U << DRV8316_CTRL2_CLR_FLT_OFFSET) /* Bit [0] */

#define DRV8316_CTRL2_PWM_MODE_MASK                                            \
    (0x03U << DRV8316_CTRL2_PWM_MODE_OFFSET) /* Bits [2:1] */

#define DRV8316_CTRL2_SLEW_MASK                                                \
    (0x03U << DRV8316_CTRL2_SLEW_OFFSET) /* Bits [4:3] */

#define DRV8316_CTRL2_SDO_MODE_MASK                                            \
    (0x01U << DRV8316_CTRL2_SDO_MODE_OFFSET) /* Bit [5] */

typedef enum {
    DRV8316_SDO_MODE_OPEN_DRAIN = 0U,
    DRV8316_SDO_MODE_PUSH_PULL = 1U
} DRV8316_SDO_Mode_t;

typedef enum {
    DRV8316_SLEW_RATE_25V_US = 0U,
    DRV8316_SLEW_RATE_50V_US = 1U,
    DRV8316_SLEW_RATE_125V_US = 2U,
    DRV8316_SLEW_RATE_200V_US = 3U
} DRV8316_SlewRate_t;

typedef enum {
    DRV8316_PWM_MODE_6X = 0U,
    DRV8316_PWM_MODE_6X_CL = 1U,
    DRV8316_PWM_MODE_3X = 2U,
    DRV8316_PWM_MODE_3X_CL = 3U
} DRV8316_PWM_Mode_t;

/* ============================================================
 * CTRL_1 — Register address 0x03
 * ============================================================ */

#endif /* DRV8316_FIELDS_H */
