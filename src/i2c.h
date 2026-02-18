#include <stdint.h>

/// @brief Read a byte from a device, syntactic sugar
/// @param devaddr the peripheral device's address
/// @param regaddr the device's register address
/// @param regval the address where this function will write the read value
/// @return 0 on success, -ERRNO otherwise
int read(uint8_t devaddr, uint8_t regaddr, uint8_t *regval);

/// @brief Write a byte from a device, syntactic sugar
/// @param devaddr the peripheral device's address
/// @param regaddr the device's register address
/// @param regval the value to write to the device
/// @return 0 on success, -ERRNO otherwise
int write(uint8_t devaddr, uint8_t regaddr, uint8_t regval);