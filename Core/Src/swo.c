#include "swo.h"
#include "main.h"

#include <stdio.h>

#define SWO_BAUDRATE 2000000UL

int _write(int file, char* ptr, int len) {
  (void)file;

  for (int i = 0; i < len; i++) {
    ITM_SendChar(ptr[i]);
  }

  return len;
}

void SWO_Init(void) {
  SystemCoreClockUpdate();

  // Enable tracing
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  // Enable trace output pin
  DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
  DBGMCU->CR &= ~DBGMCU_CR_TRACE_MODE;

  // Unlock ITM
  ITM->LAR = 0xC5ACCE55UL;

  // Configure SWO asynchronous NRZ mode
  TPI->SPPR = 2U;

  // Calculate SWO prescaler from current system clock
  TPI->ACPR = (SystemCoreClock / SWO_BAUDRATE) - 1U;

  // Enable ITM and stimulus port 0
  ITM->TCR = ITM_TCR_ITMENA_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk;

  ITM->TER = 1UL;
  ITM->TCR &= ~ITM_TCR_DWTENA_Msk;
  DWT->CTRL &= ~DWT_CTRL_EXCTRCENA_Msk;

  // Disable stdout buffering
  setvbuf(stdout, NULL, _IONBF, 0);
}
