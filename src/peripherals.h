#ifndef __PERIPHERALS_H__
#define __PERIPHERALS_H__

#include "RotaryEncoder.hpp"
#include "Switch.hpp"
#include "stdint.h"

const unsigned int N_ENCODERS = 6;
const unsigned int N_SWITCHES = 4;

/// @brief Peripheral encoders
extern RotaryEncoder encoders[N_ENCODERS];

// Multiplexing encoder for LPF resonance frequency
// and encoder for oscillator frequency
const uint8_t LPF_RES_ENC = 0;
const uint8_t OSC_FREQ_ENC = 0;

// Multiplexing encoder for LPF cutoff frequency
// and encoder for oscillator wave selection
const uint8_t LPF_CUTOFF_ENC = 1;
const uint8_t OSC_WAVE_ENC = 1;

const uint8_t OSC_VOLUME_ENC = 2;

// Multiplexing encoder for LFO frequency and
// encoder for amplitude modulator attack
const uint8_t LFO_FREQ_ENC = 3;
const uint8_t AMP_MOD_ATT_ENC = 3;

// Multiplexing encoder for LFO amplitude and
// encoder for amplitude modulator sustain
const uint8_t LFO_AMP_ENC = 4;
const uint8_t AMP_MOD_SUS_ENC = 4;

// Encoder for configuring amplitude modulator release
const uint8_t AMP_REL_ENC = 5;

/// @brief Peripheral switches
extern ThreePosSwitch switches[N_SWITCHES];

/// @brief Low Pass Filter/Oscillator selection switch
const uint8_t OSC_SEL_SW = 0;
/// @brief Switch that selects between LFOs, amplitude modulation and effects
const uint8_t EFFECTS_SEL_SW = 1;
/// @brief Switch that selects where the effect is applied
const uint8_t EFFECTS_TARGET_SW = 2;
/// @brief Switch that selects internal effects parameters. For instance, for
/// the LFO, the type of oscillator that you want
const uint8_t EFFECTS_CONF_SW = 3;

/// @brief Peripherals initialization function
/// Call this function during initialization before calling the rest of the
/// fuctions
/// @return 0 on success, -ERRNO otherwise
int init_peripherals();

/// @brief Call this function to update the encoders and switches
/// @return 0 on success, -ERRNO otherwise
int peripherals_update();

/// @brief Read the port expander's port 0
/// @return the port expander's port 0
int8_t read_port0();

/// @brief Read the port expander's port 1
/// @return the port expander's port 1
int8_t read_port1();

#endif