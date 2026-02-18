#ifndef LFO_H
#define LFO_H

#include "sine.hpp"
#include "synth.hpp"
#include <stdint.h>

const float LFO_FREQUENCIES[] = {
    0.9470,  1.0105,  1.0782,  1.1505,  1.2277,  1.3100,  1.3978,  1.4916,
    1.5916,  1.6983,  1.8122,  1.9337,  2.0633,  2.2017,  2.3493,  2.5068,
    2.6749,  2.8542,  3.0456,  3.2498,  3.4677,  3.7002,  3.9483,  4.2131,
    4.4955,  4.7970,  5.1186,  5.4618,  5.8280,  6.2188,  6.6358,  7.0807,
    7.5554,  8.0620,  8.6026,  9.1794,  9.7949,  10.4516, 11.1524, 11.9002,
    12.6981, 13.5495, 14.4579, 15.4273, 16.4617, 17.5655, 18.7433, 20.0000};

const float LFO_AMPLITUDES[]{
    0.,      2.5000,  2.5499,  2.6007,  2.6526,  2.7055,  2.7594,  2.8145,
    2.8706,  2.9279,  2.9863,  3.0458,  3.1066,  3.1685,  3.2317,  3.2962,
    3.3619,  3.4290,  3.4973,  3.5671,  3.6382,  3.7108,  3.7848,  3.8603,
    3.9373,  4.0158,  4.0959,  4.1776,  4.2609,  4.3459,  4.4326,  4.5210,
    4.6111,  4.7031,  4.7969,  4.8926,  4.9901,  5.0897,  5.1912,  5.2947,
    5.4003,  5.5080,  5.6179,  5.7299,  5.8442,  5.9607,  6.0796,  6.2009,
    6.3246,  6.4507,  6.5793,  6.7106,  6.8444,  6.9809,  7.1201,  7.2621,
    7.4070,  7.5547,  7.7054,  7.8591,  8.0158,  8.1757,  8.3387,  8.5050,
    8.6747,  8.8477,  9.0241,  9.2041,  9.3877,  9.5749,  9.7659,  9.9606,
    10.1593, 10.3619, 10.5686, 10.7793, 10.9943, 11.2136, 11.4372, 11.6654,
    11.8980, 12.1353, 12.3773, 12.6242, 12.8760, 13.1328, 13.3947, 13.6618,
    13.9343, 14.2122, 14.4957, 14.7848, 15.0796, 15.3803, 15.6871, 16.};

/*
    LFO can modulate both oscillator frequencies
    and amplitudes as well as LPF cutoff frequency
*/

/// @brief The parameter that the LFO is modulating
typedef enum lfo_target {
  NONE = -1,
  OSC1_FREQ,
  OSC2_FREQ,
  OSC1_AMP,
  OSC2_AMP,
  LPF_CUTOFF
} lfo_target_t;

class LFO {
public:
  float _frequency;
  float _sampling_frequency;
  float _amplitude;
  uint16_t _phase;

  int _frequency_enc;
  int _amplitude_enc;

  /// @brief Default LFO constructor
  LFO() : _frequency(0.), _sampling_frequency(0.), _amplitude(0.), _phase(0) {}

  /// @brief Main LFO constructor
  /// @param frequency the initialization LFO frequency
  /// @param sampling_frequency the sampling frequency, usually the audio
  /// frequency (44100)
  /// @param amplitude initialization LFO amplitude
  /// @param phase initialization LFO phase
  LFO(float frequency, float sampling_frequency, float amplitude,
      uint16_t phase) {
    this->_frequency = frequency;
    this->_sampling_frequency = sampling_frequency;
    this->_amplitude = amplitude;
    this->_phase = phase;
  }

  /// @brief Set the LFO's frequency
  /// @param freq the LFO's frequency
  void set_frequency(float freq) { _frequency = freq; }

  /// @brief Set the LFO's amplitude
  /// @param freq the LFO's amplitude
  void set_amplitude(float amp) { _amplitude = amp; }

  /// @brief Get the next LFO sample
  /// @return the next LFO sample
  float get_sample() {
    _phase += _frequency * 0x10000 / _sampling_frequency;

    return (SINE_LUT[_phase >> 6] - 0x8000) * (_amplitude / 40000.0);
  }
};

#endif // LFO_H