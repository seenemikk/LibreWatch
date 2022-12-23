#ifndef DRIVERS_SENSOR_ICM42605_REGS_
#define DRIVERS_SENSOR_ICM42605_REGS_

#include <zephyr/sys/util.h>

#define REG_DEFINE(bank, reg)   ((((bank) & 0x7) << 8) | ((reg) & 0xff))
#define BANK(bank_reg)          (((bank_reg) >> 8) & 0x7)
#define REG(bank_reg)           ((bank_reg) & 0xff)

// BANK 0
#define DEVICE_CONFIG       REG_DEFINE(0, 0x11)
#define INT_CONFIG          REG_DEFINE(0, 0x14)
#define APEX_DATA0          REG_DEFINE(0, 0x31)
#define APEX_DATA1          REG_DEFINE(0, 0x32)
#define INT_STATUS3         REG_DEFINE(0, 0x38)
#define SIGNAL_PATH_RESET   REG_DEFINE(0, 0x4b)
#define PWR_MGMT0           REG_DEFINE(0, 0x4e)
#define ACCEL_CONFIG0       REG_DEFINE(0, 0x50)
#define APEX_CONFIG0        REG_DEFINE(0, 0x56)
#define INT_CONFIG1         REG_DEFINE(0, 0x64)
#define BANK_SEL            REG_DEFINE(0, 0x76)

// BANK 1

// BANK 2

// BANK 4
#define APEX_CONFIG5        REG_DEFINE(4, 0x44)
#define INT_SOURCE6         REG_DEFINE(4, 0x4d)
#define INT_SOURCE7         REG_DEFINE(4, 0x4e)

#define INT_STATUS3_SLEEP_INT   BIT(1)
#define INT_STATUS3_WAKE_INT    BIT(2)

#endif // DRIVERS_SENSOR_ICM42605_REGS_
