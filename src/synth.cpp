#include "synth.hpp"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/time_units.h>

#include "audio.h"
#include "leds.h"
#include "peripherals.h"
#include "sine.hpp"

void Synthesizer::initialize()
{
	// Initial values for oscillator/LPF
	// to avoid setting encoders to uninitialized values
	synth._osc1.wave_enc = 20;
	synth._osc1.volume_enc = 40;
	synth._osc1.freq_shift_enc = 24;

	synth._osc2.wave_enc = 20;
	synth._osc2.volume_enc = 40;
	synth._osc2.freq_shift_enc = 24;

	synth._lpf._cutoff_enc = 0;
	synth._lpf._resonance_enc = 0;
	synth._master_volume_enc = 40;

	// Done here since the OSC sw's "previous" value is Neutral
	encoders[LPF_CUTOFF_ENC].set_state(synth._lpf._cutoff_enc);
	encoders[OSC_VOLUME_ENC].set_state(synth._master_volume_enc);
	encoders[LPF_RES_ENC].set_state(synth._lpf._resonance_enc); // no frequency shift initially

	// Define the encoders callbacks
	encoders[OSC_WAVE_ENC].set_callback(lpf_cutoff_wavetype_encoder_callback);
	encoders[OSC_VOLUME_ENC].set_callback(volume_encoder_callback);
	encoders[OSC_FREQ_ENC].set_callback(lpf_res_osc_freq_encoder_callback);

	encoders[LFO_FREQ_ENC].set_callback(lfo_freq_amp_mod_attack_callback);
	encoders[LFO_AMP_ENC].set_callback(lfo_amp_amp_mod_sustain_callback);
	encoders[AMP_REL_ENC].set_callback(amp_mod_release_callback);

	// Define the switches callbacks and call them once
	switches[OSC_SEL_SW]._callback = oscillator_selection_switch_callback;
	switches[OSC_SEL_SW].update();
	switches[EFFECTS_TARGET_SW]._callback = lfo_target_switch_callback;
	switches[EFFECTS_TARGET_SW].update();
	switches[EFFECTS_CONF_SW]._callback = effects_configuration_switch_callback;
	switches[EFFECTS_CONF_SW].update();
	switches[EFFECTS_SEL_SW]._callback = effects_selection_switch_callback;
	switches[EFFECTS_SEL_SW].update();

	// Set initial values of oscillator encoders in preparation for callback
	switch (switches[OSC_SEL_SW]._previous) {
	case Up:
		encoders[OSC_WAVE_ENC].set_state(synth._osc1.wave_enc);
		encoders[OSC_VOLUME_ENC].set_state(synth._osc1.volume_enc);
		encoders[OSC_FREQ_ENC].set_state(
			synth._osc1.freq_shift_enc); // no frequency shift initially
		break;
	case Down:
		encoders[OSC_WAVE_ENC].set_state(synth._osc2.wave_enc);
		encoders[OSC_VOLUME_ENC].set_state(synth._osc2.volume_enc);
		encoders[OSC_FREQ_ENC].set_state(
			synth._osc2.freq_shift_enc); // no frequency shift initially
		break;
	case Neutral:
		encoders[LPF_CUTOFF_ENC].set_state(synth._lpf._cutoff_enc);
		encoders[OSC_VOLUME_ENC].set_state(synth._master_volume_enc);
		encoders[LPF_RES_ENC].set_state(synth._lpf._resonance_enc);
		break;
	}

	oscillator_selection_switch_callback(switches[OSC_SEL_SW]);
	lfo_target_switch_callback(switches[EFFECTS_TARGET_SW]);
	effects_configuration_switch_callback(switches[EFFECTS_CONF_SW]);
	effects_selection_switch_callback(switches[EFFECTS_SEL_SW]);

	printuln("Synthesizer initialization finished!");
}

/**
 * Oscillator and LPF callbacks
 */
