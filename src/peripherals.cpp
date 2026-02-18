#include "peripherals.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include "usb.h"

// Port expanders registers addresses:
static const uint8_t INPUT_PORT_0 = 0x00;
static const uint8_t INPUT_PORT_1 = 0x01;
static const uint8_t OUTPUT_PORT_0 = 0x02;
static const uint8_t OUTPUT_PORT_1 = 0x03;
static const uint8_t POLARITY_INVERSION_PORT_0 = 0x04;
static const uint8_t POLARITY_INVERSION_PORT_1 = 0x05;
static const uint8_t CONFIG_PORT_0 = 0x06;
static const uint8_t CONFIG_PORT_1 = 0x07;

// Usefull port addresses
static const uint8_t INPUT_PORTS_ADDR[] = {INPUT_PORT_0, INPUT_PORT_1};
static const uint8_t CONFIG_PORTS_ADDR[] = {CONFIG_PORT_0, CONFIG_PORT_1};

// I2C address
static const uint8_t PORT_EXPANDER_ADDR0 = 0b0100000;

// Rotary encoders
RotaryEncoder encoders[N_ENCODERS];

// GPIOs
const struct gpio_dt_spec sw_osc_dn =
    GPIO_DT_SPEC_GET(DT_ALIAS(switch0), gpios);
const struct gpio_dt_spec sw_osc_up =
    GPIO_DT_SPEC_GET(DT_ALIAS(switch1), gpios);
const struct gpio_dt_spec sw1_dn = GPIO_DT_SPEC_GET(DT_ALIAS(switch4), gpios);
const struct gpio_dt_spec sw1_up = GPIO_DT_SPEC_GET(DT_ALIAS(switch5), gpios);
const struct gpio_dt_spec sw2_dn = GPIO_DT_SPEC_GET(DT_ALIAS(switch6), gpios);
const struct gpio_dt_spec sw2_up = GPIO_DT_SPEC_GET(DT_ALIAS(switch7), gpios);
const struct gpio_dt_spec sw3_dn = GPIO_DT_SPEC_GET(DT_ALIAS(switch2), gpios);
const struct gpio_dt_spec sw3_up = GPIO_DT_SPEC_GET(DT_ALIAS(switch3), gpios);

// Switches
ThreePosSwitch switches[N_SWITCHES];

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static uint8_t ports[2];

// Hardcoded port and bit assignment function
// There is no other way
int get_port_and_index(unsigned int encoder_id, unsigned int pin,
                       unsigned int &port, unsigned int &index) {
  if (encoder_id >= N_ENCODERS || pin > 1) {
    return -EINVAL;
  }

  switch (encoder_id) {
  case 0:
    port = 0;
    index = 0 + pin;
    break;
  case 1:
    port = 0;
    index = 2 + pin;
    break;
  case 2:
    port = 0;
    index = 4 + pin;
    break;
  case 3:
    port = 0;
    index = 6 + pin;
    break;
  case 4:
    port = 1;
    index = 0 + pin;
    break;
  case 5:
    port = 1;
    index = 2 + pin;
    break;
  default:
    break;
  }
  return 0;
}

int init_peripherals() {
  // Check the I2c port
  if (!device_is_ready(i2c_dev)) {
    printuln("I2C device not ready");
    return 1;
  }

  // Get the initial state of the device
  int ret = 0;
  for (int j = 0; j < 2; j++) {
    ret = i2c_write_read(i2c_dev, PORT_EXPANDER_ADDR0, &INPUT_PORTS_ADDR[j], 1,
                         &ports[j], 1);
    if (ret != 0) {
      printuln("Failed reading port expander.");
      return ret;
    }
  }

  // Instantiate the rotary encoders
  for (int i = 0; i < N_ENCODERS; i++) {
    // Get the encoder's pins
    unsigned int port;
    unsigned int index0;
    unsigned int index1;
    get_port_and_index(i, 0, port, index0);
    get_port_and_index(i, 1, port, index1);
    uint8_t pin0 = (ports[port] >> index0) & 0x01;
    uint8_t pin1 = (ports[port] >> index1) & 0x01;

    // Initialize the encoder
    encoders[i].initialize(i, pin0, pin1);
  }

  // Initialize the switches
  switches[OSC_SEL_SW].initialize(&sw_osc_up, &sw_osc_dn);
  switches[EFFECTS_SEL_SW].initialize(&sw1_up, &sw1_dn);
  switches[EFFECTS_TARGET_SW].initialize(&sw2_up, &sw2_dn);
  switches[EFFECTS_CONF_SW].initialize(&sw3_up, &sw3_dn);

  printuln("Peripherals initialization completed!");

  return 0;
}

int peripherals_update() {
  // Get the new ports states
  int ret = 0;
  for (int j = 0; j < 2; j++) {
    ret = i2c_write_read(i2c_dev, PORT_EXPANDER_ADDR0, &INPUT_PORTS_ADDR[j], 1,
                         &ports[j], 1);
    if (ret != 0) {
      printuln("Failed reading port expander.");
      return ret;
    }
  }

  // Update the switches
  for (int i = 0; i < N_SWITCHES; i++) {
    switches[i].update();
  }

  // Update the rotary encoders
  for (int i = 0; i < N_ENCODERS; i++) {
    // Get the encoder's pins
    unsigned int port;
    unsigned int index0;
    unsigned int index1;
    get_port_and_index(i, 0, port, index0);
    get_port_and_index(i, 1, port, index1);
    uint8_t pin0 = (ports[port] >> index0) & 0x01;
    uint8_t pin1 = (ports[port] >> index1) & 0x01;

    // Initialize the encoder
    encoders[i].update(pin0, pin1);
  }

  return 0;
}