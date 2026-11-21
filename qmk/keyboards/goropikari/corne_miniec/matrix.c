/* Copyright 2020 sekigon-gonnoc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "analog.h"
#include "gpio.h"
#include "keyboard.h"
#include "matrix.h"
#include "timer.h"
#include "tuning.h"
#include "util.h"
#include "wait.h"

#define CALIBRATION_STABLE_MARGIN 32
#define CALIBRATION_SETTLE_US 30
#define CALIBRATION_STABLE_ROUNDS 10
#define CALIBRATION_MAX_ROUNDS 100
#define CALIBRATION_HISTORY_SIZE 5
#define CALIBRATION_INTERVAL_MS 10000
#define CALIBRATION_FILTER_SCALE 256
#define CALIBRATION_FILTER_DIVISOR 64

static const pin_t drive_pins[] = EC_DRIVE_PINS;
// SEL0, SEL1, SEL2, from least to most significant selector bit.
static const pin_t mux_pins[] = EC_MUX_PINS;
// Module inputs S5, S6, S4, S1, S2, S3 map to matrix columns 0 through 5.
static const uint8_t mux_channels[] = {4, 5, 3, 0, 1, 2};

_Static_assert(ARRAY_SIZE(drive_pins) == EC_ROWS_PER_HAND, "Invalid local row count");
_Static_assert(ARRAY_SIZE(mux_channels) == MATRIX_COLS, "Invalid column count");

typedef struct {
    uint16_t baseline;
    uint16_t sample;
    int32_t filtered;
    uint16_t history[CALIBRATION_HISTORY_SIZE];
} key_calibration_t;

static key_calibration_t calibration[EC_ROWS_PER_HAND][MATRIX_COLS];
static uint8_t history_slot;
static uint32_t last_calibration;
static bool calibration_ready;

static void select_mux(uint8_t col) {
    for (uint8_t bit = 0; bit < ARRAY_SIZE(mux_pins); ++bit) {
        gpio_write_pin(mux_pins[bit], (mux_channels[col] >> bit) & 1);
    }
}

static uint16_t read_sample(uint8_t row) {
    // The discharge pin is driven low between samples and floats during measurement.
    wait_us(EC_DISCHARGE_US);
    gpio_set_pin_input(EC_DISCHARGE_PIN);
    gpio_write_pin_high(drive_pins[row]);
    uint16_t sample = analogReadPin(EC_ADC_PIN);
    gpio_write_pin_low(drive_pins[row]);
    gpio_set_pin_output(EC_DISCHARGE_PIN);
    return sample;
}

void matrix_init_custom(void) {
    gpio_write_pin_low(EC_DISCHARGE_PIN);
    gpio_set_pin_output(EC_DISCHARGE_PIN);

    for (uint8_t row = 0; row < EC_ROWS_PER_HAND; ++row) {
        gpio_write_pin_low(drive_pins[row]);
        gpio_set_pin_output(drive_pins[row]);
    }

    for (uint8_t bit = 0; bit < ARRAY_SIZE(mux_pins); ++bit) {
        gpio_write_pin_low(mux_pins[bit]);
        gpio_set_pin_output(mux_pins[bit]);
    }

    // Start the ADC before the first timed charge and read.
    (void)analogReadPin(EC_ADC_PIN);
}

static void calibrate_idle_levels(void) {
    for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
        select_mux(col);
        for (uint8_t row = 0; row < EC_ROWS_PER_HAND; ++row) {
            (void)read_sample(row);
            calibration[row][col].baseline = UINT16_MAX;
            wait_us(CALIBRATION_SETTLE_US);
        }
    }

    // Require a stable minimum, with a time limit for keys disturbed at startup.
    uint8_t stable_rounds = 0;
    for (uint8_t round = 0; round < CALIBRATION_MAX_ROUNDS && stable_rounds < CALIBRATION_STABLE_ROUNDS; ++round) {
        bool stable = true;
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            select_mux(col);
            for (uint8_t row = 0; row < EC_ROWS_PER_HAND; ++row) {
                key_calibration_t *key = &calibration[row][col];
                const uint16_t sample = read_sample(row);
                key->sample = sample;
                if (sample < key->baseline) {
                    key->baseline = sample;
                    stable = false;
                } else if (sample > key->baseline + CALIBRATION_STABLE_MARGIN) {
                    stable = false;
                }
                wait_us(CALIBRATION_SETTLE_US);
            }
        }
        stable_rounds = stable ? stable_rounds + 1 : 0;
    }

    for (uint8_t row = 0; row < EC_ROWS_PER_HAND; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            key_calibration_t *key = &calibration[row][col];
            key->filtered = (int32_t)key->baseline * CALIBRATION_FILTER_SCALE;
            for (uint8_t slot = 0; slot < CALIBRATION_HISTORY_SIZE; ++slot) {
                key->history[slot] = key->baseline;
            }
        }
    }
}

void keyboard_post_init_kb(void) {
    // Bootmagic has already scanned with the fixed thresholds.
    calibrate_idle_levels();
    ec_tuning_init();
    history_slot = 0;
    last_calibration = timer_read32();
    calibration_ready = true;
    keyboard_post_init_user();
}

static void update_baselines(void) {
    if (timer_elapsed32(last_calibration) < CALIBRATION_INTERVAL_MS) {
        return;
    }

    last_calibration = timer_read32();
    for (uint8_t row = 0; row < EC_ROWS_PER_HAND; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            key_calibration_t *key = &calibration[row][col];
            key->history[history_slot] = (key->filtered + CALIBRATION_FILTER_SCALE / 2) / CALIBRATION_FILTER_SCALE;

            uint32_t total = 0;
            for (uint8_t slot = 0; slot < CALIBRATION_HISTORY_SIZE; ++slot) {
                total += key->history[slot];
            }
            key->baseline = total / CALIBRATION_HISTORY_SIZE;
        }
    }
    history_slot = (history_slot + 1) % CALIBRATION_HISTORY_SIZE;
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    if (!calibration_ready) {
#ifdef BOOTMAGIC_ENABLE
        // Keep the normal matrix clear until the idle baselines are available.
        const bool left = is_keyboard_left();
        const uint8_t row = (left ? BOOTMAGIC_ROW : BOOTMAGIC_ROW_RIGHT) % EC_ROWS_PER_HAND;
        const uint8_t col = left ? BOOTMAGIC_COLUMN : BOOTMAGIC_COLUMN_RIGHT;
        const matrix_row_t mask = MATRIX_ROW_SHIFTER << col;
        const bool was_pressed = (current_matrix[row] & mask) != 0;

        select_mux(col);
        const uint16_t threshold = was_pressed ? EC_STARTUP_RELEASE_THRESHOLD : EC_STARTUP_PRESS_THRESHOLD;
        if ((read_sample(row) >= threshold) != was_pressed) {
            current_matrix[row] ^= mask;
            return true;
        }
#endif
        return false;
    }

    bool changed = false;

    for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
        select_mux(col);
        const matrix_row_t key_mask = MATRIX_ROW_SHIFTER << col;
        for (uint8_t row = 0; row < EC_ROWS_PER_HAND; ++row) {
            key_calibration_t *key = &calibration[row][col];
            const uint16_t sample = read_sample(row);
            key->sample = sample;
            const ec_offsets_t offsets = ec_tuning_offsets(row * MATRIX_COLS + col);
            const uint16_t press = key->baseline + offsets.press;
            const uint16_t release = key->baseline + offsets.release;
            bool was_pressed = (current_matrix[row] & key_mask) != 0;
            bool pressed = sample >= (was_pressed ? release : press);

            if (pressed != was_pressed) {
                current_matrix[row] ^= key_mask;
                changed = true;
            }

            if (!pressed && sample < release) {
                // Update the baseline estimate only with idle readings.
                int32_t target = (int32_t)sample * CALIBRATION_FILTER_SCALE;
                key->filtered += (target - key->filtered) / CALIBRATION_FILTER_DIVISOR;
            }
        }
    }

    update_baselines();
    return changed;
}

ec_reading_t ec_matrix_reading(uint8_t index) {
    const key_calibration_t *key = &calibration[index / MATRIX_COLS][index % MATRIX_COLS];
    return (ec_reading_t){key->sample, key->baseline};
}