void oscillator_selection_switch_callback(ThreePosSwitch &sw)
{
	// Save the previous state
	switch (sw._previous) {
	case Neutral:
		synth._master_volume_enc = encoders[OSC_VOLUME_ENC].get_state();
		synth._lpf._cutoff_freq = encoders[LPF_CUTOFF_ENC].get_state();
		synth._lpf._resonance_freq = encoders[LPF_RES_ENC].get_state();
		break;
	case Up:
		synth._osc1.volume_enc = encoders[OSC_VOLUME_ENC].get_state();
		synth._osc1.wave_enc = encoders[OSC_WAVE_ENC].get_state();
		synth._osc1.freq_shift_enc = encoders[OSC_FREQ_ENC].get_state();
		break;
	case Down:
		synth._osc2.volume_enc = encoders[OSC_VOLUME_ENC].get_state();
		synth._osc2.wave_enc = encoders[OSC_WAVE_ENC].get_state();
		synth._osc2.freq_shift_enc = encoders[OSC_FREQ_ENC].get_state();
		break;
	}

	// Load the new state
	switch (sw._current_state) {
	case Neutral:
		encoders[OSC_VOLUME_ENC].set_state(synth._master_volume_enc);
		encoders[LPF_CUTOFF_ENC].set_state(synth._lpf._cutoff_freq);
		encoders[LPF_RES_ENC].set_state(synth._lpf._resonance_freq);
		break;
	case Up:
		printuln("SW UP!");
		synth._osc1.enabled = true;
		encoders[OSC_VOLUME_ENC].set_state(synth._osc1.volume_enc);
		encoders[OSC_WAVE_ENC].set_state(synth._osc1.wave_enc);
		encoders[OSC_FREQ_ENC].set_state(synth._osc1.freq_shift_enc);
		break;
	case Down:
		printuln("SW DOWN!");
		synth._osc2.enabled = true;
		encoders[OSC_VOLUME_ENC].set_state(synth._osc2.volume_enc);
		encoders[OSC_WAVE_ENC].set_state(synth._osc2.wave_enc);
		encoders[OSC_FREQ_ENC].set_state(synth._osc2.freq_shift_enc);
		break;
	}

	// May need to also update the LFO target based on new value of this switch
	ThreeWaySwitchState lfo_target_sw = switches[EFFECTS_TARGET_SW]._current_state;
	ThreeWaySwitchState lfo_amp_mod_sel_sw = switches[EFFECTS_SEL_SW]._current_state;

	// Determine old and new values for the LFO target
	lfo_target_t prev_lfo_target = synth._lfo_target;
	lfo_target_t new_lfo_target = get_lfo_target(sw._current_state, lfo_target_sw);

	// If there was a change in target, something either needs to be saved,
	// loaded, or both
	if (new_lfo_target != prev_lfo_target) {
		// In any case, point to the new LFO target
		synth._lfo_target = new_lfo_target;
		printuln("[OSC Select Switch] LFO Target Changed - Old: %d, New: %d",
			 prev_lfo_target, new_lfo_target);

		// There was a previously valid LFO target, so save the state
		if (prev_lfo_target != NONE) {
			// Save off previous encoder values
			synth._lfos[prev_lfo_target]._frequency_enc =
				encoders[LFO_FREQ_ENC].get_state();
			synth._lfos[prev_lfo_target]._amplitude_enc =
				encoders[LFO_AMP_ENC].get_state();
		}

		// There is a new valid LFO target, so load the state
		if (new_lfo_target != NONE) {
			// Load new encoder values
			encoders[LFO_FREQ_ENC].set_state(
				synth._lfos[new_lfo_target]._frequency_enc);
			encoders[LFO_AMP_ENC].set_state(synth._lfos[new_lfo_target]._amplitude_enc);

			// Call the LFO encoders' callbacks
			encoders[LFO_FREQ_ENC]._callback(encoders[LFO_FREQ_ENC]);
			encoders[LFO_AMP_ENC]._callback(encoders[LFO_AMP_ENC]);
		}
	}

	// Call the encoder's callbacks
	encoders[OSC_VOLUME_ENC]._callback(encoders[OSC_VOLUME_ENC]);
	encoders[OSC_WAVE_ENC]._callback(encoders[OSC_WAVE_ENC]);
	encoders[OSC_FREQ_ENC]._callback(encoders[OSC_FREQ_ENC]);
}

