#include <math.h>
#include <stdint.h>

#define PI (3.14159265358979323846)

// Filter discrete cut-off frequencies, 96-entry LUT
const float CUTOFF_FREQUENCIES_96[] = {
    0.,       20.000,   20.993,   22.036,   23.131,   24.280,   25.485,
    26.751,   28.080,   29.475,   30.939,   32.476,   34.089,   35.782,
    37.559,   39.425,   41.383,   43.438,   45.596,   47.861,   50.238,
    52.733,   55.352,   58.102,   60.987,   64.017,   67.196,   70.534,
    74.037,   77.715,   81.575,   85.627,   89.880,   94.344,   99.030,
    103.949,  109.112,  114.531,  120.220,  126.191,  132.459,  139.039,
    145.945,  153.194,  160.803,  168.790,  177.173,  185.973,  195.211,
    204.907,  215.084,  225.768,  236.981,  248.752,  261.108,  274.077,
    287.690,  301.979,  316.979,  332.723,  349.249,  366.596,  384.805,
    403.918,  423.980,  445.039,  467.144,  490.347,  514.703,  540.268,
    567.103,  595.270,  624.837,  655.873,  688.450,  722.645,  758.538,
    796.214,  835.762,  877.274,  920.848,  966.586,  1014.596, 1064.991,
    1117.888, 1173.413, 1231.696, 1292.874, 1357.091, 1424.497, 1495.251,
    1569.520, 1647.477, 1729.307, 1815.201, 1905.361, 2000.000,
};

// Filter discrete cut-off frequencies, 48-entry LUT
// const float CUTOFF_FREQUENCIES_48[] = {
//   20.000, 22.059, 24.330, 26.834,
//   29.597, 32.643, 36.004, 39.710,
//   43.798, 48.307, 53.280, 58.765,
//   64.814, 71.486, 78.845, 86.962,
//   95.914, 105.787, 116.678, 128.689,
//   141.936, 156.548, 172.663, 190.438,
//   210.042, 231.665, 255.513, 281.816,
//   310.827, 342.825, 378.117, 417.041,
//   459.973, 507.324, 559.549, 617.151,
//   680.683, 750.755, 828.040, 913.281,
//   1007.297, 1110.992, 1225.361, 1351.504,
//   1490.632, 1644.083, 1813.330, 2000.000,
// };

// Implements second-order Butterworth low-pass filter
class Filter {
public:
  float _cutoff_freq;    // cutoff frequency of the LPF in Hz
  int _cutoff_enc;       // cutoff frequency encoder state
  float _resonance_freq; // TODO: implement
  int _resonance_enc;

private:
  // Frequency at which the LPF is sampled
  float sampling_freq;

  // Windows for the previous inputs and outputs
  float prev_sample[2] = {0, 0}; // x(n-1), x(n-2)
  float prev_output[2] = {0, 0}; // y(n-1), y(n-2)

  // Coefficients for the 2nd order Butterworth
  float a[3] = {0., 0., 0.}; // input sample coefficients
  float b[2] = {0., 0.};     // previous output sample coefficients

  /// @brief Update the filter coefficients from the encoder values
  /// Internal function called by the encoder's callback
  void update_coefficients() {
    // float K1 = sqrtf(2) * _cutoff_freq;
    // float K2 = powf(_cutoff_freq, 2);

    // a[0] = K2 / (1.0 + K1 + K2);
    // a[1] = 2 * a[0];
    // a[2] = a[0];

    // float K3 = a[1] / K2;

    // b[0] = -a[1] + K3;
    // b[1] = 1 - a[1] - K3;

    // float norm_freq = _cutoff_freq / sampling_freq;

    float ita = 1.0 / tanf(PI * _cutoff_freq);
    float q = sqrtf(2);

    a[0] = 1.0 / (1.0 + (q * ita) + (ita * ita));
    a[1] = 2 * a[0];
    a[2] = a[0];

    b[0] = 2.0 * ((ita * ita) - 1.0) * a[0];
    b[1] = -(1.0 - (q * ita) + (ita * ita)) * a[0];
  }

public:
  /// @brief Sound filter constructor
  /// @param sampling_freq TODO
  /// @param cutoff_freq TODO
  Filter(float sampling_freq, float cutoff_freq) {
    this->sampling_freq = sampling_freq;
    this->_cutoff_enc = 0;
    this->_resonance_enc = 0;
    set_cutoff_freq(cutoff_freq);
  }

  // Applies the filter and updates the sampling window
  float filter(float curr_sample) {
    float y = a[0] * curr_sample + a[1] * prev_sample[0] +
              a[2] * prev_sample[1] + b[0] * prev_output[0] +
              b[1] * prev_output[1];

    // update sample window
    prev_sample[1] = prev_sample[0];
    prev_sample[0] = curr_sample;
    prev_output[1] = prev_output[0];
    prev_output[0] = y;

    return y;
  }

  /// @brief Sets the filter's cut-off frequency
  /// @param cutoff the filter's cut-off frequency
  void set_cutoff_freq(float cutoff) {
    // convert from Hz to rad/sec
    // float corrected_cutoff = (1 / (2*sampling_freq)) - cutoff;

    // Normalize the cutoff frequency to the sampling frequency
    _cutoff_freq = cutoff / sampling_freq;
    update_coefficients();
  }

  /// @brief Sets the filter's resonance frequency
  /// @param cutoff the filter's resonance frequency
  void set_resonance_freq(float res) { _resonance_freq = res; }
};