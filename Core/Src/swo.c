
#include "swo.h"
#include "main.h"
#include <stdio.h>

// In main.c, add this ITM send function
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

void to_binary_str(char *buf, uint16_t val) {
    int buf_idx = 0;

    // Optional: Add a nice visual prefix
    buf[buf_idx++] = '0';
    buf[buf_idx++] = 'b';

    // Loop through all 16 bits starting from the Most Significant Bit (Bit 15)
    for (int i = 15; i >= 0; i--) {
        // Add a clean space between the high byte and low byte for readability
        if (i == 7) {
            buf[buf_idx++] = ' ';
        }

        // Check if the specific bit is set, and write the corresponding
        // character
        buf[buf_idx++] = (val & (1 << i)) ? '1' : '0';
    }

    // Always null-terminate the string!
    buf[buf_idx] = '\0';
}

void SWO_Init() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // __HAL_RCC_DBGMCU_CLK_ENABLE();
    DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
    DBGMCU->CR &= ~DBGMCU_CR_TRACE_MODE;
    ITM->LAR = 0xC5ACCE55;
    TPI->SPPR = 0x00000002; // NRZ
    TPI->ACPR = 7;          // 84MHz / 42 = 2MHz SWO
    ITM->TCR |= ITM_TCR_ITMENA_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk;
    ITM->TER = 1UL;

    setvbuf(stdout, NULL, _IONBF, 0); // disable printf buffering
}