void lpf_res_osc_freq_encoder_callback(RotaryEncoder &encoder)
{
	int state = encoder.get_state();

	switch (switches[0]._current_state) {
	case Up: // configure OSC 1 frequency
		// Clamp encoder value
		encoder.set_state_clamped(state, 0, 47);
		state = encoder.get_state();

		synth._osc1.freq_shift = SHIFT_FREQUENCIES[state];
		printuln("[Frequency Shifter] OSC1: %f Hz", SHIFT_FREQUENCIES[state]);
		break;
	case Down: // configure OSC 2 frequency
		// Clamp encoder value
		encoder.set_state_clamped(state, 0, 47);
		state = encoder.get_state();

		synth._osc2.freq_shift = SHIFT_FREQUENCIES[state];
		printuln("[Frequency Shifter] OSC2: %f Hz", SHIFT_FREQUENCIES[state]);
		break;
	case Neutral: // configure LPF resonance frequency
		printuln("[LPF Resonance Frequency] callback not implemented");
		synth._lpf.set_resonance_freq(state);
		break;
	}
}

void lpf_cutoff_wavetype_encoder_callback(RotaryEncoder &encoder)
{
	int state = encoder.get_state();
	switch (switches[0]._current_state) {
	case Up:
		synth._osc1.wave = static_cast<wavetype_t>((state >> 3) & 0x03);
		printuln("[Waveform Select] OSC1: %d", synth._osc1.wave);
		break;
	case Down:
		synth._osc2.wave = static_cast<wavetype_t>((state >> 3) & 0x03);
		printuln("[Waveform Select] OSC2: %d", synth._osc2.wave);
		break;
	case Neutral:
		// encoder.set_state_clamped(state, 0, 96);
		// state = encoder.get_state();

		// synth._lpf.set_cutoff_freq(CUTOFF_FREQUENCIES_96[state]);
		printuln("[LPF Cutoff Frequency] callback not implemented");

		break;
	}
}

void volume_encoder_callback(RotaryEncoder &encoder)
{
	int state = encoder.get_state();

	encoder.set_state_clamped(state, 0, 50);

	state = encoder.get_state();
	uint16_t volume = state * state * 16;
	uint8_t master_volume = 27 + state * 2;

	// Update the volume
	switch (switches[0]._current_state) {
	case Up:
		synth._osc1.volume = volume;
		if (volume == 0) {
			synth._osc1.enabled = false;
		} else {
			synth._osc1.enabled = true;
		}
		printuln("[Volume Encoder]: OSC1: %d", volume);
		break;
	case Down:
		synth._osc2.volume = volume;
		if (volume == 0) {
			synth._osc2.enabled = false;
		} else {
			synth._osc2.enabled = true;
		}
		printuln("[Volume Encoder]: OSC2: %d", volume);
		break;
	case Neutral:
		setVolume(master_volume);
		printuln("[Volume Encoder]: MASTER: %d", master_volume);
		break;
	default:
		break;
	}
}

lfo_target_t get_lfo_target(ThreeWaySwitchState osc_sw, ThreeWaySwitchState lfo_target_sw)
{

	switch (osc_sw) {
	case Up:
		if (lfo_target_sw == Up) {
			return OSC1_FREQ;
		} else if (lfo_target_sw == Down) {
			return OSC1_AMP;
		} else {
			return NONE;
		}
	case Down:
		if (lfo_target_sw == Up) {
			return OSC2_FREQ;
		} else if (lfo_target_sw == Down) {
			return OSC2_AMP;
		} else {
			return NONE;
		}
	case Neutral:
		if (lfo_target_sw == Neutral) {
			return LPF_CUTOFF;
		}
	}
	return NONE;
}

/**
 * LFO and Amplitude Modulator target configuration callbacks
 */
void lfo_target_switch_callback(ThreePosSwitch &sw)
{
	// Obtain previous and new LFO targets
	lfo_target_t prev_lfo_target = synth._lfo_target;
	lfo_target_t new_lfo_target =
		get_lfo_target(switches[OSC_SEL_SW]._current_state, sw._current_state);

	// Save encoders values if they were linked to LFO
	if (prev_lfo_target != NONE) {
		synth._lfos[prev_lfo_target]._frequency_enc = encoders[LFO_FREQ_ENC].get_state();
		synth._lfos[prev_lfo_target]._amplitude_enc = encoders[LFO_AMP_ENC].get_state();
	}

	// Load new encoder values if a valid LFO target has been selected by switches
	if (new_lfo_target != NONE) {
		encoders[LFO_FREQ_ENC].set_state(synth._lfos[new_lfo_target]._frequency_enc);
		encoders[LFO_AMP_ENC].set_state(synth._lfos[new_lfo_target]._amplitude_enc);
	}

	// Unconditionally update LFO target to ensure setting to NONE
	synth._lfo_target = new_lfo_target;
	printuln("[LFO Target Switch] LFO Target Changed - Old: %d, New: %d", prev_lfo_target,
		 new_lfo_target);
}

