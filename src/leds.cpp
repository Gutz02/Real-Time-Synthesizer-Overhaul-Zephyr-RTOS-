#include "leds.h"
#include "usb.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

const struct gpio_dt_spec debug_led0 =
    GPIO_DT_SPEC_GET(DT_ALIAS(debug_led0), gpios);
const struct gpio_dt_spec debug_led1 =
    GPIO_DT_SPEC_GET(DT_ALIAS(debug_led1), gpios);
const struct gpio_dt_spec debug_led2 =
    GPIO_DT_SPEC_GET(DT_ALIAS(debug_led2), gpios);
const struct gpio_dt_spec debug_led3 =
    GPIO_DT_SPEC_GET(DT_ALIAS(debug_led3), gpios);

const struct gpio_dt_spec status_led0 =
    GPIO_DT_SPEC_GET(DT_ALIAS(status_led0), gpios);
const struct gpio_dt_spec status_led1 =
    GPIO_DT_SPEC_GET(DT_ALIAS(status_led1), gpios);
const struct gpio_dt_spec status_led2 =
    GPIO_DT_SPEC_GET(DT_ALIAS(status_led2), gpios);
const struct gpio_dt_spec status_led3 =
    GPIO_DT_SPEC_GET(DT_ALIAS(status_led3), gpios);
const struct gpio_dt_spec status_led4 =
    GPIO_DT_SPEC_GET(DT_ALIAS(status_led4), gpios);

static const struct gpio_dt_spec *leds[] = {
    &debug_led0,  &debug_led1,  &debug_led2,  &debug_led3, &status_led0,
    &status_led1, &status_led2, &status_led3, &status_led4};

int init_leds() {
  for (int i = 0; i < N_LEDS; i++) {
    const struct gpio_dt_spec *led = leds[i];
    if (!gpio_is_ready_dt(led)) {
      printk("GPIO led was not ready.");
      return -1;
    }

    gpio_pin_configure_dt(led, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(led, false);
  }

  for (int i = 0; i < N_LEDS; i++) {
    reset_led(leds[i]);
  }
  printuln("LED initialization finished!");
  return 0;
}

int set_led(const struct gpio_dt_spec *led) {
  gpio_pin_set_dt(led, true);
  return 0;
}

int reset_led(const struct gpio_dt_spec *led) {
  gpio_pin_set_dt(led, false);
  return 0;
}