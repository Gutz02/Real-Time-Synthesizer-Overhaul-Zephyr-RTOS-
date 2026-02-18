#ifndef __SYNTH_H__
#define __SYNTH_H__

#include "Switch.hpp"
#include "audio.h"
#include "filter.hpp"
#include "key.hpp"
#include "lfo.hpp"
#include "peripherals.h"
#include "sine.hpp"
#include <stdint.h>

const float SHIFT_FREQUENCIES[] = {
    0.250, 0.265, 0.281, 0.297, 0.315, 0.334, 0.354, 0.375, 0.397, 0.420,
    0.445, 0.472, 0.500, 0.530, 0.561, 0.595, 0.630, 0.667, 0.707, 0.749,
    0.794, 0.841, 0.891, 0.944, 1.000, 1.059, 1.122, 1.189, 1.260, 1.335,
    1.414, 1.498, 1.587, 1.682, 1.782, 1.888, 2.000, 2.119, 2.245, 2.378,
    2.520, 2.670, 2.828, 2.997, 3.175, 3.364, 3.564, 3.775, 4.000};

const uint16_t DEFAULT_MASTER_VALUE = 25600;

/// @brief Basic oscillator waves
typedef enum wavetype {
  sine = 0,
  triangle = 1,
  square = 2,
  sawtooth = 3
} wavetype_t;

/// @brief Oscillator data structure
typedef struct osc {
  uint16_t volume;
  int volume_enc;
  wavetype_t wave;
  int wave_enc;
  float freq_shift;
  int freq_shift_enc;
  bool enabled;
} osc_t;

/// @brief Oscillators switch callback
void oscillator_selection_switch_callback(ThreePosSwitch &sw);
/// @brief Encoder 0 callback
void lpf_res_osc_freq_encoder_callback(RotaryEncoder &encoder);
/// @brief Encoder 1 callback
void lpf_cutoff_wavetype_encoder_callback(RotaryEncoder &encoder);
/// @brief Encoder 2 callback
void volume_encoder_callback(RotaryEncoder &encoder);

/// @brief LFO target selector switch callback
void lfo_target_switch_callback(ThreePosSwitch &sw);
void effects_configuration_switch_callback(ThreePosSwitch &sw);
void effects_selection_switch_callback(ThreePosSwitch &sw);

/// @brief Encoder 3 callback
void lfo_freq_amp_mod_attack_callback(RotaryEncoder &encoder);
/// @brief Encoder 4 callback
void lfo_amp_amp_mod_sustain_callback(RotaryEncoder &encoder);
/// @brief Encoder 5 callback
void amp_mod_release_callback(RotaryEncoder &encoder);

/// @brief LUT that outputs the LFO's target based on switch positions
/// @param osc_sw switch 0 state
/// @param lfo_target_sw switch 1 state
/// @param lfo_amp_mod_sel_sw switch 2 state
/// @return the LFO target
lfo_target_t get_lfo_target(ThreeWaySwitchState osc_sw,
                            ThreeWaySwitchState lfo_target_sw);

class Synthesizer {
public:
  int _master_volume_enc;
  osc_t _osc1;
  osc_t _osc2;
  Filter _lpf;
  LFO _lfos[5];
  lfo_target_t _lfo_target;

  /// @brief Synthesizer default constructor
  Synthesizer()
      : _master_volume_enc{DEFAULT_MASTER_VALUE},
        _osc1{DEFAULT_MASTER_VALUE, 0, square, 0, 1., 0, false},
        _osc2{DEFAULT_MASTER_VALUE, 0, square, 0, 1., 0, false},
        _lpf{SAMPLE_FREQUENCY, 0.} {
    // Initialize all LFOs
    for (int i = 0; i < 5; i++) {
      _lfos[i] = LFO(0., SAMPLE_FREQUENCY, 0., 0);
    }
    _lfo_target = NONE;
  }

  /// @brief Synthesizer initialization function
  /// Call this function during initialization before calling the rest of the
  /// fuctions
  void initialize();

  /// @brief Compute the next oscillator output sample
  /// @param osc the oscillator you want to generate sample for
  /// @param phase the current phase to generate sample with
  /// @return the next oscillator sample
  int get_osc_sample(osc_t osc, uint16_t phase);

  /// @brief Compute next sound value for a specific key
  /// @param key the key you want to generate sound with
  /// @return the next sound value
  float get_sound_sample(Key &key);

  /// @brief Populate the audio buffer with sound until the deadline
  /// @param block the audio buffer
  /// @param deadline timepoint to stop before overload
  /// @return true if generation stopped early (overload)
  bool makesynth(uint8_t *block, k_timepoint_t deadline);
};

extern Synthesizer synth;

#endif // __SYNTH_H__
