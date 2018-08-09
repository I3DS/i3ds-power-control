#include "power_driver.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

bool _power_initialized = false;
uint8_t _power_address;
uint8_t _power_register;
uint8_t _power_io_register;

int _power_device;

#ifndef DEBUG_PRINT_DATA
#define DEBUG_PRINT_DATA 0
#endif

// Read two bytes from the specified address
uint16_t i2c_read_u16(uint8_t daddr) { 
  char buf[3] = {0};
  int n_read = 0;
  for(uint8_t offset = 0; offset < 2; offset++) {
    buf[0] = daddr + offset;
    if (write(_power_device, buf, 1) != 1) {
      printf("Could not write address\r\n");
      return 0;
    }

    int c_read;
    if ((c_read = read(_power_device, buf + offset + 1, 1)) < 1) {
      printf("Could not read byte %d\r\n", buf[0]);
      return 0;
    }
    n_read += c_read;
  }
  uint16_t data = ((uint16_t)(buf[2]) << 8) + buf[1];
  #if DEBUG_PRINT_DATA
  printf("Got %d bytes of data: 0x%04X\r\n", n_read, data);
  #endif
  return data;
}

// Write two bytes to the specified address
bool i2c_write_u16(uint8_t daddr, uint16_t data) {
  char buf[2] = {0};
  buf[0] = daddr;
  ((uint16_t *)(&buf[1]))[0] = data;


  int bytes1, bytes2;
  if ((bytes1 = write(_power_device, buf, 2)) < 1) {
    printf("Could not write address\r\n");
    return false;
  }

  buf[0]++;
  buf[1] = data >> 8;
  if ((bytes2 = write(_power_device, buf, 2)) < 1) {
    printf("Could not write address\r\n");
    return false;
  }

  #if DEBUG_PRINT_DATA
  printf("Wrote %d bytes\r\n", bytes1 + bytes2);
  #endif

  uint16_t readback = i2c_read_u16(daddr);
  if (readback != data) {
    printf("Could read back i2c data. Expected 0x%04X, got 0x%04X\r\n", data, readback);
    return false;
  }
  return true;
}

// Initialize the power driver
bool power_initialize(const char *filename, uint8_t address, uint8_t reg, uint8_t io_reg) {
  printf("Opening %s, at 0x%02X with reg 0x%02X and ioreg 0x%02X\r\n", filename, address, reg, io_reg);
  _power_address = address;
  _power_register = reg;
  _power_io_register = io_reg;

  if ((_power_device = open(filename, O_RDWR)) < 0) {
    printf("Failed to open the bus.");
    /* ERROR HANDLING; you can check errno to see what went wrong */
    return false;
  }

  if (ioctl(_power_device, I2C_SLAVE, address) < 0) {
    printf("Failed to acquire bus access and/or talk to slave.\n");
    return false;
  }

  // Set all IO-registers for the power module to output.
  if (!i2c_write_u16(io_reg, 0x0000)) {
    printf("WARNING: Could not set IO registers to output.\r\n");
  }
  
  _power_initialized = true;
  return true;
}


// Deinitialize the power driver
void power_deinitialize() {
  _power_initialized = false;
  close(_power_device);
}


// Power control options
bool power_mask_enable(uint16_t power_mask) {
  return power_mask_set(power_mask_read() | power_mask);
}

bool power_mask_disable(uint16_t power_mask) {
  uint16_t current_mask = power_mask_read();
  uint16_t result_mask = current_mask & (~power_mask);
  printf("  Current mask:   %04X", current_mask);
  printf("  Disabling mask: %04X", power_mask);
  printf("  Setting mask:   %04X", result_mask);
  return power_mask_set(result_mask);
}

bool power_mask_set(uint16_t power_mask) {
  #if DEBUG_PRINT_DATA
  printf("Writing mask 0x%04X\r\n", power_mask);
  #endif
  if (!i2c_write_u16(_power_register, power_mask)) {
    printf("Write failed!\r\n");
    return false;
  }
  return true;
}


bool power_enable(POWER_ID power_id) {
  if ( power_id > POWER_NUMBER_OF_POWERS ) {
    printf("Power ID out of range: %d\r\n", power_id);
    return false;
  }
  return power_mask_enable(1 << (power_id - 1));
}

bool power_disable(POWER_ID power_id) {
  if ( power_id > POWER_NUMBER_OF_POWERS ) {
    printf("Power ID out of range: %d\r\n", power_id);
    return false;
  }
  return power_mask_disable(1 << (power_id -1));
}

uint16_t power_mask_read() {
  return i2c_read_u16(_power_register);
}