void effects_configuration_switch_callback(ThreePosSwitch &sw)
{
	printuln("Effects configuration switch not implemented yet.");
}

void effects_selection_switch_callback(ThreePosSwitch &sw)
{
	// Load the new state
	switch (sw._current_state) {
	case Up:
		printuln("[SW3] Configuring LFO");
		break;
	case Down:
		printuln("[SW3] Configuring amplitude modulator (not implemented)");
		break;
	case Neutral:
		printuln("[SW3] Configuring special effects (not implemented)");
		break;
	}

	// May need to also update the LFO target based on new value of this switch
	ThreeWaySwitchState lfo_target_sw = switches[EFFECTS_TARGET_SW]._current_state;
	ThreeWaySwitchState osc_sel_sw = switches[OSC_SEL_SW]._current_state;

	// Determine old and new values for the LFO target
	lfo_target_t prev_lfo_target = synth._lfo_target;
	lfo_target_t new_lfo_target = get_lfo_target(osc_sel_sw, lfo_target_sw);

	// If there was a change in target, something either needs to be saved,
	// loaded, or both
	if (new_lfo_target != prev_lfo_target) {
		// In any case, point to the new LFO target
		synth._lfo_target = new_lfo_target;
		printuln("[SW3] LFO Target Changed - Old: %d, New: %d", prev_lfo_target,
			 new_lfo_target);
		// There was a previously valid LFO target, so save the state
		if (prev_lfo_target != NONE) {
			// Save off previous encoder values
			synth._lfos[prev_lfo_target]._frequency_enc =
				encoders[LFO_FREQ_ENC].get_state();
			synth._lfos[prev_lfo_target]._amplitude_enc =
				encoders[AMP_REL_ENC].get_state(); //BUG. CHANGED FROM LFO FREQ ENC
		}

		// There is a new valid LFO target, so load the state
		if (new_lfo_target != NONE) {
			// Load new encoder values
			encoders[LFO_FREQ_ENC].set_state(
				synth._lfos[new_lfo_target]._frequency_enc);
			encoders[LFO_AMP_ENC].set_state(synth._lfos[new_lfo_target]._amplitude_enc);

			// Call the LFO encoders' callbacks
			encoders[LFO_FREQ_ENC]._callback(encoders[LFO_FREQ_ENC]);
			encoders[LFO_AMP_ENC]._callback(encoders[LFO_AMP_ENC]);
		}
	}
}

/**
 * LFO and Amplitude Modulator parameter callbacks
 */
void lfo_freq_amp_mod_attack_callback(RotaryEncoder &encoder)
{
	int state = encoder.get_state();

	// Determine whether LFO frequency or amplitude modulator attack
	// needs to be modified
	switch (switches[EFFECTS_SEL_SW]._current_state) {
	case Up: // LFO FREQUENCY
		encoder.set_state_clamped(state, 0, 47);

		// // Determine which LFO frequency to modify
		synth._lfos[synth._lfo_target].set_frequency(LFO_FREQUENCIES[encoder.get_state()]);
		printuln("[LFO Frequency] LFO #%d: %f", synth._lfo_target,
			 synth._lfos[synth._lfo_target]._frequency);
		break;
	case Down: // AMPLITUDE MODULATOR ATTACK
		printuln("[AM Attack] callback not implemented");
		break;
	case Neutral: // SPECIAL EFFECT 0
		printuln("[Special Effect] callback not implemented");
		break;
	}
}

void lfo_amp_amp_mod_sustain_callback(RotaryEncoder &encoder)
{
	int state = encoder.get_state();

	switch (switches[EFFECTS_SEL_SW]._current_state) {
	case Up: { // LFO AMPLITUDE

		encoder.set_state_clamped(state, 0, 95);

		float amplitude = LFO_AMPLITUDES[encoder.get_state()];

		// Determine which LFO amplitude to modify
		synth._lfos[synth._lfo_target].set_amplitude(amplitude);
		printuln("[LFO Amplitude] LFO #%d: %f", synth._lfo_target,
			 synth._lfos[synth._lfo_target]._amplitude);
		break;
	}
	case Down: { // AMPLITUDE MODULATOR SUSTAIN
		printuln("[AM Sustain] callback not implemented");
		break;
	}
	case Neutral: { // SPECIAL EFFECT 1
		printuln("[Special Effect] callback not implemented");
		break;
	}
	}
}

