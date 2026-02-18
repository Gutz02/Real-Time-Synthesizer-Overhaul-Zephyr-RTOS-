#ifndef __ROTARY_ENCODER_H__
#define __ROTARY_ENCODER_H__

#include "usb.h"
#include <errno.h>

class RotaryEncoder {
public:
  // Encoder's callback type definition, called when the encoder changes its
  // state
  typedef void (*re_callback)(RotaryEncoder &encoder);

  /// @brief Encoder's constructor
  /// @param id encoder's ID, from 0 to N_ENCODERS
  /// @param pin_0 initialization value of pin 0
  /// @param pin_1 initialization value of pin 0
  /// @param absolute_value absolute initialization value
  /// @param callback the callback that is called when the encoder changes its
  /// state. Default: no callback
  RotaryEncoder(uint8_t id, unsigned int pin_0, unsigned int pin_1,
                int absolute_value = 0, re_callback callback = nullptr)
      : _id{id}, _pin_0{pin_0}, _pin_1{pin_1}, _absolute_value{absolute_value},
        _callback{callback} {}

  /// @brief Default encoder's constructor
  RotaryEncoder()
      : _id{0}, _pin_0{0}, _pin_1{0}, _absolute_value{0}, _callback{nullptr} {}

  /// @brief Encoder initialization function
  /// Call this function during initialization before calling the rest of the
  /// fuctions if you have used the default constuctor
  /// @param id encoder's ID, from 0 to N_ENCODERS
  /// @param pin_0 initialization value of pin 0
  /// @param pin_1 initialization value of pin 1
  /// @param absolute_value absolute initialization value
  /// @param callback the callback that is called when the encoder changes its
  /// state. Default: no callback
  void initialize(uint8_t id, unsigned int pin_0, unsigned int pin_1,
                  re_callback callback = nullptr) {
    _id = id;
    _pin_0 = pin_0;
    _pin_1 = pin_1;
    _callback = callback;
  }

  /// @brief Reset to 0 the encoder's absolute value
  void reset() { _absolute_value = 0; }

  /// @brief Get the encoder's absolute value
  /// @return the encoder's absolute value
  int get_state() { return _absolute_value; }

  /// @brief Get the encoder's absolute value
  /// @param state he encoder's new absolute value
  void set_state(int state) { _absolute_value = state; }

  /// @brief Clamp and set the new absolute value
  /// @param state the new absolute value
  /// @param min minimum clamping value
  /// @param max maximum clamping value
  void set_state_clamped(int state, int min, int max) {
    int tmp = state;

    if (tmp > max) {
      tmp = max;
    } else if (tmp < min) {
      tmp = min;
    }

    _absolute_value = tmp;
  }

  /// @brief Get the encoder's pin 0 value
  /// @return the encoder's pin 0 value
  unsigned int get_pin0() { return _pin_0; }

  /// @brief Get the encoder's pin 1 value
  /// @return the encoder's pin 1 value
  unsigned int get_pin1() { return _pin_1; }

  /// @brief Set the encoder's callback
  /// @param callback the encoder's callback
  void set_callback(re_callback callback) { _callback = callback; }

  /// @brief Set a new value for pin 0
  /// @param pin_0
  /// @return 0 on success, -ERRNO otherwise
  int update_pin_0(int pin_0) { return update(pin_0, _pin_1); }

  /// @brief Set a new value for pin 1
  /// @param pin_1
  /// @return 0 on success, -ERRNO otherwise
  int update_pin_1(int pin_1) { return update(_pin_0, pin_1); }

  /// @brief Update the encoder's state
  /// @param pin_0
  /// @param pin_1
  /// @return 0 on success, -ERRNO otherwise
  int update(unsigned int pin_0, unsigned int pin_1) {
    // Check for invalid state
    if (((pin_0 != _pin_0) && (pin_1 != _pin_1)) || pin_0 > 1 || pin_1 > 1) {
      return -EINVAL;
    } else if ((pin_0 == _pin_0) && (pin_1 == _pin_1)) {
      return 0;
    }

    int past_value = _pin_0 + _pin_1 * 2;
    int new_value = pin_0 + pin_1 * 2;
    _pin_0 = pin_0;
    _pin_1 = pin_1;

    // Update the state machine
    switch (past_value) {
    case 0:
      if (new_value == 1)
        _absolute_value++;
      else if (new_value == 2)
        _absolute_value--;
      break;
    case 1:
      if (new_value == 3)
        _absolute_value++;
      else if (new_value == 0)
        _absolute_value--;
      break;
    case 2:
      if (new_value == 0)
        _absolute_value++;
      else if (new_value == 3)
        _absolute_value--;
      break;
    case 3:
      if (new_value == 2)
        _absolute_value++;
      else if (new_value == 1)
        _absolute_value--;
      break;
    default:
      break;
    }

    if (_callback != nullptr)
      _callback(*this);

    return 0;
  }

public:
  uint8_t _id;
  unsigned int _pin_0;
  unsigned int _pin_1;
  int _absolute_value;
  re_callback _callback;
};

#endif // __ROTARY_ENCODER_H__