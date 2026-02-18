#include <stdint.h>
#include <zephyr/drivers/i2c.h>

int read(uint8_t devaddr, uint8_t regaddr, uint8_t *regval) {
  int ret;

  const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

  if (!device_is_ready(dev)) {
    printk("Device not ready");
    return -ENODEV;
  }

  ret = i2c_write_read(dev, devaddr, &regaddr, 1, regval, 1);
  if (ret) {
    printk("Call `i2c_write_read` failed: %d", ret);
    return ret;
  }

  return 0;
}

static int write(uint8_t devaddr, uint8_t regaddr, uint8_t regval) {
  int ret;

  const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

  if (!device_is_ready(dev)) {
    printk("Device not ready");
    return -ENODEV;
  }

  uint8_t buf[2] = {regaddr, regval};

  ret = i2c_write(dev, buf, 2, devaddr);
  if (ret) {
    printk("Call `i2c_write` failed: %d", ret);
    return ret;
  }

  return 0;
}