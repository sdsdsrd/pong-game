#include <stm32f4xx.h>
#include "GLCD.h"
#include "I2C.h"
#include "JOY.h"
int gamemain(void);

void BUT_Init (void) {
  /* Enable clock and init GPIO inputs */
  RCC->AHB1ENR |= (1UL << 0) |
                  (1UL << 2) |
                  (1UL << 6) ;

  GPIOA->MODER &= ~(3UL << 2* 0);
  GPIOC->MODER &= ~(3UL << 2*13);
  GPIOG->MODER &= ~(3UL << 2*15);
}

int main(void)
{
   GLCD_Init();
	 I2C_Init();
	 JOY_Init();
	 BUT_Init();
   gamemain();
   while (1)
   {
   }
}
