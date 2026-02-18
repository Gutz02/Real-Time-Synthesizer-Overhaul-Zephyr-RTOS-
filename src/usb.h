#include <stdint.h>

extern "C" {

/// @brief USB initialization function.
/// Call this function before printing to the serial port
/// @return 0 on success, -ERRNO otherwise
int initUsb();

/// @brief Spinlock that will wait until the port is open on the PC side
/// Use it to guarantee that print functions are not called before the port is
/// open
void waitForUsb();

/// @brief Returns receiver buffer length
/// @return receiver buffer length
int usbRxBufferLen();

/// @brief Read data from the USB port
/// @param data data pointer
/// @param size size of the data pointer
/// @return number of bytes read
int usbRead(char *data, uint32_t size);

/// @brief Returns transmitter buffer length
/// @return transmitter buffer length
int usbTxBufferLen();

/// @brief Write data to the USB port
/// @param data data pointer
/// @param size number of bytes to write
/// @return number of bytes written
int usbWrite(const uint8_t *data, uint32_t size);

/// @brief basic print function
/// Does not support floating point, does not print a new line
/// @param format C standard string format
/// @param variables to parse into the string
/// @return Number of bytes written
int printu(const char *format, ...);

/// @brief basic println function
/// Does not support floating point, does print a new line
/// @param format C standard string format
/// @param variables to parse into the string
/// @return Number of bytes written
int printuln(const char *format, ...);
}