#ifndef LEDS_H
#define LEDS_H

#define N_LEDS (9)
/**
 * On-board LEDs that can be probed on the STM board pins
 * Section 6.3:
 * https://www.st.com/resource/en/user_manual/dm00039084-discovery-kit-with-stm32f407vg-mcu-stmicroelectronics.pdf
 */
// green LED
extern const struct gpio_dt_spec debug_led0;
// orange LED
extern const struct gpio_dt_spec debug_led1;
// red LED
extern const struct gpio_dt_spec debug_led2;
// blue LED
extern const struct gpio_dt_spec debug_led3;

// Status LEDs on the peripherals board
extern const struct gpio_dt_spec status_led0;
extern const struct gpio_dt_spec status_led1;
extern const struct gpio_dt_spec status_led2;
extern const struct gpio_dt_spec status_led3;
extern const struct gpio_dt_spec status_led4;

/// @brief LEDs initialization function
/// Call it before using the LEDs
/// @return 0 on success, -ERRNO otherwise
int init_leds();

/// @brief Turn on a specific LED
/// @param led to turn on
/// @return 0 on success, -ERRNO otherwise
int set_led(const struct gpio_dt_spec *led);

/// @brief Turn off a specific LED
/// @param led to turn on
/// @return 0 on success, -ERRNO otherwise
int reset_led(const struct gpio_dt_spec *led);

#endif // LEDS_H