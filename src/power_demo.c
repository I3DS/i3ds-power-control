#include <stdio.h>
#include "power_driver.h"

int main(void)
{
  char userInput = '0';

  printf("Initializing system.\r\n");
  power_initialize("/dev/i2c-2", 0x22, 0x04, 0x8C);
	
  printf("Type 'q' to quit, '1' to switch LEDs on, '0' to switch them off\r\n");
  while(userInput != 'q') 
  {
    scanf("%c", &userInput);
    
    if (userInput == '0')
    {
      printf("Power OFF\r\n");
      power_mask_set(0);
    } else if (userInput >= '1' && userInput <= '9') 
    {
      POWER_ID pid = userInput - '0';
      printf("Powering on %d\r\n", pid);
      power_enable(pid);
    } else {
      printf("Current mask: 0x%04X\r\n", power_mask_read());
    }
  }

  printf("Quitting...\r\n");

  power_deinitialize();

  return 0;
}
