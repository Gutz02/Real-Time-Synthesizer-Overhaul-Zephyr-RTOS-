/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Switch.hpp"
#include "audio.h"
#include "key.hpp"
#include "leds.h"
#include "peripherals.h"
#include "synth.hpp"
#include "usb.h"
#include <inttypes.h>
#include <math.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

Synthesizer synth;
// Function that checks key presses
static void check_keyboard()
{
	char character;
	while (usbRead(&character, 1)) {
		// ////printuln("Key pressed: %c (0x%02x)", character, character);
		auto key = Key::char_to_key(character);
		bool key_pressed = false;

		for (int i = 0; i < MAX_KEYS; i++) {
			if (key == keys[i].key && keys[i].state != IDLE) {
				keys[i].state = PRESSED;
				keys[i].hold_time = sys_timepoint_calc(K_MSEC(500));
				keys[i].release_time = sys_timepoint_calc(K_MSEC(500));
				key_pressed = true;
			}
		}
		// The second loop is necessary to avoid selecting an IDLE key when a
		// PRESSED or RELEASED key is located further away on the array
		if (!key_pressed) {
			for (int i = 0; i < MAX_KEYS; i++) {
				if (keys[i].state == IDLE) {
					keys[i].key = key;
					keys[i].state = PRESSED;
					keys[i].hold_time = sys_timepoint_calc(K_MSEC(500));
					keys[i].release_time = sys_timepoint_calc(K_MSEC(500));
					keys[i].phase1 = 0;
					keys[i].phase2 = 0;
					break;
				}
			}
		}
	}
}

/*
Do we need negative priorities?
Negative priorities do not allow preemption between them.
While positive does allow for preemption.
Should T4 preemp T1-T2 as well as T3?
We should bare in mind, that while, T4 is writing to the audio device, other tasks can run
Inside audio.cpp when task 4 is writing blocks to the i2s device, the current thread
will block, which explains:
- why the LEDS will stay for such a long time
- why the other tasks will run while T4 is high(the leds) in Logic2
- which also explains why t3 cant run while t4 is blocked, because t4 is holding the
   semaphore for blockmem Why does t4 block for so long when no sound is being produced?
   - The distance between a t4 call when no keys are pressed is of around 3ms, with a
   total duration of 47ms
   - Comapred to when a key is pressed, the distance between two t4 is of around 15ms
   with a total duration of 35ms
   - What does this mean?
   Blocking time depends on when task 4 calls i2s_write() relative to the how
   long it takes to read a block, not on whether the audio. The I2S bus clocks at a fixed rate
   */
#define TASK_1_2_STACK_SIZE 2048
#define TASK_3_STACK_SIZE   1024
#define TASK_4_STACK_SIZE   1024
#define TASK_1_2_PRIORITY   1
#define TASK_3_PRIORITY     2
#define TASK_4_PRIORITY     0

K_THREAD_STACK_DEFINE(task_1_2_stack, TASK_1_2_STACK_SIZE);
K_THREAD_STACK_DEFINE(task_3_stack, TASK_3_STACK_SIZE);
K_THREAD_STACK_DEFINE(task_4_stack, TASK_4_STACK_SIZE);

static struct k_thread task_1_2_thread;
static struct k_thread task_3_thread;
static struct k_thread task_4_thread;
static k_timepoint_t expiration;

// Audio blocks passed from T3 to T4
K_MSGQ_DEFINE(audio_block_msgq, sizeof(void *), BLOCK_COUNT, sizeof(void *));

extern "C" void task_1_2_thread_entry(void *p1, void *p2, void *p3)
{
	int period = static_cast<int>(reinterpret_cast<intptr_t>(p1));
	k_msgq *q = static_cast<k_msgq*>(p2);
 	expiration = sys_timepoint_calc(K_MSEC(period));
	while (1) {

		// T1
		set_led(&debug_led0);
		peripherals_update();
		reset_led(&debug_led0);

		k_timeout_t to_exp = sys_timepoint_timeout(expiration);
		int64_t delta_ms = k_ticks_to_ms_floor64(to_exp.ticks);
		// T2
		set_led(&debug_led1);
		check_keyboard();             
		reset_led(&debug_led1);

		k_timeout_t sleep_timeout = sys_timepoint_timeout(expiration);
		int64_t sleep_us = k_ticks_to_us_floor64(sleep_timeout.ticks);
		k_sleep(K_USEC(sleep_us - 480));
		k_busy_wait(480);
		expiration = sys_timepoint_calc(K_MSEC(period));
	}
}

