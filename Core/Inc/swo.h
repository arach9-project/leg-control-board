
#include "main.h"

#define DBG_BUF_SIZE 128
// In main.c, add this ITM send function
int _write(int file, char *ptr, int len);

/**
 * @brief Converts a 16-bit word into a formatted binary string.
 * @param buf Must be at least 19 bytes long (16 bits + 1 space + 1 prefix
 * space/char + 1 null terminator)
 * @param val The 16-bit value to convert
 */
void to_binary_str(char *buf, uint16_t val);

void SWO_Init();
