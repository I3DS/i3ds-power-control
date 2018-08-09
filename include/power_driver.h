#ifndef _POWER_DRIVER_H
#define _POWER_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>

// External triggers
typedef enum POWER_ID
{
  POWER_1 = 1,
  POWER_2,
  POWER_3,
  POWER_4,
  POWER_5,
  POWER_6,
  POWER_7,
  POWER_8,
  POWER_9,
  POWER_10,
  POWER_11,
  POWER_12,
  POWER_13,
  POWER_14,
  POWER_15,
  POWER_16,

  POWER_NUMBER_OF_POWERS
} POWER_ID;

// Initialize the power driver
bool power_initialize(const char *filename, uint8_t address, uint8_t reg, uint8_t io_reg);

// Deinitialize the power driver
void power_deinitialize();


// Power control options
bool power_mask_enable(uint16_t power_mask);
bool power_mask_disable(uint16_t power_mask);
bool power_mask_set(uint16_t power_mask);

bool power_enable(POWER_ID power_id);
bool power_disable(POWER_ID power_id);
uint16_t power_mask_read();

#ifdef __cplusplus
}
#endif

#endif // _POWER_DRIVER_H