extern "C" void task_3_thread_entry(void *p1, void *p2, void *p3)
{
	k_msgq *audio_q = static_cast<k_msgq*>(p2);
	k_msgq *q = static_cast<k_msgq*>(p3);

	static bool overload_next = false;
	while (1) {
		k_timepoint_t frame_deadline = expiration;
		bool show_overload = overload_next;
		k_timeout_t sleep_timeout;
		uint64_t sleep_ms;

		if (show_overload) {
			set_led(&debug_led2);
		}

		void *mem_block = allocBlock();

		if (mem_block == nullptr) {
			if (show_overload) {
				reset_led(&debug_led2);
			}
			overload_next = true;
			continue;
		}

		bool overloaded = false;
		overloaded = synth.makesynth(static_cast<uint8_t *>(mem_block), frame_deadline);
		if (show_overload) {
			reset_led(&debug_led2);
		}
		overload_next = overloaded;

		k_msgq_put(audio_q, &mem_block, K_FOREVER);

		if (sys_timepoint_expired(frame_deadline)) {
			//printuln("T3 missed deadline");
		}

		sleep_timeout = sys_timepoint_timeout(expiration);
		sleep_ms = k_ticks_to_ms_floor64(sleep_timeout.ticks);
		k_sleep(sleep_timeout);
	}
}

extern "C" void task_4_thread_entry(void *p1, void *p2, void *p3)
{
	k_msgq *audio_q = static_cast<k_msgq*>(p2);
    while (1) {
		void *mem_block = nullptr;
		k_msgq_get(audio_q, &mem_block, K_FOREVER);
		set_led(&debug_led3);
		writeBlock(mem_block);
        reset_led(&debug_led3);    
	}
}

void start_threads()
{
	int period_task12_ms = BLOCK_GEN_PERIOD_MS;
	int period_task3_ms = BLOCK_GEN_PERIOD_MS;

	k_thread_create(&task_1_2_thread, task_1_2_stack, K_THREAD_STACK_SIZEOF(task_1_2_stack),
			task_1_2_thread_entry,
			reinterpret_cast<void *>(static_cast<intptr_t>(period_task12_ms)), 
			nullptr, nullptr, TASK_1_2_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&task_3_thread, task_3_stack, K_THREAD_STACK_SIZEOF(task_3_stack),
			task_3_thread_entry,
			reinterpret_cast<void *>(static_cast<intptr_t>(period_task3_ms)), &audio_block_msgq,
			nullptr, TASK_3_PRIORITY, 0, K_NO_WAIT);
		

	k_thread_create(&task_4_thread, task_4_stack, K_THREAD_STACK_SIZEOF(task_4_stack),
			task_4_thread_entry,
			reinterpret_cast<void *>(static_cast<intptr_t>(10)), &audio_block_msgq,
			nullptr, TASK_4_PRIORITY, 0, K_NO_WAIT);
}

int main(void)
{
	initUsb();
	waitForUsb();

	//printuln("== Initializing... ==");

	init_leds();
	initAudio();
	init_peripherals();

	synth.initialize();

	// Buffer for writing to audio driver

	//printuln("== Finished initialization ==");

	int64_t time = k_uptime_get();
	int state = 0;
	/*
		Task Priorities:
		T4, always something being sent to the ampliefier
			Period
		T2, Latency when pressing a key should be minimal
		T1, Polling peripherals is not time critical
		T3, Synth generation can take some time, but not critical
	*/
	start_threads();
    k_thread_join(&task_1_2_thread,  K_FOREVER);
    k_thread_join(&task_3_thread,    K_FOREVER);
    k_thread_join(&task_4_thread,    K_FOREVER);

	return 0;
}
