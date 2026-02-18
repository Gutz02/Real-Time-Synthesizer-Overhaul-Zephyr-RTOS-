#include "Switch.hpp"
#include "usb.h"

#include <zephyr/drivers/gpio.h>

bool get_button(const struct gpio_dt_spec *button)
{
	return gpio_pin_get_dt(button);
}

int ThreePosSwitch::initialize(const struct gpio_dt_spec *up, const struct gpio_dt_spec *down,
			       sw3_callback callback)
{
	_callback = callback;
	_up = up;
	_down = down;

	// Configure the first GPIO
	if (!gpio_is_ready_dt(_up)) {
		printuln("GPIO up was not ready.");
		return -1;
	}
	gpio_pin_configure_dt(_up, GPIO_INPUT | GPIO_PULL_UP);

	// Configure the second GPIO
	if (!gpio_is_ready_dt(_down)) {
		printuln("GPIO down was not ready.");
		return -1;
	}
	gpio_pin_configure_dt(_down, GPIO_INPUT | GPIO_PULL_UP);

	return 0;
}

void ThreePosSwitch::update()
{
	// Get the new state
	bool up = gpio_pin_get_dt(_up);
	bool dn = gpio_pin_get_dt(_down);
	if (up == dn) {
		_current_state = Neutral;
	} else if (up) {
		_current_state = Up;
	} else {
		_current_state = Down;
	}

	// If the state is different, call the callback
	if (_current_state != _previous && _callback != nullptr) {
		printuln("Switch state changed.");
		_callback(*this);
	}

	// Update the invariant
	_previous = _current_state;
}