void amp_mod_release_callback(RotaryEncoder &encoder)
{
	printuln("[AM Release] callback not implemented");
}

int Synthesizer::get_osc_sample(osc_t osc, uint16_t phase)
{
	int sample = 0;

	switch (osc.wave) {
	case sine:
		sample = ((int)SINE_LUT[phase >> 6]) - 0x8000;
		break;
	case square:
		sample = phase <= 0x8000 ? -0x8000 : 0x8000;
		break;
	case triangle:
		sample = phase <= 0x8000 ? 2 * (phase - 0x4000)   // rising edge of triangle
					 : -2 * (phase - 0xC000); // falling edge of triangle
		break;
	case sawtooth:
		sample = phase - 0x8000;
		break;
	}

	return sample;
}

float Synthesizer::get_sound_sample(Key &key)
{
	// Oscillator 1
	int sample1 = 0;

	if (synth._osc1.enabled) {
		// Obtain sound frequency (including any modulation)
		float freq = key.get_freq() * synth._osc1.freq_shift;
		if (synth._lfo_target == OSC1_FREQ) {
			freq += synth._lfos[OSC1_FREQ].get_sample();
		}

		// Calculate current key phase to select correct oscillator sample value
		key.phase1 += freq * 0x10000 / SAMPLE_FREQUENCY;
		sample1 = get_osc_sample(_osc1, key.phase1);

		// Apply desired volume (including any modulation)
		float vol1 = (float)_osc1.volume / 40000.0;
		if (synth._lfo_target == OSC1_AMP) {
			vol1 += synth._lfos[OSC1_AMP].get_sample();
		}
		sample1 *= vol1;
	}

	// Oscillator 2
	int sample2 = 0;

	if (synth._osc2.enabled) {
		// Obtain sound frequency (including any modulation)
		float freq = key.get_freq() * synth._osc2.freq_shift;
		if (synth._lfo_target == OSC2_FREQ) {
			freq += synth._lfos[OSC2_FREQ].get_sample();
		}

		// Calculate current key phase to select correct oscillator sample value
		key.phase2 += freq * 0x10000 / SAMPLE_FREQUENCY;
		sample2 = get_osc_sample(_osc2, key.phase2);

		// Apply desired volume (including any modulation)
		float vol2 = (float)_osc2.volume / 40000.0;
		if (synth._lfo_target == OSC2_AMP) {
			vol2 += synth._lfos[OSC2_AMP].get_sample();
		}
		sample2 *= vol2;
	}

	return (float)(sample1 + sample2);
}

bool Synthesizer::makesynth(uint8_t *block, k_timepoint_t deadline)
{
	const int check_interval = 64;
	const int64_t guard_ticks = k_us_to_ticks_ceil64(240);
	bool overloaded = false;
	size_t i = 0;
	int sample_index = 0;
	for (i = 0; i < BLOCK_SIZE; i += 2, sample_index++) {
		if ((sample_index % check_interval) == 0) {
			k_timeout_t remaining = sys_timepoint_timeout(deadline);
			if (remaining.ticks <= guard_ticks) {
				overloaded = true;
				break;
			}
		}
		float sample = 0;

		// get the synthesized sound for every pressed key
		for (int j = 0; j < MAX_KEYS; j++) {
			if (keys[j].state == PRESSED && !sys_timepoint_expired(keys[j].hold_time)) {
				sample += get_sound_sample(keys[j]);
			} else if (keys[j].state == PRESSED &&
				   sys_timepoint_expired(keys[j].hold_time)) {
				keys[j].state = IDLE;
			}
		}

		// Apply LPF
		if (synth._lpf._cutoff_freq > 0.) {
			sample = synth._lpf.filter(sample);
		}

		// clamp the value
		if (sample > 0x7fff) {
			sample = 0x7fff;
		} else if (sample < -0x7fff) {
			sample = -0x7fff;
		}

		block[i] = (int16_t)sample & 0xFF;
		block[i + 1] = (int16_t)sample >> 8;
	}
	if (overloaded && i < BLOCK_SIZE) {
		memset(&block[i], 0, BLOCK_SIZE - i);
	}
	return overloaded;
}
