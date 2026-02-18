#ifndef __KEY_H_H_
#define __KEY_H_H_

#include <stdint.h>
#include <zephyr/kernel.h>

typedef enum {
  A3,
  Bb3,
  B3,
  C3,
  Ch3,
  D3,
  Dh3,
  E3,
  F3,
  Fh3,
  G3,
  Ab3,
  A4,
  Bb4,
  B4,
  C4,
  Ch4,
  D4,
  Dh4,
  E4,
} key_t;

/// @brief The state of a key
typedef enum { IDLE, PRESSED, RELEASED } key_state_t;

/// @brief Maximum number of keys. Space allocated at compile time.
const uint8_t MAX_KEYS = 4;

class Key {
public:
  /// @brief Key constructor
  Key()
      : state{IDLE}, key{A3}, hold_time{sys_timepoint_calc(K_FOREVER)},
        release_time{sys_timepoint_calc(K_FOREVER)}, elapsed_hold{0.0},
        elapsed_release{0.0}, phase1{0}, phase2{0} {}

  /// @brief Generate key_t struct from keyboard input
  /// @param c the keyboard input
  /// @return keyboard key
  static key_t char_to_key(char c);

  /// @brief Get the frequency of this specific key
  /// @return this key's frequency
  float get_freq();

  key_state_t state;
  key_t key;
  uint16_t phase1;
  uint16_t phase2;
  k_timepoint_t hold_time;
  k_timepoint_t release_time;
  float elapsed_hold;
  float elapsed_release;
};

extern Key keys[MAX_KEYS];

struct keys_buffer {
  Key keys[MAX_KEYS];
};

#endif // __KEY_H__
