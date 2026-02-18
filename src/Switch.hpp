#ifndef __BUTTONS_H__
#define __BUTTONS_H__

enum ThreeWaySwitchState { Down = -1, Neutral = 0, Up = 1 };

class ThreePosSwitch {
public:
  // Switch's callback type definition, called when the switch changes its state
  typedef void (*sw3_callback)(ThreePosSwitch &sw);

  /// @brief Default constructor
  ThreePosSwitch()
      : _current_state{Neutral}, _previous{Neutral}, _callback{nullptr},
        _up{nullptr}, _down{nullptr} {}

  /// @brief Switch's initialization function
  /// Call this function during initialization before calling the rest of the
  /// fuctions
  /// @param up up pin hardware handler
  /// @param down down pin hardware handler
  /// @param callback the callback that is called when the switch changes its
  /// state. Default: no callback
  /// @return 0 on success, -ERRNO otherwise
  int initialize(const struct gpio_dt_spec *up, const struct gpio_dt_spec *down,
                 sw3_callback callback = nullptr);

  /// @brief Switch update function.
  void update();

  ThreeWaySwitchState _current_state;
  ThreeWaySwitchState _previous;
  sw3_callback _callback;
  const struct gpio_dt_spec *_up;
  const struct gpio_dt_spec *_down;
};

#endif // __BUTTONS_H__